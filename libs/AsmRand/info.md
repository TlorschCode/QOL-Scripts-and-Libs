# AsmRand
## Purpose
An assembly library allowing developers to get truly random/pseudorandom numbers.

## Notes
<p>The assembly code is callable through C++ <small>(see Linking with C++)</small>, but it is slower than the identically-functioning C++ code in this repository's BetterRand directory. In this case, the compiler can optimize the C++ code better than even the most efficient handwritten assembly. When compiled, especially with -O3, the C++ code runs ~1.5 to ~3x faster.</p>

## Algorithms
The library uses two different algorithms for generating pseudorandom numbers:
- xoroshiro++, and
- xoroshiro**

Both have $2^{128}$ periods, which is much smaller than the Mersenne Twister's period of $2^{19937}$. However, the xoroshiro algorithms are much faster <small>(see <b>Benchmarks</b> for more details)</small> and have been shown to pass some randomness tests that the Mersenne Twister has failed.

xoroshiro** has been shown to be better quality than xoroshiro++. However, not by a great deal. In addition, it is slower. That is why both options have been provided.
`gen_rand64()` and `gen_urandint()` use xoroshiro++, while `gen_rand64HQ()` and `gen_urandintHQ()` use xoroshiro**.

`seed_state()` uses

## Benchmarks
*Note*: Using `-03 -march=native` in the build command.
Benchmarking my assembly implementation against
mt19937_64 gen_rand64: 45.0192 ms, 4.50 ns/call, checksum=1398791585773173423
mt19937_64 gen_urandint(0,100): 33.63 ms, 3.36 ns/call, checksum=500094014
mt19937_64 gen_urandint(0,1_000_000_123): 26.74 ms, 2.67 ns/call, checksum=5000940690945197

std::rand() gen_rand64: 334.91 ms, 33.49 ns/call, checksum=128850078066084192
std::rand() gen_urandint(0,100): 386.97 ms, 38.70 ns/call, checksum=0
std::rand() gen_urandint(0,1_000_000_123): 283.18 ms, 28.32 ns/call, checksum=2499939

asm gen_rand64: 39.11 ms, 3.91 ns/call, checksum=6345923905729430896
asm gen_urandint(0,100): 151.17 ms, 15.12 ns/call, checksum=500044269
asm gen_urandint(0,1_000_000_123): 141.87 ms, 14.19 ns/call, checksum=5000713979572564

## Linking with C++
The assembly code can be linked with C++ using a build commmand similar to this:
```powershell
nasm -f win64 src/lib/asm_rand.asm -o asm_rand.obj
g++ -std=c++20 -O3 -march=native -Isrc/lib ^
    src/lib/asm_rand.cpp ^
    src/main.cpp ^
    asm_rand.obj ^
    -o main.exe
```

(See asm_rand.h for more usage details).
