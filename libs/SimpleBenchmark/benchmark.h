#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdint>
#include <string>
#ifdef _MSC_VER
#include <intrin.h> // for _ReadWriteBarrier()
#endif

struct BenchResult {
    double   ms;
    uint64_t iterations;
    double nsPerCall;
};
using hr_clock = std::chrono::high_resolution_clock;
using ms_duration = std::chrono::duration<double, std::milli>;


static inline double ns_per_call(double ms, uint64_t iterations) {
    return (ms * 1.0e6) / static_cast<double>(iterations);
}


// Benchmarks a function passed into it. Returns a BenchResult
template <typename Fn>
BenchResult benchmark(Fn&& f, uint64_t iterations) {
    uint64_t sink = 0; // plain, NOT volatile — see fairness note 3 in header

    auto timeStart = hr_clock::now();
    for (uint64_t i = 0; i < iterations; ++i) {
        sink += f(i);
    }
    auto timeEnd = hr_clock::now();

    // stop the compiler from optimizing away the loop as "dead code' by tricking it into
    // thinking that the assembly being run (in this case nothing: "") is using sink
    #if defined(__GNUC__) || defined(__clang__) // for gcc and clang
        asm volatile("" : "+r"(sink)); // '+' means that sink is read from and written to. 'r' tells the compiler to store sink in a register
    #elif defined(_MSC_VER) // for msvc
        _ReadWriteBarrier();
    #endif

    double ms = std::chrono::duration_cast<ms_duration>(timeEnd - timeStart).count();
    return {
        ms,
        iterations,
        ns_per_call(ms, iterations)
    };
}


// some printing formatting stuff ever so kindly provided by AI

// Prints 
void print_header() {
    std::cout << std::left  << std::setw(52) << "Test"
        << std::right << std::setw(12) << "Total (ms)"
        << std::setw(14) << "ns / call\n"
        << std::string(90, '-') << "\n";
}

void print_result(const std::string& label, const BenchResult& r, uint8_t decimalPrecision=3) {
    std::cout << std::left  << std::setw(52) << label
        << std::right << std::setw(12) << std::fixed << std::setprecision(2) << r.ms
        << std::setw(13) << std::setprecision(decimalPrecision) << r.nsPerCall << " ns";
}

void print_section(const std::string& title) {
    std::cout << "\n" << title << "\n" << std::string(title.size(), '~') << "\n";
}
