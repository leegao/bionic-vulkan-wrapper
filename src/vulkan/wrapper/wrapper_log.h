#pragma once

#include <stdbool.h>
#include <stdint.h>

int __android_log_print(
  int prio,
  const char *tag,
  const char *fmt,
  ...
);

#define LOG(...) __android_log_print(6, "VkWrapper", __VA_ARGS__)

static int __log_level;
static FILE* __log_fd;

#define LOG_FD __log_fd
#define LOG_LEVEL_ALL 4
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_VERBOSE 2
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_NONE 0

#define LOG_STRUCT(st, obj) { \
   int can_log_level = should_log(); \
   vk_print_##st(can_log_level, LOG_LEVEL_DEBUG, LOG_FD, "  ", obj); \
   if (can_log_level >= LOG_LEVEL_DEBUG) fflush(LOG_FD); \
}

#define WLOGA(...) { if (should_log() >= LOG_LEVEL_ALL)  __wlog(__VA_ARGS__); }
#define WLOGD(...) { if (should_log() >= LOG_LEVEL_DEBUG)  __wlog(__VA_ARGS__); }
#define WLOG(...) { if (should_log() >= LOG_LEVEL_VERBOSE)  __wlog(__VA_ARGS__); }
#define WLOGE(...) { if (should_log() >= LOG_LEVEL_ERROR)  __wlog("[ERROR] " __VA_ARGS__); LOG(__VA_ARGS__); }

static void get_current_time_string(char* buffer, size_t bufferSize) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL) {
        buffer[0] = '\0';
        return;
    }
    strftime(buffer, bufferSize, "%Y_%m_%d_%H", tm_info);
}

static void cleanup_log_file(void) {
    if (__log_fd) {
        fclose(__log_fd);
        __log_fd = NULL;
    }
}

static int should_log() {
    static bool __log_initialized;
    if (__log_initialized) {
        return __log_level;
    }

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
        char time_str[20];
        get_current_time_string(time_str, sizeof(time_str));
        char path[256];
        sprintf(path, "/sdcard/Documents/Wrapper/wrapper_log_%s.%s.%d.txt", time_str, getprogname(), getpid());
        LOG("Logging at level %d to %s", __log_level, path);
        __log_fd = fopen(path, "a");
        if (!__log_fd) {
            __log_level = 0;
        } else {
            atexit(cleanup_log_file);
        }
    }

    __log_initialized = true;
    return __log_level;
}

static void __wlog(const char* fmt, ...) {
   va_list args;
   va_start(args, fmt);
   vfprintf(__log_fd, fmt, args);
   va_end(args);
   fprintf(__log_fd, "\n");
   fflush(__log_fd);
}


static void __log(const char* fmt, ...) {
   if (should_log() < LOG_LEVEL_VERBOSE) {
      return;
   }
   va_list args;
   va_start(args, fmt);
   vfprintf(__log_fd, fmt, args);
   va_end(args);
   fprintf(__log_fd, "\n");
   fflush(__log_fd);
}

static void __loge(const char* fmt, ...) {
   if (should_log() < LOG_LEVEL_ERROR) {
      return;
   }
   fprintf(__log_fd, "[ERROR] ");
   va_list args;
   va_start(args, fmt);
   vfprintf(__log_fd, fmt, args);
   va_end(args);
   fprintf(__log_fd, "\n");
   fflush(__log_fd);
}

static void __loga(const char* fmt, ...) {
   if (should_log() < LOG_LEVEL_ALL) {
      return;
   }
   va_list args;
   va_start(args, fmt);
   vfprintf(__log_fd, fmt, args);
   va_end(args);
   fprintf(__log_fd, "\n");
   fflush(__log_fd);
}
