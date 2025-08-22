#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan_core.h>

void RecordBCnArtifacts(
    struct wrapper_device* device, struct wrapper_image* wimg,
    const VkBufferImageCopy* region, VkBuffer srcBuffer,
    VkBuffer stagingBuffer, int decode_id, bool validate_bcn);

void RecordBCnSrcArtifacts(
    struct wrapper_device* device, VkFormat original_format,
    const VkBufferImageCopy* region, VkBuffer srcBuffer,
    int decode_id);

#ifdef __cplusplus
}
#endif