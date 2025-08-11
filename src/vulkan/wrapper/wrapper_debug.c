#include "wrapper_private.h"
#include "wrapper_log.h"
#include "wrapper_trampolines.h"
#include "wrapper_debug.h"

bool check_flag(const char* env, bool default_value) {
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

#define STATIC_INIT(mask, default) \
    static bool initialized = false; \
    static uint32_t mask = default; \
    if (initialized) return mask; \
    initialized = true

uint32_t get_unsupported_bcn_masks() {
    STATIC_INIT(mask, 0);
    return mask = make_bcn_masks("MASK_BCN");
}

uint32_t get_watermarked_bcn_masks() {
    STATIC_INIT(mask, 0);
    return mask = make_bcn_masks("WATERMARK_BCN");
}

uint32_t get_watermark_size() {
    STATIC_INIT(size, 32);
    const char* mask_bcn = getenv("WATERMARK_SIZE");
    if (!mask_bcn) return size = 32;

    if (strstr(mask_bcn, "XXL")) return size = 256;
    if (strstr(mask_bcn, "XL")) return size = 128;
    if (strstr(mask_bcn, "L")) return size = 64;
    if (strstr(mask_bcn, "M")) return size = 32;
    if (strstr(mask_bcn, "S")) return size = 16;
    if (strstr(mask_bcn, "XS")) return size = 8;
    if (strstr(mask_bcn, "XXS")) return size = 4;
    return size;
}

uint32_t get_host_decoding_bcn_masks() {
    STATIC_INIT(mask, 0);
    return mask = make_bcn_masks("USE_CPU_BCN");
}

uint32_t get_disabled_bcn_masks() {
    STATIC_INIT(mask, 0);
    return mask = make_bcn_masks("DISABLE_BCN");
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

struct wrapper_entry_masks wrapper_printer_masks = { 0 };

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

#define CMD(call) SET_VK_ID_##call##_ON(wrapper_printer_masks)
    if (strstr(mask_bcn, "gfx")) {
        CMD(CreateShaderModule);
        CMD(DestroyShaderModule);
        CMD(CreatePipelineCache);
        CMD(CreateGraphicsPipelines);
        CMD(DestroyPipeline);
        CMD(CreatePipelineLayout);
        CMD(DestroyPipelineLayout);
        CMD(CreateDescriptorSetLayout);
        CMD(DestroyDescriptorSetLayout);
        CMD(CreateFramebuffer);
        CMD(DestroyFramebuffer);
        CMD(CreateRenderPass);
        CMD(DestroyRenderPass);
        CMD(CmdSetViewport);
        CMD(CmdSetScissor);
        CMD(CmdSetLineWidth);
        CMD(CmdSetDepthBias);
        CMD(CmdSetBlendConstants);
    }

    if (strstr(mask_bcn, "mem")) {
        CMD(GetPhysicalDeviceMemoryProperties);
        CMD(AllocateMemory);
        CMD(MapMemory);
        CMD(BindBufferMemory);
        CMD(GetImageMemoryRequirements);
        CMD(BindImageMemory);
        CMD(CreateBuffer);
        CMD(CreateImage);
        CMD(CreateImageView);
        CMD(GetPhysicalDeviceImageFormatProperties2);
        CMD(GetPhysicalDeviceMemoryProperties2);
        CMD(GetMemoryFdKHR);
        CMD(BindBufferMemory2);
        CMD(GetBufferMemoryRequirements2);
        CMD(GetImageMemoryRequirements2);
        CMD(GetAndroidHardwareBufferPropertiesANDROID);
    }

    if (strstr(mask_bcn, "debug1")) {
        CMD(QueueSubmit);
        CMD(CreateFence);
        CMD(DestroyFence);
        CMD(ResetFences);
        CMD(WaitForFences);
        CMD(CreateBuffer);
        CMD(CreateBufferView);
        CMD(CreateImageView);
        CMD(DestroyImageView);
        CMD(CreateShaderModule);
        CMD(CreateComputePipelines);
        CMD(CreatePipelineLayout);
        CMD(CreateDescriptorSetLayout);
        CMD(DestroyDescriptorPool);
        CMD(UpdateDescriptorSets);
        CMD(ResetCommandPool);
        CMD(AllocateCommandBuffers);
        CMD(BeginCommandBuffer);
        CMD(EndCommandBuffer);
        CMD(CmdBindPipeline);
        CMD(CmdBindDescriptorSets);
        CMD(CmdDispatch);
        CMD(CmdCopyBuffer);
        CMD(CmdCopyImage);
        CMD(CmdCopyBufferToImage);
        CMD(CmdPipelineBarrier);
        CMD(CmdPushConstants);
        CMD(BindBufferMemory2);
        CMD(GetBufferMemoryRequirements2);
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


bool use_wrapper_trace() {
    static bool value;
    static bool init;
    if (init)
        return value;
    init = true;
    const char* mask_bcn = getenv("WRAPPER_CMD_LOG_LEVEL");
    if (!mask_bcn) 
        return value = false;

    if (strstr(mask_bcn, "trace")) {
        return value = true;
    }

    return value = false;
}

bool should_log_memory_debug() {
    return CHECK_FLAG("DEBUG_MEMORY");
}
