#include "wrapper_private.h"
#include "wrapper_log.h"

VKAPI_ATTR VkBool32 VKAPI_CALL
wrapper_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData)
{
    LOG("[Mesa]: %s", pCallbackData->pMessage);
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            WLOGD("[Mesa] %s", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            WLOG("[Mesa] %s", pCallbackData->pMessage);
            break;
        default:
            WLOGE("[Mesa] %s", pCallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}
