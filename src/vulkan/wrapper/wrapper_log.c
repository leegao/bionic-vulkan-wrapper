#include "wrapper_private.h"
#include "wrapper_log.h"

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
