#include "spirv_edit.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <string>

extern "C" {
#include "wrapper_log.h"
#include "wrapper_debug.h"
}

#include "spirv-tools/spirv-tools/optimizer.hpp"
#include "spirv-tools/spirv-tools/libspirv.hpp"

// Stub out custom SPIRV-Tools passes that only exist in the patched fork.
// These are no-ops for CI builds using upstream SPIRV-Tools v2024.4.rc2.
#ifndef HAVE_CUSTOM_SPIRV_PASSES
namespace spvtools {
static inline Optimizer::PassToken CreateRemoveClipCullDistPass() { return CreateNullPass(); }
static inline Optimizer::PassToken CreateFixMaliSpecConstantCompositePass() { return CreateNullPass(); }
static inline Optimizer::PassToken CreateMaliOptimizationBarrierPass() { return CreateNullPass(); }
}
#endif

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
    if (!CHECK_FLAG("LOG_DISASSEMBLY") && !CAN_LOG(LOG_LEVEL_ALL)) {
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
        WLOGE(".spv E: " MSG)
        break;
    case SPV_MSG_WARNING:
        WLOGE(".spv W: " MSG)
        break;
    case SPV_MSG_INFO:
        WLOG(".spv I: " MSG)
        break;
    case SPV_MSG_DEBUG:
        WLOGD(".spv D: " MSG)
        break;
    default:
        break;
    }
}

extern "C"
int optimize_spirv_for_size(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out) {
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
int lower_eliminate_clip_distance(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out) {
    _Atomic static int counter = 0;
    int id = counter++;
    spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1_SPIRV_1_4);

    optimizer.SetMessageConsumer([&](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
        OptimizerMessageConsumer(level, source, position, message, id);
    });

    // Mali specific pass
    optimizer.RegisterPass(spvtools::CreateRemoveClipCullDistPass());

    if (CHECK_FLAG("SPIRV_AGGRESSIVE_DCE")) {
        optimizer.RegisterPass(spvtools::CreateAggressiveDCEPass());
    }

    std::vector<uint32_t> optimized_binary;
    if (!optimizer.Run(spirv_binary, spirv_word_count, &optimized_binary)) {
        WLOGE("RemoveClipDistancePass failed");
        return 1;
    }

    if (optimized_binary.size() != spirv_word_count) {
        if (CHECK_FLAG("LOG_DISASSEMBLY"))
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

// TODO: redesign the spirv passes to avoid calling two separate passes
extern "C"
int fix_mali_spec_composite_constants(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out) {
   _Atomic static int counter = 0;
   int id = counter++;
   spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1_SPIRV_1_4);

   optimizer.SetMessageConsumer([&](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
       OptimizerMessageConsumer(level, source, position, message, id);
   });

   // Mali specific pass
   optimizer.RegisterPass(spvtools::CreateFixMaliSpecConstantCompositePass());

   std::vector<uint32_t> optimized_binary;
   if (!optimizer.Run(spirv_binary, spirv_word_count, &optimized_binary)) {
      WLOGE("FixMaliSpecConstantCompositePass failed");
      return 1;
   }

   if (optimized_binary.size() != spirv_word_count) {
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

extern "C"
int add_optimization_barriers(const uint32_t* spirv_binary, size_t spirv_word_count, SpirvCode* out) {
   _Atomic static int counter = 0;
   int id = counter++;
   spvtools::Optimizer optimizer(SPV_ENV_VULKAN_1_1_SPIRV_1_4);

   optimizer.SetMessageConsumer([&](spv_message_level_t level, const char* source, const spv_position_t& position, const char* message) {
       OptimizerMessageConsumer(level, source, position, message, id);
   });

   optimizer.RegisterPass(spvtools::CreateMaliOptimizationBarrierPass());

   std::vector<uint32_t> optimized_binary;
   if (!optimizer.Run(spirv_binary, spirv_word_count, &optimized_binary)) {
      WLOGE("AddOptimizationBarrierPass failed");
      return 1;
   }

   if (optimized_binary.size() != spirv_word_count) {
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