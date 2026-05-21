#include <vulkan/vulkan_core.h>
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
#include "wrapper_checks.h"

#if DETECT_OS_LINUX || DETECT_OS_BSD
#include <drm-uapi/drm_fourcc.h>
#endif

WRAPPER_CreateImage(VkDevice _device,
                    const VkImageCreateInfo* pCreateInfo,
                    const VkAllocationCallbacks* pAllocator,
                    VkImage* pImage)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   bool emulate_bcn = (is_bc123_image_format(pCreateInfo->format) && device->physical->needs_bc1_emulation) ||
                     (is_bc4567_image_format(pCreateInfo->format) && device->physical->needs_bc4_emulation);
   uint32_t disabled_mask = get_disabled_bcn_masks();
   uint32_t format_id = (uint32_t) pCreateInfo->format - VK_FORMAT_BC1_RGB_UNORM_BLOCK;
   if ((disabled_mask & (1 << format_id)) != 0) {
      emulate_bcn = false;
   }

   struct temporary_objects temp;
   list_inithead(&temp.objects);

   VkImageCreateInfo *create_info = TEMP_OBJECT(device, &temp, VkImageCreateInfo, pCreateInfo);
   bool is_depth_stencil_reduced = false;
   VkFormat original_format = pCreateInfo->format;
   VkFormat new_format = pCreateInfo->format;
   VkResult result;

   if (device->depth_override_mode != OVERRIDE_NONE &&
       (create_info->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {

      if (device->depth_override_mode == OVERRIDE_DISABLED) {
         WLOGE("Depth override: Blocking creation of depth image.");
         free_temp_objects(&temp);
         return VK_ERROR_FORMAT_NOT_SUPPORTED;
      }
      new_format = get_depth_stencil_vk_format(device, original_format);
      if (new_format != original_format) {
         WLOGD("Depth override: Changing image format from %d to %d", original_format, new_format);
         create_info->format = new_format;
         is_depth_stencil_reduced = true;
      }
   } else if (emulate_bcn) {
      create_info->format = (new_format = unwrap_vk_format(device, original_format)); // Done within the next layer
      WLOGD("Emulate BCn: Changing image (%dx%d) format from bcn texture: %d to %d", 
         pCreateInfo->extent.width, pCreateInfo->extent.height, original_format, new_format);
      create_info->usage |=  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
      create_info->flags &= 0xffffff7f;
      create_info->flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

      vk_foreach_struct_const(pnext, create_info->pNext) {
         switch ((int32_t)pnext->sType) {
         case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO:
            {            
               VkImageFormatListCreateInfo *ext = (VkImageFormatListCreateInfo *) pnext;
               if (ext->pViewFormats) {
                  ext->viewFormatCount = 1;
                  ((VkFormat*)ext->pViewFormats)[0] = new_format;
               }
            }
            break;
         default:
            break;
         }
      }
   }

   result = CHECK(CreateImage(_device, create_info, pAllocator, pImage));
   if (result != VK_SUCCESS) {
      if (is_depth_stencil_reduced) {
         WLOGE("CreateImage failed with modified depth format (%d -> %d), try unsetting WRAPPER_REDUCE_DEPTH_FORMAT", original_format, new_format);
      }
      free_temp_objects(&temp);
      return result;
   }

   struct wrapper_image *wimg = wrapper_image_create(device, create_info, *pImage);
   if (!wimg) {
      free_temp_objects(&temp);
      return vk_error(&device->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
   }

   // Track data about this image
   wimg->original_format = original_format;
   wimg->format = new_format;
   wimg->is_bcn_emulated = emulate_bcn;
   wimg->is_depth_stencil_reduced = is_depth_stencil_reduced;
   wimg->is_etc2_encoded = new_format == VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;

   free_temp_objects(&temp);
   return result;
}

WRAPPER_DestroyImage(VkDevice _device,
                     VkImage _image,
                     const VkAllocationCallbacks* pAllocator)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   CHECKV(DestroyImage(_device, _image, pAllocator));

   struct wrapper_image* wimg = get_wrapper_image(device, _image);
   if (!wimg) {
      WLOGD("Could not get wrapper image for %p", _image);
      return;
   }

   wrapper_image_destroy(device, wimg);
}

WRAPPER_CreateImageView(
    VkDevice device,
    const VkImageViewCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkImageView* pView)
{
    VK_FROM_HANDLE(wrapper_device, base, device);
    VkImageViewCreateInfo ci = *pCreateInfo;
    struct wrapper_image* wimg = get_wrapper_image(base, pCreateInfo->image);
    if (wimg) {
        if ((wimg->is_bcn_emulated || wimg->is_depth_stencil_reduced) && (wimg->format != wimg->original_format)) {
            WLOGD("Patching ImageView format for tracked image from %d to %d (original img format: %d)", ci.format, wimg->format, wimg->original_format);
            ci.format = wimg->format;
            // VUID-VkImageViewCreateInfo-subresourceRange-09594
            if (!HAS_STENCIL(wimg->format) && (ci.subresourceRange.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
                ci.subresourceRange.aspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
    }
    return CHECK(CreateImageView(device, &ci, pAllocator, pView));
}

WRAPPER_CmdClearDepthStencilImage(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout imageLayout,
    const VkClearDepthStencilValue* pDepthStencil,
    uint32_t rangeCount,
    const VkImageSubresourceRange* pRanges)
{
    VK_FROM_HANDLE(wrapper_command_buffer, base, commandBuffer);
    struct temporary_objects temp;
    list_inithead(&temp.objects);
    VkImageSubresourceRange* ranges = TEMP_ARRAY(base->device, &temp, VkImageSubresourceRange, rangeCount, pRanges);
    struct wrapper_image* wimg = get_wrapper_image(base->device, image);
    if (wimg) {
        for (int i = 0; i < rangeCount; i++) {
            // VUID-vkCmdClearDepthStencilImage-image-02825
            if (!HAS_STENCIL(wimg->format) && (pRanges[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
                ranges[i].aspectMask &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
    }

    CHECKV(CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, ranges));
    free_temp_objects(&temp);
}
