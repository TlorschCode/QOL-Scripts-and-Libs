#pragma once
#include <chrono>
#include <cstdint>
#include <concepts>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h> // for _ReadWriteBarrier()
#endif

#ifndef SMPL_LIB_NO_OPTIMIZE
#define SMPL_LIB_NO_OPTIMIZE
template <typename T>
[[gnu::always_inline]] inline void do_not_optimize(T& value) {
#if defined(__GNUC__) || defined(__clang__)
    asm volatile("" : "+r"(value) : : "memory"); // tell compiler value may be read from and written to in this asm bit
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
    (void)value;
#endif
}
#endif


struct BenchResult {
    double ms;
    uint64_t iterations;
    double nsPerCall;
};


static inline double ns_per_call(double ms, uint64_t iterations) {
    return (ms * 1.0e6) / static_cast<double>(iterations);
}

template<typename F, typename R = std::invoke_result_t<F>>
concept NonVoidReturn = !std::is_void_v<R>;

// Benchmarks a function passed into it. Returns a BenchResult.
// The function passed in should return some value (whether the value matters is irrevelant). This is necessary to guarantee that the compiler won't remove the loop inside `benchmark` as dead code.
template <NonVoidReturn Func>
BenchResult benchmark(Func&& function_, uint64_t iterations) {
    using hr_clock = std::chrono::high_resolution_clock;
    using ms_duration = std::chrono::duration<double, std::milli>;
    uint64_t sink = 0; // plain, NOT volatile — see fairness note 3 in header

    auto timeStart = hr_clock::now();
    for (uint64_t i = 0; i < iterations; ++i) {
        auto result = function_();
        do_not_optimize(result); // prevent dead code elimination
    }
    auto timeEnd = hr_clock::now();

    double ms = std::chrono::duration_cast<ms_duration>(timeEnd - timeStart).count();
    return {
        ms,
        iterations,
        ns_per_call(ms, iterations)
    };
}
