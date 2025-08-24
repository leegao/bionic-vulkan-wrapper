enum wsi_swapchain_blit_type wrapped__wsi_get_ahardware_buffer_blit_type(const struct wsi_device *wsi, const struct wsi_base_image_params *params, VkDevice device);

enum wsi_swapchain_blit_type wsi_get_ahardware_buffer_blit_type(const struct wsi_device *wsi, const struct wsi_base_image_params *params, VkDevice device) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    enum wsi_swapchain_blit_type __result = wrapped__wsi_get_ahardware_buffer_blit_type(wsi, params, device);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    enum wsi_swapchain_blit_type __result = wrapped__wsi_get_ahardware_buffer_blit_type(wsi, params, device);
    return __result;
  }
}

static VkResult wrapped__wsi_create_ahardware_buffer_image_mem(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image);

static VkResult wsi_create_ahardware_buffer_image_mem(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_create_ahardware_buffer_image_mem(chain, info, image);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_create_ahardware_buffer_image_mem(chain, info, image);
    return __result;
  }
}

static VkResult wrapped__wsi_create_ahardware_buffer_blit_context(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image);

static VkResult wsi_create_ahardware_buffer_blit_context(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_create_ahardware_buffer_blit_context(chain, info, image);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_create_ahardware_buffer_blit_context(chain, info, image);
    return __result;
  }
}

inline static uint32_t wrapped__to_ahardware_buffer_format(VkFormat format);

inline static uint32_t to_ahardware_buffer_format(VkFormat format) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    uint32_t __result = wrapped__to_ahardware_buffer_format(format);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    uint32_t __result = wrapped__to_ahardware_buffer_format(format);
    return __result;
  }
}

VkResult wrapped__wsi_configure_ahardware_buffer_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_base_image_params *params, struct wsi_image_info *info);

VkResult wsi_configure_ahardware_buffer_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_base_image_params *params, struct wsi_image_info *info) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_configure_ahardware_buffer_image(chain, pCreateInfo, params, info);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_configure_ahardware_buffer_image(chain, pCreateInfo, params, info);
    return __result;
  }
}
