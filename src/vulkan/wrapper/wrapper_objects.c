#include "wrapper_objects.h"

// Hashtable tagged wrapper objects:
// VkImage -> wrapper_image
// VkBuffer -> wrapper_buffer
// VkCommandPool -> wrapper_command_pool

#define MAKE_GET_FUNCTION(wtype, vtype, map) \
struct wtype * get_##wtype(struct wrapper_device *device, vtype handle) { \
   struct wtype *obj = NULL; \
   simple_mtx_lock(&device->resource_mutex); \
   obj = _mesa_hash_table_u64_search(device->map, (uint64_t) handle); \
   simple_mtx_unlock(&device->resource_mutex); \
   return obj; \
}

#define MAKE_CREATE_FUNCTION(wtype, vtype, map, vk_type, init) \
struct wtype *wtype##_create(struct wrapper_device *device, const vtype##CreateInfo *pCreateInfo, vtype dispatch_handle) {\
   struct wtype *obj = vk_zalloc2(&device->vk.alloc, NULL, sizeof(struct wtype), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT); \
   if (!obj) \
      return NULL; \
   init; \
   obj->device = device; \
   obj->dispatch_handle = dispatch_handle; \
   list_inithead(&obj->staging_resources); \
   simple_mtx_init(&obj->resource_mutex, mtx_plain); \
   simple_mtx_lock(&device->resource_mutex); \
   static uint32_t obj_id = 0; \
   obj->obj_id = obj_id++; \
   _mesa_hash_table_u64_insert(device->map, (uint64_t) dispatch_handle, obj); \
   simple_mtx_unlock(&device->resource_mutex); \
   return obj; \
}

#define MAKE_DESTROY_FUNCTION(wtype, map, destroy) \
void wtype##_destroy(struct wrapper_device *device, struct wtype *obj) { \
   free_staging_resources_final(device, obj, &obj->staging_resources); \
   simple_mtx_destroy(&obj->resource_mutex); \
   simple_mtx_lock(&device->resource_mutex); \
   _mesa_hash_table_u64_remove(device->map, (uint64_t) obj->dispatch_handle); \
   simple_mtx_unlock(&device->resource_mutex); \
   destroy; \
}

MAKE_GET_FUNCTION(wrapper_image, VkImage, image_map);

MAKE_CREATE_FUNCTION(wrapper_image, VkImage, image_map,
                     VK_OBJECT_TYPE_IMAGE,
                     vk_image_init(&device->vk, &obj->vk, pCreateInfo));

MAKE_DESTROY_FUNCTION(wrapper_image, image_map, {
    vk_image_destroy(&device->vk, NULL, &obj->vk);
});

MAKE_GET_FUNCTION(wrapper_buffer, VkBuffer, buffer_map);

MAKE_CREATE_FUNCTION(wrapper_buffer, VkBuffer, buffer_map,
                     VK_OBJECT_TYPE_BUFFER,
                     vk_buffer_init(&device->vk, &obj->vk, pCreateInfo));

MAKE_DESTROY_FUNCTION(wrapper_buffer, buffer_map, {
   vk_buffer_destroy(&device->vk, NULL, &obj->vk);
});

MAKE_GET_FUNCTION(wrapper_command_pool, VkCommandPool, command_pool_map)

MAKE_CREATE_FUNCTION(wrapper_command_pool, VkCommandPool, command_pool_map,
                     VK_OBJECT_TYPE_COMMAND_POOL,
                     {
                        vk_command_pool_init(&device->vk, &obj->vk, pCreateInfo, NULL);
                     });

MAKE_DESTROY_FUNCTION(wrapper_command_pool, command_pool_map, {
    vk_command_pool_finish(&obj->vk);
    vk_free2(&obj->vk.alloc, NULL, obj);
});

// Staging resource tracking and GC

void add_staging_resource_to(
   struct wrapper_device *device,
   struct list_head *staging_resources,
   VkObjectType type,
   void* handle,
   void* parent) {
   struct wrapper_staging_resources *obj =
         vk_alloc(&device->vk.alloc, sizeof(struct wrapper_staging_resources), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   obj->type = type;
   obj->handle = handle;
   obj->parent = parent;
   list_addtail(&obj->link_in_parent, staging_resources);
}

void free_staging_resources_if_ready(
    struct wrapper_device *device, 
    void* handle, 
    struct list_head* list,
    bool cleanup_everything) {
    // // Clean up temporary objects associated with this object, does not destroy the list nor objects that are not ready to be cleaned up
    int cleaned_up = 0;
    if (!list_is_empty(list)) {
        list_for_each_entry_safe(struct wrapper_staging_resources, resource, list, link_in_parent) {
            if (!resource->handle) {
                continue;
            }

            if (!cleanup_everything) {
                if (resource->ready == VK_NULL_HANDLE) {
                continue;
                }
                if (WDEVICE.GetFenceStatus((VkDevice) device, resource->ready) != VK_SUCCESS) {
                continue;
                }
            }

            #define FREE_STAGING_RESOURCE_(t1, t2, free) \
            case VK_OBJECT_TYPE_##t1: \
                CHECKV(free((VkDevice) device, (Vk##t2) resource->handle, NULL)); \
                cleaned_up++; \
                break;
            #define FREE_STAGING_RESOURCE(t1, t2) FREE_STAGING_RESOURCE_(t1, t2, Destroy##t2)

            switch (resource->type) {
            FREE_STAGING_RESOURCE(IMAGE_VIEW, ImageView)
            FREE_STAGING_RESOURCE(BUFFER, Buffer)
            FREE_STAGING_RESOURCE_(DEVICE_MEMORY, DeviceMemory, FreeMemory)
            default:
                WLOGE("Staging resource of type %d is not handled");
                break;
            }

            #undef FREE_STAGING_RESOURCE
            #undef FREE_STAGING_RESOURCE_

            resource->handle = VK_NULL_HANDLE;
            resource->parent = NULL;
            list_del(&resource->link_in_parent);
            vk_free(&device->vk.alloc, resource);
        }
    }

    if (cleaned_up)
        WLOGD("Released %d staging resources for handle %p", cleaned_up, handle);
}
