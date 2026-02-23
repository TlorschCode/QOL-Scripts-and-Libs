# ThreadPool
## Purpose
A premade thread pool class, allowing developers to easily queue multiple tasks to be executed in parallel.
## Usage
**Construction:**
* Empty: `ThreadPool myPool();`
* Instantiate threads: `ThreadPool myPool(size_t numThreads);`
  * Emplaces `numThreads` threads into the pool upon construction.

**Adding Threads:**
* Add one thread: `myPool.emplace_back();`
* Add multiple threads: `myPool.emplace_back(size_t numThreads)`
  * Emplaces `nuumThreads` threads into the thread pool.

**Queueing Jobs:**
* Queueing a function with no paramaters:
    ```cpp
    myPool.queue_job(foo);
    ```
* Queueing a lambda with no paramaters:
    ```cpp
    myPool.queue_job( []{
        // Execute code...
    }; );
    ```
* Queueing a function with paramaters:
    ```cpp
    myPool.queue_job( [=]{ // <- Change lambda capture depending on use case
        foo(param1, ...);
    }; );
    ```

### Things to Note
The job queue is FIFO (First In, First Out).
