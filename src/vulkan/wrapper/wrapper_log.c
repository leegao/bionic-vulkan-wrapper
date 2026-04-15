#include <time.h>

#include "wrapper_private.h"
#include "wrapper_log.h"
#include "wrapper_trampolines.h"
#include "wrapper_entrypoints.h"
#include "vk_printers.h"

#ifndef __ANDROID__
#include <errno.h>
#define getprogname() program_invocation_short_name
#endif

static int __log_level;
static FILE* __log_fd;
static bool __log_initialized;
static double __start_ms;

FILE* get_wrapper_log_fd() {
    return __log_fd;
}

static int __cmd_log_level;
static FILE* __cmd_log_fd;
static bool __cmd_log_initialized;

FILE* get_wrapper_cmd_log_fd() {
    return __cmd_log_fd;
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

static double get_current_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9;
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
    LOG("Logging cmds at %s", log_level);
    if (!log_level) {
        __cmd_log_level = VK_CMD_NONE;
    } else if (strcmp(log_level, "all") == 0) {
        __cmd_log_level = VK_CMD_ALL;
    } else if (strcmp(log_level, "name") == 0) {
        __cmd_log_level = VK_CMD_NAME;
    } else if (strcmp(log_level, "none") == 0) {
        __cmd_log_level = VK_CMD_NONE;
    } else {
        __cmd_log_level = VK_CMD_ALL;
    }

    if (__cmd_log_level != LOG_LEVEL_NONE) {
        __cmd_log_fd = open_log_file("wrapper_cmds");
        if (!__cmd_log_fd) {
            // Try to log to stdout / winelogs
            __cmd_log_fd = stdout;
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

    __start_ms = get_current_seconds();

    const char *log_level = getenv("WRAPPER_LOG_LEVEL");
    LOG("Logging logs at %s", log_level);
    if (!log_level) {
        __log_level = LOG_LEVEL_ERROR;
    } else if (strcmp(log_level, "all") == 0) {
        __log_level = LOG_LEVEL_ALL;
    } else if (strcmp(log_level, "trace") == 0) {
        __log_level = LOG_LEVEL_TRACE;
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
        // If __log_fd failed to open, still log to logcat
        if (!__log_fd) {
            // Try to log to stdout / winelogs
            __log_fd = stdout;
        } else {
            atexit(cleanup_log_file);
        }
    }

    return __log_level;
}

void wlog(const char* fmt, ...) {
    FILE* fd = __log_fd;
    if (!fd) {
        return;
    }
    fprintf(fd, "[%06.2f] ", (get_current_seconds() - __start_ms));
    va_list args;
    va_start(args, fmt);
    vfprintf(fd, fmt, args);
    va_end(args);
    fprintf(fd, "\n");
    fflush(fd);
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


static void print_vendor_fault_binary(uint32_t size, const void* dump) {
    const uint32_t* pQuads = (const uint32_t*) dump;
    const uint8_t* pChars = (const uint8_t*) dump;
    if (size == 0 || !dump) {
        return;
    }
    int i = 0;
    while (i < size) {
#define QUAD(n) pQuads[(i / 4) + n]
#define UNROLL(bytes, fmt, ...) \
        if (i + bytes - 1 < size) { \
            WLOGE("  .dump(%p)[%04x]:" fmt, dump, i, __VA_ARGS__); \
            i += bytes; \
            continue; \
        }

        UNROLL(32, "%08x %08x %08x %08x", QUAD(0), QUAD(1), QUAD(2), QUAD(3));
        UNROLL(24, "%08x %08x %08x", QUAD(0), QUAD(1), QUAD(2));
        UNROLL(16, "%08x %08x", QUAD(0), QUAD(1));
        UNROLL(8, "%08x", QUAD(0));

        if (i % 8 == 0) {
            WLOGE("  .dump(%p)[%04x]: ", dump, i);
        }
        WLOGE("      %02x", pChars[i]);
        i++;
#undef QUAD
#undef UNROLL
    }
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

    VkDeviceFaultCountsEXT fault_counts = { 0 };
    fault_counts.sType = VK_STRUCTURE_TYPE_DEVICE_FAULT_COUNTS_EXT;
    VkResult result = WCHECK(GetDeviceFaultInfoEXT((VkDevice)device, &fault_counts, NULL));
    if (result == VK_INCOMPLETE) {
        WLOGE("WARN: Got an incomplete from GetDeviceFaultInfoEXT");
    }

    if (result != VK_SUCCESS && result != VK_INCOMPLETE) {
        WLOGE("Failed to get the fault_counts from GetDeviceFaultInfoEXT");
        return;
    }

    WLOGE("Fault counts: addressInfoCount=%d, vendorInfoCount=%d, vendorBinarySize=%d",
        fault_counts.addressInfoCount, fault_counts.vendorInfoCount, fault_counts.vendorBinarySize);

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
        WLOGE("Description: %s", fault_info.description);
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
            print_vendor_fault_binary(fault_counts.vendorBinarySize, fault_info.pVendorBinaryData);
        }
        WLOGE("--- END FAULT INFO ---");
    } else {
        WLOGE("Failed to get the fault_info from GetDeviceFaultInfoEXT");
    }

    if (fault_info.pAddressInfos)
        vk_free(&device->vk.alloc, fault_info.pAddressInfos);
    if (fault_info.pVendorInfos)
        vk_free(&device->vk.alloc, fault_info.pVendorInfos);
    if (fault_info.pVendorBinaryData)
        vk_free(&device->vk.alloc, fault_info.pVendorBinaryData);
}
