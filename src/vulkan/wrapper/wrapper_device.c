#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "wrapper_trampolines.h"
#include "vk_alloc.h"
#include "vk_common_entrypoints.h"
#include "vk_device.h"
#include "vk_dispatch_table.h"
#include "vk_extensions.h"
#include "vk_queue.h"
#include "vk_util.h"
#include "vk_printers.h"
#include "util/list.h"
#include "util/simple_mtx.h"
#include "vk_meta.h" // For vk_meta_create_image_view
#include "vk_buffer.h"
#include "wrapper_trampolines.h"

#include "bcdec.h"

static const uint32_t s3tc_spv[] = {
#include "s3tc.spv.h"
};

static const uint32_t bc6_spv[] = {
#include "bc6.spv.h"
};

static const uint32_t bc7_spv[] = {
#include "bc7.spv.h"
};


#define CHECK(call) ({ \
      VkResult __result = wrapper_device_trampolines.call; \
      if (__result) { __loge("%s failed with %d", #call, __result); } \
      __result; \
   })

#define CHECKV(call) ({ \
   wrapper_device_trampolines.call; \
})

#define CHECK_W(call) ({ \
   VkResult __result = wrapper_##call; \
   if (__result) { __log("%s failed with %d", #call, __result); } \
   __result; \
})

// A struct to hold the state required by our interceptor
typedef struct InterceptorState {
   VkDevice device;
   VkPipeline pipeline;
   VkPipelineLayout pipelineLayout;
   VkDescriptorSetLayout descriptorSetLayout;
} InterceptorState;

// Initializes the Vulkan objects needed for the compute dispatch.
// Call this after intercepting vkCreateDevice.
static VkResult InterceptorState_Init(InterceptorState* state, VkDevice device, size_t spv_size, const uint32_t* spv_code);

// Cleans up the Vulkan objects.
// Call this before intercepting vkDestroyDevice.
static void InterceptorState_Cleanup(InterceptorState* state);


static InterceptorState g_interceptorState_s3tc = {0};
static InterceptorState g_interceptorState_bc6 = {0};
static InterceptorState g_interceptorState_bc7 = {0};

const struct vk_device_extension_table wrapper_device_extensions =
{
   .KHR_swapchain = true,
   .EXT_swapchain_maintenance1 = true,
   .KHR_swapchain_mutable_format = true,
#ifdef VK_USE_PLATFORM_DISPLAY_KHR
   .EXT_display_control = true,
#endif
   .KHR_present_id = true,
   .KHR_present_wait = true,
   .KHR_dynamic_rendering = true,
   .KHR_incremental_present = true,
   .EXT_map_memory_placed = true,
   .KHR_maintenance4 = true,
   .KHR_map_memory2 = true,
};

const struct vk_device_extension_table wrapper_filter_extensions =
{
   .EXT_hdr_metadata = true,
   .GOOGLE_display_timing = true,
   .KHR_shader_float_controls = true,
   .KHR_shared_presentable_image = true,
   .EXT_image_compression_control_swapchain = true,
};

static void
wrapper_filter_enabled_extensions(const struct wrapper_device *device,
                                  uint32_t *enable_extension_count,
                                  const char **enable_extensions)
{
   for (int idx = 0; idx < VK_DEVICE_EXTENSION_COUNT; idx++) {
      if (!device->vk.enabled_extensions.extensions[idx])
         continue;

      if (!device->physical->base_supported_extensions.extensions[idx])
         continue;

      if (wrapper_device_extensions.extensions[idx])
         continue;

      if (wrapper_filter_extensions.extensions[idx])
         continue;

      enable_extensions[(*enable_extension_count)++] =
         vk_device_extensions[idx].extensionName;
   }
}

static inline void
wrapper_append_required_extensions(const struct vk_device *device,
                                  uint32_t *count,
                                  const char **exts) {
#define REQUIRED_EXTENSION(name) \
   if (!device->enabled_extensions.name && device->physical->supported_extensions.name) { \
      exts[(*count)++] = "VK_" #name; \
   }
   REQUIRED_EXTENSION(KHR_external_fence);
   REQUIRED_EXTENSION(KHR_external_semaphore);
   REQUIRED_EXTENSION(KHR_external_memory);
   REQUIRED_EXTENSION(KHR_external_fence_fd);
   REQUIRED_EXTENSION(KHR_external_semaphore_fd);
   REQUIRED_EXTENSION(KHR_external_memory_fd);
   REQUIRED_EXTENSION(KHR_dedicated_allocation);
   REQUIRED_EXTENSION(EXT_queue_family_foreign);
   REQUIRED_EXTENSION(KHR_maintenance1)
   REQUIRED_EXTENSION(KHR_maintenance2)
   REQUIRED_EXTENSION(KHR_image_format_list)
   REQUIRED_EXTENSION(KHR_timeline_semaphore);
   REQUIRED_EXTENSION(EXT_external_memory_host);
   REQUIRED_EXTENSION(EXT_external_memory_dma_buf);
   REQUIRED_EXTENSION(EXT_image_drm_format_modifier);
   REQUIRED_EXTENSION(ANDROID_external_memory_android_hardware_buffer);
#undef REQUIRED_EXTENSION
}

static VkResult
wrapper_create_device_queue(struct wrapper_device *device,
                            const VkDeviceCreateInfo* pCreateInfo)
{
   const VkDeviceQueueCreateInfo *create_info;
   struct wrapper_queue *queue;
   VkResult result;

   device->queueCount = pCreateInfo->queueCreateInfoCount;
   device->queues = vk_zalloc(&device->vk.alloc,
                           sizeof(queue) * pCreateInfo->queueCreateInfoCount,
                           8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   for (int i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      create_info = &pCreateInfo->pQueueCreateInfos[i];
      for (int j = 0; j < create_info->queueCount; j++) {
         queue = vk_zalloc(&device->vk.alloc, sizeof(*queue), 8,
                           VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
         if (!queue)
            return VK_ERROR_OUT_OF_HOST_MEMORY;

         if (create_info->flags) {
            device->dispatch_table.GetDeviceQueue2(
               device->dispatch_handle,
               &(VkDeviceQueueInfo2) {
                  .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
                  .flags = create_info->flags,
                  .queueFamilyIndex = create_info->queueFamilyIndex,
                  .queueIndex = j,
               },
               &queue->dispatch_handle);;
         } else {
            device->dispatch_table.GetDeviceQueue(
               device->dispatch_handle, create_info->queueFamilyIndex,
               j, &queue->dispatch_handle);
         }
         queue->device = device;
         device->queues[i] = queue;

         result = vk_queue_init(&queue->vk, &device->vk, create_info, j);
         if (result != VK_SUCCESS) {
            vk_free(&device->vk.alloc, queue);
            return result;
         }
      }
   }

   return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_CreateDevice(VkPhysicalDevice physicalDevice,
                     const VkDeviceCreateInfo* pCreateInfo,
                     const VkAllocationCallbacks* pAllocator,
                     VkDevice* pDevice)
{
   __log("wrapper_CreateDevice");
   VK_FROM_HANDLE(wrapper_physical_device, physical_device, physicalDevice);
   const char *wrapper_enable_extensions[VK_DEVICE_EXTENSION_COUNT];
   uint32_t wrapper_enable_extension_count = 0;
   VkDeviceCreateInfo wrapper_create_info = *pCreateInfo;
   struct vk_device_dispatch_table dispatch_table;
   struct wrapper_device *device;
   VkPhysicalDeviceFeatures2 *pdf2;
   VkPhysicalDeviceFeatures *pdf;
   VkResult result;

   device = vk_zalloc2(&physical_device->instance->vk.alloc, pAllocator,
                       sizeof(*device), 8, VK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
   if (!device)
      return vk_error(physical_device, VK_ERROR_OUT_OF_HOST_MEMORY);

   list_inithead(&device->command_buffer_list);
   list_inithead(&device->device_memory_list);

   device->image_map = _mesa_hash_table_u64_create(NULL);
   device->buffer_map = _mesa_hash_table_u64_create(NULL);

   simple_mtx_init(&device->resource_mutex, mtx_plain);
   device->physical = physical_device;

   vk_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wrapper_device_entrypoints, true);
   vk_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wsi_device_entrypoints, false);
   vk_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wrapper_device_trampolines, false);

   result = vk_device_init(&device->vk, &physical_device->vk,
                           &dispatch_table, pCreateInfo, pAllocator);

   if (result != VK_SUCCESS) {
      vk_free2(&physical_device->instance->vk.alloc, pAllocator,
               device);
      return vk_error(physical_device, result);
   }
  
   wrapper_filter_enabled_extensions(device,
      &wrapper_enable_extension_count, wrapper_enable_extensions);
  
   wrapper_append_required_extensions(&device->vk,
      &wrapper_enable_extension_count, wrapper_enable_extensions);

   wrapper_create_info.enabledExtensionCount = wrapper_enable_extension_count;
   wrapper_create_info.ppEnabledExtensionNames = wrapper_enable_extensions;
   
   pdf = (void *)pCreateInfo->pEnabledFeatures;
   pdf2 = __vk_find_struct((void *)pCreateInfo->pNext,
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
    
   if (pdf && pdf->textureCompressionBC) {
      pdf->textureCompressionBC &= 
          physical_device->base_supported_features.textureCompressionBC;
   }
   if (pdf2 && pdf2->features.textureCompressionBC) {
      pdf2->features.textureCompressionBC &=
          physical_device->base_supported_features.textureCompressionBC;
   }
   if (pdf && pdf->multiViewport) {
         pdf->multiViewport &=
            physical_device->base_supported_features.multiViewport;
   }
   if (pdf2 && pdf2->features.multiViewport) {
         pdf2->features.multiViewport &=
            physical_device->base_supported_features.multiViewport;
   }
   if (pdf && pdf->depthClamp) {
        pdf->depthClamp &=
            physical_device->base_supported_features.depthClamp;
   }
   if (pdf2 && pdf2->features.depthClamp) {
        pdf2->features.depthClamp &=
            physical_device->base_supported_features.depthClamp;
   }
   if (pdf && pdf->depthBiasClamp) {
       pdf->depthBiasClamp &=
            physical_device->base_supported_features.depthBiasClamp;
   }
   if (pdf2 && pdf2->features.depthBiasClamp) {
       pdf2->features.depthBiasClamp &=
            physical_device->base_supported_features.depthBiasClamp;
   }
    if (pdf && pdf->fillModeNonSolid) {
       pdf->fillModeNonSolid &=
            physical_device->base_supported_features.fillModeNonSolid;
   }
   if (pdf2 && pdf2->features.fillModeNonSolid) {
       pdf2->features.fillModeNonSolid &=
            physical_device->base_supported_features.fillModeNonSolid;
   }
    if (pdf && pdf->shaderClipDistance) {
       pdf->shaderClipDistance &=
            physical_device->base_supported_features.shaderClipDistance;
   }
   if (pdf2 && pdf2->features.shaderClipDistance) {
       pdf2->features.shaderClipDistance &=
            physical_device->base_supported_features.shaderClipDistance;
   }
   if (pdf && pdf->shaderCullDistance) {
       pdf->shaderCullDistance &=
            physical_device->base_supported_features.shaderCullDistance;
   }
   if (pdf2 && pdf2->features.shaderCullDistance) {
       pdf2->features.shaderCullDistance &=
            physical_device->base_supported_features.shaderCullDistance;
   }
   
   result = physical_device->dispatch_table.CreateDevice(
      physical_device->dispatch_handle, &wrapper_create_info,
         pAllocator, &device->dispatch_handle);

   if (result != VK_SUCCESS) {
      wrapper_DestroyDevice(wrapper_device_to_handle(device),
                            &device->vk.alloc);
      return vk_error(physical_device, result);
   }

   void *gdpa = physical_device->instance->dispatch_table.GetInstanceProcAddr(
      physical_device->instance->dispatch_handle, "vkGetDeviceProcAddr");
   vk_device_dispatch_table_load(&device->dispatch_table, gdpa,
                                 device->dispatch_handle);


   result = InterceptorState_Init(&g_interceptorState_s3tc, wrapper_device_to_handle(device), sizeof(s3tc_spv), s3tc_spv);
   if (result != VK_SUCCESS) {
      __log("Failed to initialize InterceptorState for s3tc");
      return vk_error(physical_device, result);
   }
   result = InterceptorState_Init(&g_interceptorState_bc6, wrapper_device_to_handle(device), sizeof(bc6_spv), bc6_spv);
   if (result != VK_SUCCESS) {
      __log("Failed to initialize InterceptorState for bc6");
      return vk_error(physical_device, result);
   }
   result = InterceptorState_Init(&g_interceptorState_bc7, wrapper_device_to_handle(device), sizeof(bc7_spv), bc7_spv);
   if (result != VK_SUCCESS) {
      __log("Failed to initialize InterceptorState for bc7");
      return vk_error(physical_device, result);
   }

   result = wrapper_create_device_queue(device, pCreateInfo);
   if (result != VK_SUCCESS) {
      wrapper_DestroyDevice(wrapper_device_to_handle(device),
                            &device->vk.alloc);
      return vk_error(physical_device, result);
   }
   
   *pDevice = wrapper_device_to_handle(device);

   return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
wrapper_GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex,
                       uint32_t queueIndex, VkQueue* pQueue) {
   vk_common_GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

VKAPI_ATTR void VKAPI_CALL
wrapper_GetDeviceQueue2(VkDevice _device, const VkDeviceQueueInfo2* pQueueInfo,
                        VkQueue* pQueue) {
   VK_FROM_HANDLE(vk_device, device, _device);

   struct vk_queue *queue = NULL;
   vk_foreach_queue(iter, device) {
      if (iter->queue_family_index == pQueueInfo->queueFamilyIndex &&
          iter->index_in_family == pQueueInfo->queueIndex &&
          iter->flags == pQueueInfo->flags) {
         queue = iter;
         break;
      }
   }

   *pQueue = queue ? vk_queue_to_handle(queue) : VK_NULL_HANDLE;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
wrapper_GetDeviceProcAddr(VkDevice _device, const char* pName) {
   VK_FROM_HANDLE(wrapper_device, device, _device);
   return vk_device_get_proc_addr(&device->vk, pName);
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_QueueSubmit(VkQueue _queue, uint32_t submitCount,
                    const VkSubmitInfo* pSubmits, VkFence fence)
{
   VK_FROM_HANDLE(wrapper_queue, queue, _queue);
   VkSubmitInfo wrapper_submits[submitCount];
   VkCommandBuffer *command_buffers;
   VkResult result;

   for (int i = 0; i < submitCount; i++) {
      const VkSubmitInfo *submit_info = &pSubmits[i];
      command_buffers = malloc(sizeof(VkCommandBuffer) *
         submit_info->commandBufferCount);
      for (int j = 0; j < submit_info->commandBufferCount; j++) {
         VK_FROM_HANDLE(wrapper_command_buffer, wcb,
                        submit_info->pCommandBuffers[j]);
         command_buffers[j] = wcb->dispatch_handle;
      }
      wrapper_submits[i] = pSubmits[i];
      wrapper_submits[i].pCommandBuffers = command_buffers;
   }
   result = queue->device->dispatch_table.QueueSubmit(
      queue->dispatch_handle, submitCount, wrapper_submits, fence);

   for (int i = 0; i < submitCount; i++)
      free((void *)wrapper_submits[i].pCommandBuffers);

   return result;
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_QueueSubmit2(VkQueue _queue, uint32_t submitCount,
                     const VkSubmitInfo2* pSubmits, VkFence fence)
{
   VK_FROM_HANDLE(wrapper_queue, queue, _queue);
   VkSubmitInfo2 wrapper_submits[submitCount];
   VkCommandBufferSubmitInfo *command_buffers;
   VkResult result;

   for (int i = 0; i < submitCount; i++) {
      const VkSubmitInfo2 *submit_info = &pSubmits[i];
      command_buffers = malloc(sizeof(VkCommandBufferSubmitInfo) *
         submit_info->commandBufferInfoCount);
      for (int j = 0; j < submit_info->commandBufferInfoCount; j++) {
         VK_FROM_HANDLE(wrapper_command_buffer, wcb,
                        submit_info->pCommandBufferInfos[j].commandBuffer);
         command_buffers[j] = pSubmits[i].pCommandBufferInfos[j];
         command_buffers[j].commandBuffer = wcb->dispatch_handle;
      }
      wrapper_submits[i] = pSubmits[i];
      wrapper_submits[i].pCommandBufferInfos = command_buffers;
   }
   result = queue->device->dispatch_table.QueueSubmit2(
      queue->dispatch_handle, submitCount, wrapper_submits, fence);

   for (int i = 0; i < submitCount; i++)
      free((void *)wrapper_submits[i].pCommandBufferInfos);

   return result;
}


VKAPI_ATTR void VKAPI_CALL
wrapper_CmdExecuteCommands(VkCommandBuffer commandBuffer,
                           uint32_t commandBufferCount,
                           const VkCommandBuffer* pCommandBuffers)
{
   VK_FROM_HANDLE(wrapper_command_buffer, wcb, commandBuffer);
   VkCommandBuffer command_buffers[commandBufferCount];

   for (int i = 0; i < commandBufferCount; i++) {
      command_buffers[i] =
         wrapper_command_buffer_from_handle(pCommandBuffers[i])->dispatch_handle;
   }
   wcb->device->dispatch_table.CmdExecuteCommands(
      wcb->dispatch_handle, commandBufferCount, command_buffers);
}

static VkResult
wrapper_command_buffer_create(struct wrapper_device *device,
                              VkCommandPool pool,
                              VkCommandBuffer dispatch_handle,
                              VkCommandBuffer *pCommandBuffers) {
   struct wrapper_command_buffer *wcb;
   wcb = vk_object_zalloc(&device->vk, &device->vk.alloc,
                          sizeof(struct wrapper_command_buffer),
                          VK_OBJECT_TYPE_COMMAND_BUFFER);
   if (!wcb)
      return vk_error(&device->vk, VK_ERROR_OUT_OF_HOST_MEMORY);

   wcb->device = device;
   wcb->pool = pool;
   wcb->dispatch_handle = dispatch_handle;
   list_add(&wcb->link, &device->command_buffer_list);

   *pCommandBuffers = wrapper_command_buffer_to_handle(wcb);

   return VK_SUCCESS;
}

static void
wrapper_command_buffer_destroy(struct wrapper_device *device,
                               struct wrapper_command_buffer *wcb) {
   list_del(&wcb->link);
   vk_object_free(&device->vk, &device->vk.alloc, wcb);
}


// Helper function to create and add a pool
static VkResult add_new_temp_pool_to_cmd_buffer(struct wrapper_command_buffer *wcb) {
   struct wrapper_device *device = wcb->device;
   VkDescriptorPool new_pool_handle;
   //  // 5. Create a Descriptor Pool
   //  VkDescriptorPoolSize poolSize = {
   //      .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   //      .descriptorCount = 1024,
   //  };

   //  VkDescriptorPoolCreateInfo poolCreateInfo = {
   //      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
   //      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
   //      .maxSets = 1024,
   //      .poolSizeCount = 1,
   //      .pPoolSizes = &poolSize,
   //  };
   //  result = CHECK(CreateDescriptorPool(__device, &poolCreateInfo, NULL, &state->descriptorPool));
   VkDescriptorPoolSize pool_sizes = {
      .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
      .descriptorCount = 256
   };

   const VkDescriptorPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 256, // A reasonable number for one command buffer's temp usage
      .poolSizeCount = 1,
      .pPoolSizes = &pool_sizes,
   };

   VkResult result = CHECK(CreateDescriptorPool((VkDevice) device, &create_info, NULL, &new_pool_handle));

   if (result == VK_SUCCESS) {
      struct wrapper_cmd_buffer_pool *new_pool_node =
         vk_alloc(&wcb->device->vk.alloc, sizeof(struct wrapper_cmd_buffer_pool), 8,
                  VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
      new_pool_node->pool = new_pool_handle;
      list_addtail(&new_pool_node->link, &wcb->temp_descriptor_pools);
   }
   return result;
}


VKAPI_ATTR VkResult VKAPI_CALL
wrapper_AllocateCommandBuffers(VkDevice _device,
                               const VkCommandBufferAllocateInfo* pAllocateInfo,
                               VkCommandBuffer* pCommandBuffers)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   VkCommandBuffer dispatch_handles[pAllocateInfo->commandBufferCount];
   VkResult result;
   uint32_t i;
   
   result = device->dispatch_table.AllocateCommandBuffers(device->dispatch_handle,
                                                          pAllocateInfo,
                                                          dispatch_handles);
   if (result != VK_SUCCESS)
      return result;

   simple_mtx_lock(&device->resource_mutex);

   for (i = 0; i < pAllocateInfo->commandBufferCount; i++) {
      result = wrapper_command_buffer_create(device,
                                             pAllocateInfo->commandPool,
                                             dispatch_handles[i],
                                             &pCommandBuffers[i]);
      if (result != VK_SUCCESS)
         break;
      
      VK_FROM_HANDLE(wrapper_command_buffer, wcb, pCommandBuffers[i]);
      simple_mtx_init(&wcb->temp_pool_mutex, mtx_plain);
      list_inithead(&wcb->temp_descriptor_pools);
      list_inithead(&wcb->temp_staging_buffers);
   }

   if (result != VK_SUCCESS) {
      __loge("Failed to allocate command buffers: %d", result);
      device->dispatch_table.FreeCommandBuffers(device->dispatch_handle,
                                                pAllocateInfo->commandPool,
                                                pAllocateInfo->commandBufferCount,
                                                dispatch_handles);
      for (int q = 0; q < i; q++) {
         VK_FROM_HANDLE(wrapper_command_buffer, wcb, pCommandBuffers[q]);
         wrapper_command_buffer_destroy(device, wcb);
      }

      for (i = 0; i < pAllocateInfo->commandBufferCount; i++) {
         pCommandBuffers[i] = VK_NULL_HANDLE;
      }
   }

   simple_mtx_unlock(&device->resource_mutex);

   return result;
}


VKAPI_ATTR void VKAPI_CALL
wrapper_FreeCommandBuffers(VkDevice _device,
                           VkCommandPool commandPool,
                           uint32_t commandBufferCount,
                           const VkCommandBuffer* pCommandBuffers)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   VkCommandBuffer dispatch_handles[commandBufferCount];

   simple_mtx_lock(&device->resource_mutex);

   for (int i = 0; i < commandBufferCount; i++) {
      VK_FROM_HANDLE(wrapper_command_buffer, wcb, pCommandBuffers[i]);
      if (!wcb) {
         __loge("wrapper_FreeCommandBuffers: NULL command buffer handle at index %d, continuing...", i);
         continue;
      }
      dispatch_handles[i] = wcb->dispatch_handle;

      // Clean up temporary descriptor pools associated with this command buffer
      if (!list_is_empty(&wcb->temp_descriptor_pools)) {
         list_for_each_entry_safe(struct wrapper_cmd_buffer_pool, pool_node, &wcb->temp_descriptor_pools, link) {
            CHECKV(DestroyDescriptorPool(_device, pool_node->pool, NULL));
            list_del(&pool_node->link);
            vk_free(&wcb->device->vk.alloc, pool_node);
         }
      }
      simple_mtx_destroy(&wcb->temp_pool_mutex);

      // Clean up temporary staging buffers associated with this command buffer
      if (!list_is_empty(&wcb->temp_staging_buffers)) {
         list_for_each_entry_safe(struct wrapper_cmd_buffer_staging_buffer, staging_buf, &wcb->temp_staging_buffers, link) {
            if (staging_buf->buffer != VK_NULL_HANDLE) {
               CHECKV(DestroyBuffer(_device, staging_buf->buffer, NULL));
            }
            if (staging_buf->memory != VK_NULL_HANDLE) {
               CHECKV(FreeMemory(_device, staging_buf->memory, NULL));
            }
            list_del(&staging_buf->link);
            vk_free(&wcb->device->vk.alloc, staging_buf);
         }
      }

      wrapper_command_buffer_destroy(device, wcb);
   }

   simple_mtx_unlock(&device->resource_mutex);

   device->dispatch_table.FreeCommandBuffers(device->dispatch_handle,
                                             commandPool, commandBufferCount,
                                             dispatch_handles);
}

VKAPI_ATTR void VKAPI_CALL
wrapper_DestroyCommandPool(VkDevice _device, VkCommandPool commandPool,
                           const VkAllocationCallbacks* pAllocator)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);

   simple_mtx_lock(&device->resource_mutex);

   list_for_each_entry_safe(struct wrapper_command_buffer, wcb,
                            &device->command_buffer_list, link) {
      if (wcb->pool == commandPool) {
         wrapper_command_buffer_destroy(device, wcb);
      }
   }

   simple_mtx_unlock(&device->resource_mutex);

   device->dispatch_table.DestroyCommandPool(device->dispatch_handle,
                                             commandPool, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
wrapper_DestroyDevice(VkDevice _device, const VkAllocationCallbacks* pAllocator)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);

   simple_mtx_lock(&device->resource_mutex);

   list_for_each_entry_safe(struct wrapper_command_buffer, wcb,
                            &device->command_buffer_list, link) {
      wrapper_command_buffer_destroy(device, wcb);
   }
   list_for_each_entry_safe(struct wrapper_device_memory, mem,
                            &device->device_memory_list, link) {
      wrapper_device_memory_destroy(mem);
   }

   simple_mtx_unlock(&device->resource_mutex);


   hash_table_u64_foreach(device->image_map, entry) {
      struct wrapper_image *obj = (struct wrapper_image *) entry.data;
      wrapper_image_destroy(device, obj, pAllocator);
   }
   _mesa_hash_table_u64_destroy(device->image_map);

   hash_table_u64_foreach(device->buffer_map, entry) {
      struct wrapper_buffer *obj = (struct wrapper_buffer *) entry.data;
      wrapper_buffer_destroy(device, obj, pAllocator);
   }
   _mesa_hash_table_u64_destroy(device->buffer_map);

   list_for_each_entry_safe(struct vk_queue, queue, &device->vk.queues, link) {
      vk_queue_finish(queue);
      vk_free2(&device->vk.alloc, pAllocator, queue);
   }
   if (device->dispatch_handle != VK_NULL_HANDLE) {
      device->dispatch_table.DestroyDevice(device->
         dispatch_handle, pAllocator);
   }
   simple_mtx_destroy(&device->resource_mutex);
   vk_device_finish(&device->vk);
   vk_free2(&device->vk.alloc, pAllocator, device);
}

static uint64_t
unwrap_device_object(VkObjectType objectType,
                     uint64_t objectHandle)
{
   switch(objectType) {
   case VK_OBJECT_TYPE_DEVICE:
      return (uint64_t)(uintptr_t)wrapper_device_from_handle((VkDevice)(uintptr_t)objectHandle)->dispatch_handle;
   case VK_OBJECT_TYPE_QUEUE:
      return (uint64_t)(uintptr_t)wrapper_queue_from_handle((VkQueue)(uintptr_t)objectHandle)->dispatch_handle;
   case VK_OBJECT_TYPE_COMMAND_BUFFER:
      return (uint64_t)(uintptr_t)wrapper_command_buffer_from_handle((VkCommandBuffer)(uintptr_t)objectHandle)->dispatch_handle;
   default:
      return objectHandle;
   }
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_SetPrivateData(VkDevice _device, VkObjectType objectType,
                       uint64_t objectHandle,
                       VkPrivateDataSlot privateDataSlot,
                       uint64_t data) {
   VK_FROM_HANDLE(wrapper_device, device, _device);

   uint64_t object_handle = unwrap_device_object(objectType, objectHandle);
   return device->dispatch_table.SetPrivateData(device->dispatch_handle,
      objectType, object_handle, privateDataSlot, data);
}

VKAPI_ATTR void VKAPI_CALL
wrapper_GetPrivateData(VkDevice _device, VkObjectType objectType,
                       uint64_t objectHandle,
                       VkPrivateDataSlot privateDataSlot,
                       uint64_t* pData) {
   VK_FROM_HANDLE(wrapper_device, device, _device);

   uint64_t object_handle = unwrap_device_object(objectType, objectHandle);
   return device->dispatch_table.GetPrivateData(device->dispatch_handle,
      objectType, object_handle, privateDataSlot, pData);
}


// TODO ---

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_CreateBuffer(
    VkDevice device,
    const VkBufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkBuffer* pBuffer)
{
    VK_FROM_HANDLE(wrapper_device, base, device);
    // Add storage bit to createInfo
    VkBufferCreateInfo _pCreateInfo = *pCreateInfo;
    _pCreateInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    VkResult result = wrapper_device_trampolines.CreateBuffer(device, &_pCreateInfo, pAllocator, pBuffer);
    wrapper_buffer *wbuf = wrapper_buffer_create(base, &_pCreateInfo, pAllocator, *pBuffer);
    if (wbuf == NULL) {
      __log("ERROR: wrapper_buffer_create failed");
      return vk_error(&base->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
    }
    return result;
}


VKAPI_ATTR void VKAPI_CALL
wrapper_DestroyBuffer(VkDevice _device,
                      VkBuffer buffer,
                      const VkAllocationCallbacks* pAllocator)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   // VK_FROM_HANDLE(wrapper_buffer, wbuf, _buffer);

   if (!buffer)
      return;

   wrapper_device_trampolines.DestroyBuffer(_device, buffer, pAllocator);

   struct wrapper_buffer *wbuf = get_wrapper_buffer(device, buffer);
   wrapper_buffer_destroy(device, wbuf, pAllocator);
}


// device->dispatch_table.CmdCopyBufferToImage
#define vk_ _device->dispatch_table.

typedef struct {
    uint32_t srcFormat;
    uint32_t srcRowLength;
    uint32_t srcImageHeight;
    int32_t imageOffsetX;
    int32_t imageOffsetY;
    uint32_t imageExtentX;
    uint32_t imageExtentY;
} PushConstantData;

static VkResult InterceptorState_Init(InterceptorState* state, VkDevice __device, size_t spv_size, const uint32_t* spv_code) {
    VkResult result;
    VK_FROM_HANDLE(wrapper_device, _device, __device);
    VkDevice device = _device->dispatch_handle;
    state->device = device;

    __log("In InterceptorState_Init");

    // 1. Create Descriptor Set Layout
    VkDescriptorSetLayoutBinding setLayoutBinding[2] = {
      {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
      {
        .binding = 1,
#ifdef USE_IMAGE_VIEW_OUTPUT
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
#else
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
#endif
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      }
    };

    VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 2,
        .pBindings = setLayoutBinding,
    };
    result = CHECK(CreateDescriptorSetLayout(__device, &setLayoutCreateInfo, NULL, &state->descriptorSetLayout));
    if (result != VK_SUCCESS) return result;

    // 2. Create Push Constant Range
    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(PushConstantData),
    };

    // 3. Create Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &state->descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };
    result = CHECK(CreatePipelineLayout(__device, &pipelineLayoutCreateInfo, NULL, &state->pipelineLayout));
    if (result != VK_SUCCESS) return result;

    // 4. Create Compute Pipeline
   VkShaderModule computeShaderModule;
   VkShaderModuleCreateInfo shaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spv_size,
      .pCode = (const uint32_t*)spv_code,
   };
   result = CHECK(CreateShaderModule(__device, &shaderModuleCreateInfo, NULL, &computeShaderModule));
   if (result != VK_SUCCESS) return result;
   VkComputePipelineCreateInfo pipelineCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .layout = state->pipelineLayout,
      .stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = computeShaderModule,
            .pName = "main",
         }
   };
   result = CHECK(CreateComputePipelines(__device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &state->pipeline));
   vk_ DestroyShaderModule(device, computeShaderModule, NULL);
   if (result != VK_SUCCESS) return result;

   __log("InterceptorState_Init success");

   return result;
}

// Helper function to find a suitable memory type
static uint32_t findMemoryType(struct wrapper_physical_device* _physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    VkPhysicalDeviceMemoryProperties memProperties;
    _physicalDevice->dispatch_table.GetPhysicalDeviceMemoryProperties(_physicalDevice->dispatch_handle, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    // This is a fatal error, the application should handle it appropriately
    __log("Failed to find suitable memory type!\n");
    return 0;
}

// C implementation of the createBuffer helper function
static VkResult CreateStagingBuffer(
    struct wrapper_device* _device,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer* buffer,
    VkDeviceMemory* bufferMemory)
{
    // VK_FROM_HANDLE(wrapper_device, _device, __device);
    VkDevice device = _device->dispatch_handle;
    struct wrapper_physical_device* _physicalDevice = _device->physical;
    VkPhysicalDevice physicalDevice = _physicalDevice->dispatch_handle;

    VkResult result;
    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = CHECK(CreateBuffer((VkDevice) _device, &bufferInfo, NULL, buffer));
    if (result != VK_SUCCESS)
        return result;

    VkMemoryRequirements memRequirements;
    vk_ GetBufferMemoryRequirements(device, *buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(_physicalDevice, memRequirements.memoryTypeBits, properties);

    result = CHECK_W(AllocateMemory((VkDevice) _device, &allocInfo, NULL, bufferMemory));
    if (result != VK_SUCCESS) {
        return result;
    }

    return CHECK(BindBufferMemory((VkDevice) _device, *buffer, *bufferMemory, 0));
}

static int FindComputeQueueFamilies(struct wrapper_physical_device* physical_device) {

    // First, query the number of queue families available.
    uint32_t queueFamilyCount = 0;
    physical_device->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(
      physical_device->dispatch_handle, &queueFamilyCount, NULL);

    VkQueueFamilyProperties queueFamilies[32];
    physical_device->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(
      physical_device->dispatch_handle, &queueFamilyCount, queueFamilies);

    int i = 0;
    for (int i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            return i;
        }
    }
    return -1; // No compute queue family found
}


static void GetQueueForCompute(struct wrapper_device* _device, VkQueue* pQueue) {
   // struct vk_device* device = &_device->vk;
   static int compute_index = -1; 
   if (compute_index < 0) {
      compute_index = FindComputeQueueFamilies(_device->physical);
   }

   if (compute_index < 0) {
      __log("ERROR: No compute queue family found");
      *pQueue = VK_NULL_HANDLE;
      return;
   }

   struct vk_queue *queue = NULL;
   for (int i = 0; i < _device->queueCount; i++) {
      struct wrapper_queue *iter = _device->queues[i];
      if (iter->vk.queue_family_index == compute_index) {
         *pQueue = (VkQueue) iter;
         return;
      }
   }

   *pQueue = VK_NULL_HANDLE;
}

static VkResult SubmitOneTimeCommands(
    struct wrapper_device* _device,
    VkCommandPool commandPool,
    VkQueue queue,
    void (*recordCommands)(VkCommandBuffer, void*),
    void* pUserData
) {
    VkResult result;
    VkDevice device = _device->dispatch_handle;
    // 1. --- Allocate the Command Buffer ---
    VkCommandBufferAllocateInfo allocInfo = { 0 };
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    result = CHECK(AllocateCommandBuffers((VkDevice) _device, &allocInfo, &commandBuffer));
    if (result != VK_SUCCESS) {
        return result;
    }

    // 2. --- Begin Recording ---
    VkCommandBufferBeginInfo beginInfo = { 0 };
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vk_ BeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        return result;
    };

    // 3. --- Record Commands ---
    recordCommands(commandBuffer, pUserData);

    // 4. --- End Recording ---
    result = vk_ EndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        return result;
    }

    // 5. --- Submit to the Queue ---
    VkSubmitInfo submitInfo = { 0 };
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkFence fence;
    VkFenceCreateInfo fenceInfo = { 0 };
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    result = CHECK(CreateFence((VkDevice) _device, &fenceInfo, NULL, &fence));
    if (result != VK_SUCCESS) {
        return result;
    }

    __log("Submitting command buffer to queue %p", queue);
    result = vk_ QueueSubmit(queue, 1, &submitInfo, fence);
    if (result != VK_SUCCESS) {
        return result;
    }

    // 6. --- Wait for Completion ---
    __log("Waiting for fence %p", fence);
    CHECK(WaitForFences((VkDevice) _device, 1, &fence, VK_TRUE, UINT64_MAX));
    if (result != VK_SUCCESS) {
        return result;
    }
    __log("Command buffer execution completed");

    // 7. --- Cleanup ---
    vk_ DestroyFence(_device->dispatch_handle, fence, NULL);
    vk_ FreeCommandBuffers(_device->dispatch_handle, commandPool, 1, &commandBuffer);
    return VK_SUCCESS;
}

struct CmdComputeShaderForDecompressionArgs {
    struct wrapper_device* _device;
    struct wrapper_image* wimg;
    VkBuffer srcBuffer;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    VkBuffer stagingBuffer;
    const VkBufferImageCopy* region;
    struct InterceptorState* state;
};

static void CmdComputeShaderForDecompression(
    struct wrapper_command_buffer* _commandBuffer,
    struct CmdComputeShaderForDecompressionArgs* pArgs)
{
   struct wrapper_device* _device = pArgs->_device;
   struct wrapper_image* wimg = pArgs->wimg;
   VkBuffer srcBuffer = pArgs->srcBuffer;
#ifdef USE_IMAGE_VIEW_OUTPUT
   VkImageLayout dstImageLayout = pArgs->dstImageLayout;
#else
   VkBuffer stagingBuffer = pArgs->stagingBuffer;
#endif
   const VkBufferImageCopy* region = pArgs->region;
   VkImage dstImage = wimg->dispatch_handle;
   struct InterceptorState* state = pArgs->state;
   VkCommandBuffer commandBuffer = _commandBuffer->dispatch_handle;
   VkResult result;

   __log("CmdComputeShaderForDecompression: srcBuffer = %p, dstImage = %p", srcBuffer, dstImage);

#ifdef USE_IMAGE_VIEW_OUTPUT
   // If using image view output, we need to transition the image to the appropriate layout
   {   
      VkImageMemoryBarrier imageBarrier = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .srcAccessMask = VK_ACCESS_NONE,
         .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
         .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Assuming the image is in transfer destination layout
         .newLayout = VK_IMAGE_LAYOUT_GENERAL,
         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .image = dstImage,
         .subresourceRange = {
            .aspectMask = region->imageSubresource.aspectMask,
            .baseMipLevel = region->imageSubresource.mipLevel,
            .levelCount = 1,
            .baseArrayLayer = region->imageSubresource.baseArrayLayer,
            .layerCount = region->imageSubresource.layerCount,
         }
      };

      vk_ CmdPipelineBarrier(commandBuffer,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 0, NULL, 0, NULL, 1, &imageBarrier);
   }
#endif

   // --- 2. Allocate Descriptor Set ---
   // Use the descriptor pool from this command buffer
   simple_mtx_lock(&_commandBuffer->temp_pool_mutex);

   VkDescriptorSet descriptorSet;

   if (list_is_empty(&_commandBuffer->temp_descriptor_pools)) {
      // The current pool is full or fragmented, create a new one for this command buffer
      __loge("Descriptor pool is empty, creating a new one for command buffer %p", _commandBuffer);
      add_new_temp_pool_to_cmd_buffer(_commandBuffer);
   }

   struct wrapper_cmd_buffer_pool *last_pool_node =
      list_last_entry(&_commandBuffer->temp_descriptor_pools, struct wrapper_cmd_buffer_pool, link);

   VkDescriptorSetAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = last_pool_node->pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &state->descriptorSetLayout,
   };

   result = CHECK(AllocateDescriptorSets((VkDevice) _device, &allocInfo, &descriptorSet));
   if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
      // The current pool is full or fragmented, create a new one for this command buffer
      __loge("Descriptor pool exhausted, creating a new one for command buffer %p", _commandBuffer);
      add_new_temp_pool_to_cmd_buffer(_commandBuffer);
      last_pool_node = list_last_entry(&_commandBuffer->temp_descriptor_pools, struct wrapper_cmd_buffer_pool, link);
      allocInfo.descriptorPool = last_pool_node->pool;
      result = CHECK(AllocateDescriptorSets((VkDevice) _device, &allocInfo, &descriptorSet));
   }
   
   simple_mtx_unlock(&_commandBuffer->temp_pool_mutex);

   // If it still fails, there's a bigger problem.
   if (result != VK_SUCCESS) {
      __loge("Failed to allocate descriptor set: %d", result);
      return;
   }


   VkDescriptorBufferInfo srcBufferInfo = {
      .buffer = srcBuffer,
      .offset = region->bufferOffset,
      .range = VK_WHOLE_SIZE
   };

#ifdef USE_IMAGE_VIEW_OUTPUT
   VkImageViewCreateInfo viewCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = dstImage,
      // .viewType = wimg->vk.view_type, // Assuming the image is 2D, you can set this to VK_IMAGE_VIEW_TYPE_2D
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = unwrap_vk_format(_device, wimg->original_format),
      .subresourceRange = {
         .aspectMask = region->imageSubresource.aspectMask,
         .baseMipLevel = region->imageSubresource.mipLevel,
         .levelCount = 1,
         .baseArrayLayer = region->imageSubresource.baseArrayLayer,
         .layerCount = region->imageSubresource.layerCount,
      },
   };
    
   VkImageView dstImageView;
   result = CHECK(CreateImageView((VkDevice) _device, &viewCreateInfo, NULL, &dstImageView));

   VkDescriptorType dstDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
   VkDescriptorImageInfo dstImageInfo = {
      .imageView = dstImageView,
      .imageLayout = VK_IMAGE_LAYOUT_GENERAL, // Ensure the image is in the correct layout
   };
#else
   VkDescriptorType dstDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
   VkDescriptorBufferInfo dstBufferInfo = {
      .buffer = stagingBuffer,
      .offset = 0,
      .range = VK_WHOLE_SIZE
   };
#endif

   VkWriteDescriptorSet writeSet[2] = {
         {
               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
               .dstSet = descriptorSet,
               .dstBinding = 0,
               .descriptorCount = 1,
               .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
               .pBufferInfo = &srcBufferInfo,
         },
         {
               .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
               .dstSet = descriptorSet,
               .dstBinding = 1,
               .descriptorCount = 1,
               .descriptorType = dstDescriptorType,
               #ifdef USE_IMAGE_VIEW_OUTPUT
               .pImageInfo = &dstImageInfo,
               #else
               .pBufferInfo = &dstBufferInfo,
               #endif
         }
   };

   vk_ UpdateDescriptorSets(state->device, 2, writeSet, 0, NULL);

   vk_ CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, state->pipeline);

   // --- 3. Bind, Push, Dispatch ---
   vk_ CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           state->pipelineLayout, 0, 1, &descriptorSet, 0, NULL);        

   PushConstantData pushConstants = {
         .srcFormat = wimg->original_format,
         .srcRowLength = region->bufferRowLength,
         .srcImageHeight = region->bufferImageHeight,
         .imageOffsetX = region->imageOffset.x,
         .imageOffsetY = region->imageOffset.y,
         .imageExtentX = region->imageExtent.width,
         .imageExtentY = region->imageExtent.height
   };

   vk_ CmdPushConstants(commandBuffer, state->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(PushConstantData), &pushConstants);

   uint32_t groupCountX = (region->imageExtent.width + 7) / 8;
   uint32_t groupCountY = (region->imageExtent.height + 7) / 8;
   vk_ CmdDispatch(commandBuffer, groupCountX, groupCountY, 1);


#ifdef USE_IMAGE_VIEW_OUTPUT
   // If using image view output, we need to transition the image back to the destination layout
   {   
      VkImageMemoryBarrier imageBarrier = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
         .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
         .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT,
         .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
         .newLayout = dstImageLayout,
         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .image = dstImage,
         .subresourceRange = {
            .aspectMask = region->imageSubresource.aspectMask,
            .baseMipLevel = region->imageSubresource.mipLevel,
            .levelCount = 1,
            .baseArrayLayer = region->imageSubresource.baseArrayLayer,
            .layerCount = region->imageSubresource.layerCount,
         }
      };

      vk_ CmdPipelineBarrier(commandBuffer,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 0, NULL, 0, NULL, 1, &imageBarrier);
   }
#endif
}


static void BCnDecompression(VkFormat format,
      void* mappedSrcBase,
      void* mappedDst,
      const VkBufferImageCopy* regions) {

   const uint8_t* compressedData = (const uint8_t*)mappedSrcBase + (regions ? regions->bufferOffset : 0);

   // Determine the block size and decoding function based on the format
   // The format values are adjusted by subtracting 0x83 (VK_FORMAT_BC1_RGB_UNORM_BLOCK)
   uint32_t format_id = format - 0x83;
   uint32_t block_size = 16;
   if (format_id < 4) {
         block_size = 8; // BC1
   }
   if (format_id == 8 || format_id == 9) {
         block_size = 8; // BC4
   }

   int height = regions->imageExtent.height;
   int width = regions->imageExtent.width;

   // Loop over the image in 4x4 blocks
   for (int y = regions->imageOffset.y; y < height; y += 4) {
      for (int x = regions->imageOffset.x; x < width; x += 4) {

         // Calculate pointer to the destination 4x4 block
         void *dstPixelBlock =
                  (uint8_t *) mappedDst + (y * width * 4) + (x * 4);
         int pitch = 4 * regions->bufferRowLength;

         switch (format_id) {
            case 0: // BC1_RGB_UNORM_BLOCK
            case 1: // BC1_RGB_SRGB_BLOCK
            case 2: // BC1_RGBA_UNORM_BLOCK
            case 3: // BC1_RGBA_SRGB_BLOCK
                  bcdec_bc1(compressedData, dstPixelBlock, pitch);
                  break;

            case 4: // BC2_UNORM_BLOCK
            case 5: // BC2_SRGB_BLOCK
                  bcdec_bc2(compressedData, dstPixelBlock, pitch);
                  break;

            case 6: // BC3_UNORM_BLOCK
            case 7: // BC3_SRGB_BLOCK
                  bcdec_bc3(compressedData, dstPixelBlock, pitch);
                  break;

            case 8: // BC4_UNORM_BLOCK
            case 9: // BC4_SNORM_BLOCK
                  bcdec_bc4(compressedData, dstPixelBlock,
                                 pitch, format_id == 9);
                  break;

            case 10: // BC5_UNORM_BLOCK
            case 11: // BC5_SNORM_BLOCK
                  bcdec_bc5(compressedData, dstPixelBlock,
                                 pitch, format_id == 11);
                  break;

            default:
                  // Unknown/unsupported format, do nothing.
                  __log("ERROR: Unsupported BCn format %d", format_id);
                  break;
         }

         // Advance the source pointer to the next block
         compressedData += block_size;
      }
   }
}

// Takes in the srcBuffer and decompresses it into a staging buffer
// Both buffers must be bound to memory and are host visible (for mapping)
// This is the software fallback for BCn decompression that aren't implemented by the compute shader
static void HostSideDecompression(
      struct wrapper_device* _device,
      wrapper_buffer* srcBuffer,
      VkDeviceMemory dstMemory,
      const VkBufferImageCopy* region,
      VkFormat dstFormat
) {
   // Map the source buffer
   void* srcData;
   VkMemoryMapInfoKHR mapInfoSrc = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
      .memory = srcBuffer->memory,
      .offset = srcBuffer->memoryOffset,
      .size = VK_WHOLE_SIZE,
   };
   VkResult result = wrapper_MapMemory2KHR((VkDevice) _device, &mapInfoSrc, &srcData);
   if (result != VK_SUCCESS) {
      __loge("ERROR: Failed to map source buffer memory: %d", result);
      return;
   }

   // Map the destination memory
   void* dstData;
   VkMemoryMapInfoKHR mapInfoDst = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
      .memory = dstMemory,
      .offset = 0, // Assuming dstMemory is large enough to hold the decompressed
      .size = region->imageExtent.width * region->imageExtent.height * 4, // 4 bytes per pixel for RGBA
   };
   result = wrapper_MapMemory2KHR((VkDevice) _device, &mapInfoDst, &dstData);
   if (result != VK_SUCCESS) {
      __loge("ERROR: Failed to map destination memory: %d", result);  
      goto final;
   }

   // Decompress the data from srcData to dstData
   BCnDecompression(
      dstFormat,
      srcData,
      dstData,
      region
   );

   // Upload a magenta color for debugging
   // uint32_t* dstPixels = (uint32_t*) dstData;
   // size_t pixelCount = region->imageExtent.width * region->imageExtent.height;
   // for (size_t i = 0; i < pixelCount; ++i) {
   //    dstPixels[i] = 0xFFFF00FF; // Magenta color in RGBA format
   // }

   wrapper_UnmapMemory((VkDevice) _device, dstMemory);
final:
   wrapper_UnmapMemory((VkDevice) _device, srcBuffer->memory);
}

VKAPI_ATTR void VKAPI_CALL
wrapper_CmdCopyBufferToImage(VkCommandBuffer _commandBuffer,
                      VkBuffer srcBuffer,
                      VkImage dstImage,
                      VkImageLayout dstImageLayout,
                      uint32_t regionCount,
                      const VkBufferImageCopy* pRegions)
{
   VK_FROM_HANDLE(wrapper_command_buffer, wcb, _commandBuffer);
   struct wrapper_device *_device = wcb->device;
   VkCommandBuffer commandBuffer = wcb->dispatch_handle;
   VkDevice device = _device->dispatch_handle;
   VkResult result;

   struct wrapper_image* wimg = get_wrapper_image(_device, dstImage);
   if (!wimg) {
      __loge("ERROR: wrapper_CmdCopyBufferToImage: dstImage not tracked");
      return;
   }

   
   if (!wimg->is_bcn_emulated) {
      wrapper_device_trampolines.CmdCopyBufferToImage(_commandBuffer, srcBuffer, dstImage,
                                                      dstImageLayout, 1, pRegions);
      return;
   }

   // DEBUG: For now, let's just call the original function to verify the layer is working
   // if (true) {
   //   vk_ CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
   //   return;
   // }

   // --- Decompression Path ---
   _Atomic static int count = 0;
   count++;
   __loge("Emulating support for format=%d, count=%d", wimg->original_format, count);

   struct wrapper_buffer* _srcBuffer = get_wrapper_buffer(_device, srcBuffer);
   if (!_srcBuffer) {
      __loge("ERROR: wrapper_CmdCopyBufferToImage: srcBuffer not tracked");
      return;
   }
   
   for (uint32_t i = 0; i < regionCount; ++i) {
      const VkBufferImageCopy* region = &pRegions[i];

      // --- 2. Create resources for this region ---
#ifdef USE_IMAGE_VIEW_OUTPUT

#else
      VkBuffer stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      result = CreateStagingBuffer(
         _device, 
         region->imageExtent.width * region->imageExtent.height * region->imageExtent.depth * 4,
         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
         &stagingBuffer, &stagingBufferMemory);
      if (result != VK_SUCCESS) {
         __loge("ERROR: CreateStagingBuffer failed with %d", result);
         return;
      }
      __log("Created staging buffer: %p, memory: %p", stagingBuffer, stagingBufferMemory);

      add_new_staging_buffer(wcb, stagingBuffer, stagingBufferMemory);
#endif

      struct InterceptorState* state = &g_interceptorState_s3tc;
      // If BC7, use the BC7 shader
      if (wimg->original_format == VK_FORMAT_BC7_UNORM_BLOCK || wimg->original_format == VK_FORMAT_BC7_SRGB_BLOCK) {
         state = &g_interceptorState_bc7;
      } else if (wimg->original_format == VK_FORMAT_BC6H_SFLOAT_BLOCK || wimg->original_format == VK_FORMAT_BC6H_UFLOAT_BLOCK) {
         state = &g_interceptorState_bc6;
      }
      struct CmdComputeShaderForDecompressionArgs args = {
         ._device = _device,
         .wimg = wimg,
         .srcBuffer = srcBuffer,
#ifdef USE_IMAGE_VIEW_OUTPUT
         .dstImage = dstImage,
         .dstImageLayout = dstImageLayout,
#else
         .stagingBuffer = stagingBuffer,
#endif
         .region = region,
         .state = state,
      };
      CmdComputeShaderForDecompression(wcb, &args);

#ifdef USE_IMAGE_VIEW_OUTPUT

#else
      VkBufferMemoryBarrier bufferBarrier = {
         .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
         .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT, // Source access mask for shader write
         .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT, // Destination access mask for transfer read
         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
         .buffer = stagingBuffer,
         .offset = 0,
         .size = VK_WHOLE_SIZE
      };

      vk_ CmdPipelineBarrier(
         commandBuffer,
         // Source stage: The compute shader stage where the buffer is written (or the host copy).
         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
         // Destination stage: The transfer pipeline stage where copies occur.
         VK_PIPELINE_STAGE_TRANSFER_BIT,
         0,
         0, NULL,
         1, &bufferBarrier,
         0, NULL
      );

      VkBufferImageCopy local_region = *region;
      // Adjust the bufferOffset to point to the start of the staging buffer
      local_region.bufferOffset = 0;
      wrapper_device_trampolines.CmdCopyBufferToImage(_commandBuffer, stagingBuffer, dstImage, dstImageLayout, 1, &local_region);
#endif
   }
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_BindBufferMemory(VkDevice device, 
   VkBuffer buffer,
   VkDeviceMemory memory,
   VkDeviceSize memoryOffset) {

   VkBindBufferMemoryInfo bind = {
      .sType         = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
      .buffer        = buffer,
      .memory        = memory,
      .memoryOffset  = memoryOffset,
   };

   return wrapper_BindBufferMemory2(device, 1, &bind);
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_BindBufferMemory2(
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos)
{
   VK_FROM_HANDLE(wrapper_device, _device, device);

   if (bindInfoCount == 0 || pBindInfos == NULL) {
      __loge("ERROR: wrapper_BindBufferMemory2 called with no bind infos");
      return vk_error(&_device->vk, VK_ERROR_INVALID_EXTERNAL_HANDLE);
   }

   // Track all of the bindInfos
   for (uint32_t i = 0; i < bindInfoCount; i++) {
      wrapper_buffer *_buffer = get_wrapper_buffer(_device, pBindInfos[i].buffer);
      if (!_buffer) {
         __loge("ERROR: wrapper_BindBufferMemory2: buffer %p not tracked", pBindInfos[i].buffer);
         return vk_error(&_device->vk, VK_ERROR_INVALID_EXTERNAL_HANDLE);
      }
      _buffer->memory = pBindInfos[i].memory;
      _buffer->memoryOffset = pBindInfos[i].memoryOffset;
   }

   return _device->dispatch_table.BindBufferMemory2(_device->dispatch_handle,
                                                    bindInfoCount, pBindInfos);
}