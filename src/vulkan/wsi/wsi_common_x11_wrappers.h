static int wrapped__wsi_dri3_open(xcb_connection_t *conn, xcb_window_t root, uint32_t provider);

static int wsi_dri3_open(xcb_connection_t *conn, xcb_window_t root, uint32_t provider) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    int __result = wrapped__wsi_dri3_open(conn, root, provider);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    int __result = wrapped__wsi_dri3_open(conn, root, provider);
    return __result;
  }
}

static bool wrapped__wsi_x11_check_dri3_compatible(const struct wsi_device *wsi_dev, xcb_connection_t *conn);

static bool wsi_x11_check_dri3_compatible(const struct wsi_device *wsi_dev, xcb_connection_t *conn) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__wsi_x11_check_dri3_compatible(wsi_dev, conn);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__wsi_x11_check_dri3_compatible(wsi_dev, conn);
    return __result;
  }
}

static struct wsi_x11_connection * wrapped__wsi_x11_connection_create(struct wsi_device *wsi_dev, xcb_connection_t *conn);

static struct wsi_x11_connection * wsi_x11_connection_create(struct wsi_device *wsi_dev, xcb_connection_t *conn) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    struct wsi_x11_connection * __result = wrapped__wsi_x11_connection_create(wsi_dev, conn);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    struct wsi_x11_connection * __result = wrapped__wsi_x11_connection_create(wsi_dev, conn);
    return __result;
  }
}

static void wrapped__wsi_x11_connection_destroy(struct wsi_device *wsi_dev, struct wsi_x11_connection *conn);

static void wsi_x11_connection_destroy(struct wsi_device *wsi_dev, struct wsi_x11_connection *conn) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_x11_connection_destroy(wsi_dev, conn);
    msg_level--;
  } else {
    wrapped__wsi_x11_connection_destroy(wsi_dev, conn);
  }
}

static bool wrapped__wsi_x11_check_for_dri3(struct wsi_x11_connection *wsi_conn);

static bool wsi_x11_check_for_dri3(struct wsi_x11_connection *wsi_conn) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__wsi_x11_check_for_dri3(wsi_conn);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__wsi_x11_check_for_dri3(wsi_conn);
    return __result;
  }
}

static struct wsi_x11_connection * wrapped__wsi_x11_get_connection(struct wsi_device *wsi_dev, xcb_connection_t *conn);

static struct wsi_x11_connection * wsi_x11_get_connection(struct wsi_device *wsi_dev, xcb_connection_t *conn) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    struct wsi_x11_connection * __result = wrapped__wsi_x11_get_connection(wsi_dev, conn);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    struct wsi_x11_connection * __result = wrapped__wsi_x11_get_connection(wsi_dev, conn);
    return __result;
  }
}

static xcb_screen_t * wrapped__get_screen_for_root(xcb_connection_t *conn, xcb_window_t root);

static xcb_screen_t * get_screen_for_root(xcb_connection_t *conn, xcb_window_t root) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    xcb_screen_t * __result = wrapped__get_screen_for_root(conn, root);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    xcb_screen_t * __result = wrapped__get_screen_for_root(conn, root);
    return __result;
  }
}

static xcb_visualtype_t * wrapped__screen_get_visualtype(xcb_screen_t *screen, xcb_visualid_t visual_id, unsigned *depth);

static xcb_visualtype_t * screen_get_visualtype(xcb_screen_t *screen, xcb_visualid_t visual_id, unsigned *depth) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    xcb_visualtype_t * __result = wrapped__screen_get_visualtype(screen, visual_id, depth);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    xcb_visualtype_t * __result = wrapped__screen_get_visualtype(screen, visual_id, depth);
    return __result;
  }
}

static xcb_visualtype_t * wrapped__connection_get_visualtype(xcb_connection_t *conn, xcb_visualid_t visual_id);

static xcb_visualtype_t * connection_get_visualtype(xcb_connection_t *conn, xcb_visualid_t visual_id) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    xcb_visualtype_t * __result = wrapped__connection_get_visualtype(conn, visual_id);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    xcb_visualtype_t * __result = wrapped__connection_get_visualtype(conn, visual_id);
    return __result;
  }
}

static xcb_visualtype_t * wrapped__get_visualtype_for_window(xcb_connection_t *conn, xcb_window_t window, unsigned *depth, xcb_visualtype_t **rootvis);

static xcb_visualtype_t * get_visualtype_for_window(xcb_connection_t *conn, xcb_window_t window, unsigned *depth, xcb_visualtype_t **rootvis) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    xcb_visualtype_t * __result = wrapped__get_visualtype_for_window(conn, window, depth, rootvis);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    xcb_visualtype_t * __result = wrapped__get_visualtype_for_window(conn, window, depth, rootvis);
    return __result;
  }
}

static bool wrapped__visual_has_alpha(xcb_visualtype_t *visual, unsigned depth);

static bool visual_has_alpha(xcb_visualtype_t *visual, unsigned depth) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__visual_has_alpha(visual, depth);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__visual_has_alpha(visual, depth);
    return __result;
  }
}

static bool wrapped__visual_supported(xcb_visualtype_t *visual);

static bool visual_supported(xcb_visualtype_t *visual) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__visual_supported(visual);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__visual_supported(visual);
    return __result;
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL wrapped__wsi_GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t *connection, xcb_visualid_t visual_id);

VKAPI_ATTR VkBool32 VKAPI_CALL print_input_params_GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t *connection, xcb_visualid_t visual_id, int cmd_id);

VKAPI_ATTR VkBool32 VKAPI_CALL print_output_params_GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t *connection, xcb_visualid_t visual_id, int cmd_id);

VKAPI_ATTR VkBool32 VKAPI_CALL wsi_GetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t *connection, xcb_visualid_t visual_id) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id, cmd_id);});
    VkBool32 __result = wrapped__wsi_GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkBool32 __result = wrapped__wsi_GetPhysicalDeviceXcbPresentationSupportKHR(physicalDevice, queueFamilyIndex, connection, visual_id);
    return __result;
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL wrapped__wsi_GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display *dpy, VisualID visualID);

VKAPI_ATTR VkBool32 VKAPI_CALL print_input_params_GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display *dpy, VisualID visualID, int cmd_id);

VKAPI_ATTR VkBool32 VKAPI_CALL print_output_params_GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display *dpy, VisualID visualID, int cmd_id);

VKAPI_ATTR VkBool32 VKAPI_CALL wsi_GetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, Display *dpy, VisualID visualID) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_GetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID, cmd_id);});
    VkBool32 __result = wrapped__wsi_GetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID);
    WSI_CAN_LOGA({print_output_params_GetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkBool32 __result = wrapped__wsi_GetPhysicalDeviceXlibPresentationSupportKHR(physicalDevice, queueFamilyIndex, dpy, visualID);
    return __result;
  }
}

static xcb_connection_t* wrapped__x11_surface_get_connection(VkIcdSurfaceBase *icd_surface);

static xcb_connection_t* x11_surface_get_connection(VkIcdSurfaceBase *icd_surface) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    xcb_connection_t* __result = wrapped__x11_surface_get_connection(icd_surface);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    xcb_connection_t* __result = wrapped__x11_surface_get_connection(icd_surface);
    return __result;
  }
}

static xcb_window_t wrapped__x11_surface_get_window(VkIcdSurfaceBase *icd_surface);

static xcb_window_t x11_surface_get_window(VkIcdSurfaceBase *icd_surface) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    xcb_window_t __result = wrapped__x11_surface_get_window(icd_surface);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    xcb_window_t __result = wrapped__x11_surface_get_window(icd_surface);
    return __result;
  }
}

static VkResult wrapped__x11_surface_get_support(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, uint32_t queueFamilyIndex, VkBool32* pSupported);

static VkResult x11_surface_get_support(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, uint32_t queueFamilyIndex, VkBool32* pSupported) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_get_support(icd_surface, wsi_device, queueFamilyIndex, pSupported);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_get_support(icd_surface, wsi_device, queueFamilyIndex, pSupported);
    return __result;
  }
}

static uint32_t wrapped__x11_get_min_image_count(const struct wsi_device *wsi_device);

static uint32_t x11_get_min_image_count(const struct wsi_device *wsi_device) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    uint32_t __result = wrapped__x11_get_min_image_count(wsi_device);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    uint32_t __result = wrapped__x11_get_min_image_count(wsi_device);
    return __result;
  }
}

static VkResult wrapped__x11_surface_get_capabilities(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, const VkSurfacePresentModeEXT *present_mode, VkSurfaceCapabilitiesKHR *caps);

static VkResult x11_surface_get_capabilities(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, const VkSurfacePresentModeEXT *present_mode, VkSurfaceCapabilitiesKHR *caps) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_get_capabilities(icd_surface, wsi_device, present_mode, caps);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_get_capabilities(icd_surface, wsi_device, present_mode, caps);
    return __result;
  }
}

static VkResult wrapped__x11_surface_get_capabilities2(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, const void *info_next, VkSurfaceCapabilities2KHR *caps);

static VkResult x11_surface_get_capabilities2(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, const void *info_next, VkSurfaceCapabilities2KHR *caps) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_get_capabilities2(icd_surface, wsi_device, info_next, caps);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_get_capabilities2(icd_surface, wsi_device, info_next, caps);
    return __result;
  }
}

static int wrapped__format_get_component_bits(VkFormat format, int comp);

static int format_get_component_bits(VkFormat format, int comp) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    int __result = wrapped__format_get_component_bits(format, comp);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    int __result = wrapped__format_get_component_bits(format, comp);
    return __result;
  }
}

static bool wrapped__rgb_component_bits_are_equal(VkFormat format, const xcb_visualtype_t* type);

static bool rgb_component_bits_are_equal(VkFormat format, const xcb_visualtype_t* type) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__rgb_component_bits_are_equal(format, type);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__rgb_component_bits_are_equal(format, type);
    return __result;
  }
}

static bool wrapped__get_sorted_vk_formats(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, VkFormat *sorted_formats, unsigned *count);

static bool get_sorted_vk_formats(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, VkFormat *sorted_formats, unsigned *count) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__get_sorted_vk_formats(surface, wsi_device, sorted_formats, count);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__get_sorted_vk_formats(surface, wsi_device, sorted_formats, count);
    return __result;
  }
}

static VkResult wrapped__x11_surface_get_formats(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats);

static VkResult x11_surface_get_formats(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, uint32_t *pSurfaceFormatCount, VkSurfaceFormatKHR *pSurfaceFormats) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_get_formats(surface, wsi_device, pSurfaceFormatCount, pSurfaceFormats);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_get_formats(surface, wsi_device, pSurfaceFormatCount, pSurfaceFormats);
    return __result;
  }
}

static VkResult wrapped__x11_surface_get_formats2(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, const void *info_next, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats);

static VkResult x11_surface_get_formats2(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, const void *info_next, uint32_t *pSurfaceFormatCount, VkSurfaceFormat2KHR *pSurfaceFormats) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_get_formats2(surface, wsi_device, info_next, pSurfaceFormatCount, pSurfaceFormats);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_get_formats2(surface, wsi_device, info_next, pSurfaceFormatCount, pSurfaceFormats);
    return __result;
  }
}

static VkResult wrapped__x11_surface_get_present_modes(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes);

static VkResult x11_surface_get_present_modes(VkIcdSurfaceBase *surface, struct wsi_device *wsi_device, uint32_t *pPresentModeCount, VkPresentModeKHR *pPresentModes) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_get_present_modes(surface, wsi_device, pPresentModeCount, pPresentModes);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_get_present_modes(surface, wsi_device, pPresentModeCount, pPresentModes);
    return __result;
  }
}

static VkResult wrapped__x11_surface_get_present_rectangles(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, uint32_t* pRectCount, VkRect2D* pRects);

static VkResult x11_surface_get_present_rectangles(VkIcdSurfaceBase *icd_surface, struct wsi_device *wsi_device, uint32_t* pRectCount, VkRect2D* pRects) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_get_present_rectangles(icd_surface, wsi_device, pRectCount, pRects);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_get_present_rectangles(icd_surface, wsi_device, pRectCount, pRects);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_CreateXcbSurfaceKHR(VkInstance _instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_CreateXcbSurfaceKHR(VkInstance _instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_CreateXcbSurfaceKHR(VkInstance _instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_CreateXcbSurfaceKHR(VkInstance _instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_CreateXcbSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface, cmd_id);});
    VkResult __result = wrapped__wsi_CreateXcbSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface);
    WSI_CAN_LOGA({print_output_params_CreateXcbSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_CreateXcbSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface);
    return __result;
  }
}

VKAPI_ATTR VkResult VKAPI_CALL wrapped__wsi_CreateXlibSurfaceKHR(VkInstance _instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);

VKAPI_ATTR VkResult VKAPI_CALL print_input_params_CreateXlibSurfaceKHR(VkInstance _instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL print_output_params_CreateXlibSurfaceKHR(VkInstance _instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface, VkResult result, int cmd_id);

VKAPI_ATTR VkResult VKAPI_CALL wsi_CreateXlibSurfaceKHR(VkInstance _instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    static int counter = 0;
    int cmd_id = counter++;
    WSI_CAN_LOGA({print_input_params_CreateXlibSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface, cmd_id);});
    VkResult __result = wrapped__wsi_CreateXlibSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface);
    WSI_CAN_LOGA({print_output_params_CreateXlibSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface, __result, cmd_id);});
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_CreateXlibSurfaceKHR(_instance, pCreateInfo, pAllocator, pSurface);
    return __result;
  }
}

static void wrapped__x11_present_complete(struct x11_swapchain *swapchain, struct x11_image *image);

static void x11_present_complete(struct x11_swapchain *swapchain, struct x11_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__x11_present_complete(swapchain, image);
    msg_level--;
  } else {
    wrapped__x11_present_complete(swapchain, image);
  }
}

static void wrapped__x11_notify_pending_present(struct x11_swapchain *swapchain, struct x11_image *image);

static void x11_notify_pending_present(struct x11_swapchain *swapchain, struct x11_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__x11_notify_pending_present(swapchain, image);
    msg_level--;
  } else {
    wrapped__x11_notify_pending_present(swapchain, image);
  }
}

static void wrapped__x11_swapchain_notify_error(struct x11_swapchain *swapchain, VkResult result);

static void x11_swapchain_notify_error(struct x11_swapchain *swapchain, VkResult result) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__x11_swapchain_notify_error(swapchain, result);
    msg_level--;
  } else {
    wrapped__x11_swapchain_notify_error(swapchain, result);
  }
}

static VkResult wrapped___x11_swapchain_result(struct x11_swapchain *chain, VkResult result, const char *file, int line);

static VkResult _x11_swapchain_result(struct x11_swapchain *chain, VkResult result, const char *file, int line) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped___x11_swapchain_result(chain, result, file, line);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped___x11_swapchain_result(chain, result, file, line);
    return __result;
  }
}

static struct wsi_image * wrapped__x11_get_wsi_image(struct wsi_swapchain *wsi_chain, uint32_t image_index);

static struct wsi_image * x11_get_wsi_image(struct wsi_swapchain *wsi_chain, uint32_t image_index) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    struct wsi_image * __result = wrapped__x11_get_wsi_image(wsi_chain, image_index);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    struct wsi_image * __result = wrapped__x11_get_wsi_image(wsi_chain, image_index);
    return __result;
  }
}

static VkResult wrapped__x11_handle_dri3_present_event(struct x11_swapchain *chain, xcb_present_generic_event_t *event);

static VkResult x11_handle_dri3_present_event(struct x11_swapchain *chain, xcb_present_generic_event_t *event) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_handle_dri3_present_event(chain, event);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_handle_dri3_present_event(chain, event);
    return __result;
  }
}

static VkResult wrapped__x11_poll_for_special_event(struct x11_swapchain *chain, uint64_t abs_timeout, xcb_generic_event_t **out_event);

static VkResult x11_poll_for_special_event(struct x11_swapchain *chain, uint64_t abs_timeout, xcb_generic_event_t **out_event) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_poll_for_special_event(chain, abs_timeout, out_event);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_poll_for_special_event(chain, abs_timeout, out_event);
    return __result;
  }
}

static bool wrapped__x11_acquire_next_image_poll_has_forward_progress(struct x11_swapchain *chain);

static bool x11_acquire_next_image_poll_has_forward_progress(struct x11_swapchain *chain) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__x11_acquire_next_image_poll_has_forward_progress(chain);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__x11_acquire_next_image_poll_has_forward_progress(chain);
    return __result;
  }
}

static VkResult wrapped__x11_acquire_next_image_poll_find_index(struct x11_swapchain *chain, uint32_t *image_index);

static VkResult x11_acquire_next_image_poll_find_index(struct x11_swapchain *chain, uint32_t *image_index) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_acquire_next_image_poll_find_index(chain, image_index);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_acquire_next_image_poll_find_index(chain, image_index);
    return __result;
  }
}

static VkResult wrapped__x11_acquire_next_image_poll_x11(struct x11_swapchain *chain, uint32_t *image_index, uint64_t timeout);

static VkResult x11_acquire_next_image_poll_x11(struct x11_swapchain *chain, uint32_t *image_index, uint64_t timeout) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_acquire_next_image_poll_x11(chain, image_index, timeout);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_acquire_next_image_poll_x11(chain, image_index, timeout);
    return __result;
  }
}

static VkResult wrapped__x11_acquire_next_image_from_queue(struct x11_swapchain *chain, uint32_t *image_index_out, uint64_t timeout);

static VkResult x11_acquire_next_image_from_queue(struct x11_swapchain *chain, uint32_t *image_index_out, uint64_t timeout) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_acquire_next_image_from_queue(chain, image_index_out, timeout);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_acquire_next_image_from_queue(chain, image_index_out, timeout);
    return __result;
  }
}

static VkResult wrapped__x11_present_to_x11_dri3(struct x11_swapchain *chain, uint32_t image_index, uint64_t target_msc);

static VkResult x11_present_to_x11_dri3(struct x11_swapchain *chain, uint32_t image_index, uint64_t target_msc) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_present_to_x11_dri3(chain, image_index, target_msc);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_present_to_x11_dri3(chain, image_index, target_msc);
    return __result;
  }
}

static VkResult wrapped__x11_present_to_x11_sw(struct x11_swapchain *chain, uint32_t image_index, uint64_t target_msc);

static VkResult x11_present_to_x11_sw(struct x11_swapchain *chain, uint32_t image_index, uint64_t target_msc) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_present_to_x11_sw(chain, image_index, target_msc);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_present_to_x11_sw(chain, image_index, target_msc);
    return __result;
  }
}

static VkResult wrapped__x11_present_to_x11(struct x11_swapchain *chain, uint32_t image_index, uint64_t target_msc);

static VkResult x11_present_to_x11(struct x11_swapchain *chain, uint32_t image_index, uint64_t target_msc) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_present_to_x11(chain, image_index, target_msc);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_present_to_x11(chain, image_index, target_msc);
    return __result;
  }
}

static VkResult wrapped__x11_release_images(struct wsi_swapchain *wsi_chain, uint32_t count, const uint32_t *indices);

static VkResult x11_release_images(struct wsi_swapchain *wsi_chain, uint32_t count, const uint32_t *indices) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_release_images(wsi_chain, count, indices);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_release_images(wsi_chain, count, indices);
    return __result;
  }
}

static VkResult wrapped__x11_acquire_next_image(struct wsi_swapchain *anv_chain, const VkAcquireNextImageInfoKHR *info, uint32_t *image_index);

static VkResult x11_acquire_next_image(struct wsi_swapchain *anv_chain, const VkAcquireNextImageInfoKHR *info, uint32_t *image_index) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_acquire_next_image(anv_chain, info, image_index);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_acquire_next_image(anv_chain, info, image_index);
    return __result;
  }
}

static VkResult wrapped__x11_queue_present(struct wsi_swapchain *anv_chain, uint32_t image_index, uint64_t present_id, const VkPresentRegionKHR *damage);

static VkResult x11_queue_present(struct wsi_swapchain *anv_chain, uint32_t image_index, uint64_t present_id, const VkPresentRegionKHR *damage) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_queue_present(anv_chain, image_index, present_id, damage);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_queue_present(anv_chain, image_index, present_id, damage);
    return __result;
  }
}

static bool wrapped__x11_needs_wait_for_fences(const struct wsi_device *wsi_device, struct wsi_x11_connection *wsi_conn, VkPresentModeKHR present_mode);

static bool x11_needs_wait_for_fences(const struct wsi_device *wsi_device, struct wsi_x11_connection *wsi_conn, VkPresentModeKHR present_mode) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    bool __result = wrapped__x11_needs_wait_for_fences(wsi_device, wsi_conn, present_mode);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    bool __result = wrapped__x11_needs_wait_for_fences(wsi_device, wsi_conn, present_mode);
    return __result;
  }
}

static unsigned wrapped__x11_driver_owned_images(const struct x11_swapchain *chain);

static unsigned x11_driver_owned_images(const struct x11_swapchain *chain) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    unsigned __result = wrapped__x11_driver_owned_images(chain);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    unsigned __result = wrapped__x11_driver_owned_images(chain);
    return __result;
  }
}

static void * wrapped__x11_manage_fifo_queues(void *state);

static void * x11_manage_fifo_queues(void *state) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    void * __result = wrapped__x11_manage_fifo_queues(state);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    void * __result = wrapped__x11_manage_fifo_queues(state);
    return __result;
  }
}

static uint8_t * wrapped__alloc_shm(struct wsi_image *imagew, unsigned size);

static uint8_t * alloc_shm(struct wsi_image *imagew, unsigned size) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    uint8_t * __result = wrapped__alloc_shm(imagew, size);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    uint8_t * __result = wrapped__alloc_shm(imagew, size);
    return __result;
  }
}

static VkResult wrapped__x11_image_init(VkDevice device_h, struct x11_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks* pAllocator, struct x11_image *image);

static VkResult x11_image_init(VkDevice device_h, struct x11_swapchain *chain, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks* pAllocator, struct x11_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_image_init(device_h, chain, pCreateInfo, pAllocator, image);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_image_init(device_h, chain, pCreateInfo, pAllocator, image);
    return __result;
  }
}

static void wrapped__x11_image_finish(struct x11_swapchain *chain, const VkAllocationCallbacks* pAllocator, struct x11_image *image);

static void x11_image_finish(struct x11_swapchain *chain, const VkAllocationCallbacks* pAllocator, struct x11_image *image) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__x11_image_finish(chain, pAllocator, image);
    msg_level--;
  } else {
    wrapped__x11_image_finish(chain, pAllocator, image);
  }
}

static void wrapped__wsi_x11_get_dri3_modifiers(struct wsi_x11_connection *wsi_conn, xcb_connection_t *conn, xcb_window_t window, uint8_t depth, uint8_t bpp, VkCompositeAlphaFlagsKHR vk_alpha, uint64_t **modifiers_in, uint32_t *num_modifiers_in, uint32_t *num_tranches_in, const VkAllocationCallbacks *pAllocator);

static void wsi_x11_get_dri3_modifiers(struct wsi_x11_connection *wsi_conn, xcb_connection_t *conn, xcb_window_t window, uint8_t depth, uint8_t bpp, VkCompositeAlphaFlagsKHR vk_alpha, uint64_t **modifiers_in, uint32_t *num_modifiers_in, uint32_t *num_tranches_in, const VkAllocationCallbacks *pAllocator) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_x11_get_dri3_modifiers(wsi_conn, conn, window, depth, bpp, vk_alpha, modifiers_in, num_modifiers_in, num_tranches_in, pAllocator);
    msg_level--;
  } else {
    wrapped__wsi_x11_get_dri3_modifiers(wsi_conn, conn, window, depth, bpp, vk_alpha, modifiers_in, num_modifiers_in, num_tranches_in, pAllocator);
  }
}

static VkResult wrapped__x11_swapchain_destroy(struct wsi_swapchain *anv_chain, const VkAllocationCallbacks *pAllocator);

static VkResult x11_swapchain_destroy(struct wsi_swapchain *anv_chain, const VkAllocationCallbacks *pAllocator) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_swapchain_destroy(anv_chain, pAllocator);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_swapchain_destroy(anv_chain, pAllocator);
    return __result;
  }
}

static VkResult wrapped__x11_wait_for_present_queued(struct x11_swapchain *chain, uint64_t waitValue, uint64_t timeout);

static VkResult x11_wait_for_present_queued(struct x11_swapchain *chain, uint64_t waitValue, uint64_t timeout) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_wait_for_present_queued(chain, waitValue, timeout);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_wait_for_present_queued(chain, waitValue, timeout);
    return __result;
  }
}

static VkResult wrapped__x11_wait_for_present_polled(struct x11_swapchain *chain, uint64_t waitValue, uint64_t timeout);

static VkResult x11_wait_for_present_polled(struct x11_swapchain *chain, uint64_t waitValue, uint64_t timeout) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_wait_for_present_polled(chain, waitValue, timeout);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_wait_for_present_polled(chain, waitValue, timeout);
    return __result;
  }
}

static VkResult wrapped__x11_wait_for_present(struct wsi_swapchain *wsi_chain, uint64_t waitValue, uint64_t timeout);

static VkResult x11_wait_for_present(struct wsi_swapchain *wsi_chain, uint64_t waitValue, uint64_t timeout) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_wait_for_present(wsi_chain, waitValue, timeout);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_wait_for_present(wsi_chain, waitValue, timeout);
    return __result;
  }
}

static unsigned wrapped__x11_get_min_image_count_for_present_mode(struct wsi_device *wsi_device, struct wsi_x11_connection *wsi_conn, VkPresentModeKHR present_mode);

static unsigned x11_get_min_image_count_for_present_mode(struct wsi_device *wsi_device, struct wsi_x11_connection *wsi_conn, VkPresentModeKHR present_mode) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    unsigned __result = wrapped__x11_get_min_image_count_for_present_mode(wsi_device, wsi_conn, present_mode);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    unsigned __result = wrapped__x11_get_min_image_count_for_present_mode(wsi_device, wsi_conn, present_mode);
    return __result;
  }
}

static VkResult wrapped__x11_surface_create_swapchain(VkIcdSurfaceBase *icd_surface, VkDevice device, struct wsi_device *wsi_device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks* pAllocator, struct wsi_swapchain **swapchain_out);

static VkResult x11_surface_create_swapchain(VkIcdSurfaceBase *icd_surface, VkDevice device, struct wsi_device *wsi_device, const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks* pAllocator, struct wsi_swapchain **swapchain_out) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__x11_surface_create_swapchain(icd_surface, device, wsi_device, pCreateInfo, pAllocator, swapchain_out);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__x11_surface_create_swapchain(icd_surface, device, wsi_device, pCreateInfo, pAllocator, swapchain_out);
    return __result;
  }
}

VkResult wrapped__wsi_x11_init_wsi(struct wsi_device *wsi_device, const VkAllocationCallbacks *alloc, const struct driOptionCache *dri_options);

VkResult wsi_x11_init_wsi(struct wsi_device *wsi_device, const VkAllocationCallbacks *alloc, const struct driOptionCache *dri_options) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    VkResult __result = wrapped__wsi_x11_init_wsi(wsi_device, alloc, dri_options);
    WSI_LOGA("%s returned %d", __FUNCTION__, __result);
    msg_level--;
    return __result;
  } else {
    VkResult __result = wrapped__wsi_x11_init_wsi(wsi_device, alloc, dri_options);
    return __result;
  }
}

void wrapped__wsi_x11_finish_wsi(struct wsi_device *wsi_device, const VkAllocationCallbacks *alloc);

void wsi_x11_finish_wsi(struct wsi_device *wsi_device, const VkAllocationCallbacks *alloc) {
  if (should_log_wsi() >= 5) {
    int current_level = msg_level++;
    WSI_LOGA("Calling %s [level %d] ", __FUNCTION__, current_level);
    wrapped__wsi_x11_finish_wsi(wsi_device, alloc);
    msg_level--;
  } else {
    wrapped__wsi_x11_finish_wsi(wsi_device, alloc);
  }
}
