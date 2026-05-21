#pragma once

#include <sys/stat.h>

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
#include <vulkan/vulkan_core.h>

#include "wrapper_objects.h"
#include "wrapper_log.h"
#include "wrapper_trampolines.h"
#include "wrapper_debug.h"

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

extern const struct vk_instance_extension_table wrapper_instance_extensions;
extern const struct vk_device_extension_table wrapper_device_extensions;
extern const struct vk_device_extension_table wrapper_filter_extensions;

static int FindGraphicsComputeQueueFamilies(struct wrapper_physical_device* physical_device) {
   uint32_t queueFamilyCount = 0;
   wrapper_physical_device_trampolines.GetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice) physical_device, &queueFamilyCount, NULL);
   VkQueueFamilyProperties queueFamilies[32];
   wrapper_physical_device_trampolines.GetPhysicalDeviceQueueFamilyProperties((VkPhysicalDevice) physical_device, &queueFamilyCount, queueFamilies);

   for (int i = 0; i < queueFamilyCount; i++) {
      if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
         return i;
      }
   }
   return -1; // No compute queue family found
}

static bool has_bc1_support(struct wrapper_physical_device* pdevice) {
   return pdevice->bc1_format_properties.optimalTilingFeatures != 0;
}

static bool has_bc4_support(struct wrapper_physical_device* pdevice) {
   return pdevice->bc4_format_properties.optimalTilingFeatures != 0;
}

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

static bool is_bc123_image_format(VkFormat format) {
   switch (format) {
   case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
   case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
   case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
   case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
   case VK_FORMAT_BC2_UNORM_BLOCK:
   case VK_FORMAT_BC2_SRGB_BLOCK:
   case VK_FORMAT_BC3_UNORM_BLOCK:
   case VK_FORMAT_BC3_SRGB_BLOCK:
      return true;
   default:
      return false;
   }
}

static bool is_bc4567_image_format(VkFormat format) {
   switch (format) {
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

static VkFormat unwrap_vk_format_physical_device(struct wrapper_physical_device* pdevice, VkFormat in_format) {
    if (!pdevice) {
        WLOGE("unwrap_vk_format: null pdevice");
        return in_format;
    }
    uint32_t disabled_mask = get_disabled_bcn_masks();
    uint32_t format_id = (uint32_t) in_format - VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    if ((disabled_mask & (1 << format_id)) != 0) {
        return in_format;
    }
    bool use_cpu_bcn = (get_host_decoding_bcn_masks() & (1 << (in_format - 131))) != 0;
    bool use_compute_shader = use_compute_shader_mode() && !use_cpu_bcn;
    bool use_image_view = use_image_view_mode();
    bool use_etc2 = !use_image_view && use_compute_shader;
    // use_etc2 = false;
    
    // Replace BCn formats with R8G8B8A8_UNORM if they are emulated
    if (is_bc123_image_format(in_format) && pdevice->needs_bc1_emulation) {
        // VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131, -> VK_FORMAT_R8G8B8A8_UNORM
        // VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132, -> VK_FORMAT_R8G8B8A8_UNORM
        // VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133, -> VK_FORMAT_R8G8B8A8_UNORM
        // VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134, -> VK_FORMAT_R8G8B8A8_UNORM
        // VK_FORMAT_BC2_UNORM_BLOCK = 135, -> VK_FORMAT_R8G8B8A8_UNORM
        // VK_FORMAT_BC2_SRGB_BLOCK = 136, -> VK_FORMAT_R8G8B8A8_UNORM
        // VK_FORMAT_BC3_UNORM_BLOCK = 137, -> VK_FORMAT_R8G8B8A8_UNORM
        // VK_FORMAT_BC3_SRGB_BLOCK = 138, -> VK_FORMAT_R8G8B8A8_UNORM

        return !use_etc2 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    }
    if (is_bc4567_image_format(in_format) && pdevice->needs_bc4_emulation) {
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
        return !use_etc2 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
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

static VkFormat get_depth_stencil_vk_format(struct wrapper_device* device, VkFormat original_format) {
    switch (device->depth_override_mode) {
    case OVERRIDE_SAFE:
        if (device->supports_d16_unorm_s8_uint &&
            (original_format == VK_FORMAT_D24_UNORM_S8_UINT ||
                original_format == VK_FORMAT_D32_SFLOAT_S8_UINT)) {
            return VK_FORMAT_D16_UNORM_S8_UINT;
        } else if (device->supports_d16_unorm && original_format == VK_FORMAT_D32_SFLOAT) {
            return VK_FORMAT_D16_UNORM;
        }
        break;
    case OVERRIDE_AGGRESSIVE:
        if (device->supports_d16_unorm &&
            (original_format == VK_FORMAT_D32_SFLOAT ||
                original_format == VK_FORMAT_D24_UNORM_S8_UINT ||
                original_format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                original_format == VK_FORMAT_D16_UNORM_S8_UINT)) {
            return VK_FORMAT_D16_UNORM;
        }
        break;
    case OVERRIDE_DISABLED:
    case OVERRIDE_NONE:
        break;
    }

    return original_format;
}

static inline uint32_t get_bc_target_size(struct wrapper_physical_device* pdevice, VkFormat in_format) {
    VkFormat out_format = unwrap_vk_format_physical_device(pdevice, in_format);
    switch (out_format) {
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
        return 4;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return 8;
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
        return 4;
    default:
        WLOGE("Unknown out_format: %d", out_format);
        return 4;
        break;
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
    uint32_t unsupportedBitsBc;
    uint32_t watercoloredBitsBc;
    uint32_t watermarkerSize;
} PushConstantData;

typedef struct {
   int32_t width;
   int32_t height;
} Etc2PushConstantData;

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

void BCnDecompression(VkFormat format,
      void* mappedSrcBase,
      void* mappedDst,
      const VkBufferImageCopy* regions,
      bool is_striped);

bool is_striped(VkFormat format, void* mappedSrcBase, const VkBufferImageCopy* regions);

#define HAS_STENCIL(format) (format == VK_FORMAT_D24_UNORM_S8_UINT || \
   format == VK_FORMAT_D32_SFLOAT_S8_UINT || \
   format == VK_FORMAT_D16_UNORM_S8_UINT)

#ifdef __cplusplus
}
#endif
