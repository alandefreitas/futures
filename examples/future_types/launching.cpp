#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

int
task_that_might_fail() {
    throw std::logic_error("This always fails");
}

int
main() {
    using namespace futures;

    //[no_param Launching a simple task
    cfuture<void> f1 = futures::async([] {
        std::cout << "Task 1\n";
    });
    //]

    //[no_param_jcfuture Launching a task with a stop token
    jcfuture<void> f2 = futures::async([](stop_token st) {
        while (!st.stop_requested()) {
            std::cout << "Running task 2\n";
        }
    });
    f2.request_stop();
    //]

    //[with_params Launching a task with parameters
    auto f3 = futures::async([](int x) { std::cout << x << '\n'; }, 2);
    auto f4 = futures::async([](int x, int y) { return x + y; }, 2, 3);
    //]

    //[with_executor Launching a task with a custom executor
    asio::thread_pool custom_pool(1);
    asio::thread_pool::executor_type ex = custom_pool.executor();
    auto f5 = futures::async(ex, [] {
        std::cout << "Task in thread pool" << '\n';
    });

    auto f6 = futures::async(make_inline_executor(), [] {
        std::cout << "Inline task" << '\n';
    });
    //]

    //[waiting Waiting for tasks
    f1.wait();
    f2.wait();
    f3.wait();
    std::cout << f4.get() << '\n';
    f5.wait();
    f6.wait();
    //]

    {
        //[ready_future
        vfuture<int> f = make_ready_future(3);
        std::cout << f.get() << '\n'; // 3
        //]
    }

    {
        //[throw_exception
        cfuture<int> f = async([]() {
            return task_that_might_fail();
        });
        try {
            std::cout << f.get() << '\n';
        }
        catch (std::exception const&) {
            // handle error
        }
        //]
    }

    {
        //[query_exception
        cfuture<int> f = async([]() {
            return task_that_might_fail();
        });
        if (!f.get_exception_ptr()) {
            std::cout << f.get() << '\n';
        } else {
            // handle error
        }
        //]
    }
}