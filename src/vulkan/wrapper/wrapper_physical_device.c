#include <math.h>

#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "wrapper_trampolines.h"
#include "vk_alloc.h"
#include "vk_common_entrypoints.h"
#include "vk_dispatch_table.h"
#include "vk_extensions.h"
#include "vk_physical_device.h"
#include "vk_util.h"
#include "wsi_common.h"
#include "util/os_misc.h"
#include "vk_printers.h"

static VkResult
wrapper_setup_device_extensions(struct wrapper_physical_device *pdevice) {
   struct vk_device_extension_table *exts = &pdevice->vk.supported_extensions;
   VkExtensionProperties pdevice_extensions[VK_DEVICE_EXTENSION_COUNT];
   uint32_t pdevice_extension_count = VK_DEVICE_EXTENSION_COUNT;
   VkResult result;

   result = pdevice->dispatch_table.EnumerateDeviceExtensionProperties(
      pdevice->dispatch_handle, NULL, &pdevice_extension_count, pdevice_extensions);

   if (result != VK_SUCCESS)
      return result;
   
   static bool has_already_logged_properties = false;
   if (!has_already_logged_properties) {
      // has_already_logged_properties = true;
      WLOGD("EnumerateDeviceExtensionProperties:");
      for (int i = 0; i < pdevice_extension_count; i++) {
         LOG_STRUCT(VkExtensionProperties, &pdevice_extensions[i]);
      }
   }

   *exts = wrapper_device_extensions;

   for (int i = 0; i < pdevice_extension_count; i++) {
      int idx;
      for (idx = 0; idx < VK_DEVICE_EXTENSION_COUNT; idx++) {
         if (strcmp(vk_device_extensions[idx].extensionName,
                     pdevice_extensions[i].extensionName) == 0)
            break;
      }

      if (idx >= VK_DEVICE_EXTENSION_COUNT)
         continue;

      if (wrapper_filter_extensions.extensions[idx])
         continue;

      pdevice->base_supported_extensions.extensions[idx] =
         exts->extensions[idx] = true;
   }

   exts->KHR_present_wait = exts->KHR_timeline_semaphore;
   if (CHECK_FLAG("NO_PRESENT_WAIT")) {
      if (exts->KHR_present_wait) {
         WLOG("Disabling KHR_present_wait, some drivers misreport KHR_timeline_semaphore support (e.g. Adreno 6XX, disable by setting NO_PRESENT_WAIT=0)");
         exts->KHR_present_wait = false;
      }
   }

   // Needed by dxvk
   exts->EXT_transform_feedback = true;
   exts->EXT_host_query_reset = true;
   exts->EXT_custom_border_color = true;
   // DEBUG: Disable VK_KHR_external_memory_fd to avoid dxvk assuming it has VK_KHR_external_memory_win32
   // exts->KHR_external_memory_fd = false;

   return VK_SUCCESS;
}

static void
wrapper_apply_device_extension_blacklist(struct wrapper_physical_device *physical_device) {
   char *blacklist = getenv("WRAPPER_EXTENSION_BLACKLIST");
   if (!blacklist)
      return;
   char *extension = strtok(blacklist, ",");
   while (extension != NULL) {
      for (int i = 0; i < VK_DEVICE_EXTENSION_COUNT; i++) {
         if (strstr(extension, vk_device_extensions[i].extensionName)) {
            physical_device->vk.supported_extensions.extensions[i] = false;
         }
      }
      extension = strtok(NULL, ",");
   }
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
wrapper_wsi_proc_addr(VkPhysicalDevice physicalDevice, const char *pName)
{
   VK_FROM_HANDLE(vk_physical_device, pdevice, physicalDevice);
   return vk_instance_get_proc_addr_unchecked(pdevice->instance, pName);
}

VkResult enumerate_physical_device(struct vk_instance *_instance)
{
   struct wrapper_instance *instance = (struct wrapper_instance *)_instance;
   VkPhysicalDevice physical_devices[16];
   uint32_t physical_device_count = 16;
   VkResult result;

   result = instance->dispatch_table.EnumeratePhysicalDevices(
      instance->dispatch_handle, &physical_device_count, physical_devices);

   if (result != VK_SUCCESS)
      return result;

   for (int i = 0; i < physical_device_count; i++) {
      PFN_vkGetInstanceProcAddr get_instance_proc_addr;
      struct wrapper_physical_device *pdevice;

      pdevice = vk_zalloc(&_instance->alloc, sizeof(*pdevice), 8,
                          VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
      if (!pdevice)
         return VK_ERROR_OUT_OF_HOST_MEMORY;

      struct vk_physical_device_dispatch_table dispatch_table;
      vk_physical_device_dispatch_table_from_entrypoints(
         &dispatch_table, &wrapper_physical_device_entrypoints, true);
      vk_physical_device_dispatch_table_from_entrypoints(
         &dispatch_table, &wsi_physical_device_entrypoints, false);
      vk_physical_device_dispatch_table_from_entrypoints(
         &dispatch_table, &wrapper_physical_device_trampolines, false);

      result = vk_physical_device_init(&pdevice->vk,
                                       &instance->vk,
                                       NULL, NULL, NULL,
                                       &dispatch_table);
      if (result != VK_SUCCESS) {
         vk_free(&_instance->alloc, pdevice);
         return result;
      }

      pdevice->instance = instance;
      pdevice->dispatch_handle = physical_devices[i];
      get_instance_proc_addr = instance->dispatch_table.GetInstanceProcAddr;

      vk_physical_device_dispatch_table_load(&pdevice->dispatch_table,
                                             get_instance_proc_addr,
                                             instance->dispatch_handle);

      wrapper_setup_device_extensions(pdevice);
      wrapper_apply_device_extension_blacklist(pdevice);
      wrapper_setup_device_features(pdevice);

      struct vk_features *supported_features = &pdevice->vk.supported_features;
      pdevice->base_supported_features = *supported_features;

      supported_features->geometryShader = true;
      supported_features->presentId = true;
      supported_features->multiViewport = true;
      supported_features->depthClamp = true;
      supported_features->depthBiasClamp = true;
      supported_features->memoryMapPlaced = true;
      supported_features->memoryUnmapReserve = true;
      supported_features->textureCompressionBC = true;
      supported_features->fillModeNonSolid = true;
      supported_features->shaderClipDistance = true;
      supported_features->shaderCullDistance = true;
      supported_features->presentWait = supported_features->timelineSemaphore;
      supported_features->swapchainMaintenance1 = true;
      supported_features->imageCompressionControlSwapchain = false;

      // dxvk extension features support
      supported_features->geometryStreams = true;
      supported_features->transformFeedback = true;
      supported_features->hostQueryReset = true;
      supported_features->customBorderColors = true;
      supported_features->customBorderColorWithoutFormat = true;
      supported_features->dualSrcBlend = true; // Missing on G715 r38p1
      // supported_features->logicOp = true; // Missing on G57 r32p1
      supported_features->multiDrawIndirect = true; // Missing on G57 r32p1
      supported_features->vertexPipelineStoresAndAtomics = true; // Missing on G57 r32p1
      // supported_features->variableMultisampleRate = true; // Missing on G57 r32p1
      
      result = wsi_device_init(&pdevice->wsi_device,
                               wrapper_physical_device_to_handle(pdevice),
                               wrapper_wsi_proc_addr, &_instance->alloc, -1,
                               NULL, &(struct wsi_device_options){});
      if (result != VK_SUCCESS) {
         vk_physical_device_finish(&pdevice->vk);
         vk_free(&_instance->alloc, pdevice);
         return result;
      }
      pdevice->vk.wsi_device = &pdevice->wsi_device;
      pdevice->wsi_device.force_bgra8_unorm_first = true;
#ifdef __TERMUX__
      pdevice->wsi_device.wants_ahardware_buffer = true;
#endif

      pdevice->driver_properties = (VkPhysicalDeviceDriverProperties) {
         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
      };
      pdevice->properties2 = (VkPhysicalDeviceProperties2) {
         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
         .pNext = &pdevice->driver_properties,
      };

      WPDEVICE.GetPhysicalDeviceProperties2(
         (VkPhysicalDevice) pdevice, &pdevice->properties2);
      
      WPDEVICE.GetPhysicalDeviceMemoryProperties(
         (VkPhysicalDevice) pdevice, &pdevice->memory_properties);
      
      WLOGD("GetPhysicalDeviceProperties2:");
      LOG_STRUCT(VkPhysicalDeviceProperties2, &pdevice->properties2);
      WLOGD("GetPhysicalDeviceMemoryProperties:");
      LOG_STRUCT(VkPhysicalDeviceMemoryProperties, &pdevice->memory_properties);
      
      const char *app_name = instance->vk.app_info.app_name
         ? instance->vk.app_info.app_name : "wrapper";

      bool is_dxvk = (instance->vk.app_info.engine_name != NULL) && (strcmp(instance->vk.app_info.engine_name, "DXVK") == 0);
      if (!is_dxvk) WLOGE("WARNING: this wrapper may not be compatible for non-dxvk game engines, use at your own risks");

#define DXVK(a, b, c) (a * 0x400000 + b * 0x1000 + c);
      bool is_dxvk_2_plus = is_dxvk && instance->vk.app_info.engine_version >= DXVK(2, 0, 0);
      bool is_dxvk_1_10_3 = is_dxvk && instance->vk.app_info.engine_version == DXVK(1, 10, 3);

      WLOGD("AppInfo: app_name = %s, engine_name = %s, engine_version = %x",
         instance->vk.app_info.app_name ? instance->vk.app_info.app_name : "(unknown)",
         instance->vk.app_info.engine_name ? instance->vk.app_info.engine_name : "(unknown)",
         instance->vk.app_info.engine_version);
      WLOGD("DriverInfo: driver_id = %d, driver_name = %s, driver_info = %s",
         pdevice->driver_properties.driverID,
         pdevice->driver_properties.driverName,
         pdevice->driver_properties.driverInfo);

      if (pdevice->driver_properties.driverID == VK_DRIVER_ID_QUALCOMM_PROPRIETARY &&
          pdevice->properties2.properties.driverVersion > VK_MAKE_VERSION(512, 744, 0) &&
          strstr(app_name, "clvk")) {
         /* HACK: Fixed clvk not working on qualcomm proprietary driver. */
         supported_features->globalPriorityQuery = false;
      }

      if (pdevice->driver_properties.driverID == VK_DRIVER_ID_ARM_PROPRIETARY
            && pdevice->vk.supported_extensions.EXT_extended_dynamic_state
            && !is_dxvk_2_plus) {
         WLOG("Disabling VK_EXT_extended_dynamic_state on Mali proprietary drivers");
         pdevice->vk.supported_extensions.EXT_extended_dynamic_state = false;
         pdevice->vk.supported_extensions.EXT_extended_dynamic_state2 = false;
         pdevice->vk.supported_extensions.EXT_extended_dynamic_state3 = false;
      }

      if (pdevice->driver_properties.driverID == VK_DRIVER_ID_QUALCOMM_PROPRIETARY) {
         WLOG("Disabling VK_KHR_shader_float_controls on Qualcom proprietary drivers");
         pdevice->vk.supported_extensions.KHR_shader_float_controls = false;
      }

      pdevice->dma_heap_fd = open("/dev/dma_heap/system", O_RDONLY);
      if (pdevice->dma_heap_fd < 0)
         pdevice->dma_heap_fd = open("/dev/ion", O_RDONLY);

      // Check for BC1 and BC4 support on Xclipse devices
      WPDEVICE.GetPhysicalDeviceFormatProperties((VkPhysicalDevice) pdevice, VK_FORMAT_BC1_RGB_UNORM_BLOCK, &pdevice->bc1_format_properties);
      WLOGD("bc1 support:");
      LOG_STRUCT(VkFormatProperties, &pdevice->bc1_format_properties);
      WPDEVICE.GetPhysicalDeviceFormatProperties((VkPhysicalDevice) pdevice, VK_FORMAT_BC4_UNORM_BLOCK, &pdevice->bc4_format_properties);
      WLOGD("bc4 support:");
      LOG_STRUCT(VkFormatProperties, &pdevice->bc4_format_properties);

      pdevice->needs_bc1_emulation = !pdevice->base_supported_features.textureCompressionBC && !has_bc1_support(pdevice);
      pdevice->needs_bc4_emulation = !pdevice->base_supported_features.textureCompressionBC && !has_bc4_support(pdevice);

      if (CHECK_FLAG("FORCE_BCN_EMULATION")) {
         if (pdevice->base_supported_features.textureCompressionBC) {
            WLOG("Forcing BCn emulation (disable by setting FORCE_BCN_EMULATION=0)");
            pdevice->base_supported_features.textureCompressionBC = false;
         }
         pdevice->needs_bc1_emulation = true;
         pdevice->needs_bc4_emulation = true;
      }

      if (CHECK_FLAG("NO_BCN_EMULATION")) {
         WLOG("Disabling BCn emulation (disable by setting NO_BCN_EMULATION=0)");
         pdevice->needs_bc1_emulation = false;
         pdevice->needs_bc4_emulation = false;
      }

      if (CHECK_FLAG("NO_BC123_EMULATION")) {
         WLOG("Disabling BC123 emulation (disable by setting NO_BC123_EMULATION=0)");
         pdevice->needs_bc1_emulation = false;
      }

      list_addtail(&pdevice->vk.link, &_instance->physical_devices.list);
   }

   return VK_SUCCESS;
}

void destroy_physical_device(struct vk_physical_device *pdevice) {
   VK_FROM_HANDLE(wrapper_physical_device, wpdevice,
                  vk_physical_device_to_handle(pdevice));
   if (wpdevice->dma_heap_fd != -1)
      close(wpdevice->dma_heap_fd);
   wsi_device_finish(pdevice->wsi_device, &pdevice->instance->alloc);
   vk_physical_device_finish(pdevice);
   vk_free(&pdevice->instance->alloc, pdevice);
}

WRAPPER_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                           const char* pLayerName,
                                           uint32_t* pPropertyCount,
                                           VkExtensionProperties* pProperties)
{
   VkResult result = vk_common_EnumerateDeviceExtensionProperties(physicalDevice,
                                                       pLayerName,
                                                       pPropertyCount,
                                                       pProperties);

   // WLOGE("wrapper_EnumerateDeviceExtensionProperties called for %s:", pLayerName);
   // for (int i = 0; i < *pPropertyCount; i++) {
   //    LOG_STRUCT(VkExtensionProperties, &pProperties[i]);
   // }
   
   return result;
}

WRAPPER_GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                  VkPhysicalDeviceFeatures* pFeatures) 
{
   return vk_common_GetPhysicalDeviceFeatures(physicalDevice, pFeatures);
}

WRAPPER_GetPhysicalDeviceFeatures2(VkPhysicalDevice physicalDevice,
                                   VkPhysicalDeviceFeatures2* pFeatures) {                                                              
   vk_common_GetPhysicalDeviceFeatures2(physicalDevice, pFeatures);
   // Fake select dxvk 1.10.3 mandatory features
   vk_foreach_struct(pnext, pFeatures->pNext) {
      switch (pnext->sType) {
         case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT:
         {
            VkPhysicalDeviceTransformFeedbackFeaturesEXT *extTransformFeedback =
                (VkPhysicalDeviceTransformFeedbackFeaturesEXT*) pnext;
            extTransformFeedback->transformFeedback = true;
            extTransformFeedback->geometryStreams = true;
            break;
         }
         case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT:
         {
            VkPhysicalDeviceCustomBorderColorFeaturesEXT *extCustomBorderColor =
                (VkPhysicalDeviceCustomBorderColorFeaturesEXT*) pnext;
            extCustomBorderColor->customBorderColors = VK_TRUE;
            extCustomBorderColor->customBorderColorWithoutFormat = VK_TRUE;
            break;
         }
         case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT:
         {
            VkPhysicalDeviceHostQueryResetFeaturesEXT *extHostQueryReset =
                (VkPhysicalDeviceHostQueryResetFeaturesEXT*) pnext;
            extHostQueryReset->hostQueryReset = VK_TRUE;
            break;
         }
         default:
            break;
      }
   }
}

WRAPPER_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
                                     VkPhysicalDeviceProperties2* pProperties)
{
   VK_FROM_HANDLE(wrapper_physical_device, pdevice, physicalDevice);
   pdevice->dispatch_table.GetPhysicalDeviceProperties2(
      pdevice->dispatch_handle, pProperties);

   vk_foreach_struct(prop, pProperties->pNext) {
      switch (prop->sType) {
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAP_MEMORY_PLACED_PROPERTIES_EXT:
      {
         VkPhysicalDeviceMapMemoryPlacedPropertiesEXT *placed_prop =
               (VkPhysicalDeviceMapMemoryPlacedPropertiesEXT *)prop;
         uint64_t os_page_size;
         os_get_page_size(&os_page_size);
         placed_prop->minPlacedMemoryMapAlignment = os_page_size;
         break;
      }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TEXEL_BUFFER_ALIGNMENT_PROPERTIES_EXT:
      {
      	 VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT *texel_prop =
      	      (VkPhysicalDeviceTexelBufferAlignmentPropertiesEXT *)prop;
      	 texel_prop->storageTexelBufferOffsetAlignmentBytes = 1;
      	 texel_prop->uniformTexelBufferOffsetAlignmentBytes = 1;
      	 break;
      }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES_KHR:
      {
         VkPhysicalDeviceFloatControlsPropertiesKHR *float_prop =
              (VkPhysicalDeviceFloatControlsPropertiesKHR *)prop;
         float_prop->denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE;
         float_prop->roundingModeIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE;     
         float_prop->shaderDenormFlushToZeroFloat16 = false;
         float_prop->shaderDenormFlushToZeroFloat32 = false;
         float_prop->shaderRoundingModeRTEFloat16 = false;
         float_prop->shaderRoundingModeRTEFloat32 = false;
         float_prop->shaderSignedZeroInfNanPreserveFloat16 = false;
         float_prop->shaderSignedZeroInfNanPreserveFloat32 = false;
         break;
      }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT:
      {
         VkPhysicalDeviceTransformFeedbackPropertiesEXT *feedback_prop = 
              (VkPhysicalDeviceTransformFeedbackPropertiesEXT *)prop;
         feedback_prop->maxTransformFeedbackStreams = 4;
         feedback_prop->maxTransformFeedbackBuffers = 4;
         feedback_prop->maxTransformFeedbackBufferSize = 0xffffffff;
         feedback_prop->maxTransformFeedbackStreamDataSize = 512;
         feedback_prop->maxTransformFeedbackBufferDataSize = 512;
         feedback_prop->maxTransformFeedbackBufferDataStride = 512;
         feedback_prop->transformFeedbackQueries = true;
         feedback_prop->transformFeedbackStreamsLinesTriangles = true;
         feedback_prop->transformFeedbackRasterizationStreamSelect = false;
         feedback_prop->transformFeedbackDraw = true;
         break;
      }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES:
      {
         VkPhysicalDeviceVulkan11Properties *vk11_prop =
              (VkPhysicalDeviceVulkan11Properties *)prop;
         vk11_prop->subgroupSupportedOperations = 0;
         vk11_prop->subgroupSupportedStages = 0;
         break;
      }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES:
      {
         VkPhysicalDeviceVulkan12Properties *vk12_prop =
              (VkPhysicalDeviceVulkan12Properties *)prop;
         vk12_prop->denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE;
         vk12_prop->roundingModeIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE;
         vk12_prop->shaderDenormFlushToZeroFloat16 = false;
         vk12_prop->shaderDenormFlushToZeroFloat32 = false;
         vk12_prop->shaderRoundingModeRTEFloat16 = false;
         vk12_prop->shaderRoundingModeRTEFloat32 = false;
         vk12_prop->shaderSignedZeroInfNanPreserveFloat16 = false;
         vk12_prop->shaderSignedZeroInfNanPreserveFloat32 = false;
         break;
      }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES:
      {
         VkPhysicalDeviceVulkan13Properties *vk13_prop =
              (VkPhysicalDeviceVulkan13Properties *)prop;
         vk13_prop->storageTexelBufferOffsetAlignmentBytes = 1;
         vk13_prop->uniformTexelBufferOffsetAlignmentBytes = 1;
         break;
      }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES:
      {
         VkPhysicalDeviceSubgroupProperties *subgroup_prop =
              (VkPhysicalDeviceSubgroupProperties *)prop;
         subgroup_prop->supportedOperations = 0;
         subgroup_prop->supportedStages = 0;
         break;
      }
      default:
         break;
      }
   }
}

WRAPPER_GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice,
	                                           VkFormat format,
	                                           VkImageType type,
	                                           VkImageTiling tiling,
	                                           VkImageUsageFlags usage,
	                                           VkImageCreateFlags flags,
	                                           VkImageFormatProperties *pImageFormatProperties)
{
   VkResult result;
   VK_FROM_HANDLE(wrapper_physical_device, pdevice, physicalDevice);

   result = wrapper_physical_device_trampolines.GetPhysicalDeviceImageFormatProperties(
      physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
      
   if (result == VK_ERROR_FORMAT_NOT_SUPPORTED && is_bc_image_format(format)) {
      if (type & VK_IMAGE_TYPE_1D) {
         pImageFormatProperties->maxExtent.width = pdevice->properties2.properties.limits.maxImageDimension1D;
         pImageFormatProperties->maxExtent.height = 1;
         pImageFormatProperties->maxExtent.depth = 1;
      }
      if (type & VK_IMAGE_TYPE_2D) {
         if (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
            pImageFormatProperties->maxExtent.width = pdevice->properties2.properties.limits.maxImageDimensionCube;
            pImageFormatProperties->maxExtent.height = pdevice->properties2.properties.limits.maxImageDimensionCube;
         }
         else {
            pImageFormatProperties->maxExtent.width = pdevice->properties2.properties.limits.maxImageDimension2D;
            pImageFormatProperties->maxExtent.height = pdevice->properties2.properties.limits.maxImageDimension2D;
         }
         pImageFormatProperties->maxExtent.depth = 1;
      }
      if (type & VK_IMAGE_TYPE_3D) {
         pImageFormatProperties->maxExtent.width = pdevice->properties2.properties.limits.maxImageDimension3D;
         pImageFormatProperties->maxExtent.height = pdevice->properties2.properties.limits.maxImageDimension3D;
         pImageFormatProperties->maxExtent.depth = pdevice->properties2.properties.limits.maxImageDimension3D;
      }
      if (tiling & VK_IMAGE_TILING_LINEAR ||
             tiling & VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT ||
             flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)
             pImageFormatProperties->maxMipLevels = 1;
      else 
         pImageFormatProperties->maxMipLevels = log2(
            pImageFormatProperties->maxExtent.width > pImageFormatProperties->maxExtent.height ? pImageFormatProperties->maxExtent.width :  pImageFormatProperties->maxExtent.height 	
         );
    
      if (tiling & VK_IMAGE_TILING_LINEAR ||
            ((tiling & VK_IMAGE_TILING_OPTIMAL) && type & VK_IMAGE_TYPE_3D))
         pImageFormatProperties->maxArrayLayers = 1;
      else
         pImageFormatProperties->maxArrayLayers = pdevice->properties2.properties.limits.maxImageArrayLayers;
      // We do not handle any case here for now
      pImageFormatProperties->sampleCounts = VK_SAMPLE_COUNT_1_BIT;      
      pImageFormatProperties->maxResourceSize = 562949953421312;
      return VK_SUCCESS;
   }

   return result;   
}	                                           

WRAPPER_GetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice,
                                                const VkPhysicalDeviceImageFormatInfo2* pImageFormatInfo,
                                                VkImageFormatProperties2* pImageFormatProperties)
{
   VkResult result;
   VK_FROM_HANDLE(wrapper_physical_device, pdevice, physicalDevice);
   result = wrapper_physical_device_trampolines.GetPhysicalDeviceImageFormatProperties2(
         physicalDevice, pImageFormatInfo, pImageFormatProperties);
   if (result == VK_ERROR_FORMAT_NOT_SUPPORTED && is_bc_image_format(pImageFormatInfo->format)) {
      if (pImageFormatInfo->type & VK_IMAGE_TYPE_1D) {
         pImageFormatProperties->imageFormatProperties.maxExtent.width = pdevice->properties2.properties.limits.maxImageDimension1D;
         pImageFormatProperties->imageFormatProperties.maxExtent.height = 1;
         pImageFormatProperties->imageFormatProperties.maxExtent.depth = 1;
      }
      if (pImageFormatInfo->type & VK_IMAGE_TYPE_2D) {
         if (pImageFormatInfo->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
            pImageFormatProperties->imageFormatProperties.maxExtent.width = pdevice->properties2.properties.limits.maxImageDimensionCube;
            pImageFormatProperties->imageFormatProperties.maxExtent.height = pdevice->properties2.properties.limits.maxImageDimensionCube;
         }
         else {
            pImageFormatProperties->imageFormatProperties.maxExtent.width = pdevice->properties2.properties.limits.maxImageDimension2D;
            pImageFormatProperties->imageFormatProperties.maxExtent.height = pdevice->properties2.properties.limits.maxImageDimension2D;
         }
         pImageFormatProperties->imageFormatProperties.maxExtent.depth = 1;
      }
      if (pImageFormatInfo->type & VK_IMAGE_TYPE_3D) {
         pImageFormatProperties->imageFormatProperties.maxExtent.width = pdevice->properties2.properties.limits.maxImageDimension3D;
         pImageFormatProperties->imageFormatProperties.maxExtent.height = pdevice->properties2.properties.limits.maxImageDimension3D;
         pImageFormatProperties->imageFormatProperties.maxExtent.depth = pdevice->properties2.properties.limits.maxImageDimension3D;
      }
      // We do not handle the case where vkPhysicalDeviceImageFormatInfo pNext has
      // a handleType which does not require mipMaps
      if (pImageFormatInfo->tiling & VK_IMAGE_TILING_LINEAR ||
             pImageFormatInfo->tiling & VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT ||
             pImageFormatInfo->flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT)
             pImageFormatProperties->imageFormatProperties.maxMipLevels = 1;
      else 
         pImageFormatProperties->imageFormatProperties.maxMipLevels = log2(
            pImageFormatProperties->imageFormatProperties.maxExtent.width > pImageFormatProperties->imageFormatProperties.maxExtent.height ? pImageFormatProperties->imageFormatProperties.maxExtent.width :  pImageFormatProperties->imageFormatProperties.maxExtent.height 	
         );
    
      if (pImageFormatInfo->tiling & VK_IMAGE_TILING_LINEAR ||
            ((pImageFormatInfo->tiling & VK_IMAGE_TILING_OPTIMAL) && pImageFormatInfo->type & VK_IMAGE_TYPE_3D))
         pImageFormatProperties->imageFormatProperties.maxArrayLayers = 1;
      else
         pImageFormatProperties->imageFormatProperties.maxArrayLayers = pdevice->properties2.properties.limits.maxImageArrayLayers;
      // We do not handle any case here for now
      pImageFormatProperties->imageFormatProperties.sampleCounts = VK_SAMPLE_COUNT_1_BIT;      
      pImageFormatProperties->imageFormatProperties.maxResourceSize = 562949953421312;
      return VK_SUCCESS;
   }

   // Unset the external memory bit
   // GetPhysicalDeviceImageFormatProperties2 (id=3280)
   //    in: physicalDevice: VkPhysicalDevice (handle) = 0xb400007940754010 (id=3280)
   //    in: pImageFormatInfo: VkPhysicalDeviceImageFormatInfo2* (id=3280)
   //       .format: VkFormat = 0x2c
   //       .type: VkImageType = 0x1
   //       .tiling: VkImageTiling = 0x0
   //       .usage: VkImageUsageFlags = 0x7
   //       .flags: VkImageCreateFlags = 0x8
   //       .pNext(VkPhysicalDeviceExternalImageFormatInfo)
   //          .handleType: VkExternalMemoryHandleTypeFlagBits = 0x1
   //    out: result: VkResult = 0 (id=3280)
   //    out: pImageFormatProperties: VkImageFormatProperties2* (id=3280)
   //       .imageFormatProperties: VkImageFormatProperties
   //          .maxExtent: VkExtent3D
   //          .width: uint32_t = 0x4000
   //          .height: uint32_t = 0x4000
   //          .depth: uint32_t = 0x1
   //          .maxMipLevels: uint32_t = 0xf
   //          .maxArrayLayers: uint32_t = 0x800
   //          .sampleCounts: VkSampleCountFlags = 0x7
   //          .maxResourceSize: VkDeviceSize = 0xffffffff
   //       .pNext(VkExternalImageFormatProperties)
   //          .externalMemoryProperties: VkExternalMemoryProperties
   //          .externalMemoryFeatures: VkExternalMemoryFeatureFlags = 0x7
   //          .exportFromImportedHandleTypes: VkExternalMemoryHandleTypeFlags = 0x201
   //          .compatibleHandleTypes: VkExternalMemoryHandleTypeFlags = 0x201
   if (CHECK_FLAG("DISABLE_EXTERNAL_FD")) {
      vk_foreach_struct(pnext, pImageFormatProperties->pNext) {
         if (pnext->sType == VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES) {
            WLOGD("Unsetting VkExternalImageFormatProperties::externalMemoryFeatures");
            VkExternalImageFormatProperties* obj = (VkExternalImageFormatProperties*) pnext;
            obj->externalMemoryProperties.externalMemoryFeatures = 0;
         }
      }
   }

   return result;
}                                                

WRAPPER_GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice,
                                            VkFormat format,
                                            VkFormatProperties* pFormatProperties)
{
   VK_FROM_HANDLE(wrapper_physical_device, pdevice, physicalDevice);
   struct vk_features *supported_features = &pdevice->base_supported_features;

   VkFormat targetFormat = format;
   bool modified = false;
   if (is_bc_image_format(format) && !supported_features->textureCompressionBC) {
      targetFormat = unwrap_vk_format_physical_device(pdevice, format);
      PCHECKV(GetPhysicalDeviceFormatProperties(physicalDevice, targetFormat, pFormatProperties));
      pFormatProperties->optimalTilingFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
      pFormatProperties->linearTilingFeatures |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
      pFormatProperties->bufferFeatures |= VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;

   } else {
      // If the underlying device support BCn or if this is not a BCn texture,
      // just pass through
      PCHECKV(GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatProperties));
      return;
   }
}

