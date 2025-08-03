#pragma once

#include "util/simple_mtx.h"
#include "vulkan/runtime/vk_instance.h"
#include "vulkan/runtime/vk_physical_device.h"
#include "vulkan/runtime/vk_device.h"
#include "vulkan/runtime/vk_queue.h"
#include "vulkan/runtime/vk_command_buffer.h"
#include "vulkan/runtime/vk_command_pool.h"
#include "vulkan/runtime/vk_buffer.h"
#include "vulkan/runtime/vk_image.h"
#include "vulkan/runtime/vk_log.h"
#include "vulkan/runtime/vk_render_pass.h"
#include "vulkan/runtime/vk_framebuffer.h"
#include "vulkan/util/vk_dispatch_table.h"
#include "vulkan/wsi/wsi_common.h"

#include "wrapper_trampolines.h"

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
   VkFormatProperties bc1_format_properties;
   VkFormatProperties bc4_format_properties;
   bool needs_bc1_emulation;
   bool needs_bc4_emulation;
   struct wsi_device wsi_device;
   struct wrapper_instance *instance;
   struct vk_features base_supported_features;
   struct vk_device_extension_table base_supported_extensions;
   struct vk_physical_device_dispatch_table dispatch_table;
};

VK_DEFINE_HANDLE_CASTS(wrapper_physical_device, vk.base, VkPhysicalDevice,
                       VK_OBJECT_TYPE_PHYSICAL_DEVICE)

#define COMMON_FIELDS(T) \
   struct wrapper_device *device; \
   T dispatch_handle; \
   simple_mtx_t resource_mutex; \
   struct list_head staging_resources; \
   uint32_t obj_id

struct wrapper_queue {
   struct vk_queue vk;
   COMMON_FIELDS(VkQueue);

   bool is_gfx; // TODO
};

VK_DEFINE_HANDLE_CASTS(wrapper_queue, vk.base, VkQueue,
                       VK_OBJECT_TYPE_QUEUE)
// A struct to hold the state required by our interceptor
typedef struct InterceptorState {
   VkDevice device;
   VkPipeline pipeline;
   VkPipelineLayout pipelineLayout;
   VkDescriptorSetLayout descriptorSetLayout;
   VkBuffer constantsBuffer;
   VkDeviceMemory constantsBufferMemory;
} InterceptorState;

struct wrapper_device {
   struct vk_device vk;

   VkDevice dispatch_handle;
   simple_mtx_t resource_mutex;
   struct list_head command_buffer_list;
   struct list_head device_memory_list;

   struct hash_table_u64* image_map;
   struct hash_table_u64* buffer_map;
   struct hash_table_u64* command_pool_map;

   struct wrapper_physical_device *physical;
   struct vk_device_dispatch_table dispatch_table;

   uint32_t queueCount;
   struct wrapper_queue **queues;
   struct wrapper_queue *graphics_queue;
   uint32_t graphics_queue_idx;

   VkCommandPool computePool;

   InterceptorState s3tc;
   InterceptorState bc6;
   InterceptorState bc7;
};

VK_DEFINE_HANDLE_CASTS(wrapper_device, vk.base, VkDevice,
                       VK_OBJECT_TYPE_DEVICE)

struct wrapper_command_buffer {
   struct vk_command_buffer vk;

   struct list_head link;
   COMMON_FIELDS(VkCommandBuffer);

   VkCommandPool pool;

   // VkCommandBuffer bcnBuffer;
   // VkFence bcnBufferFinished;
   // VkSemaphore bcnBufferFinishedSemaphore;
   uint32_t bcnCommands;
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

typedef struct wrapper_staging_resources {
    struct list_head link_in_parent;
    VkObjectType type;
    bool use_vk_object;
    void* handle;
    // If set, this object must be destroyed before the parent
    // AKA this is set in the parent's staging_resources
    // Can be more (e.g. a VkImage) or less (e.g. a VkCommandBuffer) specific
    void* parent;
    // If set, this object can be destroyed
    // when the fence is ready or is no longer available
    VkFence ready;
} wrapper_staging_resources;

#define TRACK_STAGING(device, type, handle, parent) add_staging_resource_to(device, &parent->staging_resources, VK_OBJECT_TYPE_##type, handle, parent)

void add_staging_resource_to(
    struct wrapper_device *device,
    struct list_head *staging_resources,
    VkObjectType type,
    void* handle,
    void* parent);

void free_staging_resources_if_ready(
    struct wrapper_device *device, 
    void* handle, 
    struct list_head* list,
    bool cleanup_everything);

static void free_staging_resources_final(struct wrapper_device *device, 
                                         void* handle, 
                                         struct list_head* list) {
    return free_staging_resources_if_ready(device, handle, list, true);
}

#define MAKE_PROTOTYPES(wtype, vtype) \
struct wtype * get_##wtype(struct wrapper_device *device, vtype handle); \
struct wtype *wtype##_create(struct wrapper_device *device, const vtype##CreateInfo *pCreateInfo, vtype dispatch_handle); \
void wtype##_destroy(struct wrapper_device *device, struct wtype *obj)

struct wrapper_image {
   struct vk_image vk;

   COMMON_FIELDS(VkImage);

   bool is_bcn_emulated;
   VkFormat original_format;
};

MAKE_PROTOTYPES(wrapper_image, VkImage);

struct wrapper_buffer_descriptor_pool {
    struct list_head link;
    VkDescriptorPool pool;
};

typedef struct wrapper_buffer {
   struct vk_buffer vk;

   COMMON_FIELDS(VkBuffer);

   struct list_head temp_descriptor_pools;
   VkDeviceMemory memory; // Pointer to the memory allocated by vkAllocateMemory
   VkDeviceSize memoryOffset; // Size of the memory allocated by vkAllocateMemory
} wrapper_buffer;

MAKE_PROTOTYPES(wrapper_buffer, VkBuffer);

struct wrapper_command_pool {
   struct vk_command_pool vk;

   COMMON_FIELDS(VkCommandPool);

   uint32_t queue_idx;
};

MAKE_PROTOTYPES(wrapper_command_pool, VkCommandPool);
