#pragma once

uint32_t make_bcn_masks(const char* flag);

uint32_t get_unsupported_bcn_masks(void);

uint32_t get_watermarked_bcn_masks(void);

uint32_t get_host_decoding_bcn_masks(void);

void initialize_cmd_print_masks(void);

bool use_image_view_mode(void);

bool use_compute_shader_mode(void);