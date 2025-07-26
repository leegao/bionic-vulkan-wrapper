#include "wrapper_private.h"
#include "wrapper_log.h"
#include "wrapper_trampolines.h"
#include "wrapper_entrypoints.h"
#include "vk_printers.h"

static int __log_level;
static FILE* __log_fd;
static bool __log_initialized;

FILE* get_wrapper_log_fd() {
    return __log_fd;
}

static int __cmd_log_level;
static FILE* __cmd_log_fd;
static bool __cmd_log_initialized;

FILE* get_wrapper_cmd_log_fd() {
    return __log_fd;
}

static void cleanup_log_file(void) {
    if (__log_fd) {
        fclose(__log_fd);
        __log_fd = NULL;
    }
}

static void cleanup_cmd_log_file(void) {
    if (__cmd_log_fd) {
        fclose(__cmd_log_fd);
        __cmd_log_fd = NULL;
    }
}

void get_current_time_string(char* buffer, size_t bufferSize) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL) {
        buffer[0] = '\0';
        return;
    }
    strftime(buffer, bufferSize, "%Y_%m_%d_%H_%M_%S", tm_info);
}

static FILE* open_log_file(const char* prefix) {
    char time_str[20];
    get_current_time_string(time_str, sizeof(time_str));
    char path[256];
    sprintf(path, "/sdcard/Documents/Wrapper/%s_%s.%s.%d.txt", prefix, time_str, getprogname(), getpid());
    return fopen(path, "a");
}

int should_log_cmd() {
    if (__cmd_log_initialized) {
        return __cmd_log_level;
    }
    __cmd_log_initialized = true;

    const char *log_level = getenv("WRAPPER_CMD_LOG_LEVEL");
    if (!log_level) {
        __cmd_log_level = VK_CMD_NONE;
    } else if (strcmp(log_level, "all") == 0) {
        __cmd_log_level = VK_CMD_ALL;
    } else if (strcmp(log_level, "name") == 0) {
        __cmd_log_level = VK_CMD_NAME;
    } else {
        __cmd_log_level = VK_CMD_NONE;
    }

    if (__log_level != LOG_LEVEL_NONE) {
        __cmd_log_fd = open_log_file("wrapper_cmds");
        if (!__cmd_log_fd) {
            __cmd_log_level = LOG_LEVEL_NONE;
        } else {
            atexit(cleanup_cmd_log_file);
        }
    }

    return __cmd_log_level;
}

int should_log() {
    if (__log_initialized) {
        return __log_level;
    }
    __log_initialized = true;

    const char *log_level = getenv("WRAPPER_LOG_LEVEL");
    if (!log_level) {
        __log_level = LOG_LEVEL_ERROR;
    } else if (strcmp(log_level, "all") == 0) {
        __log_level = LOG_LEVEL_ALL;
    } else if (strcmp(log_level, "debug") == 0) {
        __log_level = LOG_LEVEL_DEBUG;
    } else if (strcmp(log_level, "verbose") == 0) {
        __log_level = LOG_LEVEL_VERBOSE;
    } else if (strcmp(log_level, "error") == 0) {
        __log_level = LOG_LEVEL_ERROR;
    } else {
        __log_level = LOG_LEVEL_NONE;
    }

    if (__log_level != LOG_LEVEL_NONE) {
        __log_fd = open_log_file("wrapper_log");
        if (!__log_fd) {
            __log_level = LOG_LEVEL_NONE;
        } else {
            atexit(cleanup_log_file);
        }
    }

    return __log_level;
}

void wlog(const char* fmt, ...) {
   va_list args;
   va_start(args, fmt);
   vfprintf(__log_fd, fmt, args);
   va_end(args);
   fprintf(__log_fd, "\n");
   fflush(__log_fd);
}

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

void wrapper_log_device_fault(struct wrapper_device *device, const char* call)
{
    WLOGE("FATAL: %s failed with a GPU fault, the game will now crash", call);

    if (!device || !device->physical->base_supported_extensions.EXT_device_fault) {
        WLOGE("+ VK_EXT_device_fault not supported.");
        return;
    }

    if (!device->physical->base_supported_features.deviceFault) {
        WLOGE("+ VK_EXT_device_fault supported, but the 'deviceFault' feature was not enabled.");
        return;
    }

    VkDeviceFaultCountsEXT fault_counts = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT,
    };
    VkResult result = WCHECK(GetDeviceFaultInfoEXT((VkDevice)device, &fault_counts, NULL));
    if (result != VK_SUCCESS) {
        return;
    }

    if (fault_counts.addressInfoCount == 0 &&
        fault_counts.vendorInfoCount == 0 &&
        fault_counts.vendorBinarySize == 0) {
        WLOGE("+ Device lost, but no fault info was recorded by the driver.");
        return;
    }

    const bool vendor_binary_feature_enabled =
        device->physical->base_supported_features.deviceFaultVendorBinary;
    if (!device->physical->base_supported_features.deviceFaultVendorBinary) {
        WLOGE("+ deviceFaultVendorBinary feature not available, cannot dump vendor specific fault info");
    }

    VkDeviceFaultInfoEXT fault_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_INFO_EXT,
    };

    fault_info.pAddressInfos = NULL;
    fault_info.pVendorInfos = NULL;
    fault_info.pVendorBinaryData = NULL;

    if (fault_counts.addressInfoCount > 0) {
        fault_info.pAddressInfos = VK_ALLOC2(device, VkDeviceFaultAddressInfoEXT,
                                             fault_counts.addressInfoCount * sizeof(VkDeviceFaultAddressInfoEXT));
    }

    if (fault_counts.vendorInfoCount > 0) {
        fault_info.pVendorInfos = VK_ALLOC2(device, VkDeviceFaultVendorInfoEXT,
                                            fault_counts.vendorInfoCount * sizeof(VkDeviceFaultVendorInfoEXT));
    }

    if (vendor_binary_feature_enabled && fault_counts.vendorBinarySize > 0) {
        fault_info.pVendorBinaryData = VK_ALLOC2(device, uint8_t, fault_counts.vendorBinarySize);
    }

    result = WCHECK(GetDeviceFaultInfoEXT((VkDevice)device, &fault_counts, &fault_info));
    if (result == VK_SUCCESS) {
        WLOGE("--- VULKAN DEVICE FAULT DETECTED ---");
        if (fault_info.description) {
            WLOGE("Description: %s", fault_info.description);
        }
        for (uint32_t i = 0; i < fault_counts.addressInfoCount; i++) {
            WLOGE(".pAddressInfos[%d]", i);
            LOG_STRUCT_AT(ERROR, VkDeviceFaultAddressInfoEXT, &fault_info.pAddressInfos[i]);
        }
        for (uint32_t i = 0; i < fault_counts.vendorInfoCount; i++) {
            WLOGE(".pVendorInfos[%d]", i);
            LOG_STRUCT_AT(ERROR, VkDeviceFaultVendorInfoEXT, &fault_info.pVendorInfos[i]);
        }
        if (fault_info.pVendorBinaryData && fault_counts.vendorBinarySize > 0) {
            WLOGE("  Vendor binary crash dump retrieved (%llu bytes).", fault_counts.vendorBinarySize);
        }
        WLOGE("--- END FAULT INFO ---");
    }

    if (fault_info.pAddressInfos)
        vk_free(&device->vk.alloc, fault_info.pAddressInfos);
    if (fault_info.pVendorInfos)
        vk_free(&device->vk.alloc, fault_info.pVendorInfos);
    if (fault_info.pVendorBinaryData)
        vk_free(&device->vk.alloc, fault_info.pVendorBinaryData);
}
