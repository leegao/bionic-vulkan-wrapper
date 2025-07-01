#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "vk_unwrappers.h"
#include "vk_alloc.h"
#include "vk_common_entrypoints.h"
#include "vk_dispatch_table.h"
#include "vk_extensions.h"
#include "vk_util.h"
#include "vk_printers.h"
#include "wrapper_trampolines.h"

#if DETECT_OS_LINUX || DETECT_OS_BSD
#include <drm-uapi/drm_fourcc.h>
#endif


VKAPI_ATTR VkResult VKAPI_CALL
wrapper_CreateImage(VkDevice _device,
                    const VkImageCreateInfo* pCreateInfo,
                    const VkAllocationCallbacks* pAllocator,
                    VkImage* pImage)
{

    __vk_println("CreateImage");
#ifdef NEEDS_PRINTING_CreateImage
    __vk_println("  in: device: VkDevice (handle) = %p", _device);
#endif
#ifdef NEEDS_PRINTING_CreateImage
    __vk_println("  in: pCreateInfo: VkImageCreateInfo*");
    vk_print_VkImageCreateInfo("    ", pCreateInfo);
#endif
#ifdef NEEDS_PRINTING_CreateImage
      vk_foreach_struct_const(pnext, pCreateInfo->pNext) {
         switch ((int32_t)pnext->sType) {
         case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO: {
            VkImageFormatListCreateInfo *ext = (VkImageFormatListCreateInfo *) pnext;
            __vk_println("  Emulating VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT for bcn texture - %d formats", ext->viewFormatCount);

            for (int i = 0; i < ext->viewFormatCount; i++) {
               __vk_println("    %d: Found %d from view formats", i, ext->pViewFormats[i]);
            }
         }
         }
      }
      __vk_flush();
#endif
    
   VK_FROM_HANDLE(wrapper_device, device, _device);

   if (!pCreateInfo)
      return vk_error(&device->vk, VK_ERROR_OUT_OF_HOST_MEMORY);

   bool emulate_bcn = is_bc_image_format(pCreateInfo->format)
      && !device->physical->base_supported_features.textureCompressionBC;

   VkImageCreateInfo create_info = *pCreateInfo;
   VkResult result;

   // if (!(create_info.flags & VK_IMAGE_CREATE_ALIAS_BIT) && (/*create_info.format == VK_FORMAT_R8G8B8A8_UNORM || */create_info.format == VK_FORMAT_B8G8R8A8_UNORM)) {
   //    emulate_bcn = true;
   // }

   if (emulate_bcn) {
      __log("Calling CreateImage with bcn texture: %d", pCreateInfo->format);
      // Unwrap the create_info for modification
      // unwrap_VkImageCreateInfo(device, &create_info, pCreateInfo);
      // Replace the format parameter with VK_FORMAT_R8G8B8A8_UNORM (unless it's BC6H)
      create_info.format = unwrap_vk_format(device, pCreateInfo->format);
      create_info.usage |=  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
      create_info.flags &= 0xffffff7f;
      create_info.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
      // create_info.tiling = VK_IMAGE_TILING_OPTIMAL;

      vk_foreach_struct_const(pnext, create_info.pNext) {
         switch ((int32_t)pnext->sType) {
         case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO:
            VkImageFormatListCreateInfo *ext = (VkImageFormatListCreateInfo *) pnext;
            for (int i = 0; i < ext->viewFormatCount; i++) {
               __log("  Replacing %d from view formats", ext->pViewFormats[i]);
            }
            if (ext->pViewFormats) {
               ext->viewFormatCount = 1;
               ((VkFormat*)ext->pViewFormats)[0] = unwrap_vk_format(device, pCreateInfo->format);
            }
         }
      }

      __vk_println("  in: create_info: VkImageCreateInfo* (emulated)");
      vk_print_VkImageCreateInfo("    ", &create_info);
   }

   result = device->dispatch_table.CreateImage(device->dispatch_handle,
                                               &create_info,
                                               pAllocator,
                                               pImage);
   if (result != VK_SUCCESS) {
      __loge("ERROR: CreateImage failed with result %d", result);
      return result;
   }

   struct wrapper_image *w_image = wrapper_image_create(device, &create_info, pAllocator, *pImage);
   if (!w_image) {
      __loge("ERROR: wrapper_image_create failed");
      return vk_error(&device->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
   }

   // Track data about this image
   w_image->original_format = pCreateInfo->format;
   w_image->is_bcn_emulated = emulate_bcn;

   return result;
}

VKAPI_ATTR void VKAPI_CALL
wrapper_DestroyImage(VkDevice _device,
                     VkImage _image,
                     const VkAllocationCallbacks* pAllocator)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   device->dispatch_table.DestroyImage(device->dispatch_handle, _image, pAllocator);

   struct wrapper_image* wimg = get_wrapper_image(device, _image);
   if (!_image)
       return;
   wrapper_image_destroy(device, wimg, pAllocator);
}


VKAPI_ATTR VkResult VKAPI_CALL
wrapper_CreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView)
{
    VK_FROM_HANDLE(wrapper_device, base, device);
    VkImageViewCreateInfo _pCreateInfo = *pCreateInfo;
    if (!base->physical->base_supported_features.textureCompressionBC &&
        is_bc_image_format(pCreateInfo->format)) {
        __log("Setting format for BC image view for image: %p", pCreateInfo->image);
        _pCreateInfo.format = unwrap_vk_format(base, pCreateInfo->format);
    }
    VkResult result = wrapper_device_trampolines.CreateImageView(device, &_pCreateInfo, pAllocator, pView);
    return result;
}

// #if DETECT_OS_LINUX || DETECT_OS_BSD
// VKAPI_ATTR VkResult VKAPI_CALL
// wrapper_GetImageDrmFormatModifierPropertiesEXT(VkDevice _device,
//                                                VkImage _image,
//                                                VkImageDrmFormatModifierPropertiesEXT* pProperties)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, _image);

//    return device->dispatch_table.GetImageDrmFormatModifierPropertiesEXT(
//       device->dispatch_handle, wimg->dispatch_handle, pProperties);
// }
// #endif

// VKAPI_ATTR void VKAPI_CALL
// wrapper_GetImageSubresourceLayout(VkDevice _device,
//                                   VkImage _image,
//                                   const VkImageSubresource* pSubresource,
//                                   VkSubresourceLayout* pLayout)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, _image);

//    device->dispatch_table.GetImageSubresourceLayout(device->dispatch_handle,
//                                                     wimg->dispatch_handle,
//                                                     pSubresource,
//                                                     pLayout);
// }

// VKAPI_ATTR void VKAPI_CALL
// wrapper_GetImageMemoryRequirements(VkDevice _device,
//                                    VkImage _image,
//                                    VkMemoryRequirements* pMemoryRequirements)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, _image);

//    device->dispatch_table.GetImageMemoryRequirements(device->dispatch_handle,
//                                                      wimg->dispatch_handle,
//                                                      pMemoryRequirements);
// }

// VKAPI_ATTR void VKAPI_CALL
// wrapper_GetImageMemoryRequirements2(VkDevice _device,
//                                     const VkImageMemoryRequirementsInfo2* pInfo,
//                                     VkMemoryRequirements2* pMemoryRequirements)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, pInfo->image);
//    VkImageMemoryRequirementsInfo2 info = *pInfo;
//    info.image = wimg->dispatch_handle;

//    device->dispatch_table.GetImageMemoryRequirements2(device->dispatch_handle,
//                                                       &info,
//                                                       pMemoryRequirements);
// }

// VKAPI_ATTR VkResult VKAPI_CALL
// wrapper_BindImageMemory(VkDevice _device,
//                         VkImage _image,
//                         VkDeviceMemory memory,
//                         VkDeviceSize memoryOffset)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, _image);

//    return device->dispatch_table.BindImageMemory(device->dispatch_handle,
//                                                   wimg->dispatch_handle,
//                                                   memory,
//                                                   memoryOffset);
// }

// VKAPI_ATTR VkResult VKAPI_CALL
// wrapper_BindImageMemory2(VkDevice _device,
//                          uint32_t bindInfoCount,
//                          const VkBindImageMemoryInfo* pBindInfos)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    STACK_ARRAY(VkBindImageMemoryInfo, unwrapped_infos, bindInfoCount);
//    if (!unwrapped_infos)
//       return vk_error(&device->vk, VK_ERROR_OUT_OF_HOST_MEMORY);

//    for (uint32_t i = 0; i < bindInfoCount; i++) {
//       unwrapped_infos[i] = pBindInfos[i];
//       VK_FROM_HANDLE(wrapper_image, wimg, pBindInfos[i].image);
//       unwrapped_infos[i].image = wimg->dispatch_handle;
//    }

//    VkResult result = device->dispatch_table.BindImageMemory2(device->dispatch_handle,
//                                                              bindInfoCount,
//                                                              unwrapped_infos);

//    STACK_ARRAY_FINISH(unwrapped_infos);
//    return result;
// }

// VKAPI_ATTR void VKAPI_CALL
// wrapper_GetImageSparseMemoryRequirements(
//    VkDevice _device,
//    VkImage _image,
//    uint32_t* pSparseMemoryRequirementCount,
//    VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, _image);

//    device->dispatch_table.GetImageSparseMemoryRequirements(
//       device->dispatch_handle,
//       wimg->dispatch_handle,
//       pSparseMemoryRequirementCount,
//       pSparseMemoryRequirements);
// }

// VKAPI_ATTR void VKAPI_CALL
// wrapper_GetImageSparseMemoryRequirements2(
//    VkDevice _device,
//    const VkImageSparseMemoryRequirementsInfo2* pInfo,
//    uint32_t* pSparseMemoryRequirementCount,
//    VkSparseImageMemoryRequirements2* pSparseMemoryRequirements)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, pInfo->image);
//    VkImageSparseMemoryRequirementsInfo2 info = *pInfo;
//    info.image = wimg->dispatch_handle;

//    device->dispatch_table.GetImageSparseMemoryRequirements2(
//       device->dispatch_handle,
//       &info,
//       pSparseMemoryRequirementCount,
//       pSparseMemoryRequirements);
// }
