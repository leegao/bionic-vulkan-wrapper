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

static const uint32_t s3tc_iv_spv[] = {
#include "s3tc_iv.spv.h"
};

static const uint32_t bc6_iv_spv[] = {
#include "bc6_iv.spv.h"
};

static const uint32_t bc7_iv_spv[] = {
#include "bc7_iv.spv.h"
};

#define CHECK_W(call) ({ \
   VkResult __result = wrapper_##call; \
   if (__result) { WLOGE("%s failed with %d", #call, __result); } \
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
static VkResult InterceptorState_Init(InterceptorState* state, VkDevice device, size_t spv_size, const uint32_t* spv_code, bool use_image_view);

// Cleans up the Vulkan objects.
// Call this before intercepting vkDestroyDevice.
static void InterceptorState_Cleanup(InterceptorState* state);

static bool g_use_image_view = true;
static bool g_use_compute_shader_mode;
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

static bool use_image_view_mode() {
   static bool initialized = false;
   if (initialized) {
      return g_use_image_view;
   }
   initialized = true;

   char* use_image_view_env = getenv("USE_IMAGE_VIEW");
   if (use_image_view_env) {
      if (strcmp(use_image_view_env, "1") == 0) {
         WLOG("Enabling experimental direct imageView mode");
         g_use_image_view = true;
      } else if (strcmp(use_image_view_env, "0") == 0) {
         WLOG("Disabling experimental direct imageView mode");
         g_use_image_view = false;
      }
   }

   return g_use_image_view;
}

static bool use_compute_shader_mode() {
   static bool initialized = false;
   if (initialized) {
      return g_use_compute_shader_mode;
   }
   initialized = true;

   bool use_image_view = use_image_view_mode();

   char* env = getenv("USE_COMPUTE_SHADER");
   if (env) {
      if (strcmp(env, "1") == 0) {
         WLOG("Enabling experimental compute shader mode");
         g_use_compute_shader_mode = true;
      } else if (strcmp(env, "0") == 0) {
         WLOG("Disabling experimental compute shader mode");
         g_use_compute_shader_mode = false;
         use_image_view = false;
         g_use_image_view = false;
      }
   }

   if (use_image_view) {
      g_use_compute_shader_mode = true;
      return true;
   }

   return g_use_compute_shader_mode;
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
   WLOG("wrapper_CreateDevice");
   VK_FROM_HANDLE(wrapper_physical_device, physical_device, physicalDevice);
   const char *wrapper_enable_extensions[VK_DEVICE_EXTENSION_COUNT];
   uint32_t wrapper_enable_extension_count = 0;
   VkDeviceCreateInfo wrapper_create_info = *pCreateInfo;
   struct vk_device_dispatch_table dispatch_table;
   struct wrapper_device *device;
   VkPhysicalDeviceFeatures2 *pdf2;
   VkPhysicalDeviceFeatures *pdf;
   VkResult result;

   WLOGD("wrapper_CreateDevice: Application requested the following features");
   LOG_STRUCT(VkDeviceCreateInfo, pCreateInfo);

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

#define DISABLE_EXT(extension, type, clear) \
   if (!physical_device->base_supported_extensions.extension) { \
      VK_STRUCTURE_TYPE_##type##_cast *ext = (VK_STRUCTURE_TYPE_##type##_cast *) vk_find_struct_const(pCreateInfo, type); \
      if (ext) { \
         WLOG("Faking extension support for " #extension); \
         clear; \
      } \
   }

   DISABLE_EXT(EXT_transform_feedback, PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT, {
      ext->geometryStreams = false;
      ext->transformFeedback = false;
   });
   DISABLE_EXT(EXT_host_query_reset, PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT, {
      ext->hostQueryReset = false;
   });
   DISABLE_EXT(EXT_custom_border_color, PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT, {
      ext->customBorderColors = false;
      ext->customBorderColorWithoutFormat = false;
   });

   result = vk_device_init(&device->vk, &physical_device->vk,
                           &dispatch_table, pCreateInfo, pAllocator);

   if (result != VK_SUCCESS) {
      WLOGE("wrapper_CreateDevice: vk_device_init failed with %d", result);
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
   
#define DISABLE_FEAT(feature) \
   if (!physical_device->base_supported_features.feature) { \
      WLOG("Faking feature support for " #feature); \
   } \
   if (pdf && pdf->feature) { \
      pdf->feature &= physical_device->base_supported_features.feature; \
   } \
   if (pdf2 && pdf2->features.feature) { \
      pdf2->features.feature &= physical_device->base_supported_features.feature; \
   }
   
   DISABLE_FEAT(textureCompressionBC);
   DISABLE_FEAT(multiViewport);
   DISABLE_FEAT(logicOp);
   DISABLE_FEAT(variableMultisampleRate);
   DISABLE_FEAT(fillModeNonSolid);
   DISABLE_FEAT(samplerAnisotropy);
   DISABLE_FEAT(shaderImageGatherExtended);
   DISABLE_FEAT(vertexPipelineStoresAndAtomics);
   DISABLE_FEAT(dualSrcBlend);
   DISABLE_FEAT(multiDrawIndirect);
   DISABLE_FEAT(shaderCullDistance);
   DISABLE_FEAT(shaderClipDistance);
   DISABLE_FEAT(geometryShader);
   DISABLE_FEAT(robustBufferAccess);
   DISABLE_FEAT(tessellationShader);
   DISABLE_FEAT(depthClamp);
   DISABLE_FEAT(depthBiasClamp);
   DISABLE_FEAT(shaderStorageImageExtendedFormats);
   DISABLE_FEAT(shaderStorageImageWriteWithoutFormat);
   DISABLE_FEAT(sampleRateShading);
   DISABLE_FEAT(occlusionQueryPrecise);
   DISABLE_FEAT(independentBlend);
   DISABLE_FEAT(fullDrawIndexUint32);
   DISABLE_FEAT(imageCubeArray);
   DISABLE_FEAT(drawIndirectFirstInstance);
   DISABLE_FEAT(fragmentStoresAndAtomics);
   
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

   // Initialize the BCn interceptor states
   bool use_image_view = use_image_view_mode();
   result = InterceptorState_Init(&g_interceptorState_s3tc, 
      wrapper_device_to_handle(device), 
      use_image_view ? sizeof(s3tc_iv_spv) : sizeof(s3tc_spv), 
      use_image_view ? s3tc_iv_spv : s3tc_spv, 
      use_image_view);
   if (result != VK_SUCCESS) {
      WLOGE("Failed to initialize InterceptorState for s3tc");
      return vk_error(physical_device, result);
   }
   result = InterceptorState_Init(&g_interceptorState_bc6, 
      wrapper_device_to_handle(device), 
      use_image_view ? sizeof(bc6_iv_spv) : sizeof(bc6_spv), 
      use_image_view ? bc6_iv_spv : bc6_spv, 
      use_image_view);
   if (result != VK_SUCCESS) {
      WLOGE("Failed to initialize InterceptorState for bc6");
      return vk_error(physical_device, result);
   }
   result = InterceptorState_Init(&g_interceptorState_bc7, 
      wrapper_device_to_handle(device), 
      use_image_view ? sizeof(bc7_iv_spv) : sizeof(bc7_spv), 
      use_image_view ? bc7_iv_spv : bc7_spv, 
      use_image_view);
   if (result != VK_SUCCESS) {
      WLOGE("Failed to initialize InterceptorState for bc7");
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
   result = CHECK(QueueSubmit(
      _queue, submitCount, wrapper_submits, fence));

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
   result = CHECK(QueueSubmit2(
      _queue, submitCount, wrapper_submits, fence));

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


VKAPI_ATTR VkResult VKAPI_CALL
wrapper_AllocateCommandBuffers(VkDevice _device,
                               const VkCommandBufferAllocateInfo* pAllocateInfo,
                               VkCommandBuffer* pCommandBuffers)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   VkCommandBuffer dispatch_handles[pAllocateInfo->commandBufferCount];
   VkResult result;
   uint32_t i;
   
   result = CHECK(AllocateCommandBuffers(_device, pAllocateInfo, dispatch_handles));
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
   }

   if (result != VK_SUCCESS) {
      WLOGE("Failed to allocate command buffers: %d", result);
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
   if (!device) {
      return;
   }
   VkCommandBuffer dispatch_handles[commandBufferCount];

   simple_mtx_lock(&device->resource_mutex);

   for (int i = 0; i < commandBufferCount; i++) {
      VK_FROM_HANDLE(wrapper_command_buffer, wcb, pCommandBuffers[i]);
      if (!wcb) {
         continue;
      }
      dispatch_handles[i] = wcb->dispatch_handle;
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
      vk_image_destroy(&device->vk, pAllocator, &obj->vk);
   }
   _mesa_hash_table_u64_destroy(device->image_map);


   hash_table_u64_foreach(device->buffer_map, entry) {
      struct wrapper_buffer *obj = (struct wrapper_buffer *) entry.data;
      vk_buffer_destroy(&device->vk, pAllocator, &obj->vk);
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

// Helper function to create and add a pool
static VkResult add_new_temp_pool_to_buffer(struct wrapper_buffer *wbuf) {
   struct wrapper_device *device = wbuf->device;
   VkDescriptorPool new_pool_handle;
   VkDescriptorPoolSize pool_sizes[2] = {
      {
         .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 32 // Sufficient for 32 sets, each needing one
      },
      {
         .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .descriptorCount = 32 // Sufficient for 32 sets, each needing one
      },
   };

   const VkDescriptorPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 32, // A reasonable number for one command buffer's temp usage
      .poolSizeCount = 2,
      .pPoolSizes = pool_sizes,
   };

   VkResult result = CHECK(CreateDescriptorPool((VkDevice) device, &create_info, NULL, &new_pool_handle));
   if (result == VK_SUCCESS) {
      struct wrapper_buffer_descriptor_pool *new_pool_node =
         vk_alloc(&wbuf->device->vk.alloc, sizeof(struct wrapper_buffer_descriptor_pool), 8,
                  VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
      new_pool_node->pool = new_pool_handle;
      list_addtail(&new_pool_node->link, &wbuf->temp_descriptor_pools);
      WLOG("Total of %d temp descriptor pools created for buffer %d (%p)", list_length(&wbuf->temp_descriptor_pools), wbuf->bcn_id, wbuf->dispatch_handle);
   }
   return result;
}

static void free_temp_pool_from_buffer(VkDevice device, struct wrapper_buffer* wbuf) {
   // // Clean up temporary descriptor pools associated with this command buffer
   if (!list_is_empty(&wbuf->temp_descriptor_pools)) {
      list_for_each_entry_safe(struct wrapper_buffer_descriptor_pool, pool_node, &wbuf->temp_descriptor_pools, link) {
         CHECKV(DestroyDescriptorPool(device, pool_node->pool, NULL));
         list_del(&pool_node->link);
         vk_free(&wbuf->device->vk.alloc, pool_node);
      }
   }
}

static void add_new_staging_resource_to_buffer(
   struct wrapper_buffer *wbuf, 
   VkBuffer stagingBuffer, 
   VkDeviceMemory stagingBufferMemory, 
   VkImageView stagingImageView) {
   struct wrapper_buffer_staging_resources *obj =
         vk_alloc(&wbuf->device->vk.alloc, sizeof(struct wrapper_buffer_staging_resources), 8,
                  VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   obj->stagingBuffer = stagingBuffer;
   obj->stagingBufferMemory = stagingBufferMemory;
   obj->stagingImageView = stagingImageView;
   list_addtail(&obj->link, &wbuf->staging_resources);
   WLOG("Total of %d staging resources tracked for buffer %d (%p)", list_length(&wbuf->staging_resources), wbuf->bcn_id, wbuf->dispatch_handle);
}

static void free_staging_resources_from_buffer(VkDevice device, struct wrapper_buffer* wbuf, const VkAllocationCallbacks* pAllocator) {
   // // Clean up temporary descriptor pools associated with this command buffer
   if (!list_is_empty(&wbuf->staging_resources)) {
      WLOG("Releasing %d staging resources for BCn buffer %d", list_length(&wbuf->staging_resources), wbuf->bcn_id);
      list_for_each_entry_safe(struct wrapper_buffer_staging_resources, resource, &wbuf->staging_resources, link) {
         if (resource->stagingBuffer != VK_NULL_HANDLE) {
            CHECKV(DestroyBuffer(device, resource->stagingBuffer, pAllocator));
         }
         if (resource->stagingBufferMemory != VK_NULL_HANDLE) {
            CHECKV(FreeMemory(device, resource->stagingBufferMemory, pAllocator));
         }
         if (resource->stagingImageView != VK_NULL_HANDLE) {
            CHECKV(DestroyImageView(device, resource->stagingImageView, pAllocator));
         }
         list_del(&resource->link);
         vk_free(&wbuf->device->vk.alloc, resource);
      }
   }
}

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
   VkResult result = CHECK(CreateBuffer(device, &_pCreateInfo, pAllocator, pBuffer));
   wrapper_buffer *wbuf = wrapper_buffer_create(base, &_pCreateInfo, pAllocator, *pBuffer);
   if (wbuf == NULL) {
      WLOG("ERROR: wrapper_buffer_create failed");
      return vk_error(&base->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
   }
   list_inithead(&wbuf->temp_descriptor_pools);
   list_inithead(&wbuf->staging_resources);
   simple_mtx_init(&wbuf->resource_mutex, mtx_plain);
   return result;
}

VKAPI_ATTR void VKAPI_CALL
wrapper_DestroyBuffer(VkDevice device,
                      VkBuffer buffer,
                      const VkAllocationCallbacks* pAllocator)
{
   VK_FROM_HANDLE(wrapper_device, _device, device);
   // Also free up all of the intermediate resources

   if (buffer == VK_NULL_HANDLE) {
      WLOGE("wrapper_DestroyBuffer: null buffer");
      return;
   }

   CHECKV(DestroyBuffer(device, buffer, pAllocator));

   struct wrapper_buffer *wbuf = get_wrapper_buffer(_device, buffer);
   if (!wbuf) {
      WLOGE("wrapper_DestroyBuffer: Buffer not tracked");
      return;
   }
   // Destroy all staging resources
   free_staging_resources_from_buffer(device, wbuf, pAllocator);
   free_temp_pool_from_buffer(device, wbuf);
   simple_mtx_destroy(&wbuf->resource_mutex);

   wrapper_buffer_destroy(_device, wbuf, pAllocator);
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
      WLOGE("wrapper_BindBufferMemory2 called with no bind infos");
      return vk_error(&_device->vk, VK_ERROR_INVALID_EXTERNAL_HANDLE);
   }

   // Track all of the bindInfos
   for (uint32_t i = 0; i < bindInfoCount; i++) {
      wrapper_buffer *_buffer = get_wrapper_buffer(_device, pBindInfos[i].buffer);
      if (!_buffer) {
         WLOGE("wrapper_BindBufferMemory2: buffer %p not tracked", pBindInfos[i].buffer);
         return vk_error(&_device->vk, VK_ERROR_INVALID_EXTERNAL_HANDLE);
      }
      _buffer->memory = pBindInfos[i].memory;
      _buffer->memoryOffset = pBindInfos[i].memoryOffset;
   }

   return CHECK(BindBufferMemory2(device, bindInfoCount, pBindInfos));
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

static VkResult InterceptorState_Init(InterceptorState* state, VkDevice device, size_t spv_size, const uint32_t* spv_code, bool use_image_view) {
   VkResult result;
   VK_FROM_HANDLE(wrapper_device, _device, device);
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
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      }
   };

   if (use_image_view) {
      setLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
   }

   VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = setLayoutBinding,
   };
   result = CHECK(CreateDescriptorSetLayout(device, &setLayoutCreateInfo, NULL, &state->descriptorSetLayout));
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
   result = CHECK(CreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, &state->pipelineLayout));
   if (result != VK_SUCCESS) return result;

    // 4. Create Compute Pipeline
   VkShaderModule computeShaderModule;
   VkShaderModuleCreateInfo shaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spv_size,
      .pCode = (const uint32_t*)spv_code,
   };
   result = CHECK(CreateShaderModule(device, &shaderModuleCreateInfo, NULL, &computeShaderModule));
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
   result = CHECK(CreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &state->pipeline));
   CHECKV(DestroyShaderModule(device, computeShaderModule, NULL));
   if (result != VK_SUCCESS) return result;
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
   WLOG("Failed to find suitable memory type!\n");
   return 0;
}

// C implementation of the createBuffer helper function
static VkResult CreateStagingBuffer(
   struct wrapper_device* _device,
   VkDeviceSize size,
   VkBufferUsageFlags usage,
   VkMemoryPropertyFlags properties,
   VkBuffer* buffer,
   VkDeviceMemory* bufferMemory) {
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


static void GetQueueForCompute(struct wrapper_device* _device, struct wrapper_queue** pQueue) {
   // struct vk_device* device = &_device->vk;
   static int compute_index = -1; 
   if (compute_index < 0) {
      compute_index = FindComputeQueueFamilies(_device->physical);
   }

   if (compute_index < 0) {
      WLOG("ERROR: No compute queue family found");
      *pQueue = NULL;
      return;
   }

   struct vk_queue *queue = NULL;
   for (int i = 0; i < _device->queueCount; i++) {
      struct wrapper_queue *iter = _device->queues[i];
      if (iter->vk.queue_family_index == compute_index) {
         *pQueue = iter;
         return;
      }
   }

   *pQueue = NULL;
}

static VkResult SubmitOneTimeCommands(
   struct wrapper_device* _device,
   VkCommandPool commandPool,
   struct wrapper_queue* queue,
   void (*recordCommands)(struct wrapper_command_buffer*, void*),
   void* pUserData
) {
   WLOG("Submitting one-time commands...");
   VkResult result;
   VkDevice device = (VkDevice) _device;
   VkCommandBufferAllocateInfo allocInfo = { 0 };
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandPool = commandPool;
   allocInfo.commandBufferCount = 1;

   VkCommandBuffer commandBuffer;
   result = wrapper_AllocateCommandBuffers(device, &allocInfo, &commandBuffer);
   if (result != VK_SUCCESS) {
      WLOGE("Failed to allocate command buffers");
      return result;
   }

   VkCommandBufferBeginInfo beginInfo = { 0 };
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   result = CHECK(BeginCommandBuffer(commandBuffer, &beginInfo));
   if (result != VK_SUCCESS) {
      return result;
   };

   recordCommands((struct wrapper_command_buffer*) commandBuffer, pUserData);

   result = CHECK(EndCommandBuffer(commandBuffer));
   if (result != VK_SUCCESS) {
      return result;
   }

   VkSubmitInfo submitInfo = { 0 };
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &commandBuffer;

   VkFence fence;
   VkFenceCreateInfo fenceInfo = { 0 };
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   result = CHECK(CreateFence(device, &fenceInfo, NULL, &fence));
   if (result != VK_SUCCESS) {
      return result;
   }

   WLOG("Submitting command buffer to queue %p", queue);
   result = wrapper_QueueSubmit((VkQueue) queue, 1, &submitInfo, fence);
   if (result != VK_SUCCESS) {
      return result;
   }

   WLOG("Waiting for fence %p", fence);
   CHECK(WaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
   if (result != VK_SUCCESS) {
      return result;
   }
   WLOG("Command buffer execution completed");

   CHECKV(DestroyFence(device, fence, NULL));
   wrapper_FreeCommandBuffers(device, commandPool, 1, &commandBuffer);
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

// void decode_bc6h_to_r16g16b16a16_sfloat(const void* compressedData, void* dstPixelBlock, int pitch, int isSigned) {
//     // bcdec_bc6h_half decompresses to a 4x4 block of 3-component (RGB) half-floats.
//     // We need a temporary buffer to store this intermediate result because the
//     // output format (RGBA) has a different layout than the library's output (RGB).
//     half_float temp_rgb_half_block[4][4][3];

//     // The pitch for the temporary float buffer is the size of one row in bytes.
//     // A row has 4 pixels, each with 3 half_float components.
//     const int temp_pitch_bytes = 4 * 3 * sizeof(half_float);

//     // Step 1: Decompress the BC6h block into the temporary half-float buffer.
//     // This is the most direct and efficient path for this target format.
//     bcdec_bc6h_half(compressedData, temp_rgb_half_block, temp_pitch_bytes, isSigned);

//     // Step 2: Copy the RGB half-float data to the RGBA destination and add the Alpha channel.
//     unsigned char* dst_row_bytes = (unsigned char*)dstPixelBlock;

//     // The bit representation of 1.0f in IEEE 754 half-precision format is 0x3C00.
//     // This is used for the alpha channel, as BC6H is an RGB-only format.
//     const half_float alpha_one = 0x3C00;

//     for (int y = 0; y < 4; ++y) {
//         // Get a pointer to the start of the current pixel row in the destination.
//         half_float* dst_pixel = (half_float*)dst_row_bytes;
        
//         for (int x = 0; x < 4; ++x) {
//             // Get the RGB half values from the temporary buffer.
//             const half_float r_half = temp_rgb_half_block[y][x][0];
//             const half_float g_half = temp_rgb_half_block[y][x][1];
//             const half_float b_half = temp_rgb_half_block[y][x][2];

//             // Write the RGBA values to the destination.
//             dst_pixel[0] = r_half;
//             dst_pixel[1] = g_half;
//             dst_pixel[2] = b_half;
//             dst_pixel[3] = alpha_one; // Set alpha to 1.0f

//             // Move to the next pixel in the destination row (4 half_floats).
//             dst_pixel += 4;
//         }
//         // Move to the next row in the destination buffer using the provided pitch.
//         dst_row_bytes += pitch;
//     }
// }


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

   unsigned short temp_rgb_block[16 * 3];

   // Loop over the image in 4x4 blocks
   for (int y = regions->imageOffset.y; y < height; y += 4) {
      for (int x = regions->imageOffset.x; x < width; x += 4) {

         // Calculate pointer to the destination 4x4 block
         int pitch = 4 * (regions->bufferRowLength ? regions->bufferRowLength : width);
         void *dstPixelBlock =
                  (uint8_t *) mappedDst + (y * pitch) + (x * 4);

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
            case 12:
            case 13:
                  // bcdec_bc6h_float(compressedData, dstPixelBlock, pitch, format_id==13);
                  // bcdec_bc1(compressedData, dstPixelBlock, pitch);
                  bcdec_bc6h_half(compressedData, temp_rgb_block, 4 * 3, format_id==13);
                  {
                     const unsigned short* src_pixel = temp_rgb_block;
                     unsigned char* dst_row = (unsigned char*) dstPixelBlock;

                     for (int ii = 0; ii < 4; ++ii) { // Loop over rows within the block
                           unsigned short* dst_pixel = (unsigned short*)dst_row;
                           for (int jj = 0; jj < 4; ++jj) { // Loop over pixels within the row
                              // Copy R, G, B
                              dst_pixel[0] = src_pixel[0];
                              dst_pixel[1] = src_pixel[1];
                              dst_pixel[2] = src_pixel[2];
                              // Set Alpha to 1.0
                              dst_pixel[3] = 0x3C00;

                              src_pixel += 3; // Advance to next source RGB pixel
                              dst_pixel += 4; // Advance to next destination RGBA pixel
                           }
                           dst_row += pitch; // Advance to the next row in the final image buffer
                     }
                  }
                  break;
            case 14:
            case 15:
                  bcdec_bc7(compressedData, dstPixelBlock, pitch);
                  break;
            default:
                  // Unknown/unsupported format, do nothing.
                  // WLOGE("ERROR: Unsupported BCn format %d", format_id);
                  bcdec_bc1(compressedData, dstPixelBlock, pitch);
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
      WLOGE("ERROR: Failed to map source buffer memory: %d", result);
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
      WLOGE("ERROR: Failed to map destination memory: %d", result);  
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


static void CmdComputeShaderForDecompression(
    struct wrapper_command_buffer* _commandBuffer,
    struct CmdComputeShaderForDecompressionArgs* pArgs)
{
   struct wrapper_device* _device = pArgs->_device;
   struct wrapper_image* wimg = pArgs->wimg;
   VkBuffer srcBuffer = pArgs->srcBuffer;
   VkImageLayout dstImageLayout = pArgs->dstImageLayout;
   VkBuffer stagingBuffer = pArgs->stagingBuffer;
   const VkBufferImageCopy* region = pArgs->region;
   VkImage dstImage = wimg->dispatch_handle;
   struct InterceptorState* state = pArgs->state;
   VkCommandBuffer commandBuffer = _commandBuffer->dispatch_handle;
   bool use_image_view = use_image_view_mode();
   VkResult result;

   WLOG("CmdComputeShaderForDecompression: srcBuffer = %p, dstImage = %p", srcBuffer, dstImage);


   struct wrapper_buffer* wbuf = get_wrapper_buffer(_device, srcBuffer);
   if (!wbuf) {
      WLOGE("CmdComputeShaderForDecompression: srcBuffer not tracked");
      return;
   }

   // If using image view output, we need to transition the image to the appropriate layout
   if (use_image_view) {   
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

      CHECKV(CmdPipelineBarrier((VkCommandBuffer) _commandBuffer,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 0, NULL, 0, NULL, 1, &imageBarrier));
   }

   // Use the descriptor pool from this buffer
   simple_mtx_lock(&wbuf->resource_mutex);
   VkDescriptorSet descriptorSet;
   if (list_is_empty(&wbuf->temp_descriptor_pools)) {
      // The current pool is full or fragmented, create a new one for this command buffer
      result = add_new_temp_pool_to_buffer(wbuf);
      if (result != VK_SUCCESS) {
         WLOGE("Failed to allocate temp descriptor pool for buffer %d: %d", wbuf->bcn_id, result);
         simple_mtx_unlock(&wbuf->resource_mutex);
         return;
      }
   }
   struct wrapper_buffer_descriptor_pool *last_pool_node =
      list_last_entry(&wbuf->temp_descriptor_pools, struct wrapper_buffer_descriptor_pool, link);

   VkDescriptorSetAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = last_pool_node->pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &state->descriptorSetLayout,
   };
   result = wrapper_device_trampolines.AllocateDescriptorSets((VkDevice) _device, &allocInfo, &descriptorSet);
   if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {
      // The current pool is full or fragmented, create a new one for this command buffer
      WLOG("Descriptor pool exhausted, creating a new one for buffer %d", wbuf->bcn_id);
      result = add_new_temp_pool_to_buffer(wbuf);
      if (result != VK_SUCCESS) {
         WLOGE("Failed to allocate temp descriptor pool for buffer %d: %d", wbuf->bcn_id, result);
         simple_mtx_unlock(&wbuf->resource_mutex);
         return;
      }
      last_pool_node = list_last_entry(&wbuf->temp_descriptor_pools, struct wrapper_buffer_descriptor_pool, link);
      allocInfo.descriptorPool = last_pool_node->pool;
      result = wrapper_device_trampolines.AllocateDescriptorSets((VkDevice) _device, &allocInfo, &descriptorSet);
   }
   simple_mtx_unlock(&wbuf->resource_mutex);

   // If it still fails, there's a bigger problem.
   if (result != VK_SUCCESS) {
      WLOGE("Failed to allocate descriptor set for BCn decompression: %d", result);
      return;
   }

   VkDescriptorType dstDescriptorType;

   VkDescriptorBufferInfo srcBufferInfo = {
      .buffer = srcBuffer,
      .offset = region->bufferOffset,
      .range = VK_WHOLE_SIZE
   };

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
      }
   };

   VkDescriptorImageInfo dstImageInfo = { 0 };
   VkDescriptorBufferInfo dstBufferInfo = { 0 };

   if (use_image_view) {
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
      if (result != VK_SUCCESS) {
         WLOGE("Failed to create image view for BCn decompression: %d", result);
         return;
      }

      // Track this image view so it can be cleaned up when the cmd buffer is cleaned up
      // add_new_staging_image_view(_commandBuffer, dstImageView);
      WLOG("Tracking stagingImageView (%p) for BCn buffer %d (%p)", dstImageView, wbuf->bcn_id, srcBuffer);
      add_new_staging_resource_to_buffer(wbuf, VK_NULL_HANDLE, VK_NULL_HANDLE, dstImageView);

      dstImageInfo.imageView = dstImageView;
      dstImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      writeSet[1].pImageInfo = &dstImageInfo;
      writeSet[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
   } else {
      dstBufferInfo.buffer = stagingBuffer;
      dstBufferInfo.offset = 0;
      dstBufferInfo.range = VK_WHOLE_SIZE;
      writeSet[1].pBufferInfo = &dstBufferInfo;
      writeSet[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
   }

   vk_ UpdateDescriptorSets(state->device, 2, writeSet, 0, NULL);

   vk_ CmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, state->pipeline);

   // --- 3. Bind, Push, Dispatch ---
   vk_ CmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           state->pipelineLayout, 0, 1, &descriptorSet, 0, NULL);        

   PushConstantData pushConstants = {
         .srcFormat = wimg->original_format,
         .srcRowLength = (region->bufferRowLength == 0) ? region->imageExtent.width : region->bufferRowLength,
         .srcImageHeight = (region->bufferImageHeight == 0) ? region->imageExtent.height : region->bufferImageHeight,
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


   // If using image view output, we need to transition the image back to the destination layout
   if (use_image_view) {   
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
   if (!wcb) {
      WLOGE("wrapper_CmdCopyBufferToImage: null commandBuffer");
      return;
   }
   struct wrapper_device *_device = wcb->device;
   VkCommandBuffer commandBuffer = wcb->dispatch_handle;
   VkDevice device = _device->dispatch_handle;
   VkResult result;

   struct wrapper_image* wimg = get_wrapper_image(_device, dstImage);
   if (!wimg) {
      WLOGE("ERROR: wrapper_CmdCopyBufferToImage: dstImage not tracked");
      return;
   }

   if (!wimg->is_bcn_emulated /**|| wimg->original_format == VK_FORMAT_BC7_UNORM_BLOCK/**/) { // Debug BC7
      // If no BCn emulation needed, just fall-through
      wrapper_device_trampolines.CmdCopyBufferToImage(_commandBuffer, srcBuffer, dstImage,
                                                      dstImageLayout, 1, pRegions);
      return;
   }

   // --- Decompression Path ---
   _Atomic static int count = 0;
   count++;
   WLOG("Emulating support for format=%d, count=%d", wimg->original_format, count);
   
   bool use_compute_shader = use_compute_shader_mode();
   bool use_image_view = use_image_view_mode();
   WLOGD("  + use_compute_shader=%d, use_image_view=%d", use_compute_shader, use_image_view);

   struct wrapper_buffer* wbuf = get_wrapper_buffer(_device, srcBuffer);
   if (!wbuf) {
      WLOGE("ERROR: wrapper_CmdCopyBufferToImage: srcBuffer not tracked");
   }
   if (!wbuf->bcn_id)
      wbuf->bcn_id = count;

   for (uint32_t i = 0; i < regionCount; ++i) {
      const VkBufferImageCopy* region = &pRegions[i];

      VkBuffer stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      if (!use_image_view) {
         result = CreateStagingBuffer(
            _device, 
            region->imageExtent.width * region->imageExtent.height * region->imageExtent.depth * 4,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            &stagingBuffer, &stagingBufferMemory);
         if (result != VK_SUCCESS) {
            WLOGE("ERROR: CreateStagingBuffer failed with %d", result);
            return;
         }

         // add_new_staging_buffer(wcb, stagingBuffer, stagingBufferMemory);
         WLOGD("Tracking stagingBuffer (%p / %p) for BCn buffer %d (%p)", stagingBuffer, stagingBufferMemory, count, srcBuffer);
         add_new_staging_resource_to_buffer(wbuf, stagingBuffer, stagingBufferMemory, VK_NULL_HANDLE);
      }

      if (use_compute_shader) {
         struct InterceptorState* state = &g_interceptorState_s3tc;
         if (wimg->original_format == VK_FORMAT_BC7_UNORM_BLOCK 
            || wimg->original_format == VK_FORMAT_BC7_SRGB_BLOCK) {
            state = &g_interceptorState_bc7;
         } else if (wimg->original_format == VK_FORMAT_BC6H_SFLOAT_BLOCK 
            || wimg->original_format == VK_FORMAT_BC6H_UFLOAT_BLOCK) {
            state = &g_interceptorState_bc6;
         }
         struct CmdComputeShaderForDecompressionArgs args = {
            ._device = _device,
            .wimg = wimg,
            .srcBuffer = srcBuffer,
            .region = region,
            .state = state,
         };

         if (use_image_view) {
            args.dstImage = dstImage;
            args.dstImageLayout = dstImageLayout;
         } else {
            args.stagingBuffer = stagingBuffer;
         }

         CmdComputeShaderForDecompression(wcb, &args);
      } else {
         HostSideDecompression(_device, wbuf, stagingBufferMemory, region, wimg->original_format);
      }

      // struct wrapper_queue* queue;
      // GetQueueForCompute(_device, &queue);
      // SubmitOneTimeCommands(_device, wcb->pool, queue, (void(*)(struct wrapper_command_buffer*, void*)) CmdComputeShaderForDecompression, &args);

      if (!use_image_view) {
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
      }
   }
}
