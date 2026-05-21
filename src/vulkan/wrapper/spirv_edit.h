#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SpirvCode {
    uint32_t* spirv_code;
    size_t spirv_word_count;
} SpirvCode;

int optimize_spirv_for_size(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out);

int lower_eliminate_clip_distance(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out);

int fix_mali_spec_composite_constants(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out);

int add_optimization_barriers(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out);

void log_disassembly_to_cmd_log(const uint32_t* spirv_binary, size_t spirv_word_count);

void log_disassembly(const uint32_t* spirv_binary, size_t spirv_word_count);

#ifdef __cplusplus
}
#endif
