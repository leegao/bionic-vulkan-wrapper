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
   VK_FROM_HANDLE(wrapper_device, device, _device);
   bool emulate_bcn = is_bc_image_format(pCreateInfo->format)
      && !device->physical->base_supported_features.textureCompressionBC
      ;

   VkImageCreateInfo create_info = *pCreateInfo;
   VkResult result;

   if (emulate_bcn) {
      WLOGD("Calling CreateImage (%dx%d) with bcn texture: %d", pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->format);
      create_info.format = unwrap_vk_format(device, pCreateInfo->format); // Done within the next layer
      create_info.usage |=  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
      create_info.flags &= 0xffffff7f;
      create_info.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

      vk_foreach_struct_const(pnext, create_info.pNext) {
         switch ((int32_t)pnext->sType) {
         case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO:
            VkImageFormatListCreateInfo *ext = (VkImageFormatListCreateInfo *) pnext;
            if (ext->pViewFormats) {
               ext->viewFormatCount = 1;
               ((VkFormat*)ext->pViewFormats)[0] = unwrap_vk_format(device, pCreateInfo->format);
            }
         }
      }
   }

   result = wrapper_device_trampolines.CreateImage(_device,
                                               &create_info,
                                               pAllocator,
                                               pImage);
   if (result != VK_SUCCESS) {
      WLOGE("CreateImage failed with result %d", result);
      return result;
   }

   struct wrapper_image *w_image = wrapper_image_create(device, &create_info, pAllocator, *pImage);
   if (!w_image) {
      WLOGE("wrapper_image_create failed");
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
   wrapper_device_trampolines.DestroyImage(_device, _image, pAllocator);

   struct wrapper_image* wimg = get_wrapper_image(device, _image);
   if (!wimg) {
      WLOGD("Could not get wrapper image for %p", _image);
      return;
   }

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
    _pCreateInfo.format = unwrap_vk_format(base, pCreateInfo->format);
    return wrapper_device_trampolines.CreateImageView(device, &_pCreateInfo, pAllocator, pView);
}
