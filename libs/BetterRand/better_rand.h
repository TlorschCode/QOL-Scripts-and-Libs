#pragma once

#include <stdint.h>
#include <cstddef>
#include <stdexcept>
#include <bit>
#include <concepts>
#include <immintrin.h>

#ifndef _RNGSTATE_DEF
#define _RNGSTATE_DEF
struct RNGstate {
    uint64_t state0;
    uint64_t state1;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

using ui64 = uint64_t;
using ui32 = uint32_t;
using ui16 = uint16_t;

constexpr __forceinline ui64 gen_rand64(RNGstate& state) {
    const ui64 output = std::rotl<ui64>(state.state0 + state.state1, 17) + state.state0;
    state.state1 ^= state.state0;
    state.state0 = std::rotl<ui64>(state.state0, 49) ^ state.state1 ^ (state.state1 << 21);
    state.state1 = std::rotl<ui64>(state.state1, 28);
    return output;
}

constexpr __forceinline ui64 gen_rand64HQ(RNGstate& state) {
    const ui64 output = std::rotl<ui64>(state.state0 * 5, 7) * 9;
    state.state1 ^= state.state0;
    state.state0 = std::rotl<ui64>(state.state0, 49) ^ state.state1 ^ (state.state1 << 21);
    state.state1 = std::rotl<ui64>(state.state1, 28);
    return output;
}

static constexpr __forceinline ui64 rejectionSample(ui64 randNum, const ui64 range, const ui64 threshold) {
    using i128 = __int128_t;
    ui64 hi;
    ui64 low;
    do {
        const i128 prod = static_cast<i128>(randNum) * static_cast<i128>(range);
        low = static_cast<ui64>(prod);
        hi = prod >> 64;
    } while (low < threshold);
    return hi;
}

constexpr inline ui64 gen_urandint(RNGstate& state, ui64 _min, ui64 _max) {
    if (_min == _max) return _min;
    const ui64 range = _max - _min + 1;
    const ui64 threshold = (-range) % range;
    return rejectionSample(gen_rand64(state), range, threshold);
}

constexpr inline ui64 gen_urandintHQ(RNGstate& state, ui64 _min, ui64 _max) {
    if (_min == _max) return _min;
    const ui64 range = _max - _min + 1;
    const ui64 threshold = (-range) % range;
    return rejectionSample(gen_rand64HQ(state), range, threshold);
}

__forceinline ui64 gen_seed64() {
    ui64 result;
    bool success;
    while (!(success = _rdseed64_step(&result)));
    return result;
}

__forceinline ui32 gen_seed32() {
    ui32 result;
    bool success;
    while (!(success = _rdseed32_step(&result)));
    return result;
}

__forceinline ui16 gen_seed16() {
    ui16 result;
    bool success;
    while (!(success = _rdseed16_step(&result)));
    return result;
}

constexpr inline ui64 gen_seed(uint64_t _min, uint64_t _max) {
    if (_min == _max) return _min;
    const ui64 range = _max - _min + 1;
    const ui64 threshold = (-range) % range;
    return rejectionSample(gen_seed64(), range, threshold);
}

constexpr inline ui64 splitmix64(ui64 num) {
    num += 0x9E3779B97F4A7C15;
    num = (num ^ (num >> 30)) * 0xBF58476D1CE4E5B9;
    num = (num ^ (num >> 27)) * 0x94D049BB133111EB;
    return num ^ (num >> 31);
}

inline void seed_state(RNGstate& state) {
    state.state0 = gen_seed64();
    state.state1 = splitmix64(state.state0);
}

#ifdef __cplusplus
}
#endif
