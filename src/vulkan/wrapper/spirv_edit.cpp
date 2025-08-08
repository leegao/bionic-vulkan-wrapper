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

extern "C"
void log_disassembly_to_cmd_log(const uint32_t* spirv_binary, size_t spirv_word_count) {
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1_SPIRV_1_4);
    std::string disassembly;
    tools.Disassemble({spirv_binary, spirv_binary + spirv_word_count}, &disassembly, SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
    VK_CMD_PRINTF("%s", disassembly.c_str());
}

static void print_spirv_code(const char* prefix, int size, const uint32_t* pCode) {
    if (size == 0 || !pCode) {
        return;
    }
    // Print the SPIR-V code in int32 hex with 32 bytes (8 uint32_t) per line
    int i = 0;
    while (i < size / 4) {
        if (i + 7 < size / 4) {
            WLOG("%s.spirv(%p)[%04x]: %08x %08x %08x %08x %08x %08x %08x %08x", prefix, pCode, 4 * i,
                          pCode[i], pCode[i + 1], pCode[i + 2], pCode[i + 3],
                          pCode[i + 4], pCode[i + 5], pCode[i + 6], pCode[i + 7]);
            i += 8;
            continue;
        }

        if (i % 8 == 0) {
            WLOG("%s.spirv(%p)[%04x]: ", prefix, pCode, 4 * i);
        }
        WLOG("%08x ", pCode[i]);
        i++;
    }
}

void LogDisassembly(const std::string& title, const std::vector<uint32_t>& binary, int id) {
    if (!CHECK_FLAG("LOG_DISASSEMBLY")) {
        return;
    }
    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_1_SPIRV_1_4);
    std::string disassembly;
    tools.Disassemble(binary, &disassembly, SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES);
    WLOG("--- Disassembly: %s --- (id=%d)", title.c_str(), id);
    WLOG("(id=%d)\n%s", id, disassembly.c_str());
    print_spirv_code("  ", binary.size() * 4, binary.data());
}

void OptimizerMessageConsumer(spv_message_level_t level, const char* source,
    const spv_position_t& position, const char* message, int id) {
#define MSG "%s (id=%d) (%s:%d)", message, id, source, position.line
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

    optimizer.SetMessageConsumer([&](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
        OptimizerMessageConsumer(level, source, position, message, 0);
    });

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

    // LogDisassembly("Optimized", optimized_binary);

    out->spirv_word_count = optimized_binary.size();
    out->spirv_code = static_cast<uint32_t*>(malloc(optimized_binary.size() * 4));
    if (!out->spirv_code) {
        WLOGE("Failed to allocate memory for optimized SPIR-V");
        return 1;
    }
    std::memcpy(out->spirv_code, optimized_binary.data(), optimized_binary.size() * 4);

    return 0;
}


extern "C"
int lower_eliminate_clip_distance(const uint32_t* spirv_binary, size_t spirv_word_count, struct SpirvCode* out) {
    _Atomic static int counter = 0;
    int id = counter++;
    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1_SPIRV_1_4);

    optimizer.SetMessageConsumer([&](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
        OptimizerMessageConsumer(level, source, position, message, id);
    });

    // Cannonicalization passes
    // optimizer.RegisterPass(spvtools::CreateMergeReturnPass());
    // optimizer.RegisterPass(spvtools::CreateInlineExhaustivePass());
    // optimizer.RegisterPass(spvtools::CreateEliminateDeadFunctionsPass());

    // Mali specific pass
    optimizer.RegisterPass(spvtools::CreateRemoveClipCullDistPass());

    // optimizer.RegisterPerformancePasses(); // For -O

    optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
    // optimizer.RegisterPass(spvtools::CreateCompactIdsPass());

    WLOGD("Original SPIR-V Word Count %d (id=%d)", spirv_word_count, id);

    std::vector<uint32_t> optimized_binary;
    bool success = optimizer.Run(spirv_binary, spirv_word_count, &optimized_binary);

    if (!success) {
        WLOGE("SPIRV editing failed");
        return 1;
    }

    WLOGD("Lowered SPIR-V Word Count %d (id=%d)", optimized_binary.size(), id);

    if (optimized_binary.size() != spirv_word_count) {
        LogDisassembly("Original", {spirv_binary, spirv_binary+spirv_word_count}, id);
        LogDisassembly("Lowered", optimized_binary, id);
    }

    out->spirv_word_count = optimized_binary.size();
    out->spirv_code = static_cast<uint32_t*>(malloc(optimized_binary.size() * 4));
    if (!out->spirv_code) {
        WLOGE("Failed to allocate memory for optimized SPIR-V (id=%d)", id);
        return 1;
    }
    std::memcpy(out->spirv_code, optimized_binary.data(), optimized_binary.size() * 4);

    return 0;
}
