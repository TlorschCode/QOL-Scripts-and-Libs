#pragma once
#include <stdint.h>
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
// Uses `xoroshiro++`, a general-purpose pseudoRNG algorithm.
// (Unsigned)
extern inline uint64_t gen_urand64(RNGstate& state);

// Generates a biasless random number between 0 and 2^64.
// Uses `xoroshiro++`, a general-purpose pseudoRNG algorithm.
// (Signed)
extern inline int64_t gen_rand64(RNGstate& state);


// Generates a biasless random number between 0 and 2^64.
// Uses `xoroshiro**`, which is higher quality (but slower) than `xoroshiro++`.
// (Unsigned) 
extern inline uint64_t gen_urand64HQ(RNGstate& state);

// Generates a biasless random number between 0 and 2^64.
// Uses `xoroshiro**`, which is higher quality (but slower) than `xoroshiro++`.
// (Unsigned)
extern inline int64_t gen_rand64HQ(RNGstate& state);



// Generates a biasless, truly random number using the `RDSEED` assembly instruction.
//
// It is recommended to use this to seed an RNG rather than act as a standalone one, since it is much slower than `gen_randint`/`gen_randint_fast` (which are pseudoRNG's).
extern inline uint64_t gen_seed(uint64_t _min, uint64_t _max);


// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro++`, a general-purpose pseudoRNG algorithm.
// (Unsigned)
extern inline uint64_t gen_urandint(RNGstate& state, uint64_t _min, uint64_t _max);

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro++`, a general-purpose pseudoRNG algorithm.
// (Signed)
extern inline int64_t gen_randint(RNGstate& state, int64_t _min, int64_t _max);


// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro**`, which is higher quality (but slower) than `gen_randint`'s `xoroshiro++`.
// (Unsigned)
extern inline uint64_t gen_urandintHQ(RNGstate& state, uint64_t _min, uint64_t _max);

// Generates a high-quality, biasless, pseudorandom number from `min` to `max` (inclusive).
// Uses `xoroshiro**`, which is higher quality than `gen_randint`'s `xoroshiro++`.
// (Signed)
extern inline int64_t gen_randintHQ(RNGstate& state, int64_t _min, int64_t _max);



//| MARK: GenSeed

// Generates a biasless, truly random number from 0 to 2^64 (inclusive) using assembly's `RDSEED` instruction.
extern inline uint64_t gen_seed64();
// Generates a biasless, truly random number from 0 to 2^32 (inclusive) using assembly's `RDSEED` instruction.
extern inline uint64_t gen_seed32();
// Generates a biasless, truly random number from 0 to 2^16 (inclusive) using assembly's `RDSEED` instruction.
extern inline uint64_t gen_seed16();


//|MARK: Seed State

// Seeds an RNGstate struct using a true random number generator (`RDSEED`) paired with `splitmix64`.
void seed_state(RNGstate& state);


#ifdef __cplusplus
}
#endif