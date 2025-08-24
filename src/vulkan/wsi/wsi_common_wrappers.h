bool wrapped__wsi_init_pthread_cond_monotonic(pthread_cond_t* cond);

bool wsi_init_pthread_cond_monotonic(pthread_cond_t* cond) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__wsi_init_pthread_cond_monotonic(cond);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__wsi_init_pthread_cond_monotonic(cond);
    return __result;
  }
}

static bool wrapped__present_false(VkPhysicalDevice pdevice, int fd);

static bool present_false(VkPhysicalDevice pdevice, int fd) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__present_false(pdevice, fd);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__present_false(pdevice, fd);
    return __result;
  }
}

VkResult wrapped__wsi_device_init(struct wsi_device *wsi, VkPhysicalDevice pdevice, WSI_FN_GetPhysicalDeviceProcAddr proc_addr, const VkAllocationCallbacks *alloc, int display_fd, const struct driOptionCache *dri_options, const struct wsi_device_options *device_options);

VkResult wsi_device_init(struct wsi_device *wsi, VkPhysicalDevice pdevice, WSI_FN_GetPhysicalDeviceProcAddr proc_addr, const VkAllocationCallbacks *alloc, int display_fd, const struct driOptionCache *dri_options, const struct wsi_device_options *device_options) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_device_init(wsi, pdevice, proc_addr, alloc, display_fd, dri_options, device_options);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_device_init(wsi, pdevice, proc_addr, alloc, display_fd, dri_options, device_options);
    return __result;
  }
}

void wrapped__wsi_device_finish(struct wsi_device *wsi, const VkAllocationCallbacks *alloc);

void wsi_device_finish(struct wsi_device *wsi, const VkAllocationCallbacks *alloc) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_device_finish(wsi, alloc);
    msg_level--;
  } else {
    wrapped__wsi_device_finish(wsi, alloc);
  }
}

VKAPI_ATTR void VKAPI_CALL wrapped__wsi_DestroySurfaceKHR(VkInstance _instance, VkSurfaceKHR _surface, const VkAllocationCallbacks *pAllocator);

VKAPI_ATTR void VKAPI_CALL print_input_params_DestroySurfaceKHR(VkInstance _instance, VkSurfaceKHR _surface, const VkAllocationCallbacks *pAllocator, int cmd_id);

VKAPI_ATTR void VKAPI_CALL print_output_params_DestroySurfaceKHR(VkInstance _instance, VkSurfaceKHR _surface, const VkAllocationCallbacks *pAllocator, int cmd_id);

VKAPI_ATTR void VKAPI_CALL wsi_DestroySurfaceKHR(VkInstance _instance, VkSurfaceKHR _surface, const VkAllocationCallbacks *pAllocator) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_DestroySurfaceKHR(_instance, _surface, pAllocator, cmd_id);});
    wrapped__wsi_DestroySurfaceKHR(_instance, _surface, pAllocator);
    WSI_CAN_LOGA({print_output_params_DestroySurfaceKHR(_instance, _surface, pAllocator, cmd_id);});
    msg_level--;
  } else {
    wrapped__wsi_DestroySurfaceKHR(_instance, _surface, pAllocator);
  }
}

void wrapped__wsi_device_setup_syncobj_fd(struct wsi_device *wsi_device, int fd);

void wsi_device_setup_syncobj_fd(struct wsi_device *wsi_device, int fd) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_device_setup_syncobj_fd(wsi_device, fd);
    msg_level--;
  } else {
    wrapped__wsi_device_setup_syncobj_fd(wsi_device, fd);
  }
}

static enum wsi_swapchain_blit_type wrapped__get_blit_type(const struct wsi_device *wsi, const struct wsi_base_image_params *params, VkDevice device);

static enum wsi_swapchain_blit_type get_blit_type(const struct wsi_device *wsi, const struct wsi_base_image_params *params, VkDevice device) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    enum wsi_swapchain_blit_type __result = wrapped__get_blit_type(wsi, params, device);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    enum wsi_swapchain_blit_type __result = wrapped__get_blit_type(wsi, params, device);
    return __result;
  }
}

static VkResult wrapped__configure_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_base_image_params *params, struct wsi_image_info *info);

static VkResult configure_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_base_image_params *params, struct wsi_image_info *info) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__configure_image(chain, pCreateInfo, params, info);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__configure_image(chain, pCreateInfo, params, info);
    return __result;
  }
}

VkResult wrapped__wsi_swapchain_init(const struct wsi_device *wsi, struct wsi_swapchain *chain, VkDevice _device, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_base_image_params *image_params, const VkAllocationCallbacks *pAllocator);

VkResult wsi_swapchain_init(const struct wsi_device *wsi, struct wsi_swapchain *chain, VkDevice _device, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_base_image_params *image_params, const VkAllocationCallbacks *pAllocator) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_swapchain_init(wsi, chain, _device, pCreateInfo, image_params, pAllocator);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_swapchain_init(wsi, chain, _device, pCreateInfo, image_params, pAllocator);
    return __result;
  }
}

static bool wrapped__wsi_swapchain_is_present_mode_supported(struct wsi_device *wsi, const VkSwapchainCreateInfoKHR *pCreateInfo, VkPresentModeKHR mode);

static bool wsi_swapchain_is_present_mode_supported(struct wsi_device *wsi, const VkSwapchainCreateInfoKHR *pCreateInfo, VkPresentModeKHR mode) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__wsi_swapchain_is_present_mode_supported(wsi, pCreateInfo, mode);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__wsi_swapchain_is_present_mode_supported(wsi, pCreateInfo, mode);
    return __result;
  }
}

enum VkPresentModeKHR wrapped__wsi_swapchain_get_present_mode(struct wsi_device *wsi, const VkSwapchainCreateInfoKHR *pCreateInfo);

enum VkPresentModeKHR wsi_swapchain_get_present_mode(struct wsi_device *wsi, const VkSwapchainCreateInfoKHR *pCreateInfo) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    enum VkPresentModeKHR __result = wrapped__wsi_swapchain_get_present_mode(wsi, pCreateInfo);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    enum VkPresentModeKHR __result = wrapped__wsi_swapchain_get_present_mode(wsi, pCreateInfo);
    return __result;
  }
}

void wrapped__wsi_swapchain_finish(struct wsi_swapchain *chain);

void wsi_swapchain_finish(struct wsi_swapchain *chain) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_swapchain_finish(chain);
    msg_level--;
  } else {
    wrapped__wsi_swapchain_finish(chain);
  }
}

VkResult wrapped__wsi_configure_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, VkExternalMemoryHandleTypeFlags handle_types, struct wsi_image_info *info);

VkResult wsi_configure_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, VkExternalMemoryHandleTypeFlags handle_types, struct wsi_image_info *info) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_configure_image(chain, pCreateInfo, handle_types, info);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_configure_image(chain, pCreateInfo, handle_types, info);
    return __result;
  }
}

void wrapped__wsi_destroy_image_info(const struct wsi_swapchain *chain, struct wsi_image_info *info);

void wsi_destroy_image_info(const struct wsi_swapchain *chain, struct wsi_image_info *info) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_destroy_image_info(chain, info);
    msg_level--;
  } else {
    wrapped__wsi_destroy_image_info(chain, info);
  }
}

VkResult wrapped__wsi_create_image(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image);

VkResult wsi_create_image(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_create_image(chain, info, image);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_create_image(chain, info, image);
    return __result;
  }
}

void wrapped__wsi_destroy_image(const struct wsi_swapchain *chain, struct wsi_image *image);

void wsi_destroy_image(const struct wsi_swapchain *chain, struct wsi_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_destroy_image(chain, image);
    msg_level--;
  } else {
    wrapped__wsi_destroy_image(chain, image);
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR _surface, VkBool32 *pSupported);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR _surface, VkBool32 *pSupported, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR _surface, VkBool32 *pSupported, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR _surface, VkBool32 *pSupported) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, _surface, pSupported, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, _surface, pSupported);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, _surface, pSupported, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, _surface, pSupported);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, pSurfaceCapabilities, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, pSurfaceCapabilities);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, pSurfaceCapabilities, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, pSurfaceCapabilities);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo, VkSurfaceCapabilities2KHR *pSurfaceCapabilities) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, pSurfaceInfo, pSurfaceCapabilities, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, pSurfaceInfo, pSurfaceCapabilities, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceCapabilities2KHR(physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilities2EXT *pSurfaceCapabilities);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilities2EXT *pSurfaceCapabilities, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilities2EXT *pSurfaceCapabilities, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, VkSurfaceCapabilities2EXT *pSurfaceCapabilities) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceSurfaceCapabilities2EXT(physicalDevice, _surface, pSurfaceCapabilities, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceCapabilities2EXT(physicalDevice, _surface, pSurfaceCapabilities);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceSurfaceCapabilities2EXT(physicalDevice, _surface, pSurfaceCapabilities, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceCapabilities2EXT(physicalDevice, _surface, pSurfaceCapabilities);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, pSurfaceFormatCount, pSurfaceFormats, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, pSurfaceFormatCount, pSurfaceFormats);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, pSurfaceFormatCount, pSurfaceFormats, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, pSurfaceFormatCount, pSurfaceFormats);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR * pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR * pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR * pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR * pSurfaceInfo, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfaceFormats2KHR(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, pPresentModeCount, pPresentModes, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, pPresentModeCount, pPresentModes);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, pPresentModeCount, pPresentModes, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, pPresentModeCount, pPresentModes);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pRectCount, VkRect2D *pRects);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pRectCount, VkRect2D *pRects, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pRectCount, VkRect2D *pRects, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR _surface, uint32_t *pRectCount, VkRect2D *pRects) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDevicePresentRectanglesKHR(physicalDevice, _surface, pRectCount, pRects, cmd_id);});
    VkResult __result = wrapped__wsi_GetPhysicalDevicePresentRectanglesKHR(physicalDevice, _surface, pRectCount, pRects);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDevicePresentRectanglesKHR(physicalDevice, _surface, pRectCount, pRects, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetPhysicalDevicePresentRectanglesKHR(physicalDevice, _surface, pRectCount, pRects);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_CreateSwapchainKHR(VkDevice _device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_CreateSwapchainKHR(VkDevice _device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_CreateSwapchainKHR(VkDevice _device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_CreateSwapchainKHR(VkDevice _device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_CreateSwapchainKHR(_device, pCreateInfo, pAllocator, pSwapchain, cmd_id);});
    VkResult __result = wrapped__wsi_CreateSwapchainKHR(_device, pCreateInfo, pAllocator, pSwapchain);
    WSI_CAN_LOGA({print_output_params_CreateSwapchainKHR(_device, pCreateInfo, pAllocator, pSwapchain, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_CreateSwapchainKHR(_device, pCreateInfo, pAllocator, pSwapchain);
    return __result;
  }
}

VKAPI_ATTR void VKAPI_CALL wrapped__wsi_DestroySwapchainKHR(VkDevice _device, VkSwapchainKHR _swapchain, const VkAllocationCallbacks *pAllocator);

VKAPI_ATTR void VKAPI_CALL print_input_params_DestroySwapchainKHR(VkDevice _device, VkSwapchainKHR _swapchain, const VkAllocationCallbacks *pAllocator, int cmd_id);

VKAPI_ATTR void VKAPI_CALL print_output_params_DestroySwapchainKHR(VkDevice _device, VkSwapchainKHR _swapchain, const VkAllocationCallbacks *pAllocator, int cmd_id);

VKAPI_ATTR void VKAPI_CALL wsi_DestroySwapchainKHR(VkDevice _device, VkSwapchainKHR _swapchain, const VkAllocationCallbacks *pAllocator) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_DestroySwapchainKHR(_device, _swapchain, pAllocator, cmd_id);});
    wrapped__wsi_DestroySwapchainKHR(_device, _swapchain, pAllocator);
    WSI_CAN_LOGA({print_output_params_DestroySwapchainKHR(_device, _swapchain, pAllocator, cmd_id);});
    msg_level--;
  } else {
    wrapped__wsi_DestroySwapchainKHR(_device, _swapchain, pAllocator);
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_ReleaseSwapchainImagesEXT(VkDevice _device, const VkReleaseSwapchainImagesInfoEXT *pReleaseInfo);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_ReleaseSwapchainImagesEXT(VkDevice _device, const VkReleaseSwapchainImagesInfoEXT *pReleaseInfo, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_ReleaseSwapchainImagesEXT(VkDevice _device, const VkReleaseSwapchainImagesInfoEXT *pReleaseInfo, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_ReleaseSwapchainImagesEXT(VkDevice _device, const VkReleaseSwapchainImagesInfoEXT *pReleaseInfo) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_ReleaseSwapchainImagesEXT(_device, pReleaseInfo, cmd_id);});
    VkResult __result = wrapped__wsi_ReleaseSwapchainImagesEXT(_device, pReleaseInfo);
    WSI_CAN_LOGA({print_output_params_ReleaseSwapchainImagesEXT(_device, pReleaseInfo, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_ReleaseSwapchainImagesEXT(_device, pReleaseInfo);
    return __result;
  }
}

VkResult wrapped__wsi_common_get_images(VkSwapchainKHR _swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages);

VkResult wsi_common_get_images(VkSwapchainKHR _swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_common_get_images(_swapchain, pSwapchainImageCount, pSwapchainImages);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_common_get_images(_swapchain, pSwapchainImageCount, pSwapchainImages);
    return __result;
  }
}

VkImage wrapped__wsi_common_get_image(VkSwapchainKHR _swapchain, uint32_t index);

VkImage wsi_common_get_image(VkSwapchainKHR _swapchain, uint32_t index) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkImage __result = wrapped__wsi_common_get_image(_swapchain, index);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkImage __result = wrapped__wsi_common_get_image(_swapchain, index);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages, cmd_id);});
    VkResult __result = wrapped__wsi_GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    WSI_CAN_LOGA({print_output_params_GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_AcquireNextImageKHR(VkDevice _device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_AcquireNextImageKHR(VkDevice _device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_AcquireNextImageKHR(VkDevice _device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_AcquireNextImageKHR(VkDevice _device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_AcquireNextImageKHR(_device, swapchain, timeout, semaphore, fence, pImageIndex, cmd_id);});
    VkResult __result = wrapped__wsi_AcquireNextImageKHR(_device, swapchain, timeout, semaphore, fence, pImageIndex);
    WSI_CAN_LOGA({print_output_params_AcquireNextImageKHR(_device, swapchain, timeout, semaphore, fence, pImageIndex, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_AcquireNextImageKHR(_device, swapchain, timeout, semaphore, fence, pImageIndex);
    return __result;
  }
}

static VkResult wrapped__wsi_signal_semaphore_for_image(struct vk_device *device, const struct wsi_swapchain *chain, const struct wsi_image *image, VkSemaphore _semaphore);

static VkResult wsi_signal_semaphore_for_image(struct vk_device *device, const struct wsi_swapchain *chain, const struct wsi_image *image, VkSemaphore _semaphore) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_signal_semaphore_for_image(device, chain, image, _semaphore);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_signal_semaphore_for_image(device, chain, image, _semaphore);
    return __result;
  }
}

static VkResult wrapped__wsi_signal_fence_for_image(struct vk_device *device, const struct wsi_swapchain *chain, const struct wsi_image *image, VkFence _fence);

static VkResult wsi_signal_fence_for_image(struct vk_device *device, const struct wsi_swapchain *chain, const struct wsi_image *image, VkFence _fence) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_signal_fence_for_image(device, chain, image, _fence);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_signal_fence_for_image(device, chain, image, _fence);
    return __result;
  }
}

VkResult wrapped__wsi_common_acquire_next_image2(const struct wsi_device *wsi, VkDevice _device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex);

VkResult wsi_common_acquire_next_image2(const struct wsi_device *wsi, VkDevice _device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_common_acquire_next_image2(wsi, _device, pAcquireInfo, pImageIndex);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_common_acquire_next_image2(wsi, _device, pAcquireInfo, pImageIndex);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_AcquireNextImage2KHR(VkDevice _device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_AcquireNextImage2KHR(VkDevice _device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_AcquireNextImage2KHR(VkDevice _device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_AcquireNextImage2KHR(VkDevice _device, const VkAcquireNextImageInfoKHR *pAcquireInfo, uint32_t *pImageIndex) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_AcquireNextImage2KHR(_device, pAcquireInfo, pImageIndex, cmd_id);});
    VkResult __result = wrapped__wsi_AcquireNextImage2KHR(_device, pAcquireInfo, pImageIndex);
    WSI_CAN_LOGA({print_output_params_AcquireNextImage2KHR(_device, pAcquireInfo, pImageIndex, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_AcquireNextImage2KHR(_device, pAcquireInfo, pImageIndex);
    return __result;
  }
}

static VkResult wrapped__wsi_signal_present_id_timeline(struct wsi_swapchain *swapchain, VkQueue queue, uint64_t present_id, VkFence present_fence);

static VkResult wsi_signal_present_id_timeline(struct wsi_swapchain *swapchain, VkQueue queue, uint64_t present_id, VkFence present_fence) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_signal_present_id_timeline(swapchain, queue, present_id, present_fence);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_signal_present_id_timeline(swapchain, queue, present_id, present_fence);
    return __result;
  }
}

static VkResult wrapped__handle_trace(VkQueue queue, struct vk_device *device, uint32_t current_frame);

static VkResult handle_trace(VkQueue queue, struct vk_device *device, uint32_t current_frame) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__handle_trace(queue, device, current_frame);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__handle_trace(queue, device, current_frame);
    return __result;
  }
}

VkResult wrapped__wsi_common_queue_present(const struct wsi_device *wsi, VkDevice device, VkQueue queue, int queue_family_index, const VkPresentInfoKHR *pPresentInfo);

VkResult wsi_common_queue_present(const struct wsi_device *wsi, VkDevice device, VkQueue queue, int queue_family_index, const VkPresentInfoKHR *pPresentInfo) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_common_queue_present(wsi, device, queue, queue_family_index, pPresentInfo);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_common_queue_present(wsi, device, queue, queue_family_index, pPresentInfo);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_QueuePresentKHR(_queue, pPresentInfo, cmd_id);});
    VkResult __result = wrapped__wsi_QueuePresentKHR(_queue, pPresentInfo);
    WSI_CAN_LOGA({print_output_params_QueuePresentKHR(_queue, pPresentInfo, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_QueuePresentKHR(_queue, pPresentInfo);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pCapabilities);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pCapabilities, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pCapabilities, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetDeviceGroupPresentCapabilitiesKHR(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR *pCapabilities) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetDeviceGroupPresentCapabilitiesKHR(device, pCapabilities, cmd_id);});
    VkResult __result = wrapped__wsi_GetDeviceGroupPresentCapabilitiesKHR(device, pCapabilities);
    WSI_CAN_LOGA({print_output_params_GetDeviceGroupPresentCapabilitiesKHR(device, pCapabilities, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetDeviceGroupPresentCapabilitiesKHR(device, pCapabilities);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_GetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_GetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_GetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_GetDeviceGroupSurfacePresentModesKHR(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR *pModes) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetDeviceGroupSurfacePresentModesKHR(device, surface, pModes, cmd_id);});
    VkResult __result = wrapped__wsi_GetDeviceGroupSurfacePresentModesKHR(device, surface, pModes);
    WSI_CAN_LOGA({print_output_params_GetDeviceGroupSurfacePresentModesKHR(device, surface, pModes, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_GetDeviceGroupSurfacePresentModesKHR(device, surface, pModes);
    return __result;
  }
}

bool wrapped__wsi_common_vk_instance_supports_present_wait(const struct vk_instance *instance);

bool wsi_common_vk_instance_supports_present_wait(const struct vk_instance *instance) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__wsi_common_vk_instance_supports_present_wait(instance);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__wsi_common_vk_instance_supports_present_wait(instance);
    return __result;
  }
}

VkResult wrapped__wsi_common_create_swapchain_image(const struct wsi_device *wsi, const VkImageCreateInfo *pCreateInfo, VkSwapchainKHR _swapchain, VkImage *pImage);

VkResult wsi_common_create_swapchain_image(const struct wsi_device *wsi, const VkImageCreateInfo *pCreateInfo, VkSwapchainKHR _swapchain, VkImage *pImage) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_common_create_swapchain_image(wsi, pCreateInfo, _swapchain, pImage);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_common_create_swapchain_image(wsi, pCreateInfo, _swapchain, pImage);
    return __result;
  }
}

VkResult wrapped__wsi_common_bind_swapchain_image(const struct wsi_device *wsi, VkImage vk_image, VkSwapchainKHR _swapchain, uint32_t image_idx);

VkResult wsi_common_bind_swapchain_image(const struct wsi_device *wsi, VkImage vk_image, VkSwapchainKHR _swapchain, uint32_t image_idx) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_common_bind_swapchain_image(wsi, vk_image, _swapchain, image_idx);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_common_bind_swapchain_image(wsi, vk_image, _swapchain, image_idx);
    return __result;
  }
}

VkResult wrapped__wsi_swapchain_wait_for_present_semaphore(const struct wsi_swapchain *chain, uint64_t present_id, uint64_t timeout);

VkResult wsi_swapchain_wait_for_present_semaphore(const struct wsi_swapchain *chain, uint64_t present_id, uint64_t timeout) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_swapchain_wait_for_present_semaphore(chain, present_id, timeout);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_swapchain_wait_for_present_semaphore(chain, present_id, timeout);
    return __result;
  }
}

uint32_t wrapped__wsi_select_memory_type(const struct wsi_device *wsi, VkMemoryPropertyFlags req_props, VkMemoryPropertyFlags deny_props, uint32_t type_bits);

uint32_t wsi_select_memory_type(const struct wsi_device *wsi, VkMemoryPropertyFlags req_props, VkMemoryPropertyFlags deny_props, uint32_t type_bits) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    uint32_t __result = wrapped__wsi_select_memory_type(wsi, req_props, deny_props, type_bits);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    uint32_t __result = wrapped__wsi_select_memory_type(wsi, req_props, deny_props, type_bits);
    return __result;
  }
}

uint32_t wrapped__wsi_select_device_memory_type(const struct wsi_device *wsi, uint32_t type_bits);

uint32_t wsi_select_device_memory_type(const struct wsi_device *wsi, uint32_t type_bits) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    uint32_t __result = wrapped__wsi_select_device_memory_type(wsi, type_bits);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    uint32_t __result = wrapped__wsi_select_device_memory_type(wsi, type_bits);
    return __result;
  }
}

static uint32_t wrapped__wsi_select_host_memory_type(const struct wsi_device *wsi, uint32_t type_bits);

static uint32_t wsi_select_host_memory_type(const struct wsi_device *wsi, uint32_t type_bits) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    uint32_t __result = wrapped__wsi_select_host_memory_type(wsi, type_bits);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    uint32_t __result = wrapped__wsi_select_host_memory_type(wsi, type_bits);
    return __result;
  }
}

VkResult wrapped__wsi_create_buffer_blit_context(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image, VkExternalMemoryHandleTypeFlags handle_types);

VkResult wsi_create_buffer_blit_context(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image, VkExternalMemoryHandleTypeFlags handle_types) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_create_buffer_blit_context(chain, info, image, handle_types);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_create_buffer_blit_context(chain, info, image, handle_types);
    return __result;
  }
}

VkResult wrapped__wsi_finish_create_blit_context(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image);

VkResult wsi_finish_create_blit_context(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_finish_create_blit_context(chain, info, image);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_finish_create_blit_context(chain, info, image);
    return __result;
  }
}

void wrapped__wsi_configure_buffer_image(UNUSED const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, uint32_t stride_align, uint32_t size_align, struct wsi_image_info *info);

void wsi_configure_buffer_image(UNUSED const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, uint32_t stride_align, uint32_t size_align, struct wsi_image_info *info) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_configure_buffer_image(chain, pCreateInfo, stride_align, size_align, info);
    msg_level--;
  } else {
    wrapped__wsi_configure_buffer_image(chain, pCreateInfo, stride_align, size_align, info);
  }
}

void wrapped__wsi_configure_image_blit_image(UNUSED const struct wsi_swapchain *chain, struct wsi_image_info *info);

void wsi_configure_image_blit_image(UNUSED const struct wsi_swapchain *chain, struct wsi_image_info *info) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_configure_image_blit_image(chain, info);
    msg_level--;
  } else {
    wrapped__wsi_configure_image_blit_image(chain, info);
  }
}

static VkResult wrapped__wsi_create_cpu_linear_image_mem(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image);

static VkResult wsi_create_cpu_linear_image_mem(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_create_cpu_linear_image_mem(chain, info, image);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_create_cpu_linear_image_mem(chain, info, image);
    return __result;
  }
}

static VkResult wrapped__wsi_create_cpu_buffer_image_mem(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image);

static VkResult wsi_create_cpu_buffer_image_mem(const struct wsi_swapchain *chain, const struct wsi_image_info *info, struct wsi_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_create_cpu_buffer_image_mem(chain, info, image);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_create_cpu_buffer_image_mem(chain, info, image);
    return __result;
  }
}

bool wrapped__wsi_cpu_image_needs_buffer_blit(const struct wsi_device *wsi, const struct wsi_cpu_image_params *params);

bool wsi_cpu_image_needs_buffer_blit(const struct wsi_device *wsi, const struct wsi_cpu_image_params *params) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__wsi_cpu_image_needs_buffer_blit(wsi, params);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__wsi_cpu_image_needs_buffer_blit(wsi, params);
    return __result;
  }
}

VkResult wrapped__wsi_configure_cpu_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_cpu_image_params *params, struct wsi_image_info *info);

VkResult wsi_configure_cpu_image(const struct wsi_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const struct wsi_cpu_image_params *params, struct wsi_image_info *info) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_configure_cpu_image(chain, pCreateInfo, params, info);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_configure_cpu_image(chain, pCreateInfo, params, info);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_WaitForPresentKHR(VkDevice device, VkSwapchainKHR _swapchain, uint64_t presentId, uint64_t timeout);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_WaitForPresentKHR(VkDevice device, VkSwapchainKHR _swapchain, uint64_t presentId, uint64_t timeout, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_WaitForPresentKHR(VkDevice device, VkSwapchainKHR _swapchain, uint64_t presentId, uint64_t timeout, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_WaitForPresentKHR(VkDevice device, VkSwapchainKHR _swapchain, uint64_t presentId, uint64_t timeout) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_WaitForPresentKHR(device, _swapchain, presentId, timeout, cmd_id);});
    VkResult __result = wrapped__wsi_WaitForPresentKHR(device, _swapchain, presentId, timeout);
    WSI_CAN_LOGA({print_output_params_WaitForPresentKHR(device, _swapchain, presentId, timeout, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_WaitForPresentKHR(device, _swapchain, presentId, timeout);
    return __result;
  }
}

VkImageUsageFlags wrapped__wsi_caps_get_image_usage(void);

VkImageUsageFlags wsi_caps_get_image_usage(void) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkImageUsageFlags __result = wrapped__wsi_caps_get_image_usage();
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkImageUsageFlags __result = wrapped__wsi_caps_get_image_usage();
    return __result;
  }
}

bool wrapped__wsi_device_supports_explicit_sync(struct wsi_device *device);

bool wsi_device_supports_explicit_sync(struct wsi_device *device) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__wsi_device_supports_explicit_sync(device);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__wsi_device_supports_explicit_sync(device);
    return __result;
  }
}
