#ifndef VK_TEXCOMPRESS_BC_H
#define VK_TEXCOMPRESS_BC_H

#include "vk_device.h"
#include "util/simple_mtx.h"

#ifdef __cplusplus
extern "C" {
#endif

// State object to hold the pipeline and layouts for BCn decompression.
struct vk_texcompress_bc_state {
    simple_mtx_t mutex;
    const VkAllocationCallbacks *allocator;
    VkPipelineCache pipeline_cache;

    VkDescriptorSetLayout ds_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
};

struct wrapper_device;

// Initializes the BC decompression state.
VkResult vk_texcompress_bc_init(struct wrapper_device *device,
                                struct vk_texcompress_bc_state **bc_state_out,
                                const VkAllocationCallbacks *allocator,
                                VkPipelineCache pipeline_cache);

// Destroys the BC decompression state and its resources.
void vk_texcompress_bc_finish(struct vk_device *device,
                              struct vk_texcompress_bc_state *bc_state);

// Lazily creates and returns the compute pipeline for decompression.
VkPipeline vk_texcompress_bc_get_decode_pipeline(struct vk_device *device,
                                                 struct vk_texcompress_bc_state *state);

#ifdef __cplusplus
}
#endif

#endif // VK_TEXCOMPRESS_BC_H