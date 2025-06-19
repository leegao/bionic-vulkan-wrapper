#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "vk_alloc.h"
#include "vk_common_entrypoints.h"
#include "vk_dispatch_table.h"
#include "vk_extensions.h"
#include "vk_image.h"
// #include "vk_image_view.h"
#include "vk_util.h"

// static VkResult
// wrapper_image_view_create(struct wrapper_device *device,
//                           const VkImageViewCreateInfo* pCreateInfo,
//                           const VkAllocationCallbacks* pAllocator,
//                           VkImageView dispatch_handle,
//                           VkImageView* p_wiv)
// {
//    struct wrapper_image_view *wiv;

//    wiv = vk_object_zalloc(&device->vk, pAllocator,
//                           sizeof(struct wrapper_image_view),
//                           VK_OBJECT_TYPE_IMAGE_VIEW);
//    if (!wiv)
//       return vk_error(&device->vk, VK_ERROR_OUT_OF_HOST_MEMORY);

//    vk_image_view_init(&device->vk, &wiv->vk, false, pCreateInfo);

//    wiv->device = device;
//    wiv->dispatch_handle = dispatch_handle;
//    list_add(&wiv->link, &device->image_view_list);

//    *p_wiv = wrapper_image_view_to_handle(wiv);

//    return VK_SUCCESS;
// }

// void
// wrapper_image_view_destroy(struct wrapper_device *device,
//                            struct wrapper_image_view *wiv,
//                            const VkAllocationCallbacks* pAllocator)
// {
//    list_del(&wiv->link);
//    vk_image_view_finish(&wiv->vk);
//    vk_image_view_destroy(&device->vk, pAllocator, &wiv->vk);
// }


// VKAPI_ATTR VkResult VKAPI_CALL
// wrapper_CreateImageView(VkDevice _device,
//                         const VkImageViewCreateInfo* pCreateInfo,
//                         const VkAllocationCallbacks* pAllocator,
//                         VkImageView* pView)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image, wimg, pCreateInfo->image);

//    VkImageViewCreateInfo create_info = *pCreateInfo;
//    create_info.image = wimg->dispatch_handle;

//    /* The wrapper may need to blit to/from this image view, so we need
//     * to ensure the view's usage includes the transfer bits.
//     */
//    VkImageViewUsageCreateInfo usage_info;
//    VkImageViewUsageCreateInfo *usage_info_ptr =
//       (VkImageViewUsageCreateInfo *) vk_find_struct(
//          (void *)pCreateInfo, IMAGE_VIEW_USAGE_CREATE_INFO);

//    if (usage_info_ptr) {
//       /* The app provided one, so we need to add our bits. */
//       usage_info = *usage_info_ptr;
//    } else {
//       /* No usage info provided, so the view inherits the image's usage.
//        * Since we added the transfer bits to the image, that's sufficient.
//        * However, if we're creating a view of a specific format that's
//        * different from the image's format, we might need to add our own.
//        * Let's just add one to be safe. */
//       usage_info = (VkImageViewUsageCreateInfo) {
//          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
//          .usage = wimg->vk.usage,
//       };
//    }

//    usage_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
//                        VK_IMAGE_USAGE_TRANSFER_DST_BIT;
//    __vk_append_struct(&create_info, &usage_info);

//    VkImageView dispatch_handle;
//    VkResult result = device->dispatch_table.CreateImageView(device->dispatch_handle,
//                                                             &create_info,
//                                                             pAllocator,
//                                                             &dispatch_handle);
//    if (result != VK_SUCCESS)
//       return result;

//    simple_mtx_lock(&device->resource_mutex);
//    result = wrapper_image_view_create(device, pCreateInfo, pAllocator,
//                                       dispatch_handle, pView);
//    if (result != VK_SUCCESS) {
//       device->dispatch_table.DestroyImageView(device->dispatch_handle,
//                                               dispatch_handle, pAllocator);
//    }
//    simple_mtx_unlock(&device->resource_mutex);

//    return result;
// }

// VKAPI_ATTR void VKAPI_CALL
// wrapper_DestroyImageView(VkDevice _device,
//                          VkImageView imageView,
//                          const VkAllocationCallbacks* pAllocator)
// {
//    VK_FROM_HANDLE(wrapper_device, device, _device);
//    VK_FROM_HANDLE(wrapper_image_view, wiv, imageView);

//    if (!imageView)
//       return;

//    device->dispatch_table.DestroyImageView(device->dispatch_handle,
//                                            wiv->dispatch_handle, pAllocator);

//    simple_mtx_lock(&device->resource_mutex);
//    wrapper_image_view_destroy(device, wiv, pAllocator);
//    simple_mtx_unlock(&device->resource_mutex);
// }
