#pragma once

bool check_flag(const char* env, bool default_value);

#define CHECK_FLAG(name) ({ \
    static bool __value; \
    static bool __initialized; \
    if (!__initialized) \
        __value = check_flag(name, false); \
    __value; \
})

uint32_t make_bcn_masks(const char* flag);

uint32_t get_unsupported_bcn_masks(void);

uint32_t get_watermarked_bcn_masks(void);

uint32_t get_host_decoding_bcn_masks(void);

uint32_t get_disabled_bcn_masks(void);

uint32_t get_watermark_size(void);

void initialize_cmd_print_masks(void);

bool use_image_view_mode(void);

bool use_compute_shader_mode(void);

bool use_wrapper_trace(void);