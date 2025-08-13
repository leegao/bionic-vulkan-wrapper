#include "artifacts.h"
#include "wrapper_objects.h"
#include "wrapper_private.h"

#include <string>

static FILE* open_log_file(const char* postfix, int id) {
    static char dir[256];
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        char time_str[20];
        get_current_time_string(time_str, sizeof(time_str));
        sprintf(dir, "/sdcard/Documents/Wrapper/artifacts_%s.%s.%d", time_str, getprogname(), getpid());
        if (mkdir(dir, 0777) == 0) {
            WLOGE("Failed to create the artifacts directory %s", dir);
        } else {
            WLOGD("Logging artifacts to %s", dir);
        }
    }
    std::string path = std::string(dir) + "/" + std::to_string(id) + "_" + postfix;
    return fopen(path.c_str(), "w");
}

extern "C"
void RecordBCnArtifacts(struct wrapper_device* device, const VkBufferImageCopy* region, VkBuffer srcBuffer, VkBuffer stagingBuffer, int decode_id) {
    // auto fd = open_log_file("region.txt", decode_id);
    struct wrapper_buffer* wbuf = get_wrapper_buffer(device, srcBuffer);
    if (!wbuf) {
        WLOGE("srcBuffer not tracked, skipping (decode_id=%d)", decode_id);
    }
    // TODO: Implement this
}
