#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SpirvCode {
    uint32_t* spirv_code;
    size_t spirv_word_count;
};

int optimize_spirv_for_size(const uint32_t* spirv_binary, size_t spirv_word_count, struct SpirvCode* out);

int lower_eliminate_clip_distance(const uint32_t* spirv_binary, size_t spirv_word_count, struct SpirvCode* out);

#ifdef __cplusplus
}
#endif