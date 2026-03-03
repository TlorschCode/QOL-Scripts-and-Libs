#include "asm_rand.h"
#include <string>
#include <source_location>
#include <stdexcept>

#define badRNGstate_layout_msg "Compiler added unexpected padding to RNGstate struct. Unable to access state1 at [rcx + 8]. To fix this, add `#pragma pack(push, 1)` before the RNGstate struct, and `#pragma pack(pop)` after the RNGstate struct."

static_assert(sizeof(RNGstate) == 16, badRNGstate_layout_msg);
static_assert(offsetof(RNGstate, state1) == 8, badRNGstate_layout_msg);

extern "C" {
    void _throw_bad_arg_error(const char* message, uint64_t min, uint64_t max, uint64_t output) {
        throw std::logic_error(message + std::to_string(min) + ", max: " + std::to_string(max) + ", output: " + std::to_string(output));
    }
}
