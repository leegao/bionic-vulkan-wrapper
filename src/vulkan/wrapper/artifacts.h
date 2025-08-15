#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan_core.h>

void RecordBCnArtifacts(
    struct wrapper_device* device, VkFormat original_format,
    const VkBufferImageCopy* region, VkBuffer srcBuffer,
    VkBuffer stagingBuffer, int decode_id, bool validate_bcn);

#ifdef __cplusplus
}
#endif