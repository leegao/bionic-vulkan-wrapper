#include "artifacts.h"
#include "wrapper_objects.h"
#include "wrapper_private.h"
#include "wrapper_entrypoints.h"
#include "wrapper_checks.h"
#include "vk_printers.h"

#ifndef __ANDROID__
#include <cerrno>
extern char *program_invocation_short_name;
static inline const char *getprogname() { return program_invocation_short_name; }
#endif

#define XXH_INLINE_ALL
#include "util/xxhash.h"

#include <string>
#include <sstream>
#include <iomanip>

static FILE* open_log_file(const std::string postfix, VkFormat original_format, int w, int h, int id, const char* mode) {
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

    std::string path = dir + "/" + padded_id.str() + "_fmt" + std::to_string(original_format) + "_" + std::to_string(w) + "x" + std::to_string(h) + postfix;
    return fopen(path.c_str(), mode);
}

static bool ValidateBCn(struct wrapper_device* device, VkFormat original_format,
                        const VkBufferImageCopy* region, void* srcData, void* dstData,
                        int decode_id, void** out) {
    uint32_t block_size = get_bc_target_size(device->physical, original_format);
    uint32_t data_size = block_size * region->imageExtent.width * region->imageExtent.height;

    int block_rows = (region->imageExtent.width + 3) / 4;
    int block_cols = (region->imageExtent.height + 3) / 4;
    void* stagingData = malloc(block_rows * block_cols * 16 * block_size);
    WLOGD("Decoding for validation, fmt=%d, size=%d", original_format,  data_size);
    BCnDecompression(original_format, srcData, stagingData, region, false);

    WLOGD("Calculating h1 for stagingData, fmt=%d, size=%d", original_format,  data_size);
    auto h1 = XXH32(stagingData, data_size, 0);
    WLOGD("Calculated h1 = %x for stagingData, fmt=%d, size=%d", h1, original_format,  data_size);

    WLOGD("Calculating h2 for dstData, fmt=%d, size=%d", original_format,  data_size);
    auto h2 = XXH32(dstData, data_size, 0);
    WLOGD("Calculated h2 = %x for dstData, fmt=%d, size=%d", h2, original_format,  data_size);
    *out = stagingData;
    return h1 == h2;
}

extern "C"
void RecordBCnArtifacts(struct wrapper_device* device, struct wrapper_image* wimg,
                        const VkBufferImageCopy* region, VkBuffer srcBuffer, VkBuffer stagingBuffer,
                        int decode_id, bool validate_bcn) {
    void *srcData;
    void *dstData;
    VkResult result;
    if (!wimg) {
        WLOGE("dstImage not tracked, skipping (decode_id=%d)", decode_id);
        return;
    }
    VkFormat original_format = wimg->original_format;

    struct wrapper_buffer* src_wbuf = get_wrapper_buffer(device, srcBuffer);
    if (!src_wbuf) {
        WLOGE("srcBuffer not tracked, skipping (decode_id=%d)", decode_id);
        return;
    } else if (src_wbuf->memory == VK_NULL_HANDLE) {
        WLOGE("srcBuffer not bound, skipping (decode_id=%d)", decode_id);
        return;
    }
    VkMemoryMapInfoKHR mapInfoSrc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
        .memory = src_wbuf->memory,
        .offset = src_wbuf->memoryOffset,
        .size = VK_WHOLE_SIZE,
    };
    result = WCHECK(MapMemory2KHR((VkDevice) device, &mapInfoSrc, &srcData));
    if (result != VK_SUCCESS) {
        WLOGE("Failed to map srcBuffer memory: %d", result);
        return;
    }

    struct wrapper_buffer* dst_wbuf = get_wrapper_buffer(device, stagingBuffer);
    if (!dst_wbuf) {
        WLOGE("dstBuffer not tracked, skipping (decode_id=%d)", decode_id);
    } else if (dst_wbuf->memory == VK_NULL_HANDLE) {
        WLOGE("dstBuffer not bound, skipping (decode_id=%d)", decode_id);
    }
    // Invalidate the memory to be visible to the host in case this was decoded via the compute shader
    VkMemoryMapInfoKHR mapInfoDst = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
        .memory = dst_wbuf->memory,
        .offset = dst_wbuf->memoryOffset,
        .size = VK_WHOLE_SIZE,
    };
    result = WCHECK(MapMemory2KHR((VkDevice) device, &mapInfoDst, &dstData));
    if (result != VK_SUCCESS) {
        WLOGE("Failed to map dstBuffer memory: %d", result);
        return;
    }
    VkMappedMemoryRange flushRange = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = dst_wbuf->memory,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };
    result = WCHECK(InvalidateMappedMemoryRanges((VkDevice) device, 1, &flushRange));
    if (result != VK_SUCCESS) {
        WLOGE("Failed to flush dstBuffer memory: %d", result);
    }

    void* referenceData = NULL;
    if (validate_bcn) {
        if (ValidateBCn(device, original_format, region, srcData, dstData, decode_id, &referenceData)) {
            // No difference between reference and compute implementations
            WLOGD("Reference and compute implementations of ValidateBCn(fmt=%d, id=%d) are the same.", original_format, decode_id);
            free(referenceData);
            WCHECKV(UnmapMemory((VkDevice) device, dst_wbuf->memory));
            return;
        }
        WLOGE("Reference and compute implementations of ValidateBCn(fmt=%d, id=%d) differ.", original_format, decode_id);
    }

    int width = region->imageExtent.width;
    int height = region->imageExtent.height;
    FILE* fd;
    if (region && (fd = open_log_file("_region.txt", original_format, width, height, decode_id, "w"))) {
        fprintf(fd, "src: %p\n", srcBuffer);
        fprintf(fd, "extent: %d x %d\n", wimg->vk.extent.width, wimg->vk.extent.height);
        vk_print_VkBufferImageCopy(0, 0, fd, "", region);
        fclose(fd);
    }

    if ((fd = open_log_file("_src.dat", original_format, width, height, decode_id, "wb"))) {
        int w = region->bufferRowLength ? region->bufferRowLength : region->imageExtent.width;
        int h = region->bufferImageHeight ? region->bufferImageHeight : region->imageExtent.height;
        int blocks_stride = (w + 3) / 4;
        int block_rows = (h + 3) / 4;
        int blocks = blocks_stride * block_rows;
        int block_size = get_bc_block_size(original_format);
        fwrite((uint8_t*) srcData + region->bufferOffset, block_size, blocks, fd);
        fclose(fd);
    }

    if ((fd = open_log_file("_dst.dat", original_format, width, height, decode_id, "wb"))) {
        int block_size = get_bc_target_size(device->physical, original_format);
        auto fd = open_log_file("_dst.dat", original_format, width, height, decode_id, "wb");
        fwrite(dstData, block_size, region->imageExtent.width * region->imageExtent.height, fd);
        fclose(fd);
        WCHECKV(UnmapMemory((VkDevice) device, dst_wbuf->memory));
    }

    if (referenceData && (fd = open_log_file("_ref.dat", original_format, width, height, decode_id, "wb"))) {
        int block_size = get_bc_target_size(device->physical, original_format);
        fwrite(referenceData, block_size, region->imageExtent.width * region->imageExtent.height, fd);
        fclose(fd);
        free(referenceData);
    }
}


extern "C"
void RecordBCnSrcArtifacts(struct wrapper_device* device, VkFormat original_format,
    const VkBufferImageCopy* region, VkBuffer srcBuffer, int decode_id) {
    void *srcData;
    VkResult result;

    struct wrapper_buffer* src_wbuf = get_wrapper_buffer(device, srcBuffer);
    if (!src_wbuf) {
        WLOGE("srcBuffer not tracked, skipping (decode_id=%d)", decode_id);
        return;
    } else if (src_wbuf->memory == VK_NULL_HANDLE) {
        WLOGE("srcBuffer not bound, skipping (decode_id=%d)", decode_id);
        return;
    }
    VkMemoryMapInfoKHR mapInfoSrc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO_KHR,
        .memory = src_wbuf->memory,
        .offset = src_wbuf->memoryOffset,
        .size = VK_WHOLE_SIZE,
    };
    result = WCHECK(MapMemory2KHR((VkDevice) device, &mapInfoSrc, &srcData));
    if (result != VK_SUCCESS) {
        WLOGE("Failed to map srcBuffer memory: %d", result);
        return;
    }

    int width = region->imageExtent.width;
    int height = region->imageExtent.height;

    FILE* fd;
    if (region && (fd = open_log_file("_sregion.txt", original_format, width, height, decode_id, "w"))) {
        fprintf(fd, "src: %p\n", srcBuffer);
        fprintf(fd, "src_id: %d\n", src_wbuf->obj_id);
        vk_print_VkBufferImageCopy(0, 0, fd, "", region);
        fclose(fd);
    }

    if ((fd = open_log_file("_sbuf.dat", VK_FORMAT_BC1_RGBA_UNORM_BLOCK, src_wbuf->vk.size, 1, src_wbuf->obj_id, "wb"))) {
        fwrite((uint8_t*) srcData, 1, src_wbuf->vk.size, fd);
        fclose(fd);
    }
}
