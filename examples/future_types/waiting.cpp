#include <futures/futures.hpp>
#include <iostream>

int
long_task() {
    return 2;
}

void
handle_failed_request() {}

std::string
read_some() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return "a";
}

std::chrono::system_clock::time_point
noon() {
    return std::chrono::system_clock::now();
}

int
main() {
    using namespace futures;

    {
        //[wait Waiting for task
        cfuture<int> f = async([]() { return long_task(); });
        f.wait();
        std::cout << f.get();
        //]
    }

    {
        //[wait_for Waiting for a specific duration
        cfuture<int> f = async([]() { return long_task(); });
        std::chrono::seconds timeout(1);
        std::future_status s = f.wait_for(timeout);
        if (s == std::future_status::ready) {
            std::cout << f.get();
        } else {
            // do some other work
        }
        //]
    }

    {
        //[wait_for_network Cancelling a network request
        jcfuture<std::string> f = async([](stop_token st) {
            std::string res;
            while (!st.stop_requested()) {
                res += read_some();
            }
            return res;
        });
        std::chrono::seconds timeout(1);
        std::future_status s = f.wait_for(timeout);
        if (s == std::future_status::ready) {
            std::cout << f.get();
        } else {
            f.request_stop();
            handle_failed_request();
        }
        //]
    }

    {
        //[wait_until Waiting until a time-point
        cfuture<int> f = async([]() { return long_task(); });
        std::chrono::system_clock::time_point limit = noon();
        std::future_status s = f.wait_until(limit);
        if (s == std::future_status::ready) {
            std::cout << f.get();
        } else {
            // do some other work
        }
        //]
    }

    {
        auto should_close_window = []() {
            return true;
        };
        auto render_window_contents = []() {
        };
        {
            //[is_ready Checking if the future is ready
            cfuture<int> f = async([]() { return long_task(); });
            while (!should_close_window()) {
                if (f.is_ready()) {
                    std::cout << "Task results: " << f.get() << '\n';
                }
                render_window_contents();
            }
            //]
        }
        {
            //[free_is_ready Checking if any future is ready
            std::future<int> f = std::async([]() { return long_task(); });
            while (!should_close_window()) {
                if (is_ready(f)) {
                    std::cout << "Task results: " << f.get() << '\n';
                }
                render_window_contents();
            }
            //]
        }

    }

    {
        //[await Waiting for task
        auto f = async([]() { return long_task(); });
        std::cout << await(f);
        //]
    }

    {
        //[await_tuple Waiting for multiple tasks
        auto f1 = async([]() { return long_task(); });
        auto f2 = async([]() { return long_task(); });
        auto f3 = async([]() { return long_task(); });
        std::tuple<int, int, int> r = await(f1, f2, f3);
        std::cout << std::get<0>(r) << '\n';
        std::cout << std::get<1>(r) << '\n';
        std::cout << std::get<2>(r) << '\n';
        //]
    }

    {
        auto f1 = async([]() { return long_task(); });
        auto f2 = async([]() { return long_task(); });
        auto f3 = async([]() { return long_task(); });
        //[await_tuple_bindings Waiting for multiple tasks
        auto [r1, r2, r3] = await(f1, f2, f3);
        std::cout << r1 << '\n';
        std::cout << r2 << '\n';
        std::cout << r3 << '\n';
        //]
    }

    {
        auto f1 = async([]() { return long_task(); });
        auto f2 = async([]() { return long_task(); });
        auto f3 = async([]() { return long_task(); });
        //[wait_for_all Waiting for multiple tasks
        wait_for_all(f1, f2, f3);
        std::cout << f1.get() << '\n';
        std::cout << f2.get() << '\n';
        std::cout << f3.get() << '\n';
        //]
    }

    {
        auto f1 = async([]() { return long_task(); });
        auto f2 = async([]() { return long_task(); });
        auto f3 = async([]() { return long_task(); });
        //[wait_for_all_for Waiting for multiple tasks
        std::chrono::seconds d(1);
        std::future_status s = wait_for_all_for(d, f1, f2, f3);
        if (s == std::future_status::ready) {
            std::cout << f1.get() << '\n';
            std::cout << f2.get() << '\n';
            std::cout << f3.get() << '\n';
        }
        //]
    }

    {
        auto f1 = async([]() { return long_task(); });
        auto f2 = async([]() { return long_task(); });
        auto f3 = async([]() { return long_task(); });
        //[wait_for_any_for Waiting for multiple tasks
        std::chrono::seconds d(1);
        std::size_t idx = wait_for_any_for(d, f1, f2, f3);
        switch (idx) {
        case 0:
            std::cout << f1.get() << '\n';
            break;
        case 1:
            std::cout << f2.get() << '\n';
            break;
        case 2:
            std::cout << f3.get() << '\n';
            break;
        default:
            std::cout << "Timeout\n";
        }
        //]
    }

    return 0;
}