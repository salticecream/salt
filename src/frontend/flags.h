#pragma once

#include <cstring>
#include "../common.h"
#include <map>
#include <unordered_set>

enum class Flags_e : uint64_t {
    DEBUG_OUTPUT,
    DEBUG_OUTPUT_VERBOSE,
    TOTAL,
};

struct CompilerFlag {
    Flags_e flag;
    std::string data;
    CompilerFlag(Flags_e flag);
    CompilerFlag(Flags_e flag, const std::string& data);
};

namespace Flags {
    extern std::map<std::string, Flags_e> all_flags;
}


