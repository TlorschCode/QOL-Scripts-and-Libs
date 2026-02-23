#pragma once
#include <queue>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <cstdint>
#include <future>
#include <algorithm>
#include <memory>

class ThreadPool {
private:
    std::mutex jobQueueMtx;

    std::atomic<uint32_t> threadsToEnd;
    std::condition_variable threadSleeper;
    
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobs;

    void threadPoolWorker() {
        while (true) {
            std::function<void()> job;

            {
                std::unique_lock<std::mutex> queueLock(jobQueueMtx);
                threadSleeper.wait(queueLock, [&]{
                    return threadsToEnd.load() > 0 || !jobs.empty(); // wait until the pool is being destructed or there is a job.
                });                                                  // Only lets one thread through at a time.

                if (threadsToEnd.load() > 0) {
                    this->threadsToEnd.store(threadsToEnd.load() - 1);
                    return; // shutdown
                }

                // Safely get the next job
                if (!jobs.empty()) {
                    job = std::move(jobs.front());
                    jobs.pop();
                }
            }

            // Execute the job if it is validi
            if (job) job();
        }
    }
public:
    // Initializes an empty thread pool
    ThreadPool() : threads(), jobs(), threadsToEnd(0) {}
    /*
    Initializes `numThreads` threads in the thread pool upon construction.
    
    Note: Significantly more threads than `hardware_concurrency` in the
    thread pool may decrease performance. However, in some cases, initializing
    a few more threads than `hardware_concurrency` can increase performance.
    */
    ThreadPool(size_t numThreads) : jobs(), threadsToEnd(0) {
        emplace_back(numThreads);
    }
    ~ThreadPool() {
        join_threads();
    }
    /*
    Adds a job to the process queue.

    Pass a lambda that calls the function to include paramaters. e.g.
    `threadPool.queue_job(
        std::move([]{ foo(1, 2); }));`
    */
    void queue_job(std::function<void()>&& function) {
        std::unique_lock<std::mutex> jobQueueLock(jobQueueMtx);
        jobs.push(std::move(function));
        jobQueueLock.unlock();
        threadSleeper.notify_one();
    }

    /*
    Adds a job to the process queue.
    Returns a future object that can be used to get the result of the job.

    Pass a lambda that calls the function to include paramaters. e.g.
    `threadPool.queue_job(std::move([]{ foo(1, 2); }));`
    */
    template<typename F>
    [[nodiscard]] auto queue_async_job(F&& func) -> std::future<std::invoke_result_t<F>> {
        /*                                         ^^^
                                    adds && onto whatever is passed in
            (Taking a template type by && is called a universal/forwarding reference.)
            
            Can collapse into lvalue, C++ reference collapse rules:
            && + & = &
            & + && = &
            && + && = &&
            So only passing an inline lambda (rvalue) or std::move()ing a function into this function will result in func being an rvlaue.
        */
        using R = std::invoke_result_t<F>; // R becomes the type that is returned by the function of type F
        std::shared_ptr<std::packaged_task<R()>> task = std::make_shared<std::packaged_task<R()>>( // a shared pointer of a packaged task containing a function returning type R
            /* std::forward keeps the lvalueness or
            rvalueness of func when it was originally
            passed in. See above ref-collapse rules */
            std::forward<F>(func)
            /* e.g. if an inline lambda (rvalue) was
            passed in as func, forward would return an 
            std::move(func), but if a normal function
            or named lambda (variable) was passed in,
            it'd return a copy of func. */
        );
        std::future<R> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(jobQueueMtx);
            jobs.emplace(
                [task = task](){ // copies task into lambda as 'task'
                    (*task)();
                }
            );
        }
        return result;
    }

    // Joins `num` threads in the thread pool
    void join_threads(size_t num) {
        for (size_t i = std::min(threads.size(), num); i > 0; i--) {
            threadsToEnd.store(num);
            threadSleeper.notify_all();
        }
    }

    // Joins all threads
    void join_threads() {
        threadsToEnd.store(num_threads());
        threadSleeper.notify_all(); // Wake up all threads
        while (!threads.empty()) {
            if (threads.back().joinable()) {
                threads.back().join();
            }
            threads.pop_back();
        }
    }

    /*
    Adds `num` threads to the thread pool
    
    Note: Significantly more threads than `hardware_concurrency` in the
    thread pool may decrease performance. However, in some cases, initializing
    a few more threads than `hardware_concurrency` can increase performance.
    */
    void emplace_back(size_t num) {
        for (size_t i = 0; i < num; i++) {
            threads.emplace_back(&threadPoolWorker, this);
        }
    }

    // Adds 1 thread to the thread pool
    void emplace_back() {
        threads.emplace_back(&threadPoolWorker, this);
    }

    inline size_t num_jobs() const {
        return jobs.size();
    }
    inline size_t num_threads() const {
        return threads.size();
    }
};
