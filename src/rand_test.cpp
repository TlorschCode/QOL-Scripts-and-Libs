// validate_rand.cpp
//
// Build:
//   g++ -O3 -march=native -std=c++20 validate_rand.cpp -o validate_rand
//
// Run:
//   ./validate_rand [samples]
//   default samples: 10,000,000
//
// Tests performed
// ---------------
//   1. AVERAGE SPEED (unbounded + bounded)
//      Each generator is timed over SPEED_TRIALS independent runs.
//      Unbounded (raw) and bounded (rejection-sampled) are timed separately.
//      Mean, min, and max ns/call are reported for each.
//
//   2. CHI-SQUARED UNIFORMITY TEST
//      Draws `samples` values into BINS buckets and computes χ² against
//      a perfectly flat expected distribution.
//      - Unsigned generators draw from [0, BINS).
//      - Signed generators draw from [-BINS/2, BINS/2), which specifically
//        exercises the cross-sign-boundary path of the bounded functions.
//      The p-value is estimated via the Wilson-Hilferty normal approximation.
//
//   3. MEAN / VARIANCE TEST
//      Draws raw unbounded values and checks sample statistics against the
//      theoretical values for a uniform distribution over the full 64-bit range.
//
//      For UNSIGNED generators — U[0, 2^64):
//        theoretical mean     = 2^63
//        theoretical variance = (2^128 - 1) / 12
//
//      For SIGNED generators — U[-2^63, 2^63-1]:
//        theoretical mean     = -0.5
//        theoretical variance = (2^128 - 1) / 12  (identical; same spread of bits)
//
//      Because the signed theoretical mean (-0.5) is close to zero, a relative
//      error metric is meaningless for it (it would be astronomically large for
//      any finite sample size even for a perfectly correct generator).
//      Both mean tests therefore use a z-score instead:
//
//        z = (sample_mean − μ) / (σ / √n)
//
//      This is N(0,1) under H₀ and normalises correctly regardless of whether
//      μ is large (2^63) or small (-0.5). Variance uses relative error, which
//      works fine because the theoretical variance is a large number far from 0.
//
// Requires "asm_rand.h" and a C++ namespace "cppimp" (e.g. in "better_rand.h")
// exposing the following functions with matching signatures:
//
//   ASM (global C linkage):
//     uint64_t gen_urand64    (RNGstate&)
//     int64_t  gen_rand64     (RNGstate&)
//     uint64_t gen_urand64HQ  (RNGstate&)
//     int64_t  gen_rand64HQ   (RNGstate&)
//     uint64_t gen_urandint   (RNGstate&, uint64_t min, uint64_t max)
//     int64_t  gen_randint    (RNGstate&, int64_t  min, int64_t  max)
//     uint64_t gen_urandintHQ (RNGstate&, uint64_t min, uint64_t max)
//     int64_t  gen_randintHQ  (RNGstate&, int64_t  min, int64_t  max)
//     void     seed_state     (RNGstate&)
//
//   C++ (norm_):
//     — identical signatures —

#include <bits/stdc++.h>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "asm_rand.h"
#include "better_rand.h"

using hr_clock    = std::chrono::high_resolution_clock;
using ms_duration = std::chrono::duration<double, std::milli>;

// ============================================================
// Constants
// ============================================================
static constexpr uint64_t BINS         = 256;
static constexpr int64_t  CHI2_LO      = -(int64_t)(BINS / 2);      // -128
static constexpr int64_t  CHI2_HI      =  (int64_t)(BINS / 2) - 1;  //  127
static constexpr int      SPEED_TRIALS = 3;

// ============================================================
// Generator descriptor
//
// raw_u      — always populated; returns raw bits as uint64_t.
//              For unsigned generators: direct output.
//              For signed generators:   std::bit_cast<uint64_t> of the output.
//              Used for the unbounded speed test.
//
// raw_s      — populated only when is_signed; returns int64_t directly.
//              Used in the mean/variance test for signed generators.
//
// chi2_draw  — always returns a value in [0, BINS) for bin counting.
//              Unsigned: calls gen_urandint(0, BINS-1).
//              Signed:   calls gen_randint(CHI2_LO, CHI2_HI) offset to [0, BINS).
//              This ensures the test exercises each generator's actual bounded
//              function, including the cross-sign-boundary path for signed ones.
//              Also used for the bounded speed test.
//
// is_signed  — selects which raw function and theoretical mean to use in test 3.
// ============================================================
struct Generator {
    std::string                             name;
    bool                                    is_signed;
    std::function<uint64_t()>               raw_u;
    std::function<int64_t()>                raw_s;       // only valid when is_signed
    std::function<uint64_t()>               chi2_draw;   // always returns [0, BINS)
    std::function<void()>                   reseed;
};

// ============================================================
// Build generator list
// ============================================================
std::vector<Generator> make_generators(RNGstate& state) {
    // All generators share one RNGstate and are reseeded with the same
    // (ASM) seed_state before each trial to keep conditions identical.
    auto rs = [&]() { seed_state(state); };

    return {
        // ---- ASM xoroshiro++ ----
        {
            "ASM xoroshiro++  uint",
            false,
            [&]() -> uint64_t { return gen_urand64(state); },
            {},
            [&]() -> uint64_t { return gen_urandint(state, 0ULL, BINS - 1); },
            rs
        },
        {
            "ASM xoroshiro++  int",
            true,
            [&]() -> uint64_t { return std::bit_cast<uint64_t>(gen_rand64(state)); },
            [&]() -> int64_t  { return gen_rand64(state); },
            [&]() -> uint64_t {
                // [-128, 127] → offset to [0, 255]; tests cross-sign bounded path.
                return static_cast<uint64_t>(gen_randint(state, CHI2_LO, CHI2_HI) - CHI2_LO);
            },
            rs
        },

        // ---- ASM xoroshiro** ----
        {
            "ASM xoroshiro**  uint",
            false,
            [&]() -> uint64_t { return gen_urand64HQ(state); },
            {},
            [&]() -> uint64_t { return gen_urandintHQ(state, 0ULL, BINS - 1); },
            rs
        },
        {
            "ASM xoroshiro**  int",
            true,
            [&]() -> uint64_t { return std::bit_cast<uint64_t>(gen_rand64HQ(state)); },
            [&]() -> int64_t  { return gen_rand64HQ(state); },
            [&]() -> uint64_t {
                return static_cast<uint64_t>(gen_randintHQ(state, CHI2_LO, CHI2_HI) - CHI2_LO);
            },
            rs
        },

        // ---- C++ xoroshiro++ ----
        {
            "C++ xoroshiro++  uint",
            false,
            [&]() -> uint64_t { return norm_gen_urand64(state); },
            {},
            [&]() -> uint64_t { return norm_gen_urandint(state, 0ULL, BINS - 1); },
            rs
        },
        {
            "C++ xoroshiro++  int",
            true,
            [&]() -> uint64_t { return std::bit_cast<uint64_t>(norm_gen_rand64(state)); },
            [&]() -> int64_t  { return norm_gen_rand64(state); },
            [&]() -> uint64_t {
                return static_cast<uint64_t>(norm_gen_randint(state, CHI2_LO, CHI2_HI) - CHI2_LO);
            },
            rs
        },

        // ---- C++ xoroshiro** ----
        {
            "C++ xoroshiro**  uint",
            false,
            [&]() -> uint64_t { return norm_gen_urand64HQ(state); },
            {},
            [&]() -> uint64_t { return norm_gen_urandintHQ(state, 0ULL, BINS - 1); },
            rs
        },
        {
            "C++ xoroshiro**  int",
            true,
            [&]() -> uint64_t { return std::bit_cast<uint64_t>(norm_gen_rand64HQ(state)); },
            [&]() -> int64_t  { return norm_gen_rand64HQ(state); },
            [&]() -> uint64_t {
                return static_cast<uint64_t>(norm_gen_randintHQ(state, CHI2_LO, CHI2_HI) - CHI2_LO);
            },
            rs
        },
    };
}

// ============================================================
// Formatting
// ============================================================
void print_section(const std::string& title) {
    std::cout << "\n" << title << "\n" << std::string(title.size(), '=') << "\n";
}

// ============================================================
// 1. AVERAGE SPEED
//
// 1a. Unbounded — raw_u for all generators.
//     Since gen_rand64 and gen_urand64 are aliases to the same machine code,
//     their numbers will be identical; this is expected and confirms it.
//
// 1b. Bounded — chi2_draw for all generators.
//     Exercises the full rejection-sampling loop via each generator's own
//     bounded function (gen_urandint / gen_randint / HQ variants).
// ============================================================
static void run_speed_half(
    std::vector<Generator>&                    gens,
    uint64_t                                   samples,
    const std::string&                         label,
    std::function<uint64_t(Generator&)>        call)
{
    constexpr int W1 = 26, W2 = 13;

    std::cout << "\n  " << label << "\n"
              << "  " << std::string(label.size(), '-') << "\n";
    std::cout << "  " << std::left  << std::setw(W1) << "Generator"
              << std::right
              << std::setw(W2) << "Mean ns/call"
              << std::setw(W2) << "Min ns/call"
              << std::setw(W2) << "Max ns/call"
              << "\n  " << std::string(W1 + W2 * 3, '-') << "\n";

    for (auto& g : gens) {
        std::vector<double> times;
        times.reserve(SPEED_TRIALS);

        for (int t = 0; t < SPEED_TRIALS; ++t) {
            g.reseed();
            uint64_t sink = 0;
            auto t0 = hr_clock::now();
            for (uint64_t i = 0; i < samples; ++i)
                sink += call(g);
            auto t1 = hr_clock::now();
            asm volatile("" : "+r"(sink));

            const double elapsed_ms =
                std::chrono::duration_cast<ms_duration>(t1 - t0).count();
            times.push_back((elapsed_ms * 1.0e6) / static_cast<double>(samples));
        }

        double sum = 0.0, mn = times[0], mx = times[0];
        for (double v : times) { sum += v; mn = std::min(mn, v); mx = std::max(mx, v); }

        std::cout << "  " << std::left  << std::setw(W1) << g.name
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(W2) << (sum / SPEED_TRIALS)
                  << std::setw(W2) << mn
                  << std::setw(W2) << mx
                  << "\n";
    }
}

void bench_average_speed(std::vector<Generator>& gens, uint64_t samples) {
    print_section("1. Average Speed  (" + std::to_string(SPEED_TRIALS)
                  + " runs × " + std::to_string(samples) + " samples)");

    run_speed_half(gens, samples,
        "1a. Unbounded (raw 64-bit output)",
        [](Generator& g) -> uint64_t { return g.raw_u(); });

    run_speed_half(gens, samples,
        "1b. Bounded (rejection-sampled, " + std::to_string(BINS) + " bins)",
        [](Generator& g) -> uint64_t { return g.chi2_draw(); });
}

// ============================================================
// 2. CHI-SQUARED UNIFORMITY TEST
// ============================================================
static double chi2_p_upper(double x, double df) {
    const double z = (std::pow(x / df, 1.0 / 3.0) - (1.0 - 2.0 / (9.0 * df)))
                   / std::sqrt(2.0 / (9.0 * df));
    return 0.5 * std::erfc(z / std::sqrt(2.0));
}

void test_chi_squared(std::vector<Generator>& gens, uint64_t samples) {
    print_section("2. Chi-Squared Uniformity Test  (bins=" + std::to_string(BINS)
                  + ", samples=" + std::to_string(samples) + ")");

    std::cout << "  Unsigned: draws from [0, "    << BINS - 1 << "] via gen_urandint.\n"
              << "  Signed:   draws from ["       << CHI2_LO  << ", " << CHI2_HI
              << "] via gen_randint (cross-sign boundary), offset to [0, "
              << BINS - 1 << "] for counting.\n\n";

    constexpr double WARN_P  = 0.01;
    constexpr double FAIL_P  = 0.001;
    const double     expected = static_cast<double>(samples) / static_cast<double>(BINS);

    std::cout << "  " << std::left  << std::setw(26) << "Generator"
              << std::right
              << std::setw(16) << "χ² statistic"
              << std::setw(12) << "p-value"
              << std::setw(8)  << "Result"
              << "\n  " << std::string(64, '-') << "\n";

    std::vector<uint64_t> counts(BINS);

    for (auto& g : gens) {
        g.reseed();
        std::fill(counts.begin(), counts.end(), 0ULL);
        bool out_of_range = false;

        for (uint64_t i = 0; i < samples; ++i) {
            const uint64_t bin = g.chi2_draw();
            if (__builtin_expect(bin >= BINS, 0)) {
                // A buggy generator could produce a value outside [0, BINS).
                // Report it rather than silently indexing out of bounds.
                if (!out_of_range) {
                    std::cerr << "\n  ERROR: " << g.name
                              << " produced out-of-range bin " << bin
                              << " — check bounded function implementation.\n";
                    out_of_range = true;
                }
                continue;
            }
            ++counts[bin];
        }

        double chi2 = 0.0;
        for (uint64_t c : counts) {
            const double d = static_cast<double>(c) - expected;
            chi2 += (d * d) / expected;
        }

        const double df   = static_cast<double>(BINS - 1);
        const double pval = chi2_p_upper(chi2, df);

        const char* verdict =
            (pval < FAIL_P) ? "FAIL" :
            (pval < WARN_P) ? "WARN" : "PASS";

        std::cout << "  " << std::left  << std::setw(26) << g.name
                  << std::right << std::fixed
                  << std::setw(16) << std::setprecision(2) << chi2
                  << std::setw(12) << std::setprecision(4) << pval
                  << std::setw(8)  << verdict
                  << "\n";
    }

    std::cout << "\n  Thresholds: WARN p < " << WARN_P << "  FAIL p < " << FAIL_P << "\n"
              << "  ~1 % of runs will produce a WARN by chance from a correct generator.\n"
              << "  A consistent FAIL across repeated runs is statistically significant.\n";
}

// ============================================================
// 3. MEAN / VARIANCE TEST
// ============================================================
void test_mean_variance(std::vector<Generator>& gens, uint64_t samples) {
    print_section("3. Mean / Variance Test  (samples=" + std::to_string(samples) + ")");

    // ---- Theoretical values ----
    constexpr long double U_MEAN = static_cast<long double>(1ULL << 63); // 2^63
    constexpr long double S_MEAN = -0.5L;  // (INT64_MIN + INT64_MAX) / 2

    // Variance = (N^2 - 1) / 12  for N = 2^64  →  (2^128 - 1) / 12
    // Computed exactly in __uint128_t; long double has a 64-bit mantissa on
    // x86 so it represents this ~2^125 value without meaningful rounding error.
    const __uint128_t u64max         = static_cast<__uint128_t>(UINT64_MAX);
    const __uint128_t var128         = (u64max * (u64max + 2)) / 12;
    const long double THEORETICAL_VAR = static_cast<long double>(var128);
    const long double SIGMA           = sqrtl(THEORETICAL_VAR);

    // Standard error of the sample mean and expected variance relative error
    const long double n_ld      = static_cast<long double>(samples);
    const long double SE_MEAN   = SIGMA / sqrtl(n_ld);
    const double EXPECTED_V_ERR = std::sqrt(2.0 / static_cast<double>(samples - 1));

    // Thresholds
    constexpr double WARN_Z       = 3.0;   // ~0.3 % false-positive rate
    constexpr double FAIL_Z       = 5.0;   // ~0.00006 % false-positive rate
    constexpr double WARN_VAR_REL = 0.01;  // 1 %
    constexpr double FAIL_VAR_REL = 0.05;  // 5 %

    std::cout << "  Theoretical σ    ≈ " << std::scientific << std::setprecision(4)
              << static_cast<double>(SIGMA) << "\n"
              << "  SE(x̄) at n=" << samples << " ≈ "
              << static_cast<double>(SE_MEAN) << "\n"
              << "  Expected var rel-err ≈ "
              << std::fixed << std::setprecision(4) << (EXPECTED_V_ERR * 100.0) << " %\n\n"
              << "  Unsigned theoretical mean = 2^63 ≈ "
              << std::scientific << static_cast<double>(U_MEAN) << "\n"
              << "  Signed   theoretical mean = " << std::fixed << std::setprecision(1)
              << static_cast<double>(S_MEAN) << "\n\n";

    std::cout << "  " << std::left  << std::setw(26) << "Generator"
              << std::right
              << std::setw(10) << "Mean z"
              << std::setw(16) << "Var rel-err"
              << std::setw(8)  << "Result"
              << "\n  " << std::string(62, '-') << "\n";

    for (auto& g : gens) {
        g.reseed();

        // Welford's online algorithm accumulates in long double to minimise
        // floating-point error over the full sample run.
        long double m1 = 0.0L;  // running mean
        long double m2 = 0.0L;  // running sum of squared deviations (→ variance)

        if (g.is_signed) {
            for (uint64_t i = 0; i < samples; ++i) {
                const long double x  = static_cast<long double>(g.raw_s());
                const long double d1 = x - m1;
                m1 += d1 / static_cast<long double>(i + 1);
                m2 += d1 * (x - m1);
            }
        } else {
            for (uint64_t i = 0; i < samples; ++i) {
                const long double x  = static_cast<long double>(g.raw_u());
                const long double d1 = x - m1;
                m1 += d1 / static_cast<long double>(i + 1);
                m2 += d1 * (x - m1);
            }
        }

        const long double sample_var  = m2 / static_cast<long double>(samples - 1);
        const long double theory_mean = g.is_signed ? S_MEAN : U_MEAN;

        // z-score: correct for both signed (μ = -0.5) and unsigned (μ = 2^63)
        // since we divide by SE_MEAN rather than by μ itself.
        const double z_mean = static_cast<double>((m1 - theory_mean) / SE_MEAN);

        // Relative error: safe here because THEORETICAL_VAR >> 0.
        const double var_rel = static_cast<double>(
            fabsl((sample_var - THEORETICAL_VAR) / THEORETICAL_VAR));

        const bool warn = std::abs(z_mean) > WARN_Z || var_rel > WARN_VAR_REL;
        const bool fail = std::abs(z_mean) > FAIL_Z || var_rel > FAIL_VAR_REL;

        const char* verdict = fail ? "FAIL" : warn ? "WARN" : "PASS";

        std::cout << "  " << std::left  << std::setw(26) << g.name
                  << std::right << std::fixed
                  << std::setw(10) << std::setprecision(3) << z_mean
                  << std::setw(14) << std::setprecision(4) << (var_rel * 100.0) << " %"
                  << std::setw(8)  << verdict
                  << "\n";
    }

    std::cout << "\n  Mean:  z ~ N(0,1) under H₀.  WARN |z| > " << WARN_Z
              << "  FAIL |z| > " << FAIL_Z << "\n"
              << "  Var:   WARN rel-err > " << (WARN_VAR_REL * 100.0)
              << " %  FAIL rel-err > " << (FAIL_VAR_REL * 100.0) << " %\n"
              << "  Note: z-score is used for the mean because the signed theoretical\n"
              << "  mean is −0.5, making relative error meaningless (it would be ~10^15\n"
              << "  even for a perfect generator at this sample size).\n";
}

// ============================================================
int main(int argc, char** argv) {
    RNGstate state_;
    seed_state(state_);
    std::cout << "Test urand\n";
    std::cout << gen_urandint(state_, 0, 100) << "\n";
    std::cout << "Test rand\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << gen_randint(state_, -100, 100) << "\n";
    std::cout << "Test urandHQ\n";
    std::cout << gen_urandintHQ(state_, 0, 100) << "\n";
    std::cout << "Test randHQ\n";
    std::cout << gen_randintHQ(state_, 0, 100) << "\n";
    
    uint64_t samples = 10'000'000ULL;
    if (argc >= 2) {
        samples = std::stoull(argv[1]);
        if (samples < 10'000) {
            std::cerr << "samples must be >= 10000\n";
            return 1;
        }
    }

    std::cout << "validate_rand — samples: " << samples << "\n"
              << "Build flags:  -O3 -march=native -std=c++20\n"
              << "NOTE: entropy-based seeding — results vary between runs.\n";

    RNGstate state;
    auto gens = make_generators(state);

    bench_average_speed(gens, samples);
    test_chi_squared(gens, samples);
    test_mean_variance(gens, samples);

    std::cout << "\nDone.\n";
    return 0;
}