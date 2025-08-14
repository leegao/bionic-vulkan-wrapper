#include "artifacts.h"
#include "wrapper_objects.h"
#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "wrapper_checks.h"
#include "vk_printers.h"

#include <string>
#include <sstream>
#include <iomanip>

static FILE* open_log_file(const std::string postfix, VkFormat original_format, int id, const char* mode) {
    static std::string dir;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        char time_str[20];
        get_current_time_string(time_str, sizeof(time_str));
        dir = std::string("/sdcard/Documents/Wrapper/artifacts_") + time_str + "." + getprogname() + "." + std::to_string(getpid());
        if (mkdir(dir.c_str(), 0777) == 0) {
            WLOGE("Failed to create the artifacts directory %s", dir.c_str());
        } else {
            WLOGD("Logging artifacts to %s", dir.c_str());
        }
    }
    std::stringstream padded_id;
    padded_id << std::setw(5) << std::setfill('0') << id;

    std::string path = dir + "/" + padded_id.str() + "_fmt" + std::to_string(original_format) + postfix;
    return fopen(path.c_str(), mode);
}

extern "C"
void RecordBCnArtifacts(struct wrapper_device* device, VkFormat original_format,
                        const VkBufferImageCopy* region, VkBuffer srcBuffer, VkBuffer stagingBuffer, int decode_id) {
    if (region) {
        auto fd = open_log_file("_region.txt", original_format, decode_id, "w");
        fprintf(fd, "src: %p\n", srcBuffer);
        vk_print_VkBufferImageCopy(0, 0, fd, "", region);
        fclose(fd);
    }

    struct wrapper_buffer* wbuf = get_wrapper_buffer(device, srcBuffer);
    if (!wbuf) {
        WLOGE("srcBuffer not tracked, skipping (decode_id=%d)", decode_id);
    } else if (wbuf->memory == VK_NULL_HANDLE) {
        WLOGE("srcBuffer not bound, skipping (decode_id=%d)", decode_id);
    } else {
        void* srcData;
        VkMemoryMapInfoKHR mapInfoSrc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
            .memory = wbuf->memory,
            .offset = wbuf->memoryOffset,
            .size = VK_WHOLE_SIZE,
        };
        VkResult result = WCHECK(MapMemory2KHR((VkDevice) device, &mapInfoSrc, &srcData));
        if (result != VK_SUCCESS) {
            WLOGE("ERROR: Failed to map srcBuffer memory: %d", result);
        } else {
            int w = region->bufferRowLength ? region->bufferRowLength : region->imageExtent.width;
            int h = region->bufferImageHeight ? region->bufferImageHeight : region->imageExtent.height;
            int blocks_stride = (w + 3) / 4;
            int block_rows = (h + 3) / 4;
            int blocks = blocks_stride * block_rows;
            int block_size = get_bc_block_size(original_format);
            auto fd = open_log_file("_src.dat", original_format, decode_id, "wb");
            fwrite(srcData, block_size, blocks, fd);
            fclose(fd);
        }
    }

    wbuf = get_wrapper_buffer(device, stagingBuffer);
    if (!wbuf) {
        WLOGE("dstBuffer not tracked, skipping (decode_id=%d)", decode_id);
    } else if (wbuf->memory == VK_NULL_HANDLE) {
        WLOGE("dstBuffer not bound, skipping (decode_id=%d)", decode_id);
    } else {
        void* dstData;
        VkMemoryMapInfoKHR mapInfoSrc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
            .memory = wbuf->memory,
            .offset = 0,
            .size = VK_WHOLE_SIZE,
        };
        VkResult result = WCHECK(MapMemory2KHR((VkDevice) device, &mapInfoSrc, &dstData));
        if (result != VK_SUCCESS) {
            WLOGE("ERROR: Failed to map dstBuffer memory: %d", result);
        } else {
            int block_size = get_bc_target_size(device->physical, original_format);
            auto fd = open_log_file("_dst.dat", original_format, decode_id, "wb");
            fwrite(dstData, block_size, region->imageExtent.width * region->imageExtent.height, fd);
            fclose(fd);
            WCHECKV(UnmapMemory((VkDevice) device, wbuf->memory));
        }
    }
}
