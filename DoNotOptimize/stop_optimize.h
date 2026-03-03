#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifndef SMPL_LIB_NO_OPTIMIZE
#define SMPL_LIB_NO_OPTIMIZE

template <typename T>
[[gnu::always_inline]] inline void do_not_optimize(T& value) {
#if defined(__GNUC__) || defined(__clang__)
    // Tells compiler "value" is read/write and may alias memory,
    // prevents DCE and reordering, but emits no instructions.
    asm volatile("" : "+r"(value) : : "memory");
#elif defined(_MSC_VER)
    // Compiler fence/barrier: prevents reordering around this point.
    _ReadWriteBarrier();
    (void)value;
#endif
}

#endif