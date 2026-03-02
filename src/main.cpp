// benchmark_rand.cpp
//
// Build:
//   g++ -O3 -march=native -std=c++20 benchmark_rand.cpp -o benchmark_rand
//
// Run:
//   ./benchmark_rand [iterations]
//   default iterations: 10,000,000
//
// Requires "asm_rand.h" exposing:
//   RNGstate
//   void     seed_state(RNGstate&)
//   uint64_t gen_rand64(RNGstate&)
//   uint64_t gen_urandint(RNGstate&, uint64_t min, uint64_t max)
//
// Fairness guarantees
// -------------------
//   1. All bounded tests use the same Lemire rejection-sampling algorithm.
//      MT and std::rand go through the lemire_bounded<> template below.
//      The ASM generator uses its own built-in gen_urandint, which the header
//      guarantees already implements Lemire — so both sides run the same math.
//
//   2. lemire_bounded<> is [[gnu::always_inline]] and takes its next() callback
//      as a template parameter, so the generator call is always inlined at the
//      instantiation site. No indirect call overhead for any generator.
//
//   3. The accumulator sink is a plain uint64_t, NOT volatile. Declaring it
//      volatile would force a memory store on every iteration, which would
//      artificially inflate (and equalise) timings — especially harmful for fast
//      generators. Instead, an asm volatile("" : "+r"(sink)) fence after the
//      loop tells the compiler sink may escape, preventing dead-code elimination
//      without injecting any per-iteration memory traffic.
//
//   4. __forceinline is an MSVC extension and does not exist on GCC. All
//      force-inline annotations use [[gnu::always_inline]] throughout.
//
//   5. c_gen64() computes RAND_MAX's bit-width as a constexpr, accumulates into
//      unsigned __int128 to avoid shift overflow, then takes the low 64 bits.
//      No dead code, no hard-coded "31".
//
//   6. All generators are re-seeded from the same constant before each sub-test
//      so every test begins from a fresh state. The ASM seed_state() seeds from
//      entropy and cannot accept an explicit value; its checksums will therefore
//      vary between runs, which is expected and noted.

#include <bits/stdc++.h>
#include <chrono>
#include <random>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include "better_rand.h"
#include "asm_rand.h"

using hr_clock    = std::chrono::high_resolution_clock;
using ms_duration = std::chrono::duration<double, std::milli>;

// ============================================================
// Lemire's rejection sampling — shared by every generator.
//
// Produces a uniform result in [0, bound-1] and returns it.
// The caller adds min after the fact.
//
// Template parameter Next must be a zero-capture or reference-capture
// callable returning uint64_t; it is inlined into every instantiation
// because the function itself is [[gnu::always_inline]].
//
// If bound == 0, interprets the request as the full 2^64 range and
// simply returns one raw 64-bit value.
// ============================================================
template <typename Next>
[[gnu::always_inline]] inline uint64_t lemire_bounded(Next&& next, uint64_t bound) {
    if (__builtin_expect(bound == 0, 0))
        return next();

    // threshold = (-bound) % bound  avoids any initial rejection bias.
    const uint64_t threshold = static_cast<uint64_t>(-bound) % bound;

    for (;;) {
        const __uint128_t prod = static_cast<__uint128_t>(next())
                               * static_cast<__uint128_t>(bound);
        const uint64_t low  = static_cast<uint64_t>(prod);         // low 64 bits
        const uint64_t high = static_cast<uint64_t>(prod >> 64);   // result in [0, bound)
        if (__builtin_expect(low >= threshold, 1))
            return high;
        // Rejection: loop and try again. Expected trips < 2.
    }
}

// ============================================================
// std::mt19937_64
// ============================================================
struct MTState {
    std::mt19937_64 engine;
    void seed(uint64_t s) { engine.seed(s); }
};

[[gnu::always_inline]] inline uint64_t mt_gen64(MTState& s) {
    return s.engine();
}

[[gnu::always_inline]] inline uint64_t mt_gen_bounded(MTState& s,
                                                       uint64_t min,
                                                       uint64_t max) {
    const uint64_t bound = max - min + 1;
    // Lambda is inlined into lemire_bounded<> by the template instantiation.
    return min + lemire_bounded([&s]() -> uint64_t { return s.engine(); }, bound);
}

// ============================================================
// std::rand()
//
// RAND_MAX is at least 32767 (2^15 - 1) by the C standard.
// On virtually all modern platforms it is 2^31 - 1 (31 bits).
// We compute the usable bit-count at compile time and call rand()
// exactly ceil(64 / RAND_BITS) times per 64-bit result, accumulating
// into unsigned __int128 to avoid any shift overflow.
// ============================================================
static constexpr int RAND_BITS = []() constexpr -> int {
    int b = 0;
    unsigned long long rm = static_cast<unsigned long long>(RAND_MAX) + 1ULL;
    while (rm > 1ULL) { rm >>= 1; ++b; }
    return b;
}();
static constexpr int RAND_CALLS_FOR_64 = (64 + RAND_BITS - 1) / RAND_BITS;

static_assert(RAND_BITS >= 15, "Implausibly small RAND_MAX");

[[gnu::always_inline]] inline uint64_t c_gen64() {
    unsigned __int128 acc = 0;
    for (int i = 0; i < RAND_CALLS_FOR_64; ++i)
        acc = (acc << RAND_BITS) | static_cast<unsigned long long>(std::rand());
    // We have RAND_CALLS_FOR_64 * RAND_BITS >= 64 bits accumulated.
    // The low 64 bits are uniform over [0, 2^64).
    return static_cast<uint64_t>(acc);
}

[[gnu::always_inline]] inline uint64_t c_gen_bounded(uint64_t min, uint64_t max) {
    const uint64_t bound = max - min + 1;
    return min + lemire_bounded([]() -> uint64_t { return c_gen64(); }, bound);
}

// ============================================================
// Benchmark harness
// ============================================================
struct BenchResult {
    double   ms;
    uint64_t iterations;
    uint64_t checksum; // accumulated to prevent the loop being eliminated
};

template <typename Fn>
BenchResult run_bench(Fn&& f, uint64_t iterations) {
    uint64_t sink = 0; // plain, NOT volatile — see fairness note 3 in header

    auto t0 = hr_clock::now();
    for (uint64_t i = 0; i < iterations; ++i)
        sink += f(i);
    auto t1 = hr_clock::now();

    // Tell the compiler sink may escape so the loop cannot be removed,
    // without generating a memory store on every iteration.
    asm volatile("" : "+r"(sink));

    return {
        std::chrono::duration_cast<ms_duration>(t1 - t0).count(),
        iterations,
        sink
    };
}

inline double ns_per_call(const BenchResult& r) {
    return (r.ms * 1.0e6) / static_cast<double>(r.iterations);
}

void print_header() {
    std::cout << std::left  << std::setw(52) << "Test"
              << std::right << std::setw(12) << "Total (ms)"
              << std::setw(14) << "ns / call"
              << "  checksum\n"
              << std::string(90, '-') << "\n";
}

void print_result(const std::string& label, const BenchResult& r) {
    std::cout << std::left  << std::setw(52) << label
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << r.ms
              << std::setw(13) << std::setprecision(3) << ns_per_call(r) << " ns"
              << "  " << r.checksum << "\n";
}

void print_section(const std::string& title) {
    std::cout << "\n" << title << "\n" << std::string(title.size(), '~') << "\n";
}

// ============================================================
int main(int argc, char** argv) {
    uint64_t iterations = 10'000'000ULL;
    if (argc >= 2) {
        iterations = std::stoull(argv[1]);
        if (iterations == 0) iterations = 1;
    }

    std::cout << "benchmark_rand — iterations: " << iterations << "\n";
    std::cout << "Build flags:  -O3 -march=native -std=c++20\n";
    std::cout << "RAND_MAX:     " << RAND_MAX
              << "  (" << RAND_BITS << " usable bits, "
              << RAND_CALLS_FOR_64 << " call(s) per uint64)\n";
    std::cout << "NOTE: ASM checksums vary between runs (entropy-based seed).\n";

    // Deterministic seed for the reproducible generators.
    constexpr uint64_t SEED = 0xDEADBEEFCAFEBABEULL;

    print_header();

    // ----------------------------------------------------------
    // std::mt19937_64
    // ----------------------------------------------------------
    print_section("std::mt19937_64");
    MTState mt;

    mt.seed(SEED);
    print_result("mt19937_64  gen_rand64",
        run_bench([&](uint64_t) { return mt_gen64(mt); }, iterations));

    mt.seed(SEED);
    print_result("mt19937_64  gen_urandint(0, 100)",
        run_bench([&](uint64_t) { return mt_gen_bounded(mt, 0ULL, 100ULL); }, iterations));

    mt.seed(SEED);
    print_result("mt19937_64  gen_urandint(0, 1'000'000'123)",
        run_bench([&](uint64_t) { return mt_gen_bounded(mt, 0ULL, 1'000'000'123ULL); }, iterations));

    // ----------------------------------------------------------
    // std::rand
    // ----------------------------------------------------------
    print_section("std::rand");

    std::srand(static_cast<unsigned>(SEED));
    print_result("std::rand   gen_rand64",
        run_bench([](uint64_t) { return c_gen64(); }, iterations));

    std::srand(static_cast<unsigned>(SEED));
    print_result("std::rand   gen_urandint(0, 100)",
        run_bench([](uint64_t) { return c_gen_bounded(0ULL, 100ULL); }, iterations));

    std::srand(static_cast<unsigned>(SEED));
    print_result("std::rand   gen_urandint(0, 1'000'000'123)",
        run_bench([](uint64_t) { return c_gen_bounded(0ULL, 1'000'000'123ULL); }, iterations));

    // ----------------------------------------------------------
    // ASM xoroshiro++ (via asm_rand.h)
    //
    // gen_rand64 and gen_urandint are called directly — no wrapper
    // layer, no additional indirection.  gen_urandint uses the
    // assembly's own built-in Lemire implementation, which is the
    // same algorithm as lemire_bounded<> above.
    //
    // seed_state() seeds from entropy; we call it before each sub-test
    // to give the generator a fresh state, matching what we do for the
    // other two generators.
    // ----------------------------------------------------------
    print_section("ASM xoroshiro++");
    RNGstate as;

    seed_state(as);
    print_result("asm xoroshiro++  gen_rand64",
        run_bench([&](uint64_t) { return gen_rand64(as); }, iterations));

    seed_state(as);
    print_result("asm xoroshiro++  gen_urandint(0, 100)",
        run_bench([&](uint64_t) { return gen_urandint(as, 0ULL, 100ULL); }, iterations));

    seed_state(as);
    print_result("asm xoroshiro++  gen_urandint(0, 1'000'000'123)",
        run_bench([&](uint64_t) { return gen_urandint(as, 0ULL, 1'000'000'123ULL); }, iterations));

    // ----------------------------------------------------------
    // C++ xoroshiro++ (norm_ prefix)
    //
    // Identical algorithm to the ASM xoroshiro++ section but compiled
    // from the C++ implementation in asm_rand.h. Comparing these two
    // sections directly shows the codegen delta between the hand-written
    // assembly and what the compiler produces from the C++ source.
    // ----------------------------------------------------------
    print_section("C++ xoroshiro++ (norm)");

    seed_state(as);
    print_result("c++ xoroshiro++  norm_gen_rand64",
        run_bench([&](uint64_t) { return norm_gen_rand64(as); }, iterations));

    seed_state(as);
    print_result("c++ xoroshiro++  norm_gen_urandint(0, 100)",
        run_bench([&](uint64_t) { return norm_gen_urandint(as, 0ULL, 100ULL); }, iterations));

    seed_state(as);
    print_result("c++ xoroshiro++  norm_gen_urandint(0, 1'000'000'123)",
        run_bench([&](uint64_t) { return norm_gen_urandint(as, 0ULL, 1'000'000'123ULL); }, iterations));

    // ----------------------------------------------------------
    // C++ xoroshiro** (norm_ HQ variant)
    //
    // Identical algorithm to the ASM xoroshiro** section but compiled
    // from C++. Comparing this section against the ASM xoroshiro** block
    // shows the assembly benefit for the stronger mixing step specifically.
    // ----------------------------------------------------------
    print_section("C++ xoroshiro** (norm HQ)");

    seed_state(as);
    print_result("c++ xoroshiro**  norm_gen_rand64HQ",
        run_bench([&](uint64_t) { return norm_gen_rand64HQ(as); }, iterations));

    seed_state(as);
    print_result("c++ xoroshiro**  norm_gen_urandintHQ(0, 100)",
        run_bench([&](uint64_t) { return norm_gen_urandintHQ(as, 0ULL, 100ULL); }, iterations));

    seed_state(as);
    print_result("c++ xoroshiro**  norm_gen_urandintHQ(0, 1'000'000'123)",
        run_bench([&](uint64_t) { return norm_gen_urandintHQ(as, 0ULL, 1'000'000'123ULL); }, iterations));

    // ----------------------------------------------------------
    // ASM xoroshiro** (star-star / HQ variant)
    //
    // Uses gen_rand64HQ, gen_urandintHQ, etc. from asm_rand.h.
    // xoroshiro** has stronger statistical properties than xoroshiro++
    // at the cost of a few extra operations per output; this section
    // measures whether that difference is detectable in practice.
    //
    // Bounded calls go through the header's own gen_urandintHQ, which
    // carries the same built-in Lemire implementation as the ++ variant,
    // keeping the comparison fair.
    // ----------------------------------------------------------
    print_section("ASM xoroshiro** (HQ)");

    seed_state(as);
    print_result("asm xoroshiro**  gen_rand64HQ",
        run_bench([&](uint64_t) { return gen_rand64HQ(as); }, iterations));

    seed_state(as);
    print_result("asm xoroshiro**  gen_urandintHQ(0, 100)",
        run_bench([&](uint64_t) { return gen_urandintHQ(as, 0ULL, 100ULL); }, iterations));

    seed_state(as);
    print_result("asm xoroshiro**  gen_urandintHQ(0, 1'000'000'123)",
        run_bench([&](uint64_t) { return gen_urandintHQ(as, 0ULL, 1'000'000'123ULL); }, iterations));

    std::cout << "\nDone.\n";
    return 0;
}