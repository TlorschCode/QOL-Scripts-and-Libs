# Simple Scripts & Libraries
This repository provides these following drop-in C++ and x86 Assembly libraries:
- DoNotOptimize - a tiny C++ utility that you can drop in for microbenchmarks, etc. to prevent the compiler from chopping off what it might decide to be "dead code".
- AsmRand - a fast assembly library for generating truly/pseudo-random numbers
- QuickRand - the C++ version of AsmRand
- SimpleBenchmark - a quick and easy benchmarking util for C++

This repository provides these following executable scripts (along with their source code):
- getFolderStructure - a command line utility that outputs the folder structure of the specified directory down to a certain depth.

---

# DoNotOptimize
## Purpose
> Platform: GCC, Clang, and MSVC C++

A small, quick function that prevents the compiler from optimizing away what it assumes is "dead code". Mainly used for microbenchmarking.

This function does not entirely prevent optimizations, it just prevents the compiler from entirely optimizing something away as "dead code". The compiler can still inline and constant fold code, etc. but the code *must* be executed.

</br>

## Usage
There is only one function in this mini-library:
`void do_not_optimize(T& value)`.
Its usage is highly use-case dependent, but it will likely look similar to this:
```cpp
for (int i = 0; i < 10'000; i++) { // tight loop
    int result = foo();
    do_not_optimize(result); // tells compiler that it can't get rid of result
    //                          or any code that contributes to it because result
    //                          is visible to outside scope.
}
```

---

# AsmRand
> Platform: Intel x86

## Purpose
An extremely fast x86 ASM library allowing developers to get high quality true-random/pseudorandom numbers.

> *Note*: While the code can be linked and compiled with C++, it is recommended to only use this ASM library for projects which require x86 ASM. There is an identically functioning but ~2x (1 ns) faster library written in C++. You can find it in this repo under `libs/QuickRand`.
> It is faster due to the extreme optimizations the compiler can apply to C++ code, some of which are not feasibly possible to implement by hand. However, in some scenarios, if **not** compiled with -O3, this ASM random generator beats the C++ version for spead

</br>

## Usage
- `uint64_t gen_urand64(RNGstate& state)`: Generates a random 64-bit number. It does not accept a range. `state` is modified and progressed to its next state.
- `uint64_t gen_urand64HQ(RNGstate& state)`: Identical in usage to `gen_urand64`. However, the underlying algorithm is higher quality (and slower).
- `int64_t gen_rand64(RNGstate& state)`: Identical to `gen_urandint64`, but it returns a signed number.
- `int64_t gen_rand64HQ(RNGstate& state)` Identical to `gen_rand64`, but it uses a higher quality (and slower) algorithm to generate the random number.
- `uint64_t gen_urandint(RNGstate& state, uint64_t min, uint64_t max)`: Uses `gen_urand64`, but applies Lemire's rejection sampling to conform the result to a range without introducing bias. The result is unsigned.
- `uint64_t gen_urandintHQ(RNGstate& state, uint64_t min, uint64_t max)`: Identical in usage to `gen_urandint`. However, it uses `gen_urand64`, which uses a higher quality but slower algorithm.
- `int64_t gen_randint(RNGstate& state, int64_t min, int64_t max)` Identical to `gen_urandint`, except it can accept a signed minimum and maximum, and it returns a signed number.
- `int64_t gen_randintHQ(RNGstate& state, int64_t min, int64_t max)` Identical to `gen_randint`, except it uses a higher quality algorithm to generate the random number.
- `uint64_t gen_seed64()`, `uint32_t gen_seed32()`, and `uint16_t gen_seed16()`: Return truly random 64-bit, 32-bit, and 16-bit integers (respectively) that are generated from hardware entropy. These functions are much slower than pseudorandom generators.
- `void seed_state(RNGstate& state)`: Uses `gen_seed64` and `splitmix64` to fill the RNGstate passed into it. `state.state1` is set to the direct result of `gen_seed64`, and `state.state0` is set to that result after it is passed through the `splitmix64` function.
> *Note*: The `splitmix64` function is declared as static, so it is not directly callable from other files. The same is true for `rejectionSample`.

</br>

## Algorithms
The library uses two different algorithms for generating pseudorandom numbers:
- xoroshiro++, and
- xoroshiro**

Both have $2^{128}$ periods, which is much smaller than the Mersenne Twister's period of $2^{19937}$. However, the xoroshiro algorithms are much faster <small>(see <b>Benchmarks</b> for more details)</small> and have been shown to pass some randomness tests that the Mersenne Twister has failed.

Xoroshiro** is better quality than xoroshiro++. However, it is slower. That is why both options have been provided.
`gen_rand64()` and `gen_urandint()` use xoroshiro++, while `gen_rand64HQ()` and `gen_urandintHQ()` use xoroshiro**.

`seed_state()` uses assembly's RDSEED instruction, which generates truly random numbers based off of hardware entropy. It fills state0 with the direct result of the RDSEED instruction, then fills state1 with the same result run through the splitmix64 function.

</br>

## Benchmarks
> *Note*: `-03 -march=native` was used in the build command.

> *Note*: The AsmRand code has Lemire's rejection sampling built in. Lemire's rejection sampling (LRS) is the fastest way to conform a 64-bit random number to a range without introducing bias.
> However, mt19937_64 and std::rand do not come with LRS. Because LRS can be slightly slower than lower-quality techniques, I implemented C++ implementation of Lemire's rejection sampling and ran the raw 64-bit integers generated by mt19937_64 and std::rand through it. (I used C++ so that the function call could be inlined and optimized by the compiler)

> *Note*: Hardware: i9 intel core.

<h4 align="center">Results</h4>
<hr style="height: 1.5px; margin-top: -5px">

*(each is sorted from fastest to slowest)*

**Unbounded**:
(Lemire's rejection sampling was forgone completely)
| Algorithm | ~ns/call |
| --- | --- |
| C++ xoroshiro++ | 0.796
| C++ xoroshiro** | 0.894
| mt19937_64 | 1.440 |
| ASM xoroshiro++ | 2.636
| ASM xoroshiro** | 2.843
| std::rand | 32.634

**Bounded (0-100)**:
| Algorithm | ~ns/call |
| --- | --- |
| C++ xoroshiro++ | 1.053
| C++ xoroshiro** | 1.234
| mt19937_64 | 1.349
| ASM xoroshiro++ | 2.803
| ASM xoroshiro** | 2.877
| std::rand | 32.470

**Bounded (0-1,000,000,123)**:
| Algorithm | ~ns/call |
| --- | --- |
| C++ xoroshiro++ | 1.071
| C++ xoroshiro** | 1.064
| mt19937_64 | 1.241
| ASM xoroshiro++ | 2.737
| ASM xoroshiro** | 2.902
| std::rand | 34.560

</br>

## Linking with C++
The assembly code can be linked with C++ if desired using a build commmand similar to this:
```powershell
nasm -f win64 asm_rand.asm -o asm_rand.obj
g++ -std=c++20 ^
    asm_rand.cpp ^
    main.cpp ^
    asm_rand.obj ^
    -o main.exe
```

---

# QuickRand
> Platform: GCC and likely Clang/MSVC (needs to be tested for those) C++

## Purpose
An extremely fast C++ library allowing developers to get high quality true-random/pseudorandom numbers.

</br>

## Usage
- `uint64_t gen_urand64(RNGstate& state)`: Generates a random 64-bit number. It does not accept a range. `state` is modified and progressed to its next state.
- `uint64_t gen_urand64HQ(RNGstate& state)`: Identical in usage to `gen_urand64`. However, the underlying algorithm is higher quality (and slower).
- `int64_t gen_rand64(RNGstate& state)`: Identical to `gen_urandint64`, but it returns a signed number.
- `int64_t gen_rand64HQ(RNGstate& state)` Identical to `gen_rand64`, but it uses a higher quality (and slower) algorithm to generate the random number.
- `uint64_t gen_urandint(RNGstate& state, uint64_t min, uint64_t max)`: Uses `gen_urand64`, but applies Lemire's rejection sampling to conform the result to a range without introducing bias. The result is unsigned.
- `uint64_t gen_urandintHQ(RNGstate& state, uint64_t min, uint64_t max)`: Identical in usage to `gen_urandint`. However, it uses `gen_urand64`, which uses a higher quality but slower algorithm.
- `int64_t gen_randint(RNGstate& state, int64_t min, int64_t max)` Identical to `gen_urandint`, except it can accept a signed minimum and maximum, and it returns a signed number.
- `int64_t gen_randintHQ(RNGstate& state, int64_t min, int64_t max)` Identical to `gen_randint`, except it uses a higher quality algorithm to generate the random number.
- `uint64_t gen_seed64()`, `uint32_t gen_seed32()`, and `uint16_t gen_seed16()`: Return truly random 64-bit, 32-bit, and 16-bit integers (respectively) that are generated from hardware entropy. These functions are much slower than pseudorandom generators.
- `void seed_state(RNGstate& state)`: Uses `gen_seed64` and `splitmix64` to fill the RNGstate passed into it. `state.state1` is set to the direct result of `gen_seed64`, and `state.state0` is set to that result after it is passed through the `splitmix64` function.
> *Note*: The `splitmix64` function is declared as static, so it is not directly callable from other files. The same is true for `rejectionSample`.

</br>

## Algorithms
The library uses two different algorithms for generating pseudorandom numbers:
- xoroshiro++, and
- xoroshiro**

Both have $2^{128}$ periods, which is much smaller than the Mersenne Twister's period of $2^{19937}$. However, the xoroshiro algorithms are much faster <small>(see <b>Benchmarks</b> for more details)</small> and have been shown to pass some randomness tests that the Mersenne Twister has failed.

Xoroshiro** is better quality than xoroshiro++. However, it is slower. That is why both options have been provided.
`gen_rand64()` and `gen_urandint()` use xoroshiro++, while `gen_rand64HQ()` and `gen_urandintHQ()` use xoroshiro**.

`seed_state()` uses assembly's RDSEED instruction, which generates truly random numbers based off of hardware entropy. It fills state0 with the direct result of the RDSEED instruction, then fills state1 with the same result run through the splitmix64 function.

</br>

## Benchmarks
> *Note*: `-03 -march=native` was used in the build command.

> *Note*: The AsmRand code has Lemire's rejection sampling built in. Lemire's rejection sampling (LRS) is the fastest way to conform a 64-bit random number to a range without introducing bias.
> However, mt19937_64 and std::rand do not come with LRS. Because LRS can be slightly slower than lower-quality techniques, I implemented C++ implementation of Lemire's rejection sampling and ran the raw 64-bit integers generated by mt19937_64 and std::rand through it. (I used C++ so that the function call could be inlined and optimized by the compiler)

> *Note*: Hardware: i9 intel core.

<h4 align="center">Results</h4>
<hr style="height: 1.5px; margin-top: -5px">

*(each is sorted from fastest to slowest)*

**Unbounded**:
(Lemire's rejection sampling was forgone completely for these tests)
| Algorithm | ~ns/call |
| --- | --- |
| C++ xoroshiro++ | 0.796
| C++ xoroshiro** | 0.894
| mt19937_64 | 1.440 |
| ASM xoroshiro++ | 2.636
| ASM xoroshiro** | 2.843
| std::rand | 32.634

**Bounded (0-100)**:
(Using Lemire's rejection sampling)
| Algorithm | ~ns/call |
| --- | --- |
| C++ xoroshiro++ | 1.053
| C++ xoroshiro** | 1.234
| mt19937_64 | 1.349
| ASM xoroshiro++ | 2.803
| ASM xoroshiro** | 2.877
| std::rand | 32.470

**Bounded (0-1,000,000,123)**:
(Using Lemire's rejection sampling)
| Algorithm | ~ns/call |
| --- | --- |
| C++ xoroshiro++ | 1.071
| C++ xoroshiro** | 1.064
| mt19937_64 | 1.241
| ASM xoroshiro++ | 2.737
| ASM xoroshiro** | 2.902
| std::rand | 34.560

---

# SimpleBenchmark
> Platform: GCC, Clang, and MSVC

## Purpose
A helpful utility to benchmark functions. It works for any level of compiler optimization because (through some trickery) it prevents the compiler from removing the benchmark loop as 'dead code'.

</br>

## Usage
There is only one function in this mini-library:
`BenchResult benchmark(Fn&& function_, uint64_t iterations)`.
Note that `Fn&& function_` must be callable and it must not return void.

Call like:
`BenchResult result = benchmark([]{ std::cout << "Running Function :D" << std::endl; }, 10'000);`

> *Note*: Type `Func` must satisfy the `NonVoidReturn` concept (meaning `function_` can't return void).
> This allows `benchmark` to create a data dependency with `function_`'s return value to prevent the compiler from eleminating the loop within `benchmark` as dead code.
> This does not mean, however, that the return value must be meaningful.

## Struct: `BenchResult`
Contains:
- `double ms` (total milliseconds the benchmark took)
- `uint64_t iterations` (iterations of benchmark)
- `double nsPerCall` (approx. nanoseconds per call of function)

---

# ThreadPool
## Purpose
A thread pool class, allowing developers to easily queue multiple tasks to be executed in parallel.
## Usage
**Construction:**
* Empty: `ThreadPool myPool();`
* Instantiate threads: `ThreadPool myPool(size_t numThreads);`
  * Emplaces `numThreads` threads into the pool upon construction.

**Adding Threads:**
* Add one thread: `myPool.emplace_back();`
* Add multiple threads: `myPool.emplace_back(size_t numThreads)`
  * Emplaces `numThreads` threads into the thread pool.

**Queueing Jobs:**
* Queueing a function:
    ```cpp
    myPool.queue_job(foo);
    ```
* Lambdas work too:
    ```cpp
    myPool.queue_job(
        [](){
            int x = 5 * 5;
            foo(x);
            // Etc...
        };
    );
    ```
* Queueing a function that returns something:
    ```cpp
    // (must be same type as lambda return type)
    //          vvv
    std::future<int> pendingResult = myPool.queue_job_async(
        // (lambda return type)
        //       vvv
        [=]() -> int {
            int x = 1;
            x += 5;
            // Etc...
            return x;
        };
    );
    int result = pendingResult.get(); // blocks until pendingResult is done
    ```
    > *Note*: `std::future<void>` can be used if the lambda does not return. Calling .get() will block until the lambda is done executing.
* 

### Things to Note
- The job queue is FIFO (First In, First Out).
- There is no limit to how many threads can be in the threadpool. However, although it is use-case dependent, it is generally recommended to never have more threads than twice your hardware's thread capacity (for optimal speed, that is).
