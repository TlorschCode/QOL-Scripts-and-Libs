#pragma once

#include <cstdint>
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


// Generates a biasless random number between 0 and 2^64.
// Uses `xoroshiro++`, a general purpose pseudoRNG algorithm.
// (Unsigned)
constexpr __forceinline uint64_t norm_gen_urand64(RNGstate& state) noexcept {
    const uint64_t output = std::rotl<uint64_t>(state.state0 + state.state1, 17) + state.state0;
    state.state1 ^= state.state0;
    state.state0 = std::rotl<uint64_t>(state.state0, 49) ^ state.state1 ^ (state.state1 << 21);
    state.state1 = std::rotl<uint64_t>(state.state1, 28);
    return output;
}

// Generates a biasless random number between 0 and 2^64.
// Uses `xoroshiro**`, which is higher quality (but slower) than `xoroshiro++`.
// (Unsigned)
constexpr __forceinline uint64_t norm_gen_urand64HQ(RNGstate& state) noexcept {
    const uint64_t output = std::rotl<uint64_t>(state.state0 * 5, 7) * 9;
    state.state1 ^= state.state0;
    state.state0 = std::rotl<uint64_t>(state.state0, 49) ^ state.state1 ^ (state.state1 << 21);
    state.state1 = std::rotl<uint64_t>(state.state1, 28);
    return output;
}

static constexpr __forceinline uint64_t rejectionSample(uint64_t randNum, const uint64_t range, const uint64_t threshold) noexcept {
    using i128 = __int128_t;
    uint64_t hi;
    uint64_t low;
    do {
        const i128 prod = static_cast<i128>(randNum) * static_cast<i128>(range);
        low = static_cast<uint64_t>(prod);
        hi = prod >> 64;
    } while (low < threshold);
    return hi;
}

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro++`, a general purpose pseudoRNG algorithm.
// (Unsigned)
constexpr inline uint64_t norm_gen_urandint(RNGstate& state, uint64_t _min, uint64_t _max) {
    if (_min == _max) return _min;
    if (_min > _max) throw std::logic_error("Invalid argument(s). Arg min cannot be greater than arg max for RNG range specification.");
    const uint64_t range = _max - _min + 1;
    const uint64_t threshold = (-range) % range;
    return rejectionSample(norm_gen_urand64(state), range, threshold);
}

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro**`, which is higher quality (but slower) than `norm_gen_randint`'s `xoroshiro++`.
// (Unsigned)
constexpr inline uint64_t norm_gen_urandintHQ(RNGstate& state, uint64_t _min, uint64_t _max) {
    if (_min == _max) return _min;
    if (_min > _max) throw std::logic_error("Invalid argument(s). Arg min cannot be greater than arg max for RNG range specification.");
    const uint64_t range = _max - _min + 1;
    const uint64_t threshold = (-range) % range;
    return rejectionSample(norm_gen_urand64HQ(state), range, threshold);
}

// Generates a biasless, truly random number from 0 to 2^64 (inclusive) using assembly's `RDSEED` instruction.
__forceinline uint64_t norm_gen_seed64() noexcept {
    uint64_t result;
    bool success;
    while (!(success = _rdseed64_step(&result)));
    return result;
}

// Generates a biasless, truly random number from 0 to 2^32 (inclusive) using assembly's `RDSEED` instruction.
__forceinline uint32_t norm_gen_seed32() noexcept {
    uint32_t result;
    bool success;
    while (!(success = _rdseed32_step(&result)));
    return result;
}

// Generates a biasless, truly random number from 0 to 2^16 (inclusive) using assembly's `RDSEED` instruction.
__forceinline uint16_t norm_gen_seed16() noexcept {
    uint16_t result;
    bool success;
    while (!(success = _rdseed16_step(&result)));
    return result;
}

// Generates a biasless, truly random number using the `RDSEED` assembly instruction.
//
// It is recommended to use this to seed an RNG rather than act as a standalone one, since it is much slower than `norm_gen_randint`/`gen_randint_fast` (which are pseudoRNG's).
inline uint64_t norm_gen_seed(uint64_t _min, uint64_t _max) {
    if (_min == _max) return _min;
    if (_min > _max) throw std::logic_error("Invalid argument(s). Arg min cannot be greater than arg max for RNG range specification.");
    const uint64_t range = _max - _min + 1;
    const uint64_t threshold = (-range) % range;
    return rejectionSample(norm_gen_seed64(), range, threshold);
}

static constexpr inline uint64_t splitmix64(uint64_t num) noexcept {
    num += 0x9E3779B97F4A7C15;
    num = (num ^ (num >> 30)) * 0xBF58476D1CE4E5B9;
    num = (num ^ (num >> 27)) * 0x94D049BB133111EB;
    return num ^ (num >> 31);
}

// Seeds an RNGstate struct using a true random number generator (`RDSEED`) paired with `splitmix64`.
inline void norm_seed_state(RNGstate& state) {
    state.state0 = norm_gen_seed64();
    state.state1 = splitmix64(state.state0);
}


// Generates a biasless random number between 0 and 2^64.
// Uses `xoroshiro++`, a general purpose pseudoRNG algorithm.
// (Signed)
constexpr __forceinline int64_t norm_gen_rand64(RNGstate& state) {
    return std::bit_cast<int64_t>(norm_gen_urand64(state));
}

// Generates a biasless random number between 0 and 2^64.
// Uses `xoroshiro**`, which is higher quality (but slower) than `xoroshiro++`.
// (Unsigned)
constexpr __forceinline int64_t norm_gen_rand64HQ(RNGstate& state) {
    return std::bit_cast<int64_t>(norm_gen_urand64HQ(state));
}

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro++`, a general purpose pseudoRNG algorithm.
// (Signed)
constexpr inline int64_t norm_gen_randint(RNGstate& state, int64_t _min, int64_t _max) {
    if (_min == _max) return _min;
    if (_min > _max) throw std::logic_error("Invalid argument(s). Arg min cannot be greater than arg max for RNG range specification.");
    const uint64_t range = static_cast<uint64_t>(_max) - static_cast<uint64_t>(_min) + 1;
    const uint64_t threshold = (-range) % range;
    const uint64_t offset = rejectionSample(norm_gen_rand64(state), range, threshold);
    return std::bit_cast<int64_t>(static_cast<uint64_t>(_min) + offset);
}

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro**`, which is higher quality than `norm_gen_randint`'s `xoroshiro++`.
// (Signed)
constexpr inline int64_t norm_gen_randintHQ(RNGstate& state, int64_t _min, int64_t _max) {
    if (_min == _max) return _min;
    if (_min > _max) throw std::logic_error("Invalid argument(s). Arg min cannot be greater than arg max for RNG range specification.");
    const uint64_t range = static_cast<uint64_t>(_max) - static_cast<uint64_t>(_min) + 1;
    const uint64_t threshold = (-range) % range;
    const uint64_t offset = rejectionSample(norm_gen_rand64HQ(state), range, threshold);
    return std::bit_cast<int64_t>(static_cast<uint64_t>(_min) + offset);
}

#ifdef __cplusplus
}
#endif