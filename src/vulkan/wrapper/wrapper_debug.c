#include "wrapper_private.h"
#include "wrapper_log.h"
#include "wrapper_trampolines.h"
#include "wrapper_debug.h"

uint32_t make_bcn_masks(const char* flag) {
    uint32_t mask = 0;
    const char* mask_bcn = getenv(flag);
    if (!mask_bcn) return mask;

    if (strstr(mask_bcn, "all")) return 0xffff;

#define MASK_BIT(format) WLOG("Turning on BCn format " #format " for %s", flag); mask |= 1 << (format - 131)
    if (strstr(mask_bcn, "uncommon")) {
        // 132, 134, 135, 136, 138, 139, 140, 142, 144, 146
        MASK_BIT(132);
        MASK_BIT(134);
        MASK_BIT(135);
        MASK_BIT(136);
        MASK_BIT(138);
        MASK_BIT(139);
        MASK_BIT(140);
        MASK_BIT(142);
        MASK_BIT(144);
        MASK_BIT(146);
    }
    
    if (strstr(mask_bcn, "srgb")) {
        // 132, 134, 135, 136, 138, 146
        MASK_BIT(132);
        MASK_BIT(134);
        MASK_BIT(135);
        MASK_BIT(136);
        MASK_BIT(138);
        MASK_BIT(146);
    }
    if (strstr(mask_bcn, "snorm")) {
        MASK_BIT(140);
        MASK_BIT(142);
    }

    if (strstr(mask_bcn, "bc1")) {
        MASK_BIT(131);
        MASK_BIT(132);
        MASK_BIT(133);
        MASK_BIT(134);
    }

    if (strstr(mask_bcn, "bc2")) {
        MASK_BIT(135);
        MASK_BIT(136);
    }
    
    if (strstr(mask_bcn, "bc3")) {
        MASK_BIT(137);
        MASK_BIT(138);
    }
    if (strstr(mask_bcn, "bc4")) {
        MASK_BIT(139);
        MASK_BIT(140);
    }
    if (strstr(mask_bcn, "bc5")) {
        MASK_BIT(141);
        MASK_BIT(142);
    }
    if (strstr(mask_bcn, "bc6")) {
        MASK_BIT(143);
        MASK_BIT(144);
    }
    if (strstr(mask_bcn, "bc7")) {
        MASK_BIT(135);
        MASK_BIT(136);
    }
#undef MASK_BIT

#define MASK_BIT(format) \
    bool mask_##format = strstr(mask_bcn, #format); \
    if (mask_##format) { \
        WLOG("Turning on BCn format " #format " for %s", flag); \
        mask |= 1 << (format - 131); \
    }

    MASK_BIT(131);
    MASK_BIT(132);
    MASK_BIT(133);
    MASK_BIT(134);
    MASK_BIT(135);
    MASK_BIT(136);
    MASK_BIT(137);
    MASK_BIT(138);
    MASK_BIT(139);
    MASK_BIT(140);
    MASK_BIT(141);
    MASK_BIT(142);
    MASK_BIT(143);
    MASK_BIT(144);
    MASK_BIT(145);
    MASK_BIT(146);
    return mask;
#undef MASK_BIT
}

#define STATIC_INIT_MASKS(mask) \
    static bool initialized = false; \
    static uint32_t mask = 0; \
    if (initialized)  return mask; \
    initialized = true

uint32_t get_unsupported_bcn_masks() {
    STATIC_INIT_MASKS(mask);
    return mask = make_bcn_masks("MASK_BCN");
}

uint32_t get_watermarked_bcn_masks() {
    STATIC_INIT_MASKS(mask);
    return mask = make_bcn_masks("WATERMARK_BCN");
}

uint32_t get_host_decoding_bcn_masks() {
    STATIC_INIT_MASKS(mask);
    return mask = make_bcn_masks("USE_CPU_BCN");
}

static void parse_hex_to_struct(struct wrapper_entry_masks *masks, const char *hex_string) {
    uint64_t *fields[16] = {
        &masks->f0, &masks->f1, &masks->f2, &masks->f3, &masks->f4,
        &masks->f5, &masks->f6, &masks->f7, &masks->f8, &masks->f9, 
        &masks->f10, &masks->f11, &masks->f12, &masks->f13, &masks->f14, &masks->f15
    };
    int len = strlen(hex_string);
    const char *ptr = hex_string + len;

    for (int i = 0; i < 16; ++i) {
        if (ptr <= hex_string) {
            break;
        }

        const char *start = ptr - 16;
        if (start < hex_string) {
            start = hex_string;
        }

        char chunk[17]; // 16 chars + null terminator
        int chunk_len = ptr - start;
        strncpy(chunk, start, chunk_len);
        chunk[chunk_len] = '\0';
        *fields[i] = strtoull(chunk, NULL, 16);
        ptr = start;
    }
}

void initialize_cmd_print_masks() {
    // set the various bits in wrapper_printer_masks
    const char* mask_bcn = getenv("WRAPPER_CMD_LOG_LEVEL");
    if (!mask_bcn) 
        return;

    if (strcmp(mask_bcn, "all") == 0) {
        wrapper_printer_masks.f0 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f1 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f2 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f3 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f4 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f5 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f6 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f7 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f8 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f9 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f10 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f11 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f12 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f13 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f14 = 0xFFFFFFFFFFFFFFFFULL,
        wrapper_printer_masks.f15 = 0xFFFFFFFFFFFFFFFFULL;
        return;
    }

    if (strstr(mask_bcn, "0x")) {
        // This is a 704-bit hex-encoded integer
        parse_hex_to_struct(&wrapper_printer_masks, mask_bcn + 2);
        return;
    }

    if (strstr(mask_bcn, "gfx")) {
        // full mask = 0x1f000f03391c000000000000000
        // f0 0x4000000000000000 = vkCreateShaderModule
        // f0 0x8000000000000000 = vkDestroyShaderModule
        wrapper_printer_masks.f0 |= 0xc000000000000000ULL;
        // f1 0x0000000000000001 = vkCreatePipelineCache
        // f1 0x0000000000000010 = vkCreateGraphicsPipelines
        // f1 0x0000000000000080 = vkDestroyPipeline
        // f1 0x0000000000000100 = vkCreatePipelineLayout
        // f1 0x0000000000000200 = vkDestroyPipelineLayout
        // f1 0x0000000000001000 = vkCreateDescriptorSetLayout
        // f1 0x0000000000002000 = vkDestroyDescriptorSetLayout
        // f1 0x0000000000100000 = vkCreateFramebuffer
        // f1 0x0000000000200000 = vkDestroyFramebuffer
        // f1 0x0000000000400000 = vkCreateRenderPass
        // f1 0x0000000000800000 = vkDestroyRenderPass
        // f1 0x0000001000000000 = vkCmdSetViewport
        // f1 0x0000002000000000 = vkCmdSetScissor
        // f1 0x0000004000000000 = vkCmdSetLineWidth
        // f1 0x0000008000000000 = vkCmdSetDepthBias
        // f1 0x0000010000000000 = vkCmdSetBlendConstants
        wrapper_printer_masks.f1 |= 0x000001f000f03391ULL;
    }

    if (strstr(mask_bcn, "gfx")) {
        // full mask = 0x1f000f03391c000000000000000
        // f0 0x4000000000000000 = vkCreateShaderModule
        // f0 0x8000000000000000 = vkDestroyShaderModule
        wrapper_printer_masks.f0 |= 0xc000000000000000ULL;
        // f1 0x0000000000000001 = vkCreatePipelineCache
        // f1 0x0000000000000010 = vkCreateGraphicsPipelines
        // f1 0x0000000000000080 = vkDestroyPipeline
        // f1 0x0000000000000100 = vkCreatePipelineLayout
        // f1 0x0000000000000200 = vkDestroyPipelineLayout
        // f1 0x0000000000001000 = vkCreateDescriptorSetLayout
        // f1 0x0000000000002000 = vkDestroyDescriptorSetLayout
        // f1 0x0000000000100000 = vkCreateFramebuffer
        // f1 0x0000000000200000 = vkDestroyFramebuffer
        // f1 0x0000000000400000 = vkCreateRenderPass
        // f1 0x0000000000800000 = vkDestroyRenderPass
        // f1 0x0000001000000000 = vkCmdSetViewport
        // f1 0x0000002000000000 = vkCmdSetScissor
        // f1 0x0000004000000000 = vkCmdSetLineWidth
        // f1 0x0000008000000000 = vkCmdSetDepthBias
        // f1 0x0000010000000000 = vkCmdSetBlendConstants
        wrapper_printer_masks.f1 |= 0x000001f000f03391ULL;
    }

    if (strstr(mask_bcn, "mem")) {
        // full mask = 0x800000000000002800000000200000001002200000000000000000000000000000000000012200001c1400080
        // f0 0x0000000000000080 = vkGetPhysicalDeviceMemoryProperties
        // f0 0x0000000000400000 = vkAllocateMemory
        // f0 0x0000000001000000 = vkMapMemory
        // f0 0x0000000040000000 = vkBindBufferMemory
        // f0 0x0000000080000000 = vkGetImageMemoryRequirements
        // f0 0x0000000100000000 = vkBindImageMemory
        // f0 0x0020000000000000 = vkCreateBuffer
        // f0 0x0200000000000000 = vkCreateImage
        // f0 0x1000000000000000 = vkCreateImageView
        wrapper_printer_masks.f0 |= 0x12200001c1400080ULL;
        // f3 0x0000000000020000 = vkGetPhysicalDeviceImageFormatProperties2
        // f3 0x0000000000200000 = vkGetPhysicalDeviceMemoryProperties2
        // f3 0x0000000100000000 = vkGetMemoryFdKHR
        wrapper_printer_masks.f3 |= 0x0000000100220000ULL;
        // f4 0x0000000000000002 = vkBindBufferMemory2
        // f4 0x0000008000000000 = vkGetBufferMemoryRequirements2
        // f4 0x0000020000000000 = vkGetImageMemoryRequirements2
        wrapper_printer_masks.f4 |= 0x0000028000000002ULL;
        // f5 0x0000000800000000 = vkGetAndroidHardwareBufferPropertiesANDROID
        wrapper_printer_masks.f5 |= 0x0000000800000000ULL;
    }

// #define CHECK_CMD_MASK(cmd) \
//     if (strstr(mask_bcn, #cmd)) SET_VK_ID_##cmd##_ON(wrapper_printer_masks);
//     UNROLL_ENTRY_POINTS(CHECK_CMD_MASK)
// #undef CHECK_CMD_MASK
}

static bool g_use_image_view = true;
static bool g_use_compute_shader_mode = true;

bool use_image_view_mode() {
   static bool initialized = false;
   if (initialized) {
      return g_use_image_view;
   }
   initialized = true;

   char* use_image_view_env = getenv("USE_IMAGE_VIEW");
   if (use_image_view_env) {
      if (strcmp(use_image_view_env, "1") == 0) {
         WLOG("Enabling experimental direct imageView mode");
         g_use_image_view = true;
      } else if (strcmp(use_image_view_env, "0") == 0) {
         WLOG("Disabling experimental direct imageView mode");
         g_use_image_view = false;
      }
   }

   return g_use_image_view;
}

bool use_compute_shader_mode() {
   static bool initialized = false;
   if (initialized) {
      return g_use_compute_shader_mode;
   }
   initialized = true;

   bool use_image_view = use_image_view_mode();

   char* env = getenv("USE_COMPUTE_SHADER");
   if (env) {
      if (strcmp(env, "1") == 0) {
         WLOG("Enabling experimental compute shader mode");
         g_use_compute_shader_mode = true;
      } else if (strcmp(env, "0") == 0) {
         WLOG("Disabling experimental compute shader mode");
         g_use_compute_shader_mode = false;
         use_image_view = false;
         g_use_image_view = false;
      }
   }

   if (use_image_view) {
      g_use_compute_shader_mode = true;
      return true;
   }

   return g_use_compute_shader_mode;
}
