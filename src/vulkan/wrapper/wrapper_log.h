#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan_core.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "wrapper_debug.h"

extern int __android_log_print(
  int prio,
  const char *tag,
  const char *fmt,
  ...
);

#define LOG(...) __android_log_print(6, "VkWrapper", __VA_ARGS__)

// For wrapper log
#define LOG_FD get_wrapper_log_fd()
#define LOG_LEVEL_ALL 5
#define LOG_LEVEL_TRACE 4
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_VERBOSE 2
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_NONE 0

#define LOG_STRUCT_AT(level, st, obj) { \
   int can_log_level = should_log(); \
   FILE* log_fd = LOG_FD; \
   vk_print_##st(can_log_level, LOG_LEVEL_##level, log_fd, "  ", obj); \
   if (can_log_level >= LOG_LEVEL_##level) fflush(log_fd); \
}

#define LOG_STRUCT(st, obj) LOG_STRUCT_AT(DEBUG, st, obj)

#define WFORMAT(fmt, ...) fmt " \t (%s:%d)", ## __VA_ARGS__, __FUNCTION__, __LINE__

#define __WLOG__(LEVEL, fmt, ...) \
    if (should_log() >= LEVEL) { \
        wlog(WFORMAT(fmt, ## __VA_ARGS__)); \
        LOG(WFORMAT(fmt, ## __VA_ARGS__)); \
    }

#define WLOGT(fmt, ...) __WLOG__(LOG_LEVEL_TRACE, "" fmt, ## __VA_ARGS__)
#define WLOGA(fmt, ...) __WLOG__(LOG_LEVEL_ALL, "! " fmt, ## __VA_ARGS__)
#define WLOGD(fmt, ...) __WLOG__(LOG_LEVEL_DEBUG, fmt, ## __VA_ARGS__)
#define WLOG(fmt, ...)  __WLOG__(LOG_LEVEL_VERBOSE, fmt, ## __VA_ARGS__)
#define WLOGE(fmt, ...) __WLOG__(LOG_LEVEL_ERROR, "[ERROR] " fmt, ## __VA_ARGS__)
#define CAN_LOG(level) (should_log() >= level)

// For wrapper cmd-log
#define VK_CMD_FD get_wrapper_cmd_log_fd()
#define VK_CMD_CAN_LOG_LEVEL should_log_cmd()
#define VK_CMD_ALL 2
#define VK_CMD_NAME 1
#define VK_CMD_NONE 0

#define VK_CMD_LOG_FD(fd, fmt, ...) { \
    FILE* __fd = (fd); \
    if (__fd) { \
        fprintf(__fd, fmt "\n", ## __VA_ARGS__); \
    } \
}
#define VK_CMD_LOG_UNCONDITIONAL(fmt, ...) VK_CMD_LOG_FD(VK_CMD_FD, fmt, ## __VA_ARGS__)

#define __VK_CMD_LOG(can_log_level, level, fd, fmt, ...) { \
    if (can_log_level >= level) { \
        VK_CMD_LOG_FD(fd, fmt, ## __VA_ARGS__); \
    } \
}

#define VK_CMD_LOG(...) __VK_CMD_LOG(VK_CMD_CAN_LOG_LEVEL, VK_CMD_NAME, VK_CMD_FD, __VA_ARGS__)
#define VK_CMD_LOGA(...) __VK_CMD_LOG(VK_CMD_CAN_LOG_LEVEL, VK_CMD_ALL, VK_CMD_FD, __VA_ARGS__)

#define VK_CMD_CAN_LOGA ((VK_CMD_CAN_LOG_LEVEL >= VK_CMD_ALL) && (VK_CMD_FD))
#define VK_CMD_CAN_TRACE ((VK_CMD_CAN_LOG_LEVEL >= VK_CMD_ALL) && (VK_CMD_FD) && use_wrapper_trace())

#define VK_CMD_PRINTF(fmt, ...) { if ((VK_CMD_CAN_LOG_LEVEL >= VK_CMD_ALL) && (VK_CMD_FD)) fprintf(VK_CMD_FD, fmt, ## __VA_ARGS__); }
#define VK_CMD_FLUSH() { if ((VK_CMD_CAN_LOG_LEVEL >= VK_CMD_ALL) && (VK_CMD_FD)) fflush(VK_CMD_FD); }

void get_current_time_string(char* buffer, size_t bufferSize);

FILE* get_wrapper_log_fd(void);

FILE* get_wrapper_cmd_log_fd(void);

int should_log(void);

int should_log_cmd(void);

int should_log_wsi(void);

extern __thread int msg_level;

#define INC_MSG if(CAN_LOG(LOG_LEVEL_TRACE)) msg_level++
#define DEC_MSG if(CAN_LOG(LOG_LEVEL_TRACE)) msg_level--


#define WLOGD_OPEN(fmt, ...) if(CAN_LOG(LOG_LEVEL_DEBUG)) { __WLOG__(LOG_LEVEL_DEBUG, fmt, ## __VA_ARGS__); msg_level++; }
#define WLOGD_CLOSE(fmt, ...) if(CAN_LOG(LOG_LEVEL_DEBUG)) { msg_level--; __WLOG__(LOG_LEVEL_DEBUG, fmt, ## __VA_ARGS__); }

void wlog(const char* fmt, ...);

void wlog_wsi(const char* fmt, ...);

VKAPI_ATTR VkBool32 VKAPI_CALL
wrapper_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT                  messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
    void*                                            pUserData);

struct wrapper_device;

void wrapper_log_device_fault(struct wrapper_device *device, const char* call);

#ifdef __cplusplus
}
#endif
