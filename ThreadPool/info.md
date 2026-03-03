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
