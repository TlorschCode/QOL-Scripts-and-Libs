# SimpleBenchmark
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

