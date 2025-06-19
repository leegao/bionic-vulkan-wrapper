#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "vk_alloc.h"
#include "vk_common_entrypoints.h"
#include "vk_dispatch_table.h"
#include "vk_extensions.h"
#include "vk_util.h"

#if DETECT_OS_LINUX || DETECT_OS_BSD
#include <drm-uapi/drm_fourcc.h>
#endif


static VkResult
wrapper_image_create(struct wrapper_device *device,
                     const VkImageCreateInfo *pCreateInfo,
                     const VkAllocationCallbacks *pAllocator,
                     VkImage dispatch_handle,
                     VkImage *p_wimg)
{
   struct wrapper_image *wimg;

   wimg = vk_object_zalloc(&device->vk, pAllocator,
                           sizeof(struct wrapper_image),
                           VK_OBJECT_TYPE_IMAGE);
   if (!wimg)
      return vk_error(&device->vk, VK_ERROR_OUT_OF_HOST_MEMORY);

   vk_image_init(&device->vk, &wimg->vk, pCreateInfo);

   wimg->device = device;
   wimg->dispatch_handle = dispatch_handle;
   list_add(&wimg->link, &device->image_list);

   *p_wimg = wrapper_image_to_handle(wimg);

   return VK_SUCCESS;
}

void
wrapper_image_destroy(struct wrapper_device *device,
                      struct wrapper_image *wimg,
                      const VkAllocationCallbacks* pAllocator)
{
   list_del(&wimg->link);
   vk_image_finish(&wimg->vk);
   vk_image_destroy(&device->vk, pAllocator, &wimg->vk);
}

VKAPI_ATTR VkResult VKAPI_CALL
wrapper_CreateImage(VkDevice _device,
                    const VkImageCreateInfo* pCreateInfo,
                    const VkAllocationCallbacks* pAllocator,
                    VkImage* pImage)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   VkImageCreateInfo create_info = *pCreateInfo;
   VkImage dispatch_handle;
   VkResult result;

   /* The wrapper may need to perform blits or copies. */
   create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT;

   result = device->dispatch_table.CreateImage(device->dispatch_handle,
                                               &create_info,
                                               pAllocator,
                                               &dispatch_handle);
   if (result != VK_SUCCESS)
      return result;

   simple_mtx_lock(&device->resource_mutex);

   result = wrapper_image_create(device, &create_info, pAllocator,
                                 dispatch_handle, pImage);

   if (result != VK_SUCCESS) {
      device->dispatch_table.DestroyImage(device->dispatch_handle,
                                          dispatch_handle, pAllocator);
   }

   simple_mtx_unlock(&device->resource_mutex);

   return result;
}

VKAPI_ATTR void VKAPI_CALL
wrapper_DestroyImage(VkDevice _device,
                     VkImage _image,
                     const VkAllocationCallbacks* pAllocator)
{
   VK_FROM_HANDLE(wrapper_device, device, _device);
   VK_FROM_HANDLE(wrapper_image, wimg, _image);

   if (!_image)
       return;

   device->dispatch_table.DestroyImage(device->dispatch_handle,
                                       wimg->dispatch_handle, pAllocator);

   simple_mtx_lock(&device->resource_mutex);
   wrapper_image_destroy(device, wimg, pAllocator);
   simple_mtx_unlock(&device->resource_mutex);
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
