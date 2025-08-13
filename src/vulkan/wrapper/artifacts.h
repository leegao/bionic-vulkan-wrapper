#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan_core.h>

void RecordBCnArtifacts(struct wrapper_device* device, const VkBufferImageCopy* region, VkBuffer srcBuffer, VkBuffer stagingBuffer, int decode_id);

#ifdef __cplusplus
}
#endif