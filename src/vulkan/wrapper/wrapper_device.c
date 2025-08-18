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
#include "wrapper_debug.h"
#include "vk_unwrappers.h"
#include "vk_printers.h"
#include "spirv_edit.h"
#include "artifacts.h"

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

// Initializes the Vulkan objects needed for the compute dispatch.
// Call this after intercepting vkCreateDevice.
static VkResult InterceptorState_Init(InterceptorState* state, VkDevice device, size_t spv_size, const uint32_t* spv_code, bool use_image_view, int bc_mode);

// Cleans up the Vulkan objects.
// Call this before intercepting vkDestroyDevice.
static void InterceptorState_Cleanup(InterceptorState* state);

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
   // .KHR_shader_float_controls = true, // per @bylaws, this only affects Qualcomm
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
   REQUIRED_EXTENSION(EXT_device_fault); // for fault reporting, if available
   // REQUIRED_EXTENSION(KHR_uniform_buffer_standard_layout); // for std430 UBOs
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
         
         list_inithead(&queue->staging_resources);
         simple_mtx_init(&queue->resource_mutex, mtx_plain);
         static uint32_t obj_id = 0;
         queue->obj_id = obj_id++;
      }
   }

   return VK_SUCCESS;
}

WRAPPER_CreateDevice(VkPhysicalDevice physicalDevice,
                     const VkDeviceCreateInfo* pCreateInfo,
                     const VkAllocationCallbacks* pAllocator,
                     VkDevice* pDevice)
{
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
   device->command_pool_map = _mesa_hash_table_u64_create(NULL);
   

   simple_mtx_init(&device->resource_mutex, mtx_plain);
   device->physical = physical_device;

   vk_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wrapper_device_entrypoints, true);
   vk_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wsi_device_entrypoints, false);
   vk_device_dispatch_table_from_entrypoints(
      &dispatch_table, &wrapper_device_trampolines, false);

   VkPhysicalDeviceFaultFeaturesEXT fault_features_ext = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT,
      .pNext = (void*) wrapper_create_info.pNext,
   };
   if (physical_device->base_supported_extensions.EXT_device_fault) {
      WLOGD("Turning on VK_EXT_device_fault for better GPU fault reporting.");
      VkPhysicalDeviceFaultFeaturesEXT *ext =
         (VkPhysicalDeviceFaultFeaturesEXT*) vk_find_struct_const(pCreateInfo, PHYSICAL_DEVICE_FAULT_FEATURES_EXT);

      if (ext == NULL) {
         wrapper_create_info.pNext = &fault_features_ext;
         ext = &fault_features_ext;
      }
      ext->deviceFault = physical_device->base_supported_features.deviceFault;
      ext->deviceFaultVendorBinary = physical_device->base_supported_features.deviceFaultVendorBinary;
   }

#define DISABLE_EXT(extension, type, feature) \
   if (!physical_device->base_supported_features.feature) { \
      VK_STRUCTURE_TYPE_##type##_cast *ext = (VK_STRUCTURE_TYPE_##type##_cast *) vk_find_struct_const(pCreateInfo, type); \
      if (ext) { \
         WLOG("Faking extension support for " #extension "->" #feature); \
         ext->feature = ext->feature & physical_device->base_supported_features.feature; \
      } \
   }

   DISABLE_EXT(EXT_transform_feedback, PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT, geometryStreams);
   DISABLE_EXT(EXT_transform_feedback, PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT, transformFeedback);
   DISABLE_EXT(EXT_host_query_reset, PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT, hostQueryReset);
   DISABLE_EXT(EXT_custom_border_color, PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT, customBorderColors);
   DISABLE_EXT(EXT_custom_border_color, PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT, customBorderColorWithoutFormat);

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
   
   #define DISABLE_FEATURE(feature) \
   if (!physical_device->base_supported_features.feature) WLOG("Faking feature support for " #feature); \
   if (pdf && pdf->feature) { \
      pdf->feature &= physical_device->base_supported_features.feature; \
   } \
   if (pdf2 && pdf2->features.feature) { \
      pdf2->features.feature &= physical_device->base_supported_features.feature; \
   }

   DISABLE_FEATURE(geometryShader);
   DISABLE_FEATURE(textureCompressionBC);
   DISABLE_FEATURE(multiViewport);
   DISABLE_FEATURE(depthClamp);
   DISABLE_FEATURE(depthBiasClamp);
   DISABLE_FEATURE(fillModeNonSolid);
   DISABLE_FEATURE(shaderClipDistance);
   DISABLE_FEATURE(shaderCullDistance);
   DISABLE_FEATURE(dualSrcBlend);
   DISABLE_FEATURE(multiDrawIndirect); // Missing on G57 r32p1
   DISABLE_FEATURE(vertexPipelineStoresAndAtomics); // Missing on G57 r32p1
   // DISABLE_FEATURE(logicOp); // Missing on G57 r32p1
   // DISABLE_FEATURE(variableMultisampleRate); // Missing on G57 r32p1
   
   // DEBUG: Disable ClipDistance
   if (CHECK_FLAG("FORCE_CLIP_DISTANCE")) {
      if (pdf && pdf->shaderClipDistance) {
         pdf->shaderClipDistance = false;
      }
      if (pdf2 && pdf2->features.shaderClipDistance) {
         pdf2->features.shaderClipDistance = false;
      }
      if (pdf && pdf->shaderCullDistance) {
         pdf->shaderCullDistance = false;
      }
      if (pdf2 && pdf2->features.shaderCullDistance) {
         pdf2->features.shaderCullDistance = false;
      }
   }

   result = WPDEVICE.CreateDevice(
      physicalDevice, &wrapper_create_info,
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
   bool validate_bcn = get_validate_bcn_masks() > 0;
   bool dump_artifacts = (get_dump_bcn_masks() > 0) || validate_bcn;
   bool use_image_view = use_image_view_mode() && !dump_artifacts;
   
   result = InterceptorState_Init(&device->s3tc, 
      wrapper_device_to_handle(device), 
      use_image_view ? sizeof(s3tc_iv_spv) : sizeof(s3tc_spv), 
      use_image_view ? s3tc_iv_spv : s3tc_spv, 
      use_image_view, 1);
   if (result != VK_SUCCESS) {
      WLOGE("Failed to initialize InterceptorState for s3tc");
      return vk_error(physical_device, result);
   }
   result = InterceptorState_Init(&device->bc6, 
      wrapper_device_to_handle(device), 
      use_image_view ? sizeof(bc6_iv_spv) : sizeof(bc6_spv), 
      use_image_view ? bc6_iv_spv : bc6_spv, 
      use_image_view, 6);
   if (result != VK_SUCCESS) {
      WLOGE("Failed to initialize InterceptorState for bc6");
      return vk_error(physical_device, result);
   }
   result = InterceptorState_Init(&device->bc7, 
      wrapper_device_to_handle(device), 
      use_image_view ? sizeof(bc7_iv_spv) : sizeof(bc7_spv), 
      use_image_view ? bc7_iv_spv : bc7_spv, 
      use_image_view, 7);
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

   // Allocate a dedicated pool for graphics/compute tasks
   int graphics_queue_idx = FindGraphicsComputeQueueFamilies(physical_device);
   if (graphics_queue_idx >= 0) {
      device->graphics_queue = device->queues[graphics_queue_idx];
      device->graphics_queue_idx = graphics_queue_idx;
      VkCommandPoolCreateInfo commandPoolInfo = {
         .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
         .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
         .queueFamilyIndex = graphics_queue_idx,
      };

      WCHECK(CreateCommandPool((VkDevice) device, &commandPoolInfo, NULL, &device->computePool));
   } else {
      WLOGE("Could not find a graphics & compute queue");
   }
   
   *pDevice = wrapper_device_to_handle(device);

   return VK_SUCCESS;
}

WRAPPER_GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex,
                       uint32_t queueIndex, VkQueue* pQueue) {
   vk_common_GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
}

WRAPPER_GetDeviceQueue2(VkDevice _device, const VkDeviceQueueInfo2* pQueueInfo,
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

WRAPPER_GetDeviceProcAddr(VkDevice _device, const char* pName) {
   VK_FROM_HANDLE(wrapper_device, device, _device);
   return vk_device_get_proc_addr(&device->vk, pName);
}

WRAPPER_QueueSubmit(VkQueue _queue, uint32_t submitCount,
                    const VkSubmitInfo* pSubmits, VkFence fence)
{
   return CHECK(QueueSubmit(_queue, submitCount, pSubmits, fence));
}

WRAPPER_QueueSubmit2(VkQueue _queue, uint32_t submitCount,
                     const VkSubmitInfo2* pSubmits, VkFence fence)
{
   return CHECK(QueueSubmit2(_queue, submitCount, pSubmits, fence));
}

WRAPPER_CmdExecuteCommands(VkCommandBuffer commandBuffer,
                           uint32_t commandBufferCount,
                           const VkCommandBuffer* pCommandBuffers)
{
   VK_FROM_HANDLE(wrapper_command_buffer, wcb, commandBuffer);
   VkCommandBuffer command_buffers[commandBufferCount];

   for (int i = 0; i < commandBufferCount; i++) {
      command_buffers[i] =
         wrapper_command_buffer_from_handle(pCommandBuffers[i])->dispatch_handle;
   }
   // TODO: unwrap wcbs
   wcb->device->dispatch_table.CmdExecuteCommands(
      wcb->dispatch_handle, commandBufferCount, command_buffers);
}

WRAPPER_AllocateCommandBuffers(VkDevice _device,
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
      CHECKV(FreeCommandBuffers(_device,
                                 pAllocateInfo->commandPool,
                                 pAllocateInfo->commandBufferCount,
                                 dispatch_handles));
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

WRAPPER_FreeCommandBuffers(VkDevice _device,
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

   CHECKV(FreeCommandBuffers(_device, commandPool, commandBufferCount, dispatch_handles));
}

WRAPPER_CreateCommandPool(VkDevice device,
    const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkCommandPool* pCommandPool) {
   VK_FROM_HANDLE(wrapper_device, _device, device);
   VkResult result = CHECK(CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool));
   struct wrapper_command_pool *pool = wrapper_command_pool_create(_device, pCreateInfo, *pCommandPool);
   pool->queue_idx = pCreateInfo->queueFamilyIndex;
   return result;
}

WRAPPER_DestroyCommandPool(VkDevice _device, VkCommandPool commandPool,
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

   struct wrapper_command_pool *pool = get_wrapper_command_pool(device, commandPool);
   if (pool) {
      wrapper_command_pool_destroy(device, pool);
   }

   CHECKV(DestroyCommandPool(_device, commandPool, pAllocator));
}

WRAPPER_DestroyDevice(VkDevice _device, const VkAllocationCallbacks* pAllocator)
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
      wrapper_image_destroy(device, obj);
   }
   _mesa_hash_table_u64_destroy(device->image_map);

   hash_table_u64_foreach(device->buffer_map, entry) {
      struct wrapper_buffer *obj = (struct wrapper_buffer *) entry.data;
      wrapper_buffer_destroy(device, obj);
   }
   _mesa_hash_table_u64_destroy(device->buffer_map);

   hash_table_u64_foreach(device->command_pool_map, entry) {
      struct wrapper_command_pool *obj = (struct wrapper_command_pool *) entry.data;
      wrapper_command_pool_destroy(device, obj);
   }
   _mesa_hash_table_u64_destroy(device->command_pool_map);

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

WRAPPER_SetPrivateData(VkDevice _device, VkObjectType objectType,
                       uint64_t objectHandle,
                       VkPrivateDataSlot privateDataSlot,
                       uint64_t data) {
   uint64_t object_handle = unwrap_device_object(objectType, objectHandle);
   return CHECK(SetPrivateData(_device,
      objectType, object_handle, privateDataSlot, data));
}

WRAPPER_GetPrivateData(VkDevice _device, VkObjectType objectType,
                       uint64_t objectHandle,
                       VkPrivateDataSlot privateDataSlot,
                       uint64_t* pData) {
   uint64_t object_handle = unwrap_device_object(objectType, objectHandle);
   CHECKV(GetPrivateData(_device, objectType, object_handle, privateDataSlot, pData));
}

// Helper function to create and add a pool
static VkResult add_new_temp_pool_to_buffer(struct wrapper_buffer *wbuf) {
   // TODO: move this into our gc system
   struct wrapper_device *device = wbuf->device;
   VkDescriptorPool new_pool_handle;
   VkDescriptorPoolSize pool_sizes[3] = {
      {
         .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .descriptorCount = 32 // Sufficient for 32 sets, each needing one
      },
      {
         .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .descriptorCount = 32 // Sufficient for 32 sets, each needing one
      },
      {
         .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 32 // Sufficient for 32 sets, each needing one
      },
   };

   const VkDescriptorPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = 32, // A reasonable number for one command buffer's temp usage
      .poolSizeCount = 3,
      .pPoolSizes = pool_sizes,
   };

   VkResult result = WCHECK(CreateDescriptorPool((VkDevice) device, &create_info, NULL, &new_pool_handle));
   if (result == VK_SUCCESS) {
      struct wrapper_buffer_descriptor_pool *new_pool_node =
         vk_alloc(&wbuf->device->vk.alloc, sizeof(struct wrapper_buffer_descriptor_pool), 8,
                  VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
      new_pool_node->pool = new_pool_handle;
      list_addtail(&new_pool_node->link, &wbuf->temp_descriptor_pools);
      WLOG("Total of %d temp descriptor pools created for buffer %d (%p)", list_length(&wbuf->temp_descriptor_pools), wbuf->obj_id, wbuf->dispatch_handle);
   }
   return result;
}

static void free_temp_pool_from_buffer(VkDevice device, struct wrapper_buffer* wbuf) {
   // // Clean up temporary descriptor pools associated with this command buffer
   if (!list_is_empty(&wbuf->temp_descriptor_pools)) {
      list_for_each_entry_safe(struct wrapper_buffer_descriptor_pool, pool_node, &wbuf->temp_descriptor_pools, link) {
         WCHECKV(DestroyDescriptorPool(device, pool_node->pool, NULL));
         list_del(&pool_node->link);
         vk_free(&wbuf->device->vk.alloc, pool_node);
      }
   }
}

WRAPPER_CreateBuffer(
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
   wrapper_buffer *wbuf = wrapper_buffer_create(base, &_pCreateInfo, *pBuffer);
   if (wbuf == NULL) {
      WLOGE("wrapper_buffer_create failed");
      return vk_error(&base->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
   }
   list_inithead(&wbuf->temp_descriptor_pools);
   return result;
}

WRAPPER_DestroyBuffer(VkDevice device,
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
   free_temp_pool_from_buffer(device, wbuf);
   wrapper_buffer_destroy(_device, wbuf);
}

WRAPPER_BindBufferMemory(VkDevice device, 
   VkBuffer buffer,
   VkDeviceMemory memory,
   VkDeviceSize memoryOffset) {

   VkBindBufferMemoryInfo bind = {
      .sType         = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
      .buffer        = buffer,
      .memory        = memory,
      .memoryOffset  = memoryOffset,
   };

   return WCHECK(BindBufferMemory2(device, 1, &bind));
}

WRAPPER_BindBufferMemory2(
    VkDevice device,
    uint32_t bindInfoCount,
    const VkBindBufferMemoryInfo* pBindInfos)
{
   VK_FROM_HANDLE(wrapper_device, wdev, device);

   if (bindInfoCount == 0 || pBindInfos == NULL) {
      WLOGE("wrapper_BindBufferMemory2 called with no bind infos");
      return vk_error(&wdev->vk, VK_ERROR_INVALID_EXTERNAL_HANDLE);
   }

   // Track all of the bindInfos
   for (uint32_t i = 0; i < bindInfoCount; i++) {
      wrapper_buffer *_buffer = get_wrapper_buffer(wdev, pBindInfos[i].buffer);
      if (!_buffer) {
         WLOG("wrapper_BindBufferMemory2: buffer %p not tracked", pBindInfos[i].buffer);
         // return vk_error(&_device->vk, VK_ERROR_INVALID_EXTERNAL_HANDLE);
         // TODO(leegao): figure out what's going wrong here, but there are reports of this
         continue;
      }
      _buffer->memory = pBindInfos[i].memory;
      _buffer->memoryOffset = pBindInfos[i].memoryOffset;
   }

   return CHECK(BindBufferMemory2(device, bindInfoCount, pBindInfos));
}

// Helper function to find a suitable memory type
static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
   VkPhysicalDeviceMemoryProperties memProperties;
   WPCHECKV(GetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties));

   for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
         return i;
      }
   }

   WLOGE("Failed to find suitable memory type for filter=%x and properties=%x\n", typeFilter, properties);
   return -1;
}

static VkResult CreateConstantsUniformBuffer(
   struct wrapper_device* _device,
   VkDeviceSize size,
   VkBuffer* buffer,
   VkDeviceMemory* bufferMemory) {
   VkDevice device = (VkDevice) _device;
   struct wrapper_physical_device* physicalDevice = _device->physical;

   VkResult result;
   VkBufferCreateInfo bufferInfo = {0};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = size;
   bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
   bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   result = WCHECK(CreateBuffer(device, &bufferInfo, NULL, buffer));
   if (result != VK_SUCCESS)
      return result;

   VkMemoryRequirements memRequirements;
   WCHECKV(GetBufferMemoryRequirements(device, *buffer, &memRequirements));

   VkMemoryAllocateInfo allocInfo = {0};
   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize = memRequirements.size;
   allocInfo.memoryTypeIndex = FindMemoryType((VkPhysicalDevice) physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

   result = WCHECK(AllocateMemory(device, &allocInfo, NULL, bufferMemory));
   if (result != VK_SUCCESS) {
      return result;
   }

   return WCHECK(BindBufferMemory(device, *buffer, *bufferMemory, 0));
}

static VkResult InterceptorState_Init(InterceptorState* state, VkDevice device, size_t spv_size, const uint32_t* spv_code, bool use_image_view, int bc_mode) {
   VkResult result;
   VK_FROM_HANDLE(wrapper_device, _device, device);
   VkDescriptorSetLayoutBinding setLayoutBinding[3] = {
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
      },
      {
         .binding = 2,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .descriptorCount = 1,
         .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      }
   };

   int bindingCount = 2;

   if (bc_mode == 7) {
      bindingCount = 3;

      CreateConstantsUniformBuffer(
         _device,
         sizeof(Bc7Constants),
         &state->constantsBuffer,      // Pass pointers to be filled
         &state->constantsBufferMemory // Pass pointers to be filled
      );

      void* data;
      VkMemoryMapInfoKHR mapInfoSrc = {
         .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
         .memory = state->constantsBufferMemory,
         .offset = 0,
         .size = sizeof(Bc7Constants),
      };
      VkResult result = WCHECK(MapMemory2KHR(device, &mapInfoSrc, &data));
      if (result != VK_SUCCESS) {
         return result;
      }
      Bc7Constants bc7_uniform_constants;
      populate_bc7_decoding_constants(&bc7_uniform_constants);
      memcpy(data, &bc7_uniform_constants, sizeof(Bc7Constants));
      WCHECKV(UnmapMemory(device, state->constantsBufferMemory));
   } else if (bc_mode == 6) {
      bindingCount = 3;

      CreateConstantsUniformBuffer(
         _device,
         sizeof(Bc6Constants),
         &state->constantsBuffer,      // Pass pointers to be filled
         &state->constantsBufferMemory // Pass pointers to be filled
      );

      void* data;
      VkMemoryMapInfoKHR mapInfoSrc = {
         .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
         .memory = state->constantsBufferMemory,
         .offset = 0,
         .size = sizeof(Bc6Constants),
      };
      VkResult result = WCHECK(MapMemory2KHR(device, &mapInfoSrc, &data));
      if (result != VK_SUCCESS) {
         return result;
      }
      Bc6Constants bc6_uniform_constants;
      populate_bc6_decoding_constants(&bc6_uniform_constants);
      memcpy(data, &bc6_uniform_constants, sizeof(Bc6Constants));
      WCHECKV(UnmapMemory(device, state->constantsBufferMemory));
   }

   if (use_image_view) {
      setLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
   }

   VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = bindingCount,
      .pBindings = setLayoutBinding,
   };
   result = WCHECK(CreateDescriptorSetLayout(device, &setLayoutCreateInfo, NULL, &state->descriptorSetLayout));
   if (result != VK_SUCCESS) return result;

   VkPushConstantRange pushConstantRange = {
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .offset = 0,
      .size = sizeof(PushConstantData),
   };

   VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &state->descriptorSetLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstantRange,
   };
   result = WCHECK(CreatePipelineLayout(device, &pipelineLayoutCreateInfo, NULL, &state->pipelineLayout));
   if (result != VK_SUCCESS) return result;

   // TEST: run a debug run of the pass of the lowering pass on the compute shader
   if (CHECK_FLAG("OPT_SHADERS")) {
      struct SpirvCode spirv_code = { 0 };
      optimize_spirv_for_size(spv_code, spv_size / sizeof(uint32_t), &spirv_code);
      spv_size = spirv_code.spirv_word_count * 4;
      spv_code = spirv_code.spirv_code;
   }

   VkShaderModule computeShaderModule;
   VkShaderModuleCreateInfo shaderModuleCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = spv_size,
      .pCode = (const uint32_t*)spv_code,
   };
   result = WCHECK(CreateShaderModule(device, &shaderModuleCreateInfo, NULL, &computeShaderModule));
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
   result = WCHECK(CreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &state->pipeline));
   WCHECKV(DestroyShaderModule(device, computeShaderModule, NULL));
   return result;
}

// TODO: deprecate this pass in the future
static VkResult CreateStagingBuffer(
   struct wrapper_device* _device,
   VkDeviceSize size,
   VkBufferUsageFlags usage,
   VkMemoryPropertyFlags properties,
   VkBuffer* buffer,
   VkDeviceMemory* bufferMemory) {
   VkDevice device = (VkDevice) _device;
   struct wrapper_physical_device* physicalDevice = _device->physical;

   VkResult result;
   VkBufferCreateInfo bufferInfo = {0};
   bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   bufferInfo.size = size;
   bufferInfo.usage = usage;
   bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   result = WCHECK(CreateBuffer(device, &bufferInfo, NULL, buffer));
   if (result != VK_SUCCESS)
      return result;

   VkMemoryRequirements memRequirements;
   WCHECKV(GetBufferMemoryRequirements(device, *buffer, &memRequirements));

   VkMemoryAllocateInfo allocInfo = {0};
   allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   allocInfo.allocationSize = memRequirements.size;
   allocInfo.memoryTypeIndex = FindMemoryType((VkPhysicalDevice) physicalDevice, memRequirements.memoryTypeBits, properties);

   result = WCHECK(AllocateMemory(device, &allocInfo, NULL, bufferMemory));
   if (result != VK_SUCCESS) {
      return result;
   }

   return WCHECK(BindBufferMemory(device, *buffer, *bufferMemory, 0));
}

static int FindComputeQueueFamilies(struct wrapper_physical_device* physical_device) {
   // First, query the number of queue families available.
   uint32_t queueFamilyCount = 0;
   WPCHECKV(GetPhysicalDeviceQueueFamilyProperties(
   (VkPhysicalDevice) physical_device, &queueFamilyCount, NULL));

   VkQueueFamilyProperties queueFamilies[32];
   WPCHECKV(GetPhysicalDeviceQueueFamilyProperties(
   (VkPhysicalDevice) physical_device, &queueFamilyCount, queueFamilies));

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
   _Atomic static int counter = 0;
   int id = counter++;
   WLOGD("Submitting one-time commands for id=%d", id);
   VkResult result;
   VkDevice device = (VkDevice) _device;
   VkCommandBufferAllocateInfo allocInfo = { 0 };
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandPool = commandPool;
   allocInfo.commandBufferCount = 1;

   VkCommandBuffer commandBuffer;
   result = WCHECK(AllocateCommandBuffers(device, &allocInfo, &commandBuffer));
   if (result != VK_SUCCESS) {
      WLOGE("Failed to allocate command buffers");
      return result;
   }

   VkCommandBufferBeginInfo beginInfo = { 0 };
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   result = WCHECK(BeginCommandBuffer(commandBuffer, &beginInfo));
   if (result != VK_SUCCESS) {
      return result;
   };

   recordCommands((struct wrapper_command_buffer*) commandBuffer, pUserData);

   result = WCHECK(EndCommandBuffer(commandBuffer));
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
   result = WCHECK(CreateFence(device, &fenceInfo, NULL, &fence));
   if (result != VK_SUCCESS) {
      return result;
   }

   WLOGD("Submitting command buffer to queue %p for id=%d", queue, id);
   result = WCHECK(QueueSubmit((VkQueue) queue, 1, &submitInfo, fence));
   if (result != VK_SUCCESS) {
      return result;
   }

   WLOGD("Waiting for fence %p for id=%d", fence, id);
   result = WCHECK(WaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
   if (result != VK_SUCCESS) {
      return result;
   }
   WLOGD("Command buffer execution completed for id=%d", id);

   WCHECKV(DestroyFence(device, fence, NULL));
   WCHECKV(FreeCommandBuffers(device, commandPool, 1, &commandBuffer));
   return VK_SUCCESS;
}

// Takes in the srcBuffer and decompresses it into a staging buffer
// Both buffers must be bound to memory and are host visible (for mapping)
// This is the software fallback for BCn decompression that aren't implemented by the compute shader
static VkResult HostSideDecompression(
      struct wrapper_device* _device,
      wrapper_buffer* srcBuffer,
      VkDeviceMemory dstMemory,
      const VkBufferImageCopy* region,
      VkFormat in_format,
      bool is_striped
) {
   // Map the source buffer
   void* srcData;
   VkMemoryMapInfoKHR mapInfoSrc = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
      .memory = srcBuffer->memory,
      .offset = srcBuffer->memoryOffset,
      .size = VK_WHOLE_SIZE,
   }; 
   VkResult result = WCHECK(MapMemory2KHR((VkDevice) _device, &mapInfoSrc, &srcData)); 
   if (result != VK_SUCCESS) {
      WLOGE("ERROR: Failed to map source buffer memory: %d", result);
      return result;
   }

   // Map the destination memory
   void* dstData;
   VkMemoryMapInfoKHR mapInfoDst = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
      .memory = dstMemory,
      .offset = 0,
      .size = region->imageExtent.width * region->imageExtent.height * get_bc_target_size(_device->physical, in_format),
   };
   result = WCHECK(MapMemory2KHR((VkDevice) _device, &mapInfoDst, &dstData));
   if (result != VK_SUCCESS) {
      goto final;
   }

   // Decompress the data from srcData to dstData
   BCnDecompression(
      in_format,
      srcData,
      dstData,
      region,
      is_striped
   );

   // Upload a magenta color for debugging
   // uint32_t* dstPixels = (uint32_t*) dstData;
   // size_t pixelCount = region->imageExtent.width * region->imageExtent.height;
   // for (size_t i = 0; i < pixelCount; ++i) {
   //    dstPixels[i] = 0xFFFF00FF; // Magenta color in RGBA format
   // }

   VkMappedMemoryRange flushRange = {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
      .memory = dstMemory,
      .offset = 0,
      .size = VK_WHOLE_SIZE,
   };
   result = WCHECK(FlushMappedMemoryRanges((VkDevice) _device, 1, &flushRange));

   WCHECKV(UnmapMemory((VkDevice) _device, dstMemory));
final:
   // TODO: this breaks some applications that maps the srcBuffer, e.g. 32bit emulated placeAddr
   // WCHECKV(UnmapMemory((VkDevice) _device, srcBuffer->memory));
   return result;
}

static VkDeviceSize calculate_bc_copy_size(const VkBufferImageCopy* region, uint32_t block_size_in_bytes) {
    // For BC1 (and other BC formats), the block dimensions are 4x4 texels.
    uint32_t copy_extent_in_blocks_x = (region->imageExtent.width + 3) / 4;
    uint32_t copy_extent_in_blocks_y = (region->imageExtent.height + 3) / 4;

    VkDeviceSize row_pitch_in_bytes;
    if (region->bufferRowLength == 0) {
        row_pitch_in_bytes = copy_extent_in_blocks_x * block_size_in_bytes;
    } else {
        uint32_t row_length_in_blocks = region->bufferRowLength / 4;
        row_pitch_in_bytes = row_length_in_blocks * block_size_in_bytes;
    }
    VkDeviceSize last_row_size_in_bytes = copy_extent_in_blocks_x * block_size_in_bytes;
    VkDeviceSize offset_to_last_row = (copy_extent_in_blocks_y - 1) * row_pitch_in_bytes;

    return offset_to_last_row + last_row_size_in_bytes;
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
   bool use_image_view;
};

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
   bool use_image_view = pArgs->use_image_view;
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
         .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

      WCHECKV(CmdPipelineBarrier((VkCommandBuffer) _commandBuffer,
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
         WLOGE("Failed to allocate temp descriptor pool for buffer %d: %d", wbuf->obj_id, result);
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
      WLOG("Descriptor pool exhausted, creating a new one for buffer %d", wbuf->obj_id);
      result = add_new_temp_pool_to_buffer(wbuf);
      if (result != VK_SUCCESS) {
         WLOGE("Failed to allocate temp descriptor pool for buffer %d: %d", wbuf->obj_id, result);
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

   bool is_bc1 = wimg->original_format >= VK_FORMAT_BC1_RGB_UNORM_BLOCK && wimg->original_format <= VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
   bool is_bc4 = wimg->original_format == VK_FORMAT_BC4_UNORM_BLOCK || wimg->original_format == VK_FORMAT_BC4_SNORM_BLOCK;
   bool is_bc6 = wimg->original_format == VK_FORMAT_BC6H_UFLOAT_BLOCK || wimg->original_format == VK_FORMAT_BC6H_SFLOAT_BLOCK;
   bool is_bc7 = wimg->original_format == VK_FORMAT_BC7_UNORM_BLOCK || wimg->original_format == VK_FORMAT_BC7_SRGB_BLOCK;

   VkDescriptorType dstDescriptorType;

   // Calculate the buffer size for an image
   // Normally 4x4 block is compressed to 16 bytes (1bpp), except for BC1 and BC4 (0.5 bpp)
   uint32_t srcBufferSize = calculate_bc_copy_size(region, 16);
   if (is_bc1 || is_bc4) {
      srcBufferSize = calculate_bc_copy_size(region, 8);
   }

   VkDescriptorBufferInfo srcBufferInfo = {
      .buffer = srcBuffer,
      .offset = region->bufferOffset,
      .range = VK_WHOLE_SIZE,
   };

   VkDescriptorBufferInfo uniformConstantBufferInfo = {
      .buffer = state->constantsBuffer,
      .offset = 0,
      .range = is_bc7 ? sizeof(Bc7Constants) : (is_bc6 ? sizeof(Bc6Constants) : 0),
   };

   VkWriteDescriptorSet writeSet[3] = {
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
      },
      {
         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = descriptorSet,
         .dstBinding = 2,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &uniformConstantBufferInfo,
      }
   };

   VkDescriptorImageInfo dstImageInfo = { 0 };
   VkDescriptorBufferInfo dstBufferInfo = { 0 };

   if (use_image_view) {
      VkImageViewCreateInfo viewCreateInfo = {
         .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
         .image = dstImage,
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
      result = WCHECK(CreateImageView((VkDevice) _device, &viewCreateInfo, NULL, &dstImageView));
      if (result != VK_SUCCESS) {
         return;
      }

      // Track this image view so it can be cleaned up when the image is cleaned up
      WLOGD("Tracking stagingImageView (%p) for %d", dstImageView, wimg->obj_id);
      TRACK_STAGING(_device, IMAGE_VIEW, dstImageView, wimg);

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

   int writeSetCount = is_bc6 || is_bc7 ? 3 : 2;
   WCHECKV(UpdateDescriptorSets((VkDevice) _device, writeSetCount, writeSet, 0, NULL));

   WCHECKV(CmdBindPipeline((VkCommandBuffer) _commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, state->pipeline));

   WCHECKV(CmdBindDescriptorSets((VkCommandBuffer) _commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                           state->pipelineLayout, 0, 1, &descriptorSet, 0, NULL));

   PushConstantData pushConstants = {
         .srcFormat = wimg->original_format,
         .srcRowLength = (region->bufferRowLength == 0) ? region->imageExtent.width : region->bufferRowLength,
         .srcImageHeight = (region->bufferImageHeight == 0) ? region->imageExtent.height : region->bufferImageHeight,
         .imageOffsetX = region->imageOffset.x,
         .imageOffsetY = region->imageOffset.y,
         .imageExtentX = region->imageExtent.width,
         .imageExtentY = region->imageExtent.height,
         .srcBufferSize = srcBufferSize,
         .unsupportedBitsBc = get_unsupported_bcn_masks(),
         .watercoloredBitsBc = get_watermarked_bcn_masks(),
         .watermarkerSize = get_watermark_size(),
   };

   WCHECKV(CmdPushConstants((VkCommandBuffer) _commandBuffer, state->pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT,
                     0, sizeof(PushConstantData), &pushConstants));

   uint32_t groupCountX = (region->imageExtent.width + 7) / 8;
   uint32_t groupCountY = (region->imageExtent.height + 7) / 8;
   WCHECKV(CmdDispatch((VkCommandBuffer) _commandBuffer, groupCountX, groupCountY, 1));


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

      WCHECKV(CmdPipelineBarrier((VkCommandBuffer) _commandBuffer,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           0, 0, NULL, 0, NULL, 1, &imageBarrier));
   }
}

static VkResult CreateBcnCommandBuffer(struct wrapper_device* device, struct wrapper_command_buffer* wcb) {
   VkResult result;
   VkCommandBufferAllocateInfo allocInfo = { 0 };
   allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   allocInfo.commandPool = device->computePool;
   allocInfo.commandBufferCount = 1;

   VkCommandBuffer bcnBuffer;
   VkFence bcnBufferFinished;
   VkSemaphore bcnBufferFinishedSemaphore;

   result = WCHECK(AllocateCommandBuffers((VkDevice) device, &allocInfo, &bcnBuffer));
   if (result != VK_SUCCESS) {
      WLOGE("Failed to allocate bcnBuffer command buffers");
      goto failure;
   }

   VkCommandBufferBeginInfo beginInfo = { 0 };
   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   VkFenceCreateInfo fenceInfo = { 0 };
   fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
   result = WCHECK(CreateFence((VkDevice) device, &fenceInfo, NULL, &bcnBufferFinished));
   if (result != VK_SUCCESS) {
      goto failure;
   }

   VkSemaphoreCreateInfo semaphoreInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
   };
   result = WCHECK(CreateSemaphore((VkDevice) device, &semaphoreInfo, NULL, &bcnBufferFinishedSemaphore));
   if (result != VK_SUCCESS) {
      goto failure;
   }

   simple_mtx_lock(&wcb->resource_mutex);
   // wcb->bcnBuffer = bcnBuffer;
   // wcb->bcnBufferFinished = bcnBufferFinished;
   // wcb->bcnBufferFinishedSemaphore = bcnBufferFinishedSemaphore;
   wcb->bcnCommands = 0;
   simple_mtx_unlock(&wcb->resource_mutex);

   result = WCHECK(BeginCommandBuffer(bcnBuffer, &beginInfo));
failure:
   return result;
}

WRAPPER_CmdCopyBufferToImage(VkCommandBuffer commandBuffer,
                      VkBuffer srcBuffer,
                      VkImage dstImage,
                      VkImageLayout dstImageLayout,
                      uint32_t regionCount,
                      const VkBufferImageCopy* pRegions)
{
   VK_FROM_HANDLE(wrapper_command_buffer, wcb, commandBuffer);
   if (!wcb) {
      WLOGE("wrapper_CmdCopyBufferToImage: null commandBuffer");
      return;
   }
   struct wrapper_device *_device = wcb->device;
   VkDevice device = _device->dispatch_handle;
   VkResult result;

   struct wrapper_image* wimg = get_wrapper_image(_device, dstImage);
   if (!wimg) {
      WLOGE("wrapper_CmdCopyBufferToImage: dstImage not tracked");
      return;
   }

   _Atomic static int counter = 0;
   int decode_id = counter++;

   bool dump_src_bcn = (get_dump_src_bcn_masks() & (1 << (wimg->original_format - 131))) != 0;
   if (dump_src_bcn) {
      WLOGD("Dumping bcn src artifacts for format=%d, decode_id=%d", wimg->original_format, decode_id);
      RecordBCnSrcArtifacts(_device, wimg->original_format, pRegions, srcBuffer, decode_id);
   }

   if (!wimg->is_bcn_emulated) {
      // If no BCn emulation needed, just fall-through
      CHECKV(CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage,
                                                      dstImageLayout, 1, pRegions));
      return;
   }

   // --- Decompression Path ---
   WLOG("Emulating support for format=%d, decode_id=%d", wimg->original_format, decode_id);
   VK_CMD_LOG("\nEmulating BCn for format=%d, decode_id=%d", wimg->original_format, decode_id);

   bool validate_bcn = (get_validate_bcn_masks() & (1 << (wimg->original_format - 131))) != 0;
   bool dump_artifacts = ((get_dump_bcn_masks() & (1 << (wimg->original_format - 131))) != 0) || validate_bcn;
   bool use_cpu_bcn = (get_host_decoding_bcn_masks() & (1 << (wimg->original_format - 131))) != 0;
   bool use_compute_shader = use_compute_shader_mode() && !use_cpu_bcn;
   bool use_image_view = use_image_view_mode() && !use_cpu_bcn && get_validate_bcn_masks() == 0 && get_dump_bcn_masks() == 0;
   bool check_for_striping = CHECK_FLAG("CHECK_FOR_STRIPING");
   // Check if the queues are the same
   struct wrapper_command_pool *pool = get_wrapper_command_pool(_device, wcb->pool);
   if (!pool) {
      WLOGE("wrapper_CmdCopyBufferToImage: wcb->pool not tracked");
   } else {
      if (pool->queue_idx != _device->graphics_queue_idx) {
         // TODO: do queue migration in the future
         WLOGE("[FATAL!] Device is not using the same queue as the command buffer, cannot use compute shader");
         use_compute_shader = false;
      }
      wcb->bcnCommands++;
   }

   struct wrapper_buffer* wbuf = get_wrapper_buffer(_device, srcBuffer);
   if (!wbuf) {
      WLOG("wrapper_CmdCopyBufferToImage: srcBuffer not tracked");
   }

   for (uint32_t i = 0; i < regionCount; ++i) {
      const VkBufferImageCopy* region = &pRegions[i];
      bool striped = false;
      if (check_for_striping) {
         if (!wbuf) {
            WLOGE("srcBuffer not tracked, skipping (decode_id=%d)", decode_id);
            return;
         } else if (wbuf->memory == VK_NULL_HANDLE) {
            WLOGE("srcBuffer not bound, skipping (decode_id=%d)", decode_id);
            return;
         }
         VkMemoryMapInfoKHR mapInfoSrc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
            .memory = wbuf->memory,
            .offset = wbuf->memoryOffset,
            .size = VK_WHOLE_SIZE,
         };
         void* srcData;
         result = WCHECK(MapMemory2KHR((VkDevice) _device, &mapInfoSrc, &srcData));
         if (result != VK_SUCCESS) {
            WLOGE("Failed to map srcBuffer memory: %d", result);
            return;
         }
         // Check if we have enough space
         int block_size = get_bc_block_size(wimg->original_format);
         int buffer_size = block_size * ((region->imageExtent.width + 3) / 4) * ((region->imageExtent.height + 3) / 4);
         // TODO: figure out the right boundary size
         striped = is_striped(wimg->original_format, srcData, region) && wbuf->vk.size >= 4 * buffer_size;
         if (striped) {
            WLOG("WARN: BCn Buffer %p (fmt=%d) is striped (decode_id=%d)", srcBuffer, wimg->original_format, decode_id);
            // TODO: Compute shader de-striping not supported
            use_compute_shader = false;
            use_image_view = false;
         }
         // TODO: cleanup memory mapping when wrapper_device_memory is virtualized
      }

      WLOGD("use_compute_shader=%d, use_image_view=%d", use_compute_shader, use_image_view);

      VkBuffer stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      if (!use_image_view) {
         result = CreateStagingBuffer(
            _device, 
            region->imageExtent.width * region->imageExtent.height * get_bc_target_size(_device->physical, wimg->original_format),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            &stagingBuffer, &stagingBufferMemory);
         if (result != VK_SUCCESS) {
            WLOGE("CreateStagingBuffer failed with %d", result);
            return;
         }

         // TODO: track these using the fence/semaphore for the submitted queue
         // This should be safe to do now with the queue synchronization, check with vvl
         WLOGD("Tracking new staging buffer for image %d", wimg->obj_id);
         TRACK_STAGING(_device, BUFFER, stagingBuffer, wimg);
         TRACK_STAGING(_device, DEVICE_MEMORY, stagingBufferMemory, wimg);
      }

      if (use_compute_shader) {
         struct InterceptorState* state = &_device->s3tc;
         if (wimg->original_format == VK_FORMAT_BC7_UNORM_BLOCK 
            || wimg->original_format == VK_FORMAT_BC7_SRGB_BLOCK) {
            state = &_device->bc7;
         } else if (wimg->original_format == VK_FORMAT_BC6H_SFLOAT_BLOCK 
            || wimg->original_format == VK_FORMAT_BC6H_UFLOAT_BLOCK) {
            state = &_device->bc6;
         }
         struct CmdComputeShaderForDecompressionArgs args = {
            ._device = _device,
            .wimg = wimg,
            .srcBuffer = srcBuffer,
            .region = region,
            .state = state,
            .use_image_view = use_image_view,
         };

         if (use_image_view) {
            args.dstImage = dstImage;
            args.dstImageLayout = dstImageLayout;
         } else {
            args.stagingBuffer = stagingBuffer;
         }

         if (CHECK_FLAG("WRAPPER_ONE_BY_ONE") || dump_artifacts) {
            WLOGD("Submitting decode_id %d", decode_id);
            result = SubmitOneTimeCommands(
               _device, 
               wcb->pool, 
               _device->graphics_queue, 
               (void (*)(struct wrapper_command_buffer*, void*)) &CmdComputeShaderForDecompression,
               &args);
            if (result != VK_SUCCESS) {
               WLOGE("GPU BCn decompression failed, expect visual glitches.");
               return;
            }
         } else {
            CmdComputeShaderForDecompression(wcb, &args);
         }
      } else {
         result = HostSideDecompression(_device, wbuf, stagingBufferMemory, region, wimg->original_format, striped);
         if (result != VK_SUCCESS) {
            WLOGE("Host BCn decompression failed, expect visual glitches.");
            return;
         }
      }

      if (dump_artifacts) {
         // Invariant: srcBuffer contains the BCn blocks, stagingBuffer contains the output
         RecordBCnArtifacts(_device, wimg->original_format, region, srcBuffer, stagingBuffer, decode_id, validate_bcn);
      }

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

         WCHECKV(CmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, NULL,
            1, &bufferBarrier,
            0, NULL
         ));

         VkBufferImageCopy local_region = *region;
         // Adjust the bufferOffset to point to the start of the staging buffer and tightly packed src stride
         local_region.bufferOffset = 0;
         local_region.bufferRowLength = 0;
         local_region.bufferImageHeight = 0;
         CHECKV(CmdCopyBufferToImage(commandBuffer, stagingBuffer, dstImage, dstImageLayout, 1, &local_region));
      }

      // Speculatively flush the Bcn emulation commands if the buffer is full by calling 
      // TODO: fix the synchronization scheme here to have n-way buffering needed to properly support this
      // if (wcb->bcnCommands >= 64) {
      //    WLOGD("Preemptively flushing BCn emulation command buffer for %d images", wcb->bcnCommands);
      //    result = SubmitSecondaryBcnCommandBuffer((VkQueue) _device->graphics_queue, wcb);
      //    if (result != VK_SUCCESS) {
      //       WLOGE("Failed to pre-emptively submit secondary BCn command buffer.");
      //    }
      // }
   }
}

WRAPPER_CreateShaderModule(VkDevice device,
    const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkShaderModule* pShaderModule) {
   VK_FROM_HANDLE(wrapper_device, wdev, device);
   bool needs_emulation = !wdev->physical->base_supported_features.shaderClipDistance && !CHECK_FLAG("DISABLE_CLIP_DISTANCE");

   if (CHECK_FLAG("FORCE_CLIP_DISTANCE")) {
      needs_emulation = true;
   }

   if (!needs_emulation) {
      return CHECK(CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule));
   }

   size_t orig_size = pCreateInfo->codeSize;
   const uint32_t* orig_code = pCreateInfo->pCode;
   struct SpirvCode lowered = { 0 };
   if (lower_eliminate_clip_distance(orig_code, orig_size / 4, &lowered)) {
      WLOGE("lower_eliminate_clip_distance failed");
      return CHECK(CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule));
   }

   VkShaderModuleCreateInfo newCreateInfo = *pCreateInfo;
   newCreateInfo.codeSize = lowered.spirv_word_count * 4;
   newCreateInfo.pCode = lowered.spirv_code;
   return CHECK(CreateShaderModule(device, &newCreateInfo, pAllocator, pShaderModule));
}

#define TEMP_ALLOC(wdev, temp, type, size) ({ \
   type* __output = wdev ? \
      ((type *) vk_zalloc(&wdev->vk.alloc, size, alignof(type), VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)) : \
      ((type *) malloc(size)); \
   if (__output) { \
      struct temp_object_node *node = wdev ? \
         vk_alloc(&wdev->vk.alloc, sizeof(struct temp_object_node), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT) : \
         malloc(sizeof(struct temp_object_node)); \
      node->ptr = __output; \
      node->device = wdev; \
      list_addtail(&node->link, &(temp)->objects); \
   } \
   __output; })

#define TEMP_ARRAY(wdev, temp, type, len, orig) ({ \
      type* __output2 = TEMP_ALLOC(wdev, temp, type, (sizeof(type) * len)); \
      for (int _i = 0; _i < len; _i++) { \
         __output2[_i] = (orig)[_i]; \
      } \
      __output2; \
   })

WRAPPER_CreateGraphicsPipelines(
    VkDevice device,
    VkPipelineCache pipelineCache,
    uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines) {
   VK_FROM_HANDLE(wrapper_device, wdev, device);
   struct temporary_objects temp;
   list_inithead(&temp.objects);
   VkGraphicsPipelineCreateInfo* create_infos = TEMP_ARRAY(wdev, &temp, VkGraphicsPipelineCreateInfo, createInfoCount, pCreateInfos);

   // Check for fillModeNonSolid support
   if (!wdev->physical->base_supported_features.fillModeNonSolid) {
      for (int i = 0; i < createInfoCount; i++) {
         if (!create_infos[i].pRasterizationState || create_infos[i].pRasterizationState->polygonMode != VK_POLYGON_MODE_LINE) {
            continue;
         }
         WLOG("VK_POLYGON_MODE_LINE requested, but fillModeNonSolid is not supported on this device, using VK_POLYGON_MODE_FILL instead");
         ((VkPipelineRasterizationStateCreateInfo*) create_infos[i].pRasterizationState)->polygonMode = VK_POLYGON_MODE_FILL;
      }
   }

   // Check for geometryShader support, needed for PVR GPUs
   // See https://gist.github.com/leegao/e24afbb5f55fe678139197d703d7f600#file-gistfile1-txt-L69
   if (!wdev->physical->base_supported_features.geometryShader) {
      for (int i = 0; i < createInfoCount; i++) {
         VkGraphicsPipelineCreateInfo* ci = &create_infos[i];
         // Get the number of non-geometryShader stages
         int stageCount = 0;
         for (int j = 0; j < ci->stageCount; j++) {
            if ((ci->pStages[j].stage & VK_SHADER_STAGE_GEOMETRY_BIT) == 0) {
               stageCount++;
            }
         }
         if (stageCount != ci->stageCount) {
            continue;
         }
         int idx = 0;
         VkPipelineShaderStageCreateInfo* stages = TEMP_ALLOC(
            wdev, &temp, VkPipelineShaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo) * stageCount);
         for (int j = 0; j < ci->stageCount; j++) {
            if ((ci->pStages[j].stage & VK_SHADER_STAGE_GEOMETRY_BIT) == 0) {
               stages[idx++] = ci->pStages[j];
            }
         }
         WLOG("A VK_SHADER_STAGE_GEOMETRY stage requested, but geometryShader not supported, disabling it instead.");
         ci->stageCount = stageCount;
         ci->pStages = stages;
      }
   }

   VkResult result = CHECK(CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines));
   free_temp_objects(&temp);
   return result;
}