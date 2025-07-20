#pragma once

#include "wrapper_private.h"
#include "wrapper_log.h"

static uint32_t make_bcn_masks(const char* flag) {
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

static uint32_t get_unsupported_bcn_masks() {
    STATIC_INIT_MASKS(mask);
    return mask = make_bcn_masks("MASK_BCN");
}

static uint32_t get_watermarked_bcn_masks() {
    STATIC_INIT_MASKS(mask);
    return mask = make_bcn_masks("WATERMARK_BCN");
}

static uint32_t get_host_decoding_bcn_masks() {
    STATIC_INIT_MASKS(mask);
    return mask = make_bcn_masks("USE_CPU_BCN");
}
