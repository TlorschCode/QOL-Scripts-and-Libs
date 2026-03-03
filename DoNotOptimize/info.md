# DoNotOptimize
## Purpose
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
