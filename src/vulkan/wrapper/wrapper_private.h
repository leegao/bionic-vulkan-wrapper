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

struct wrapper_image {
   struct vk_image vk;

   struct wrapper_device *device;
   VkImage dispatch_handle;

   bool is_bcn_emulated;
   VkFormat original_format;
};

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


#define CREATE_FROM_HANDLE_FUNCTION(wtype, vtype, map) \
static struct wtype * get_##wtype(struct wrapper_device *device, vtype handle) { \
   struct wtype *obj = NULL; \
   simple_mtx_lock(&device->resource_mutex); \
   obj = _mesa_hash_table_u64_search(device->map, (uint64_t) handle); \
   simple_mtx_unlock(&device->resource_mutex); \
   return obj; \
}

CREATE_FROM_HANDLE_FUNCTION(wrapper_image, VkImage, image_map)

#define CREATE_FUNCTION_BODY(wtype, vtype, map, vk_type, init) \
   struct wtype *obj = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(struct wtype), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT); \
   if (!obj) \
      return NULL; \
   init; \
   obj->device = device; \
   obj->dispatch_handle = dispatch_handle; \
   simple_mtx_lock(&device->resource_mutex); \
   _mesa_hash_table_u64_insert(device->map, (uint64_t) dispatch_handle, obj); \
   simple_mtx_unlock(&device->resource_mutex); \
   return obj;

static struct wrapper_image *wrapper_image_create(struct wrapper_device *device,
                     const VkImageCreateInfo *pCreateInfo,
                     const VkAllocationCallbacks *pAllocator,
                     VkImage dispatch_handle)
{
   CREATE_FUNCTION_BODY(wrapper_image, VkImage, image_map,
                        VK_OBJECT_TYPE_IMAGE,
                        vk_image_init(&device->vk, &obj->vk, pCreateInfo));
}

#define CREATE_DESTROY_FUNCTION(wtype, map, destroy) \
static void wtype##_destroy(struct wrapper_device *device, struct wtype *obj, const VkAllocationCallbacks* pAllocator) { \
   simple_mtx_lock(&device->resource_mutex); \
   _mesa_hash_table_u64_remove(device->map, (uint64_t) obj->dispatch_handle); \
   simple_mtx_unlock(&device->resource_mutex); \
   destroy; \
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

   struct wrapper_device *device;
   VkBuffer dispatch_handle;

   VkDeviceMemory memory; // Pointer to the memory allocated by vkAllocateMemory
   VkDeviceSize memoryOffset; // Size of the memory allocated by vkAllocateMemory

   simple_mtx_t resource_mutex;
   struct list_head staging_resources;
   struct list_head temp_descriptor_pools;
   int bcn_id;
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