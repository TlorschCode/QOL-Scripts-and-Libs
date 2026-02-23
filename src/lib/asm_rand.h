#pragma once
#include <stdint.h>
#include <cstddef>
#include <stdexcept>
#include <bit>
#include <concepts>
#include <immintrin.h>

struct RNGstate {
    uint64_t state0;
    uint64_t state1;
};

#ifdef __cplusplus
extern "C" {
#endif

//? functions defined in asm

using ui64 = uint64_t;
using ui32 = uint32_t;
using ui16 = uint16_t;

//|MARK: Gen Normal

constexpr __forceinline ui64 norm_gen_rand64(RNGstate& state) {
    const ui64 output = std::rotl<ui64>(state.state0 + state.state1, 17) + state.state0;
    state.state1 ^= state.state0;
    state.state0 = std::rotl<ui64>(state.state0, 49) ^ state.state1 ^ (state.state1 << 21);
    state.state1 = std::rotl<ui64>(state.state1, 28);
    return output;
}

constexpr __forceinline ui64 norm_gen_rand64HQ(RNGstate& state) {
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

constexpr inline ui64 norm_gen_urandint(RNGstate& state, ui64 _min, ui64 _max) {
    if (_min == _max) return _min;
    const ui64 range = _max - _min + 1;
    const ui64 threshold = (-range) % range;
    return rejectionSample(norm_gen_rand64(state), range, threshold);
}

constexpr inline ui64 norm_gen_urandintHQ(RNGstate& state, ui64 _min, ui64 _max) {
    if (_min == _max) return _min;
    const ui64 range = _max - _min + 1;
    const ui64 threshold = (-range) % range;
    return rejectionSample(norm_gen_rand64HQ(state), range, threshold);
}

__forceinline ui64 norm_gen_seed64() {
    ui64 result;
    bool success;
    while (!(success = _rdseed64_step(&result)));
    return result;
}

__forceinline ui32 norm_gen_seed32() {
    ui32 result;
    bool success;
    while (!(success = _rdseed32_step(&result)));
    return result;
}

__forceinline ui16 norm_gen_seed16() {
    ui16 result;
    bool success;
    while (!(success = _rdseed16_step(&result)));
    return result;
}

constexpr inline ui64 norm_gen_seed(uint64_t _min, uint64_t _max) {
    if (_min == _max) return _min;
    const ui64 range = _max - _min + 1;
    const ui64 threshold = (-range) % range;
    return rejectionSample(norm_gen_seed64(), range, threshold);
}

// Generates a biasless, truly random number using the `RDSEED` assembly instruction.
//
// It is recommended to use this to seed an RNG rather than act as a standalone one, since it is ~500x slower than `gen_randint`/`gen_randint_fast` (which are pseudoRNG's).
extern inline uint64_t gen_seed(uint64_t min, uint64_t max);

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro++`, a general purpose pseudoRNG algorithm.
extern inline uint64_t gen_urandint(RNGstate& state, uint64_t min, uint64_t max);

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro**`, which is higher quality than `gen_randint`'s `xoroshiro++`.
// Is ~15% slower than `gen_randint`.
extern inline uint64_t gen_urandintHQ(RNGstate& state, uint64_t min, uint64_t max);


//| MARK: Gen 64

// Generates a biasless, truly random number from 0 to 2^64 (inclusive) using assembly's `RDSEED` instruction.
extern inline uint64_t gen_seed64();

// Generates a high-quality, biasless random number from 0 to 2^64 (inclusive).
// Uses a `xoroshiro++`, a general-purpose pseudoRNG.
extern inline uint64_t gen_rand64(RNGstate& state);

// Generates a high-quality, biasless random number from 0 to 2^64 (inclusive).
// Uses a higher-quality pseudoRNG (`xoroshiro**`) than `gen_rand64`, which uses `xoroshiro++`.
// Is ~15% slower than `gen_rand64`.
extern inline uint64_t gen_rand64HQ(RNGstate& state);



//|MARK: Seed State

// Seeds an RNGstate struct using a true random number generator (`RDSEED`) paired with `splitmix64`.
void seed_state(RNGstate& state);


#ifdef __cplusplus
}
#endif