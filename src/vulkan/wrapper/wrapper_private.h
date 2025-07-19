#pragma once

#include <sys/stat.h>

#include "vulkan/runtime/vk_instance.h"
#include "vulkan/runtime/vk_physical_device.h"
#include "vulkan/runtime/vk_device.h"
#include "vulkan/runtime/vk_queue.h"
#include "vulkan/runtime/vk_command_buffer.h"
#include "vulkan/runtime/vk_buffer.h"
#include "vulkan/runtime/vk_image.h"
#include "vulkan/runtime/vk_log.h"
#include "vulkan/runtime/vk_render_pass.h"
#include "vulkan/runtime/vk_framebuffer.h"

#include "vulkan/util/vk_dispatch_table.h"
#include "vulkan/wsi/wsi_common.h"
#include "util/simple_mtx.h"
#include "wrapper_log.h"
#include "wrapper_trampolines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Bitfield enum of additional driver features that can be used with adrenotools_open_libvulkan
 */
enum {
    ADRENOTOOLS_DRIVER_CUSTOM = 1 << 0,
    ADRENOTOOLS_DRIVER_FILE_REDIRECT = 1 << 1,
    ADRENOTOOLS_DRIVER_GPU_MAPPING_IMPORT = 1 << 2,
};

#define ADRENOTOOLS_GPU_MAPPING_SUCCEEDED_MAGIC 0xDEADBEEF

/**
 * @brief A replacement GPU memory mapping for use with ADRENOTOOLS_DRIVER_GPU_MAPPING_IMPORT
 */
struct adrenotools_gpu_mapping {
    void *host_ptr;
    uint64_t gpu_addr; //!< The GPU address of the mapping to import, if mapping import/init succeeds this will be set to ADRENOTOOLS_GPU_MAPPING_SUCCEEDED_MAGIC
    uint64_t size;
    uint64_t flags;
};
void *adrenotools_open_libvulkan(int dlopenMode, int featureFlags, const char *tmpLibDir, const char *hookLibDir, const char *customDriverDir, const char *customDriverName, const char *fileRedirectDir, void **userMappingHandle);

bool adrenotools_import_user_mem(void *handle, void *hostPtr, uint64_t size);

bool adrenotools_mem_gpu_allocate(void *handle, uint64_t *size);

bool adrenotools_mem_cpu_map(void *handle, void *hostPtr, uint64_t size);

bool adrenotools_validate_gpu_mapping(void *handle);

void adrenotools_set_turbo(bool turbo);

#ifdef __cplusplus
}
#endif

#define CHECK(call) ({ \
      VkResult __result = wrapper_device_trampolines.call; \
      if (__result) { WLOGE("%s failed with %d", #call, __result); } \
      __result; \
   })

#define CHECKV(call) ({ \
   wrapper_device_trampolines.call; \
})

// #define NEEDS_PRINTING_CmdCopyBuffer 1;
// #define NEEDS_PRINTING_AllocateDescriptorSets 1;
// #define NEEDS_PRINTING_BindBufferMemory 1;
// #define NEEDS_PRINTING_CmdBeginRenderPass 1;
// #define NEEDS_PRINTING_CmdBindDescriptorSets 1;
// #define NEEDS_PRINTING_CmdBindIndexBuffer 1;
// #define NEEDS_PRINTING_CmdBindPipeline 1;
// #define NEEDS_PRINTING_CmdBindVertexBuffers 1;
// #define NEEDS_PRINTING_CmdDraw 1;
// #define NEEDS_PRINTING_CmdPipelineBarrier 1;
// #define NEEDS_PRINTING_CmdPushConstants 1;
// #define NEEDS_PRINTING_CmdSetScissor 1;
// #define NEEDS_PRINTING_CmdSetViewport 1;
// #define NEEDS_PRINTING_CreateBuffer 1;
// #define NEEDS_PRINTING_CreateDescriptorPool 1;
// #define NEEDS_PRINTING_CreateDescriptorSetLayout 1;
// #define NEEDS_PRINTING_CreateDescriptorUpdateTemplate 1;
// #define NEEDS_PRINTING_CreateFramebuffer 1;
// #define NEEDS_PRINTING_CreateGraphicsPipelines 1;
// #define NEEDS_PRINTING_CreatePipelineLayout 1;
// #define NEEDS_PRINTING_CreateRenderPass 1;
// #define NEEDS_PRINTING_CreateShaderModule 1;
// #define NEEDS_PRINTING_DestroyShaderModule 1;
// #define NEEDS_PRINTING_GetBufferMemoryRequirements2 1;
// #define NEEDS_PRINTING_MapMemory 1;
// #define NEEDS_PRINTING_UpdateDescriptorSetWithTemplate 1;
// #define NEEDS_PRINTING_CmdCopyBufferToImage 1;
// #define NEEDS_PRINTING_CreateImage 1
// #define NEEDS_PRINTING_DestroyImage 1
// #define NEEDS_PRINTING_CreateImageView 1;
// #define NEEDS_PRINTING_GetAndroidHardwareBufferPropertiesANDROID 1;
// #define NEEDS_PRINTING_CreateBuffer 1;
// #define NEEDS_PRINTING_WaitForFences 1;
// #define NEEDS_PRINTING_GetBufferMemoryRequirements2 1;
// #define NEEDS_PRINTING_GetImageMemoryRequirements2 1;
// #define NEEDS_PRINTING_GetImageMemoryRequirements 1
// #define NEEDS_PRINTING_BindImageMemory 1;
// #define NEEDS_PRINTING_QueueSubmit2 1;
// #define NEEDS_PRINTING_AllocateMemory 1;
// #define NEEDS_PRINTING_GetPhysicalDeviceImageFormatProperties2 1
// #define NEEDS_PRINTING_AllocateDescriptorSets 1;

extern const struct vk_instance_extension_table wrapper_instance_extensions;
extern const struct vk_device_extension_table wrapper_device_extensions;
extern const struct vk_device_extension_table wrapper_filter_extensions;

struct wrapper_instance {
   struct vk_instance vk;

   VkInstance dispatch_handle;
   struct vk_instance_dispatch_table dispatch_table;
   struct vk_debug_utils_messenger* internal_debug_messenger;
};

VK_DEFINE_HANDLE_CASTS(wrapper_instance, vk.base, VkInstance,
                       VK_OBJECT_TYPE_INSTANCE)

struct wrapper_physical_device {
   struct vk_physical_device vk;

   int dma_heap_fd;
   VkPhysicalDevice dispatch_handle;
   VkPhysicalDeviceProperties2 properties2;
   VkPhysicalDeviceDriverProperties driver_properties;
   VkPhysicalDeviceMemoryProperties memory_properties;
   struct wsi_device wsi_device;
   struct wrapper_instance *instance;
   struct vk_features base_supported_features;
   struct vk_device_extension_table base_supported_extensions;
   struct vk_physical_device_dispatch_table dispatch_table;
};

VK_DEFINE_HANDLE_CASTS(wrapper_physical_device, vk.base, VkPhysicalDevice,
                       VK_OBJECT_TYPE_PHYSICAL_DEVICE)

struct wrapper_queue {
   struct vk_queue vk;
   struct wrapper_device *device;
   VkQueue dispatch_handle;
};

VK_DEFINE_HANDLE_CASTS(wrapper_queue, vk.base, VkQueue,
                       VK_OBJECT_TYPE_QUEUE)

struct wrapper_device {
   struct vk_device vk;

   VkDevice dispatch_handle;
   simple_mtx_t resource_mutex;
   struct list_head command_buffer_list;
   struct list_head device_memory_list;

   struct hash_table_u64* image_map;
   struct hash_table_u64* buffer_map;

   struct wrapper_physical_device *physical;
   struct vk_device_dispatch_table dispatch_table;

   uint32_t queueCount;
   struct wrapper_queue **queues;
};

VK_DEFINE_HANDLE_CASTS(wrapper_device, vk.base, VkDevice,
                       VK_OBJECT_TYPE_DEVICE)


typedef struct wrapper_cmd_buffer_staging_buffer {
   struct list_head link;
   VkBuffer buffer;
   VkDeviceMemory memory;
} wrapper_cmd_buffer_staging_buffer;

typedef struct wrapper_cmd_buffer_staging_image_view {
   struct list_head link;
   VkImageView imageView;
} wrapper_cmd_buffer_staging_image_view;

struct wrapper_command_buffer {
   struct vk_command_buffer vk;

   struct wrapper_device *device;
   struct list_head link;
   VkCommandPool pool;
   VkCommandBuffer dispatch_handle;
};

VK_DEFINE_HANDLE_CASTS(wrapper_command_buffer, vk.base, VkCommandBuffer,
                       VK_OBJECT_TYPE_COMMAND_BUFFER)

struct wrapper_device_memory {
   struct AHardwareBuffer *ahardware_buffer;
   struct wrapper_device *device;
   struct list_head link;
   int dmabuf_fd;
   void *map_address;
   size_t map_size;
   size_t alloc_size;
   VkDeviceMemory dispatch_handle;
   const VkAllocationCallbacks *alloc;
};

VkResult enumerate_physical_device(struct vk_instance *_instance);
void destroy_physical_device(struct vk_physical_device *pdevice);

void
wrapper_setup_device_features(struct wrapper_physical_device *physical_device);

uint32_t
wrapper_select_device_memory_type(struct wrapper_device *device,
                                  VkMemoryPropertyFlags flags);

VkResult
wrapper_device_memory_create(struct wrapper_device *device,
                             const VkAllocationCallbacks *alloc,
                             struct wrapper_device_memory **out_mem);

void
wrapper_device_memory_destroy(struct wrapper_device_memory *mem);

// hash_map based lookup types
#define COMMON_FIELDS(T) \
   struct wrapper_device *device; \
   T dispatch_handle; \
   simple_mtx_t resource_mutex; \
   struct list_head staging_resources; \
   uint32_t obj_id

#define CREATE_FROM_HANDLE_FUNCTION(wtype, vtype, map) \
static struct wtype * get_##wtype(struct wrapper_device *device, vtype handle) { \
   struct wtype *obj = NULL; \
   simple_mtx_lock(&device->resource_mutex); \
   obj = _mesa_hash_table_u64_search(device->map, (uint64_t) handle); \
   simple_mtx_unlock(&device->resource_mutex); \
   return obj; \
}

#define CREATE_FUNCTION_BODY(wtype, vtype, map, vk_type, init) \
   struct wtype *obj = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(struct wtype), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT); \
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
   return obj;

typedef struct wrapper_staging_resources {
   struct list_head link;
   VkObjectType type;
   bool use_vk_object;
   void* handle;
   void* parent; // the parent object tracking this resource
   // In the future, add a fence for when this object is no longer needed
} wrapper_staging_resources;

#define TRACK_STAGING(device, type, handle, parent) add_staging_resource_to(device, &parent->staging_resources, VK_OBJECT_TYPE_##type, handle, parent)

static void add_staging_resource_to(
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
   list_addtail(&obj->link, staging_resources);
}

static void free_staging_resources_from(struct wrapper_device *device, void* handle, struct list_head* list) {
   // // Clean up temporary objects associated with this object
   if (!list_is_empty(list)) {
      WLOGD("Releasing %d staging resources for BCn handle %p", list_length(list), handle);
      list_for_each_entry_safe(struct wrapper_staging_resources, resource, list, link) {
         if (!resource->handle) {
            continue;
         }
         switch (resource->type) {
#define FREE_STAGING_RESOURCE_(t1, t2, free) \
         case VK_OBJECT_TYPE_##t1: \
            CHECKV(free((VkDevice) device, (Vk##t2) resource->handle, NULL)); \
            break;
#define FREE_STAGING_RESOURCE(t1, t2) FREE_STAGING_RESOURCE_(t1, t2, Destroy##t2)
         FREE_STAGING_RESOURCE(IMAGE_VIEW, ImageView)
         FREE_STAGING_RESOURCE(BUFFER, Buffer)
         FREE_STAGING_RESOURCE_(DEVICE_MEMORY, DeviceMemory, FreeMemory)
#undef FREE_STAGING_RESOURCE
#undef FREE_STAGING_RESOURCE_
         default:
            WLOGE("Staging resource of type %d is not handled");
            break;
         }
         resource->handle = NULL;
         list_del(&resource->link);
         vk_free(&device->vk.alloc, resource);
      }
   }
}

#define CREATE_DESTROY_FUNCTION(wtype, map, destroy) \
static void wtype##_destroy(struct wrapper_device *device, struct wtype *obj, const VkAllocationCallbacks* pAllocator) { \
   free_staging_resources_from(device, obj, &obj->staging_resources); \
   simple_mtx_destroy(&obj->resource_mutex); \
   simple_mtx_lock(&device->resource_mutex); \
   _mesa_hash_table_u64_remove(device->map, (uint64_t) obj->dispatch_handle); \
   simple_mtx_unlock(&device->resource_mutex); \
   destroy; \
}

struct wrapper_image {
   struct vk_image vk;

   COMMON_FIELDS(VkImage);

   bool is_bcn_emulated;
   VkFormat original_format;
};

CREATE_FROM_HANDLE_FUNCTION(wrapper_image, VkImage, image_map)

static struct wrapper_image *wrapper_image_create(struct wrapper_device *device,
                     const VkImageCreateInfo *pCreateInfo,
                     const VkAllocationCallbacks *pAllocator,
                     VkImage dispatch_handle)
{
   CREATE_FUNCTION_BODY(wrapper_image, VkImage, image_map,
                        VK_OBJECT_TYPE_IMAGE,
                        vk_image_init(&device->vk, &obj->vk, pCreateInfo));
}

CREATE_DESTROY_FUNCTION(wrapper_image, image_map, {
   // vk_image_finish(&obj->vk);
   // vk_free2(&device->vk.alloc, pAllocator, obj);
   vk_image_destroy(&device->vk, pAllocator, &obj->vk);
});

struct wrapper_buffer_descriptor_pool {
    struct list_head link;
    VkDescriptorPool pool;
};

typedef struct wrapper_buffer_staging_resources {
   struct list_head link;
   VkBuffer stagingBuffer;
   VkDeviceMemory stagingBufferMemory;
   VkImageView stagingImageView;
} wrapper_buffer_staging_resources;

typedef struct wrapper_buffer {
   struct vk_buffer vk;

   COMMON_FIELDS(VkBuffer);

   struct list_head temp_descriptor_pools;
   VkDeviceMemory memory; // Pointer to the memory allocated by vkAllocateMemory
   VkDeviceSize memoryOffset; // Size of the memory allocated by vkAllocateMemory
} wrapper_buffer;

CREATE_FROM_HANDLE_FUNCTION(wrapper_buffer, VkBuffer, buffer_map)

static struct wrapper_buffer *wrapper_buffer_create(struct wrapper_device *device,
                     const VkBufferCreateInfo *pCreateInfo,
                     const VkAllocationCallbacks *pAllocator,
                     VkBuffer dispatch_handle)
{
   CREATE_FUNCTION_BODY(wrapper_buffer, VkBuffer, buffer_map,
                        VK_OBJECT_TYPE_BUFFER,
                        vk_buffer_init(&device->vk, &obj->vk, pCreateInfo));
}

CREATE_DESTROY_FUNCTION(wrapper_buffer, buffer_map, {
   // vk_buffer_finish(&obj->vk);
   vk_buffer_destroy(&device->vk, pAllocator, &obj->vk);
});

// For BCn emulation
static bool is_bc_image_format(VkFormat format) {
   switch (format) {
   case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
   case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
   case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
   case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
   case VK_FORMAT_BC2_UNORM_BLOCK:
   case VK_FORMAT_BC2_SRGB_BLOCK:
   case VK_FORMAT_BC3_UNORM_BLOCK:
   case VK_FORMAT_BC3_SRGB_BLOCK:
   case VK_FORMAT_BC4_UNORM_BLOCK:
   case VK_FORMAT_BC4_SNORM_BLOCK:
   case VK_FORMAT_BC5_UNORM_BLOCK:
   case VK_FORMAT_BC5_SNORM_BLOCK:
   case VK_FORMAT_BC6H_UFLOAT_BLOCK:
   case VK_FORMAT_BC6H_SFLOAT_BLOCK:
   case VK_FORMAT_BC7_UNORM_BLOCK:
   case VK_FORMAT_BC7_SRGB_BLOCK:
      return true;
   default:
      return false;
   }
}

static inline uint32_t get_bc_block_size(VkFormat format) {
   if (!is_bc_image_format(format)) return 4;
    switch (format) {
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
        case VK_FORMAT_BC4_UNORM_BLOCK:
        case VK_FORMAT_BC4_SNORM_BLOCK:
            return 8;
        default:
            return 16;
    }
}


typedef enum VkLayerFunction_ {
    VK_LAYER_FUNCTION_LINK = 0,
    VK_LAYER_FUNCTION_DEVICE = 1,
    VK_LAYER_FUNCTION_INSTANCE = 2
} VkLayerFunction;
/*
 * When creating the device chain the loader needs to pass
 * down information about it's device structure needed at
 * the end of the chain. Passing the data via the
 * VkLayerInstanceInfo avoids issues with finding the
 * exact instance being used.
 */
typedef struct VkLayerInstanceInfo_ {
    void* instance_info;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
} VkLayerInstanceInfo;
typedef struct VkLayerInstanceLink_ {
    struct VkLayerInstanceLink_* pNext;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
} VkLayerInstanceLink;
/*
 * When creating the device chain the loader needs to pass
 * down information about it's device structure needed at
 * the end of the chain. Passing the data via the
 * VkLayerDeviceInfo avoids issues with finding the
 * exact instance being used.
 */
typedef struct VkLayerDeviceInfo_ {
    void* device_info;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
} VkLayerDeviceInfo;
typedef struct {
    VkStructureType sType;  // VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO
    const void* pNext;
    VkLayerFunction function;
    union {
        VkLayerInstanceLink* pLayerInfo;
        VkLayerInstanceInfo instanceInfo;
    } u;
} VkLayerInstanceCreateInfo;
typedef struct VkLayerDeviceLink_ {
    struct VkLayerDeviceLink_* pNext;
    PFN_vkGetInstanceProcAddr pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr pfnNextGetDeviceProcAddr;
} VkLayerDeviceLink;
typedef struct {
    VkStructureType sType;  // VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO
    const void* pNext;
    VkLayerFunction function;
    union {
        VkLayerDeviceLink* pLayerInfo;
        VkLayerDeviceInfo deviceInfo;
    } u;
} VkLayerDeviceCreateInfo;

static VkFormat unwrap_vk_format_physical_device(struct wrapper_physical_device* pdevice, VkFormat in_format) {
   if (!pdevice) {
      WLOGE("unwrap_vk_format: null pdevice");
      return in_format;
   }
   // Replace BCn formats with R8G8B8A8_UNORM if they are emulated
   if (is_bc_image_format(in_format) 
      && !pdevice->base_supported_features.textureCompressionBC
   ) {
      // VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC2_UNORM_BLOCK = 135, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC2_SRGB_BLOCK = 136, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC3_UNORM_BLOCK = 137, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC3_SRGB_BLOCK = 138, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC4_UNORM_BLOCK = 139, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC4_SNORM_BLOCK = 140, -> VK_FORMAT_R8G8B8A8_SNORM *
      // VK_FORMAT_BC5_UNORM_BLOCK = 141, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC5_SNORM_BLOCK = 142, -> VK_FORMAT_R8G8B8A8_SNORM *
      // VK_FORMAT_BC6H_UFLOAT_BLOCK = 143, -> VK_FORMAT_R16G16B16A16_SFLOAT *
      // VK_FORMAT_BC6H_SFLOAT_BLOCK = 144, -> VK_FORMAT_R16G16B16A16_SFLOAT *
      // VK_FORMAT_BC7_UNORM_BLOCK = 145, -> VK_FORMAT_R8G8B8A8_UNORM
      // VK_FORMAT_BC7_SRGB_BLOCK = 146, -> VK_FORMAT_R8G8B8A8_UNORM

      // VK_FORMAT_R16G16B16A16_SFLOAT Formats
      if (in_format == VK_FORMAT_BC6H_UFLOAT_BLOCK || in_format == VK_FORMAT_BC6H_SFLOAT_BLOCK) {
         return VK_FORMAT_R16G16B16A16_SFLOAT;
      }
      // VK_FORMAT_R8G8B8A8_SNORM Formats
      if (in_format == VK_FORMAT_BC4_SNORM_BLOCK || in_format == VK_FORMAT_BC5_SNORM_BLOCK) {
         return VK_FORMAT_R8G8B8A8_SNORM;
      }
      return VK_FORMAT_R8G8B8A8_UNORM;
   }
   return in_format;
}

static VkFormat unwrap_vk_format(struct wrapper_device* device, VkFormat in_format) {
   if (!device) {
      WLOGE("unwrap_vk_format: null device");
      return in_format;
   }
   // Replace BCn formats with R8G8B8A8_UNORM if they are emulated
   return unwrap_vk_format_physical_device(device->physical, in_format);
}

static bool check_flag(const char* env, bool default_value) {
   const char* value = getenv(env);
   if (!value) return default_value;
   if (strcmp(value, "1") == 0) {
      return true;
   } else if (strcmp(value, "0") == 0) {
      return false;
   } else {
      return default_value;
   }
}

typedef struct {
    uint32_t srcFormat;
    uint32_t srcRowLength;
    uint32_t srcImageHeight;
    int32_t imageOffsetX;
    int32_t imageOffsetY;
    uint32_t imageExtentX;
    uint32_t imageExtentY;
    uint32_t srcBufferSize;
} PushConstantData;

typedef struct __attribute__((aligned(16))) ivec2_t_std140 {
    int32_t x;
    int32_t y;
} ivec2_t_std140;

typedef struct __attribute__((aligned(16))) int32_t_std140 {
    int32_t x;
} int32_t_std140;

// This struct will be mapped directly to the GPU buffer.
// The `alignas(16)` on the struct itself ensures it starts on a 16-byte boundary.
typedef struct __attribute__((aligned(16))) Bc7Constants {
   int32_t_std140 weight_table2[16];
   int32_t_std140 weight_table3[16];
   int32_t_std140 weight_table4[16];

   int32_t_std140 partition_table3[64];
   int32_t_std140 partition_table2[64];
   int32_t_std140 anchor_table2[64];
   ivec2_t_std140 anchor_table3[64];
} Bc7Constants;

// Macro to pack 16 2-bit values into a 32-bit integer
#define P3(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
    (((a) << 0) | ((b) << 2) | ((c) << 4) | ((d) << 6) | \
    ((e) << 8) | ((f) << 10) | ((g) << 12) | ((h) << 14) | \
    ((i) << 16) | ((j) << 18) | ((k) << 20) | ((l) << 22) | \
    ((m) << 24) | ((n) << 26) | ((o) << 28) | ((p) << 30))

// Macro to pack 16 1-bit values into a 32-bit integer
#define P2(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) \
    (((a) << 0) | ((b) << 1) | ((c) << 2) | ((d) << 3) | \
    ((e) << 4) | ((f) << 5) | ((g) << 6) | ((h) << 7) | \
    ((i) << 8) | ((j) << 9) | ((k) << 10) | ((l) << 11) | \
    ((m) << 12) | ((n) << 13) | ((o) << 14) | ((p) << 15))

#pragma GCC diagnostic ignored "-Wmissing-braces"
// --- Weight Tables ---
static const int32_t_std140 BC7_WEIGHT_TABLE2_DATA[16] = {0, 21, 43, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const int32_t_std140 BC7_WEIGHT_TABLE3_DATA[16] = {0, 9, 18, 27, 37, 46, 55, 64, 0, 0, 0, 0, 0, 0, 0, 0};
static const int32_t_std140 BC7_WEIGHT_TABLE4_DATA[16] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

// --- Partition Tables ---
static const int32_t_std140 BC7_PARTITION_TABLE2_DATA[64] = {
    P2(0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1), P2(0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1),
    P2(0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1), P2(0,0,0,1,0,0,1,1,0,0,1,1,0,1,1,1),
    P2(0,0,0,0,0,0,0,1,0,0,0,1,0,0,1,1), P2(0,0,1,1,0,1,1,1,0,1,1,1,1,1,1,1),
    P2(0,0,0,1,0,0,1,1,0,1,1,1,1,1,1,1), P2(0,0,0,0,0,0,0,1,0,0,1,1,0,1,1,1),
    P2(0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,1), P2(0,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1),
    P2(0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,1), P2(0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1),
    P2(0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1), P2(0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1),
    P2(0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1), P2(0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1),
    P2(0,0,0,0,1,0,0,0,1,1,1,0,1,1,1,1), P2(0,1,1,1,0,0,0,1,0,0,0,0,0,0,0,0),
    P2(0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,0), P2(0,1,1,1,0,0,1,1,0,0,0,1,0,0,0,0),
    P2(0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,0), P2(0,0,0,0,1,0,0,0,1,1,0,0,1,1,1,0),
    P2(0,0,0,0,0,0,0,0,1,0,0,0,1,1,0,0), P2(0,1,1,1,0,0,1,1,0,0,1,1,0,0,0,1),
    P2(0,0,1,1,0,0,0,1,0,0,0,1,0,0,0,0), P2(0,0,0,0,1,0,0,0,1,0,0,0,1,1,0,0),
    P2(0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0), P2(0,0,1,1,0,1,1,0,0,1,1,0,1,1,0,0),
    P2(0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0), P2(0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0),
    P2(0,1,1,1,0,0,0,1,1,0,0,0,1,1,1,0), P2(0,0,1,1,1,0,0,1,1,0,0,1,1,1,0,0),
    P2(0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1), P2(0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1),
    P2(0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0), P2(0,0,1,1,0,0,1,1,1,1,0,0,1,1,0,0),
    P2(0,0,1,1,1,1,0,0,0,0,1,1,1,1,0,0), P2(0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0),
    P2(0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1), P2(0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1),
    P2(0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,0), P2(0,0,0,1,0,0,1,1,1,1,0,0,1,0,0,0),
    P2(0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0), P2(0,0,1,1,1,0,1,1,1,1,0,1,1,1,0,0),
    P2(0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0), P2(0,0,1,1,1,1,0,0,1,1,0,0,0,0,1,1),
    P2(0,1,1,0,0,1,1,0,1,0,0,1,1,0,0,1), P2(0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0),
    P2(0,1,0,0,1,1,1,0,0,1,0,0,0,0,0,0), P2(0,0,1,0,0,1,1,1,0,0,1,0,0,0,0,0),
    P2(0,0,0,0,0,0,1,0,0,1,1,1,0,0,1,0), P2(0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,0),
    P2(0,1,1,0,1,1,0,0,1,0,0,1,0,0,1,1), P2(0,0,1,1,0,1,1,0,1,1,0,0,1,0,0,1),
    P2(0,1,1,0,0,0,1,1,1,0,0,1,1,1,0,0), P2(0,0,1,1,1,0,0,1,1,1,0,0,0,1,1,0),
    P2(0,1,1,0,1,1,0,0,1,1,0,0,1,0,0,1), P2(0,1,1,0,0,0,1,1,0,0,1,1,1,0,0,1),
    P2(0,1,1,1,1,1,1,0,1,0,0,0,0,0,0,1), P2(0,0,0,1,1,0,0,0,1,1,1,0,0,1,1,1),
    P2(0,0,0,0,1,1,1,1,0,0,1,1,0,0,1,1), P2(0,0,1,1,0,0,1,1,1,1,1,1,0,0,0,0),
    P2(0,0,1,0,0,0,1,0,1,1,1,0,1,1,1,0), P2(0,1,0,0,0,1,0,0,0,1,1,1,0,1,1,1)
};
static const int32_t_std140 BC7_PARTITION_TABLE3_DATA[64] = {
    P3(0,0,1,1,0,0,1,1,0,2,2,1,2,2,2,2), P3(0,0,0,1,0,0,1,1,2,2,1,1,2,2,2,1),
    P3(0,0,0,0,2,0,0,1,2,2,1,1,2,2,1,1), P3(0,2,2,2,0,0,2,2,0,0,1,1,0,1,1,1),
    P3(0,0,0,0,0,0,0,0,1,1,2,2,1,1,2,2), P3(0,0,1,1,0,0,1,1,0,0,2,2,0,0,2,2),
    P3(0,0,2,2,0,0,2,2,1,1,1,1,1,1,1,1), P3(0,0,1,1,0,0,1,1,2,2,1,1,2,2,1,1),
    P3(0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2), P3(0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2),
    P3(0,0,0,0,1,1,1,1,2,2,2,2,2,2,2,2), P3(0,0,1,2,0,0,1,2,0,0,1,2,0,0,1,2),
    P3(0,1,1,2,0,1,1,2,0,1,1,2,0,1,1,2), P3(0,1,2,2,0,1,2,2,0,1,2,2,0,1,2,2),
    P3(0,0,1,1,0,1,1,2,1,1,2,2,1,2,2,2), P3(0,0,1,1,2,0,0,1,2,2,0,0,2,2,2,0),
    P3(0,0,0,1,0,0,1,1,0,1,1,2,1,1,2,2), P3(0,1,1,1,0,0,1,1,2,0,0,1,2,2,0,0),
    P3(0,0,0,0,1,1,2,2,1,1,2,2,1,1,2,2), P3(0,0,2,2,0,0,2,2,0,0,2,2,1,1,1,1),
    P3(0,1,1,1,0,1,1,1,0,2,2,2,0,2,2,2), P3(0,0,0,1,0,0,0,1,2,2,2,1,2,2,2,1),
    P3(0,0,0,0,0,0,1,1,0,1,2,2,0,1,2,2), P3(0,0,0,0,1,1,0,0,2,2,1,0,2,2,1,0),
    P3(0,1,2,2,0,1,2,2,0,0,1,1,0,0,0,0), P3(0,0,1,2,0,0,1,2,1,1,2,2,2,2,2,2),
    P3(0,1,1,0,1,2,2,1,1,2,2,1,0,1,1,0), P3(0,0,0,0,0,1,1,0,1,2,2,1,1,2,2,1),
    P3(0,0,2,2,1,1,0,2,1,1,0,2,0,0,2,2), P3(0,1,1,0,0,1,1,0,2,0,0,2,2,2,2,2),
    P3(0,0,1,1,0,1,2,2,0,1,2,2,0,0,1,1), P3(0,0,0,0,2,0,0,0,2,2,1,1,2,2,2,1),
    P3(0,0,0,0,0,0,0,2,1,1,2,2,1,2,2,2), P3(0,2,2,2,0,0,2,2,0,0,1,2,0,0,1,1),
    P3(0,0,1,1,0,0,1,2,0,0,2,2,0,2,2,2), P3(0,1,2,0,0,1,2,0,0,1,2,0,0,1,2,0),
    P3(0,0,0,0,1,1,1,1,2,2,2,2,0,0,0,0), P3(0,1,2,0,1,2,0,1,2,0,1,2,0,1,2,0),
    P3(0,1,2,0,2,0,1,2,1,2,0,1,0,1,2,0), P3(0,0,1,1,2,2,0,0,1,1,2,2,0,0,1,1),
    P3(0,0,1,1,1,1,2,2,2,2,0,0,0,0,1,1), P3(0,1,0,1,0,1,0,1,2,2,2,2,2,2,2,2),
    P3(0,0,0,0,0,0,0,0,2,1,2,1,2,1,2,1), P3(0,0,2,2,1,1,2,2,0,0,2,2,1,1,2,2),
    P3(0,0,2,2,0,0,1,1,0,0,2,2,0,0,1,1), P3(0,2,2,0,1,2,2,1,0,2,2,0,1,2,2,1),
    P3(0,1,0,1,2,2,2,2,2,2,2,2,0,1,0,1), P3(0,0,0,0,2,1,2,1,2,1,2,1,2,1,2,1),
    P3(0,1,0,1,0,1,0,1,0,1,0,1,2,2,2,2), P3(0,2,2,2,0,1,1,1,0,2,2,2,0,1,1,1),
    P3(0,0,0,2,1,1,1,2,0,0,0,2,1,1,1,2), P3(0,0,0,0,2,1,1,2,2,1,1,2,2,1,1,2),
    P3(0,2,2,2,0,1,1,1,0,1,1,1,0,2,2,2), P3(0,0,0,2,1,1,1,2,1,1,1,2,0,0,0,2),
    P3(0,1,1,0,0,1,1,0,0,1,1,0,2,2,2,2), P3(0,0,0,0,0,0,0,0,2,1,1,2,2,1,1,2),
    P3(0,1,1,0,0,1,1,0,2,2,2,2,2,2,2,2), P3(0,0,2,2,0,0,1,1,0,0,1,1,0,0,2,2),
    P3(0,0,2,2,1,1,2,2,1,1,2,2,0,0,2,2), P3(0,0,0,0,0,0,0,0,0,0,0,0,2,1,1,2),
    P3(0,0,0,2,0,0,0,1,0,0,0,2,0,0,0,1), P3(0,2,2,2,1,2,2,2,0,2,2,2,1,2,2,2),
    P3(0,1,0,1,2,2,2,2,2,2,2,2,2,2,2,2), P3(0,1,1,1,2,0,1,1,2,2,0,1,2,2,2,0)
};

// --- Anchor Tables ---
static const int32_t_std140 BC7_ANCHOR_TABLE2_DATA[64] = {
    15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
    15,2,8,2,2,8,8,15,2,8,2,2,8,8,2,2,
    15,15,6,8,2,8,15,15,2,8,2,2,2,15,15,6,
    6,2,6,8,15,15,2,2,15,15,15,15,15,2,2,15
};
static const ivec2_t_std140 BC7_ANCHOR_TABLE3_DATA[64] = {
    {3,15},{3,8},{15,8},{15,3},{8,15},{3,15},{15,3},{15,8},
    {8,15},{8,15},{6,15},{6,15},{6,15},{5,15},{3,15},{3,8},
    {3,15},{3,8},{8,15},{15,3},{3,15},{3,8},{6,15},{10,8},
    {5,3},{8,15},{8,6},{6,10},{8,15},{5,15},{15,10},{15,8},
    {8,15},{15,3},{3,15},{5,10},{6,10},{10,8},{8,9},{15,10},
    {15,6},{3,15},{15,8},{5,15},{15,3},{15,6},{15,6},{15,8},
    {3,15},{15,3},{5,15},{5,15},{5,15},{8,15},{5,15},{10,15},
    {5,15},{10,15},{8,15},{13,15},{15,3},{12,15},{3,15},{3,8}
};

static void populate_bc7_decoding_constants(Bc7Constants* constants) {
    memcpy(constants->weight_table2,     BC7_WEIGHT_TABLE2_DATA,     sizeof(BC7_WEIGHT_TABLE2_DATA));
    memcpy(constants->weight_table3,     BC7_WEIGHT_TABLE3_DATA,     sizeof(BC7_WEIGHT_TABLE3_DATA));
    memcpy(constants->weight_table4,     BC7_WEIGHT_TABLE4_DATA,     sizeof(BC7_WEIGHT_TABLE4_DATA));
    memcpy(constants->partition_table2,  BC7_PARTITION_TABLE2_DATA,  sizeof(BC7_PARTITION_TABLE2_DATA));
    memcpy(constants->partition_table3,  BC7_PARTITION_TABLE3_DATA,  sizeof(BC7_PARTITION_TABLE3_DATA));
    memcpy(constants->anchor_table2,     BC7_ANCHOR_TABLE2_DATA,     sizeof(BC7_ANCHOR_TABLE2_DATA));
    memcpy(constants->anchor_table3,     BC7_ANCHOR_TABLE3_DATA,     sizeof(BC7_ANCHOR_TABLE3_DATA));
}

typedef struct __attribute__((aligned(16))) Bc6Constants {
   int32_t_std140 weight_table3[16];
   int32_t_std140 weight_table4[16];

   int32_t_std140 partition_table2[32];
   int32_t_std140 anchor_table2[32];
} Bc6Constants;


static const int32_t_std140 BC6_WEIGHT_TABLE3_DATA[16] = {0, 9, 18, 27, 37, 46, 55, 64, 0,0,0,0,0,0,0,0};
static const int32_t_std140 BC6_WEIGHT_TABLE4_DATA[16] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

static const int32_t_std140 BC6_PARTITION_TABLE2_DATA[32] = {
    P2(0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1),
    P2(0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1),
    P2(0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1),
    P2(0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1),
    P2(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1),
    P2(0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1),
    P2(0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1),
    P2(0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1),

    P2(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1),
    P2(0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    P2(0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1),
    P2(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1),
    P2(0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    P2(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1),
    P2(0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
    P2(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1),

    P2(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1),
    P2(0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0),
    P2(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0),
    P2(0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0),
    P2(0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0),
    P2(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0),
    P2(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0),
    P2(0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1),

    P2(0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0),
    P2(0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0),
    P2(0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0),
    P2(0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0),
    P2(0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0),
    P2(0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0),
    P2(0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0),
    P2(0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0)
};

static const int32_t_std140 BC6_ANCHOR_TABLE2_DATA[32] = {
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 2, 8, 2, 2, 8, 8, 15,
    2, 8, 2, 2, 8, 8, 2, 2
};

#pragma GCC diagnostic pop

static void populate_bc6_decoding_constants(Bc6Constants* constants) {
    memcpy(constants->weight_table3,     BC6_WEIGHT_TABLE3_DATA,     sizeof(BC6_WEIGHT_TABLE3_DATA));
    memcpy(constants->weight_table4,     BC6_WEIGHT_TABLE4_DATA,     sizeof(BC6_WEIGHT_TABLE4_DATA));
    memcpy(constants->partition_table2,  BC6_PARTITION_TABLE2_DATA,  sizeof(BC6_PARTITION_TABLE2_DATA));
    memcpy(constants->anchor_table2,     BC6_ANCHOR_TABLE2_DATA,     sizeof(BC6_ANCHOR_TABLE2_DATA));
}
