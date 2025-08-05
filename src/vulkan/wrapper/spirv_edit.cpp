#include "spirv_edit.h"
#include <iostream>
#include <vector>
#include <string>

extern "C" {
#include "wrapper_log.h"
#include "wrapper_debug.h"
}

#include "spirv-tools/spirv-tools/optimizer.hpp"
#include "spirv-tools/spirv-tools/libspirv.hpp"

void LogDisassembly(const std::string& title, const std::vector<uint32_t>& binary) {
    if (!CAN_LOG(LOG_LEVEL_ALL)) {
        return;
    }
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1_SPIRV_1_4);
    std::string disassembly;
    tools.Disassemble(binary, &disassembly, SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
    WLOGA("--- %s ---", title.c_str());
    WLOGA("%s", disassembly.c_str());
}

void OptimizerMessageConsumer(spv_message_level_t level, const char* source,
    const spv_position_t& position, const char* message) {
#define MSG "%s (%s:%d)", message, source, position.line
    switch (level) {
    case SPV_MSG_FATAL:
    case SPV_MSG_INTERNAL_ERROR:
    case SPV_MSG_ERROR:
        WLOGE("SPIRV Error: " MSG)
        break;
    case SPV_MSG_WARNING:
        WLOGE("SPIRV Warning: " MSG)
        break;
    case SPV_MSG_INFO:
        WLOG("SPIRV Info: " MSG)
        break;
    default:
        break;
    }
}

extern "C"
int optimize_spirv_for_size(const uint32_t* spirv_binary, size_t spirv_word_count, struct SpirvCode* out) {
    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1_SPIRV_1_4);

    optimizer.SetMessageConsumer(OptimizerMessageConsumer);

    optimizer.RegisterPass(spvtools::CreateStripDebugInfoPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
    optimizer.RegisterPass(spvtools::CreateCompactIdsPass());

    optimizer.RegisterPerformancePasses(); // For -O
    optimizer.RegisterSizePasses();        // For -Os

    WLOGD("Original SPIR-V Word Count %d", spirv_word_count);

    std::vector<uint32_t> optimized_binary;
    bool success = optimizer.Run(spirv_binary, spirv_word_count, &optimized_binary);

    if (!success) {
        WLOGE("SPIRV editing failed");
        return 1;
    }

    WLOGD("Optimized SPIR-V Word Count %d", optimized_binary.size());

    LogDisassembly("Optimized", optimized_binary);

    out->spirv_word_count = optimized_binary.size();
    out->spirv_code = static_cast<uint32_t*>(malloc(optimized_binary.size() * 4));
    std::memcpy(out->spirv_code, optimized_binary.data(), optimized_binary.size() * 4);

    return 0;
}


extern "C"
int lower_eliminate_clip_distance(const uint32_t* spirv_binary, size_t spirv_word_count, struct SpirvCode* out) {
    // TODO - implement me
    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1_SPIRV_1_4);

    optimizer.SetMessageConsumer(OptimizerMessageConsumer);

    optimizer.RegisterPass(spvtools::CreateStripDebugInfoPass());
    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
    optimizer.RegisterPass(spvtools::CreateCompactIdsPass());

    optimizer.RegisterPerformancePasses(); // For -O
    optimizer.RegisterSizePasses();        // For -Os

    WLOGD("Original SPIR-V Word Count %d", spirv_word_count);

    std::vector<uint32_t> optimized_binary;
    bool success = optimizer.Run(spirv_binary, spirv_word_count, &optimized_binary);

    if (!success) {
        WLOGE("SPIRV editing failed");
        return 1;
    }

    WLOGD("Optimized SPIR-V Word Count %d", optimized_binary.size());

    LogDisassembly("Optimized", optimized_binary);

    out->spirv_word_count = optimized_binary.size();
    out->spirv_code = static_cast<uint32_t*>(malloc(optimized_binary.size() * 4));
    std::memcpy(out->spirv_code, optimized_binary.data(), optimized_binary.size() * 4);

    return 0;
}
