#include <futures/futures.hpp>
//
#include <catch2/catch.hpp>
#include <future>

#if defined(FUTURES_USE_STANDALONE_ASIO)
#    include <asio/io_context.hpp>
#elif defined(FUTURES_USE_BOOST_ASIO)
#    include <boost/asio/io_context.hpp>
#else
#    include <futures/detail/bundled/asio/io_context.hpp>
#endif

#ifdef assert
#    undef assert
#endif
#define assert(x) REQUIRE((x))

/*
 * Some mock functions used in the snippets
 */
template <class... Args>
int
long_task(Args...) {
    return 0;
}

int
shorter_task() {
    return 0;
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
task_that_might_fail() {
    return 0;
}

void
some_task() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bool
try_operation(int) {
    static bool first_time = true;
    if (first_time) {
        first_time = false;
        return false;
    } else {
        return true;
    }
}

int
handle_success() {
    return 0;
}

template <class... Args>
int
handle_success_vals(Args...) {
    return 0;
}

int
handle_error() {
    return 1;
}

int
handle_timeout() {
    return 1;
}

TEST_CASE("Snippets") {
    using namespace futures;

    SECTION("Quickstart") {
        SECTION("Launching") {
            //[launching Basic Usage
            cfuture<void> f1 = async([] {
                // Task 1 in default executor: a thread pool
                long_task();
            });

            // analogous to:
            std::future<void> f2 = std::async([] {
                // Task 2 in a new thread provided by std::async.
                long_task();
            });
            //]

            //[launch_executor Custom executor
            asio::thread_pool custom_pool(1);
            asio::thread_pool::executor_type ex = custom_pool.executor();
            auto f3 = async(ex, [] {
                // Task 3 in a custom executor
                long_task();
            });
            //]

            //[launch_stoppable Stop token
            auto f4 = async(ex, [](stop_token st) {
                int a = 0;
                while (!st.stop_requested()) {
                    ++a;
                }
                // Task 4 stopped from another thread
                assert(a >= 0);
            });
            // ...
            f4.request_stop();
            //]

            //[launch_deferred Deferred sender
            auto f5 = schedule([] {
                // Deferred task
                long_task();
            });
            //]

            //[launch_interop Interoperation
            wait_for_all(f1, f2, f3, f4, f5);
            assert(f1.is_ready());
            assert(is_ready(f2));
            assert(f3.is_ready());
            assert(f4.is_ready());
            assert(f5.is_ready());
            //]
        }

        SECTION("Continuations") {
            //[basic_continuation Basic
            auto f1 = async([]() -> int { return 42; });
            auto f1_cont = then(f1, [](int x) { return x * 2; });
            assert(f1_cont.get() == 84);
            //]

            //[continuation_operator Operator>>
            auto f2 = std::async([]() -> int { return 63; });
            auto f2_cont = f2 >> [](int x) {
                return x * 2;
            };
            assert(f2_cont.get() == 126);
            //]

            //[continuation_unwrapping Unwrapping parameters
            auto f3 = std::async([]() { return std::make_tuple(1, 2.5, 'c'); });
            auto f3_cont = f3 >> [](int x, double y, char z) {
                assert(x == 1);
                assert(y == 2.5);
                assert(z == 'c');
            };
            f3_cont.wait();
            //]
        }

        SECTION("Conjunctions") {
            //[conjunctions Conjunctions
            auto f1 = futures::async([] { long_task(); });
            auto f2 = futures::async([] { long_task(); });
            auto f3 = futures::async([] { long_task(); });
            auto f4 = futures::async([] { long_task(); });
            auto f5 = futures::when_all(f1, f2, f3, f4);
            f5.wait();
            //]

            //[when_all_operator Operator&&
            auto f6 = futures::async([] {
                long_task();
                return 6;
            });
            auto f7 = futures::async([] {
                long_task();
                return 7;
            });
            auto f8 = futures::async([] {
                long_task();
                return 8;
            });
            auto f9 = f6 && f7 && f8;
            //]

            //[when_all_unwrapping Unwrap conjunction
            auto f10 = futures::then(f9, [](int a, int b, int c) {
                return a * b * c;
            });
            assert(f10.get() == 6 * 7 * 8);
            //]
        }

        SECTION("Disjunctions") {
            //[disjunctions Disjunctions
            auto f1 = futures::async([]() -> int { return 10; });
            auto f2 = futures::async([]() -> int { return 11; });
            auto f3 = futures::async([]() -> int { return 12; });
            auto f4 = futures::when_any(f1, f2, f3);
            auto f5 = futures::then(f4, [](int first_ready) {
                assert(first_ready >= 10 && first_ready <= 12);
            });
            f5.wait();
            //]

            //[when_any_operators Operator||
            auto f6 = futures::async([]() -> int { return 15; });
            auto f7 = futures::async([]() -> int { return 16; });
            auto f8 = f6 || f7;
            //]

            //[when_any_observers Disjunction observers
            auto r = f8.get();
            if (r.index == 0) {
                assert(std::get<0>(r.tasks).get() == 15);
            } else {
                assert(std::get<1>(r.tasks).get() == 16);
            }
            //]
        }

        SECTION("Algorithms") {
            //[algorithms_algorithms Algorithms
            std::vector<int> v(50000);
            std::iota(v.begin(), v.end(), 1);
            assert(futures::reduce(v, 0) == 1250025000);
            //]

            //[algorithms_executor Custom executors
            asio::thread_pool custom_pool(4);
            asio::thread_pool::executor_type ex = custom_pool.executor();
            futures::for_each(ex, v.begin(), v.begin() + 10, [](int x) {
                assert(x >= 0 && x <= 50000);
            });
            //]

            //[algorithms_partitioner Custom partitioners
            auto halve = [](auto first, auto last) {
                return std::next(first, (last - first) / 2);
            };
            auto it = futures::find(ex, halve, v, 3000);
            if (it != v.end()) {
                assert(*it >= 0 && *it <= 50000);
                std::ptrdiff_t pos = it - v.begin();
                assert(pos >= 0 && pos <= 50000);
            }
            //]
        }
    }

    SECTION("Motivation") {
        SECTION("Polling continuations") {
            // Waiting
            {
                std::future A = std::async([]() { return 2; });
                int A_result = A.get();
                assert(A_result == 2);
            }

            // Polling
            {
                std::future A = std::async([]() { return 2; });
                std::future B = std::async([&]() {
                    int A_result = A.get();
                    assert(A_result == 2);
                });
                B.wait();
            }

            // Lazy continuations
            {
                auto A = futures::async([]() { return 2; });
                auto B = futures::then(A, [](int A_result) {
                    assert(A_result == 2);
                });
                B.wait();
            }
        }
    }

    SECTION("Future types") {
        SECTION("Launching") {
            //[no_param Launching a simple task
            cfuture<void> f1 = futures::async([] {
                // Task 1
                long_task();
            });
            //]

            //[no_param_jcfuture Launching a task with a stop token
            jcfuture<void> f2 = futures::async([](stop_token st) {
                while (!st.stop_requested()) {
                    // Running task 2
                    shorter_task();
                }
            });
            f2.request_stop();
            //]

            //[with_params Launching a task with parameters
            auto f3 = futures::async([](int x) { assert(x == 2); }, 2);
            auto f4 = futures::async([](int x, int y) { return x + y; }, 2, 3);
            //]

            //[with_executor Launching a task with a custom executor
            asio::thread_pool custom_pool(1);
            asio::thread_pool::executor_type ex = custom_pool.executor();
            auto f5 = futures::async(ex, [] {
                // Task in thread pool
                long_task();
            });

            auto f6 = futures::async(make_inline_executor(), [] {
                // Inline task
                long_task();
            });
            //]

            //[waiting Waiting for tasks
            f1.wait();
            f2.wait();
            f3.wait();
            assert(f4.get() == 5);
            f5.wait();
            f6.wait();
            //]

            {
                //[ready_future
                vfuture<int> f = make_ready_future(3);
                assert(f.get() == 3);
                //]
            }

            {
                //[throw_exception
                cfuture<int> f = futures::async([]() {
                    return task_that_might_fail();
                });
                try {
                    assert(f.get() == 0);
                }
                catch (std::exception const&) {
                    handle_error();
                }
                //]
            }

            {
                //[query_exception
                cfuture<int> f = async([]() { return task_that_might_fail(); });
                if (!f.get_exception_ptr()) {
                    assert(f.get() == 0);
                } else {
                    // handle error
                }
                //]
            }
        }

        SECTION("Scheduling") {
            asio::thread_pool custom_pool(1);
            asio::thread_pool::executor_type ex = custom_pool.executor();

            //[schedule Scheduling deferred tasks
            auto f1 = schedule([] {
                // No parameters
                long_task();
            });
            auto f2 = schedule([](int x) { assert(x == 2); }, 2);
            auto f3 = schedule([](int x, int y) { return x + y; }, 2, 3);
            auto f4 = schedule(ex, [] {
                // Custom executor
                long_task();
            });
            auto f5 = schedule(make_inline_executor(), [] {
                // Inline executor
                long_task();
            });

            // Tasks are only launched now!
            f1.wait();
            f2.wait();
            assert(f3.get() == 5);
            f4.wait();
            f5.wait();
            //]

            {
                //[no_alloc Allocations not required
                auto f = schedule([]() { return 1; });
                assert(f.get() == 1);
                //]
            }

            {
                //[then Deferred continuations
                auto fa = schedule([]() { return 1; });
                auto fb = fa.then([](int a) { return a * 2; });
                assert(fb.get() == 2);
                //]
            }
        }

        SECTION("Waiting") {
            {
                //[wait Waiting for task
                cfuture<int> f = async([]() { return long_task(); });
                f.wait();
                assert(f.get() == 0);
                //]
            }

            {
                //[wait_for Waiting for a specific duration
                cfuture<int> f = async([]() { return long_task(); });
                std::chrono::seconds timeout(1);
                future_status s = f.wait_for(timeout);
                if (s == future_status::ready) {
                    assert(f.get() == 0);
                } else {
                    // do some other work
                }
                //]
            }

            {
                //[wait_for_all Waiting for all futures
                cfuture<int> f1 = async([]() { return long_task(); });
                cfuture<int> f2 = async([]() { return long_task(); });
                wait_for_all(f1, f2);
                assert(f1.get() == 0);
                assert(f2.get() == 0);
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
                future_status s = f.wait_for(timeout);
                if (s == future_status::ready) {
                    handle_success_vals(s);
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
                future_status s = f.wait_until(limit);
                if (s == future_status::ready) {
                    assert(f.get() == 0);
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
                            // Task results
                            assert(f.get() == 0);
                        }
                        render_window_contents();
                    }
                    //]
                }
                {
                    //[free_is_ready Checking if any future is ready
                    std::future<int> f = std::async([]() {
                        return long_task();
                    });
                    while (!should_close_window()) {
                        if (is_ready(f)) {
                            // Task results
                            assert(f.get() == 0);
                        }
                        render_window_contents();
                    }
                    //]
                }
            }

            {
                //[await Waiting for task
                auto f = async([]() { return long_task(); });
                assert(await(f) == 0);
                //]
            }

            {
                //[await_tuple Waiting for multiple tasks
                auto f1 = async([]() { return long_task(); });
                auto f2 = async([]() { return long_task(); });
                auto f3 = async([]() { return long_task(); });
                std::tuple<int, int, int> r = await(f1, f2, f3);
                assert(std::get<0>(r) == 0);
                assert(std::get<1>(r) == 0);
                assert(std::get<2>(r) == 0);
                //]
            }

            {
                auto f1 = async([]() { return long_task(); });
                auto f2 = async([]() { return long_task(); });
                auto f3 = async([]() { return long_task(); });
                //[await_tuple_bindings Waiting for multiple tasks
                auto [r1, r2, r3] = await(f1, f2, f3);
                assert(r1 == 0);
                assert(r2 == 0);
                assert(r3 == 0);
                //]
            }

            {
                auto f1 = async([]() { return long_task(); });
                auto f2 = async([]() { return long_task(); });
                auto f3 = async([]() { return long_task(); });
                //[types_wait_for_all Waiting for multiple tasks
                wait_for_all(f1, f2, f3);
                assert(f1.get() == 0);
                assert(f2.get() == 0);
                assert(f3.get() == 0);
                //]
            }

            {
                auto f1 = async([]() { return long_task(); });
                auto f2 = async([]() { return long_task(); });
                auto f3 = async([]() { return long_task(); });
                //[wait_for_all_for Waiting for multiple tasks
                std::chrono::seconds d(1);
                future_status s = wait_for_all_for(d, f1, f2, f3);
                if (s == future_status::ready) {
                    assert(f1.get() == 0);
                    assert(f2.get() == 0);
                    assert(f3.get() == 0);
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
                    assert(f1.get() == 0);
                    break;
                case 1:
                    assert(f2.get() == 0);
                    break;
                case 2:
                    assert(f3.get() == 0);
                    break;
                default:
                    handle_timeout();
                }
                //]
            }
        }

        SECTION("Interoperability") {
            //[interop_std_async C++ std::async
            std::future<void> future1 = std::async([] {
                // std::async task
                long_task();
            });
            //]

            //[interop_cfuture Launching a continuable future
            cfuture<void> future2 = futures::async([] {
                // continuable task
                long_task();
            });
            //]

            //[interop_jcfuture Launching a stoppable future
            jcfuture<void> future3 = futures::async([](stop_token st) {
                int a = 0;
                while (!st.stop_requested()) {
                    ++a;
                }
                // task stopped
                assert(a >= 0);
            });
            //]

            //[interop_jcfuture_stop Requesting task to stop
            future3.request_stop();
            //]

            //[interop_wait_for_all All of these types interoperate!
            wait_for_all(future1, future2, future3);
            assert(is_ready(future1));
            assert(future2.is_ready());
            assert(future3.is_ready());
            //]
        }

        SECTION("Shared") {
            {
                //[create_shared Creating a shared future
                cfuture<int> f1 = async([] { return 1; });
                shared_cfuture<int> f2 = f1.share();
                //]
            }

            {
                //[invalidate_unique Previous future is invalidated
                cfuture<int> f1 = async([] { return 1; });
                shared_cfuture<int> f2 = f1.share();
                assert(!f1.valid());
                assert(f2.valid());
                //]
            }

            {
                //[single_step Creating a shared future
                shared_cfuture<int> f = async([] { return 1; }).share();
                assert(f.get() == 1);
                //]
            }

            //[share_state Sharing the future state
            shared_cfuture<int> f1 = async([] { return 1; }).share();

            // OK to copy
            shared_cfuture<int> f2 = f1;
            //]

            //[get_state Sharing the future state
            // OK to get
            assert(f1.get() == 1);

            // OK to call get on the other future
            assert(f2.get() == 1);

            // OK to call get twice
            assert(f1.get() == 1);
            assert(f2.get() == 1);
            //]

            {
                {
                    //[future_vector Future vector (value is moved)
                    cfuture<std::vector<int>> f = async([] {
                        return std::vector<int>(1000, 0);
                    });
                    std::vector<int> v = f.get(); // value is moved
                    assert(!f.valid());           // future is now invalid
                    //]
                }

                {
                    //[shared_future_vector Shared vector (value is copied)
                    shared_cfuture<std::vector<int>>
                        f = async([] {
                                return std::vector<int>(1000, 0);
                            }).share();
                    std::vector<int> v = f.get();  // value is copied
                    assert(f.valid());             // future is still valid
                    std::vector<int> v2 = f.get(); // value is copied again
                    //]
                }
            }
        }

        SECTION("Continuable") {
            {
                //[std_async Simple std::async task
                std::future<int> f = std::async([]() {
                    // Parallel work
                    return 65;
                });
                // Main work
                assert(f.get() == 65);
                //]
            }

            {
                //[wait_for_next Always waiting for the next task
                std::future<int> A = std::async([]() { return 65; });

                std::future<char> B = std::
                    async([](int v) { return static_cast<char>(v); }, A.get());

                std::future<void>
                    C = std::async([](char c) { assert(c == 'A'); }, B.get());

                C.wait();
                //]
            }

            {
                //[polling Polling the previous task
                std::future<int> A = std::async([]() { return 65; });

                std::future<char> B = std::async([&A]() {
                    // B waits for A in its turn
                    int v = A.get();
                    // Use the value
                    return static_cast<char>(v);
                });

                std::future<void> C = std::async([&B]() {
                    assert(B.get() == 'A');
                });

                C.wait();
                //]
            }

            {
                //[continuables Continuable futures
                cfuture<int> A = async([]() { return 65; });
                cfuture<char> B = A.then([](int v) {
                    return static_cast<char>(v);
                });
                cfuture<void> C = then(B, [](char c) { assert(c == 'A'); });
                C.wait();
                //]
            }

            {
                // clang-format off
                //[chaining Chaining continuations
                cfuture<void> C = async([]() {
                    return 65;
                }).then([](int v) {
                    return static_cast<char>(v);
                }).then([](char c) {
                    assert(c == 'A');
                });
                C.wait();
                //]
                // clang-format on
            }

            {
                //[deferred_continuables Continuable futures
                auto A = schedule([]() { return 65; });

                auto B = then(A, [](int v) { return static_cast<char>(v); });

                auto C = then(B, [](char c) { assert(c == 'A'); });

                C.wait(); // launch A now!
                //]
            }
        }

        SECTION("Stoppable") {
            //[stoppable Stoppable task
            jcfuture<void> f = async([](stop_token s) {
                while (!s.stop_requested()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
            });
            //]

            //[not_ready
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            assert(!is_ready(f));
            //]

            //[request_stop Requesting task to stop
            f.request_stop();
            f.wait();
            assert(is_ready(f));
            //]
        }

        SECTION("Promises") {
            //[promise_inline Inline promise
            promise<int> p1;
            cfuture<int> f1 = p1.get_future();
            p1.set_value(2);
            assert(f1.get() == 2);
            //]

            //[promise_thread Promise set by new thread
            promise<int> p2;
            cfuture<int> f2 = p2.get_future();
            std::thread t2([&p2]() { p2.set_value(2); });
            assert(f2.get() == 2);
            t2.join();
            //]

            //[promise_thread-executor Promise set by new thread executor
            auto f3 = async(make_new_thread_executor(), []() { return 2; });
            assert(f3.get() == 2);
            //]

            //[promise_thread_pool Promise set by thread pool
            promise<int> p4;
            cfuture<int> f4 = p4.get_future();
            asio::thread_pool pool(1);
            asio::post(pool, [&p4]() { p4.set_value(2); });
            assert(f4.get() == 2);
            //]

            //[promise_custom_options Promise with custom options
            promise<int, future_options<>> p5;
            vfuture<int> f5 = p5.get_future();
            std::thread t5([&p5]() { p5.set_value(2); });
            assert(f5.get() == 2);
            t5.join();
            //]
        }

        SECTION("Packaged task") {
            //[packaged_task_inline Inline packaged task
            packaged_task<int()> p1([]() { return 2; });
            auto f1 = p1.get_future();
            p1();
            assert(f1.get() == 2);
            //]

            //[packaged_task_thread Packaged task invoked by thread
            packaged_task<int()> p2([]() { return 2; });
            auto f2 = p2.get_future();
            std::thread t(std::move(p2));
            assert(f2.get() == 2);
            t.join();
            //]

            //[packaged_task_executor Packaged task invoked by executor
            packaged_task<int()> p3([]() { return 2; });
            auto f3 = p3.get_future();
            asio::thread_pool pool(1);
            asio::post(pool, std::move(p3));
            assert(f3.get() == 2);
            //]
        }
    }

    SECTION("Adaptors") {
        SECTION("Continuations") {
            //[continuable Continuation to a continuable future
            cfuture<int> f1 = async([]() -> int { return 42; });
            cfuture<void> f2 = then(f1, [](int x) {
                // Another task in the executor
                assert(x == 42);
            });
            //]

            //[std_future Continuation to a std::future
            std::future<int> f3 = std::async([]() -> int { return 63; });
            cfuture<void> f4 = then(f3, [](int x) {
                // Another task in the default executor
                assert(x == 63);
            });
            //]

            //[deferred Continuation to a deferred future
            auto f5 = schedule([]() -> int { return 63; });
            auto f6 = then(f5, [](int x) { assert(x == 63); });
            //]

            //[executor_change Continuation with another executor
            cfuture<int> f7 = async([] { return 2; });
            asio::thread_pool pool(1);
            auto ex = pool.executor();
            cfuture<int> f8 = then(ex, f7, [](int v) { return v * 2; });
            //]

            //[operator_then operator>>
            cfuture<int> f9 = f8 >> [](int x) {
                return x * 2;
            };
            //]

            //[operator_ex operators >> and %
            auto inline_executor = make_inline_executor();
            auto f10 = f9 >> inline_executor % [](int x) {
                return x + 2;
            };
            assert(f10.get() == 10);
            //]
        }

        SECTION("Continuations unwrap") {
            {
                //[error Continuations with no unwrapping
                cfuture<void> f1 = async([]() { task_that_might_fail(); });
                cfuture<void> f2 = then(f1, [](cfuture<void> f) {
                    if (!f.get_exception_ptr()) {
                        handle_success();
                    } else {
                        handle_error();
                    }
                });
                //]
            }

            {
                //[verbose_create Task returning 3 values
                auto f1 = async([]() {
                    return std::make_tuple(
                        make_ready_future(1),
                        make_ready_future(2.0),
                        make_ready_future<std::string>("3"));
                });
                //]
                //[verbose_continue Verbose continuations
                cfuture<void> f2 = then(
                    f1,
                    [](cfuture<std::tuple<
                           vfuture<int>,
                           vfuture<double>,
                           vfuture<std::string>>> f) {
                    // retrieve futures
                    auto t = f.get();
                    vfuture<int> fa = std::move(std::get<0>(t));
                    vfuture<double> fb = std::move(std::get<1>(t));
                    vfuture<std::string> fc = std::move(std::get<2>(t));
                    // get their values
                    int a = fa.get();
                    double b = fb.get();
                    std::string c = fc.get();
                    // use values
                    assert(a == 1);
                    assert(b == 2.0);
                    assert(c == "3");
                    });
                //]
            }

            {
                // clang-format off
                //[unwrap_void Unwrap void
                cfuture<void> f1 = async([]() {
                    // Task
                    long_task();
                });
                cfuture<int> f2 = f1 >> []() { return 6; };
                assert(f2.get() == 6);
                //]
                // clang-format on
            }


            {
                //[unwrap_fail_void Unwrap and query
                cfuture<void> f1 = async([]() { task_that_might_fail(); });

                cfuture<int> f2 = f1 >> []() {
                    return 6;
                };

                if (!f2.get_exception_ptr()) {
                    handle_success_vals(f2.get());
                } else {
                    handle_error();
                }
                //]
            }

            {
                //[value_unwrap Forward value directly
                auto f1 = async([]() { return 6; });
                auto f2 = f1 >> [](int x) {
                    return x * 2;
                };
                assert(f2.get() == 12);
                //]
            }

            {
                //[double_unwrap Double value unwrap
                auto f1 = async([]() { return make_ready_future(6); });
                auto f2 = f1 >> [](int x) {
                    return x * 2;
                };
                assert(f2.get() == 12);
                //]
            }

            {
                //[tuple_unwrap Unwrap tuple
                auto f1 = async([]() { return make_ready_future(6); });
                auto f2 = f1 >> [](int x) {
                    return std::make_tuple(x * 1, x * 2, x * 3);
                };
                cfuture<int> f3 = f2 >> [](int a, int b, int c) {
                    return a * b * c;
                };
                assert(f3.get() == 6 * 1 * 6 * 2 * 6 * 3);
                //]
            }

            {
                //[double_tuple_unwrap Unwrap tuple
                auto f1 = async([]() { return make_ready_future(6); });
                auto f2 = f1 >> [](int x) {
                    return std::make_tuple(
                        make_ready_future(1 * x),
                        make_ready_future(2 * x),
                        make_ready_future(3 * x));
                };
                auto f3 = f2 >> [](int a, int b, int c) {
                    return a + b + c;
                };
                assert(f3.get() == 1 * 6 + 2 * 6 + 3 * 6);
                //]
            }

            {
                //[when_all_unwrap Unwrap future conjunction
                auto f1 = async([]() { return 1; });
                auto f2 = async([]() { return 2; });
                auto f3 = async([]() { return 3; });
                auto f4 = async([]() { return 4; });
                auto f5 = when_all(f1, f2, f3, f4);
                auto f6 = f5 >> [](int a, int b, int c, int d) {
                    return a + b + c + d;
                };
                assert(f6.get() == 1 + 2 + 3 + 4);
                //]
            }

            {
                //[when_any_unwrap Unwrap future disjunction
                cfuture<int> f1 = async([]() { return 1; });
                cfuture<int> f2 = async([]() { return 2; });
                when_any_future<std::tuple<cfuture<int>, cfuture<int>>>
                    f3 = when_any(f1, f2);
                auto f4 = f3 >>
                          [](std::size_t idx,
                             std::tuple<cfuture<int>, cfuture<int>> prev) {
                    if (idx == 0) {
                        return std::get<0>(prev).get();
                    } else {
                        return std::get<1>(prev).get();
                    }
                };
                int r = f4.get();
                assert(r == 1 || r == 2);
                //]
            }

            {
                //[when_any_explode_unwrap Explode future conjunction
                auto f1 = async([]() { return 1; });
                auto f2 = async([]() { return 2; });
                auto f3 = when_any(f1, f2);
                auto f4 = f3 >>
                          [](std::size_t idx, cfuture<int> f1, cfuture<int> f2) {
                    if (idx == 0) {
                        return f1.get();
                    } else {
                        return f2.get();
                    }
                };
                int r = f4.get();
                assert(r >= 1 && r <= 2);
                //]
            }

            {
                //[when_any_single_result Homogenous disjunction
                auto f1 = async([]() { return 1; });
                auto f2 = async([]() { return 2; });
                auto f3 = when_any(f1, f2);
                auto f4 = f3 >> [](cfuture<int> f) {
                    return f.get();
                };
                int r = f4.get();
                assert(r >= 1 && r <= 2);
                //]
            }

            {
                //[when_any_single_result_unwrap Homogenous unwrap
                auto f1 = async([]() { return 1; });
                auto f2 = async([]() { return 2; });
                auto f3 = when_any(f1, f2);
                auto f4 = f3 >> [](int v) {
                    return v * 2;
                };
                int r = f4.get();
                assert(r == 2 || r == 4);
                //]
            }

            {
                //[ambiguous Ambiguous unwrapping
                cfuture<int> f1 = async([]() { return 1; });
                auto f2 = f1 >> [](auto f) -> decltype(f.get()) {
                    // Is `f` a `cfuture<int>` or `int`?
                    // `cfuture<int>` has highest priority
                    return f.get();
                };
                assert(f2.get() == 1);
                //]
                // see:
                // https://stackoverflow.com/questions/64186621/why-doesnt-stdis-invocable-work-with-templated-operator-which-return-type-i
            }

            {
                //[return_future Getting a future from a future
                cfuture<cfuture<int>> f = async([]() {
                    return async([]() { return 1; });
                });
                assert(f.get().get() == 1);
                //]
            }

            {
                //[stop_source Continuation stop source
                auto f1 = async([](stop_token st) {
                    while (!st.stop_requested()) {
                        some_task();
                    }
                });
                auto ss = f1.get_stop_source();
                auto f2 = f1 >> []() {
                    // f1 done
                    handle_success();
                };
                // f1.request_stop() won't work anymore
                ss.request_stop();
                f2.get();
                //]
            }
        }

        SECTION("Conjunctions") {
            {
                //[when_all_small_graph Task graph with conjunction
                shared_cfuture<int> A = async([]() { return 2; }).share();

                cfuture<int> B = then(A, [](int a) { return a * 3; });
                cfuture<int> C = then(A, [](int a) { return a * 2; });

                cfuture<int> D = then(when_all(B, C), [](int b, int c) {
                    return b + c;
                });

                assert(D.get() == 10);
                //]
            }

            {
                //[conjunction Future conjunction
                auto f1 = async([]() { return 2; });
                auto f2 = async([]() { return 3.5; });
                auto f3 = async([]() -> std::string { return "name"; });
                auto all = when_all(f1, f2, f3);
                //]

                //[conjunction_return Conjunction as tuple
                auto [r1, r2, r3] = all.get(); // get ready futures
                assert(r1.get() == 2);
                assert(r2.get() == 3.5);
                assert(r3.get() == "name");
                //]
            }

            {
                //[conjunction_range Future conjunction as range
                std::vector<cfuture<int>> fs;
                fs.emplace_back(async([]() { return 2; }));
                fs.emplace_back(async([]() { return 3; }));
                fs.emplace_back(async([]() { return 4; }));
                auto all = when_all(fs);

                auto rs = all.get();
                assert(rs[0].get() == 2);
                assert(rs[1].get() == 3);
                assert(rs[2].get() == 4);
                //]
            }

            {
                //[operator_when_all operator&&
                auto f1 = async([]() { return 2; });
                auto f2 = async([]() { return 3.5; });
                auto f3 = async([]() -> std::string { return "name"; });
                auto all = f1 && f2 && f3;
                //]

                //[continuation_unwrap Future conjunction as range
                auto f4 = then(all, [](int a, double b, std::string c) {
                    assert(a == 2);
                    assert(b == 3.5);
                    assert(c == "name");
                });
                //]
            }

            {
                // clang-format off
                //[when_all_operator_lambda Lambda conjunction
                auto f1 = []() { return 2; } &&
                          []() { return 3.5; } &&
                          []() -> std::string { return "name"; };

                auto f2 = then(f1, [](int a, double b, std::string c) {
                    assert(a == 2);
                    assert(b == 3.5);
                    assert(c == "name");
                });
                //]
                // clang-format on
            }
        }

        SECTION("Conjunctions unwrap") {
            // Direct
            auto f1 = async([]() { return 2; }) && async([]() { return 3.5; });
            auto f1c = then(f1, [](std::tuple<cfuture<int>, cfuture<double>> r) {
                return std::get<0>(r).get()
                       + static_cast<int>(std::get<1>(r).get());
            });
            assert(f1c.get() == 5);

            // Unwrap tuple
            auto f2 = async([]() { return 2; }) && async([]() { return 3.5; });
            auto f2c = then(f2, [](cfuture<int> r1, cfuture<double> r2) {
                return r1.get() + static_cast<int>(r2.get());
            });
            assert(f2c.get() == 5);

            // Unwrap values
            auto f3 = async([]() { return 2; }) && async([]() { return 3.5; });
            auto f3c = then(f3, [](int r1, double r2) {
                return r1 + static_cast<int>(r2);
            });
            assert(f3c.get() == 5);
        }

        SECTION("Disjunctions") {
            {
                //[when_any_small_graph Task graph with conjunction
                shared_cfuture<int> A = async([]() { return 2; }).share();

                cfuture<int> B = then(A, [](int a) { return a * 2; });
                cfuture<int> C = then(A, [](int a) { return a * 3; });

                cfuture<int> D = then(when_any(B, C), [](int b_or_c) {
                    return b_or_c * 3;
                });

                int r = D.get();
                assert(r == 12 || r == 18);
                //]
            }

            {
                //[disjunction Tuple disjunction
                cfuture<int> f1 = async([]() { return 2; });
                cfuture<double> f2 = async([]() { return 3.5; });
                cfuture<std::string> f3 = async([]() -> std::string {
                    return "name";
                });
                auto f = when_any(f1, f2, f3); // or f1 || f2 || f3
                //]

                //[disjunction_result Get disjunction result
                when_any_result any_r = f.get();
                size_t i = any_r.index;
                auto [r1, r2, r3] = std::move(any_r.tasks);
                if (i == 0) {
                    assert(r1.get() == 2);
                } else if (i == 1) {
                    assert(r2.get() == 3.5);
                } else {
                    assert(r3.get() == "name");
                }
                //]
            }

            {
                //[operator_when_any operator||
                cfuture<int> f1 = async([]() { return 2; });
                cfuture<int> f2 = async([]() { return 3; });
                cfuture<int> f3 = async([]() { return 4; });
                auto any = f1 || f2 || f3;
                //]

                // clang-format off
                //[disjunction_result_unwrap Get disjunction result
                auto f4 = any >> [](int first) { return first; };
                int r = f4.get();
                assert(r >= 2 && r <= 4);
                //]
                // clang-format on
            }

            {
                // clang-format off
                //[when_any_operator_lambda Lambda conjunction
                auto f1 = []() { return 2; } ||
                          []() { return 3; } ||
                          []() { return 4; };
                auto f2 = then(f1, [](int first) {
                    assert(first >= 2 && first <= 4);
                });
                //]
                // clang-format on
            }
        }

        SECTION("Disjunctions unwrap") {
            // Direct
            auto f1 = async([]() { return 2; }) || async([]() { return 3.5; });
            auto f1c = then(
                f1,
                [](when_any_result<std::tuple<cfuture<int>, cfuture<double>>>
                       r) {
                // future at r.index is ready
                assert(r.index < 2);
                });
            f1c.wait();

            // To tuple
            auto f2 = async([]() { return 2; }) || async([]() { return 3.5; });
            auto f2c = then(
                f2,
                [](size_t index, std::tuple<cfuture<int>, cfuture<double>>) {
                // index is ready
                assert(index < 2);
                });
            f2c.wait();

            // To futures
            auto f3 = async([]() { return 2; }) || async([]() { return 3.5; });
            auto f3c = then(f3, [](size_t index, cfuture<int>, cfuture<double>) {
                // futures at index is ready
                assert(index < 2);
            });
            f3c.wait();

            // To ready future (when types are the same)
            auto f4 = async([]() { return 2; }) || async([]() { return 3; });
            auto f4c = then(f4, [](cfuture<int> v) {
                int r = v.get();
                assert(r == 2 || r == 3);
            });
            f4c.wait();

            // To ready value (when types are the same)
            auto f5 = async([]() { return 2; }) || async([]() { return 3; });
            auto f5c = then(f5, [](int v) { assert(v == 2 || v == 3); });
            f5c.wait();
        }

        SECTION("Task graph") {
            {
                //[dag Direct Acyclic Task Graph
                cfuture<int> A = async([]() { return 2; });

                cfuture<bool> B = then(A, [](int a) {
                    return try_operation(a);
                });

                inline_executor ex = make_inline_executor();
                auto C_or_D = then(ex, B, [](bool ok) {
                    return ok ? async(handle_success) : async(handle_error);
                });

                int r = C_or_D.get().get();
                assert(r == 0 || r == 1);
                //]
            }

            {
                //[reschedule_struct Structure to reschedule operation
                struct graph_launcher {
                    promise<int> end_;
                    // ...
                    //]

                    //[reschedule_start Starting the subgraph
                    // struct graph_launcher {
                    // ...
                    cfuture<int>
                    start() {
                        cfuture<int> A = async([]() { return 2; });
                        inline_executor ex = make_inline_executor();
                        then(ex, A, [this](int a) { schedule_B(a); }).detach();
                        return end_.get_future();
                    }
                    // ...
                    //]

                    //[reschedule_schedule_B Scheduling or rescheduling B
                    // struct graph_launcher {
                    // ...
                    void
                    schedule_B(int a) {
                        cfuture<bool> B = async(
                            [](int ra) { return try_operation(ra); },
                            a);
                        inline_executor ex = make_inline_executor();
                        then(ex, B, [this, a](bool ok) {
                            if (ok) {
                                schedule_C();
                            } else {
                                handle_error();
                                schedule_B(a);
                            }
                        }).detach();
                    }
                    // ...
                    //]

                    //[schedule_C Setting the promise
                    // struct graph_launcher {
                    // ...
                    void
                    schedule_C() {
                        async([this]() {
                            int r = handle_success();
                            end_.set_value(1);
                            return r;
                        }).detach();
                    }
                };
                //]

                //[wait_for_graph Waiting for the graph
                graph_launcher g;
                cfuture<int> f = g.start();
                assert(f.get() == 1);
                //]
            }

            {
                //[loop_struct Structure to reschedule operation
                struct graph_launcher {
                    promise<int> end_;

                    cfuture<int>
                    start() {
                        schedule_A();
                        return end_.get_future();
                    }
                    // ...
                    //]

                    //[loop_schedule_A Scheduling A
                    // struct graph_launcher {
                    // ...
                    void
                    schedule_A() {
                        cfuture<int> A = async([]() { return 2; });
                        inline_executor ex = make_inline_executor();
                        then(ex, A, [this](int a) { schedule_B(a); }).detach();
                    }
                    // ...
                    //]

                    //[loop_schedule_B Scheduling B
                    // struct graph_launcher {
                    // ...
                    void
                    schedule_B(int a) {
                        cfuture<bool> B = async(
                            [](int ra) { return try_operation(ra); },
                            a);
                        inline_executor ex = make_inline_executor();
                        then(ex, B, [this](bool ok) {
                            if (ok) {
                                schedule_C();
                            } else {
                                handle_error();
                                schedule_A();
                            }
                        }).detach();
                    }
                    // ...
                    //]

                    //[loop_schedule_C Setting the promise
                    // struct graph_launcher {
                    // ...
                    void
                    schedule_C() {
                        async([this]() {
                            int r = handle_success();
                            end_.set_value(1);
                            return r;
                        }).detach();
                    }
                };
                //]

                //[loop_wait_for_graph Waiting for the graph
                graph_launcher g;
                cfuture<int> f = g.start();
                assert(f.get() == 1);
                //]
            }
        }
    }

    SECTION("Algorithms") {
        std::vector<int> v(50000);
        std::iota(v.begin(), v.end(), 1);

        {
            //[algorithm Parallel Algorithms
            int c = futures::reduce(v.begin(), v.end(), 0); // parallel by
                                                            // default
            assert(c == 1250025000);
            //]
        }

        {
            //[algorithm_range Ranges
            int c = futures::reduce(v, 0); // parallel by default
            assert(c == 1250025000);
            //]
        }

        //[custom_executor Custom executor
        asio::thread_pool pool(4);
        auto ex = pool.executor();
        futures::for_each(ex, v.begin(), v.begin() + 10, [](int x) {
            assert(x >= 0 && x <= 50000);
            long_task(x);
        });
        //]

        {
            //[seq_policy Execution policy
            int c = futures::reduce(futures::seq, v, 0); // sequential execution
            assert(c == 1250025000);
            //]
        }

        {
            //[inline_executor Inline executor
            int c = futures::reduce(make_inline_executor(), v, 0); // sequential
                                                                   // execution
            assert(c == 1250025000);
            //]
        }

        {
            //[constexpr Compile-time algorithms
            constexpr std::array<int, 5> a = { 1, 2, 3, 4, 5 };
            constexpr int n = futures::reduce(a);
            constexpr std::array<int, n> b{};
            assert(b.size() == 15);
            //]
        }

        //[partitioner Defining a custom partitioner
        auto p = [](auto first, auto last) {
            return std::next(first, (last - first) / 2);
        };
        //]

        //[partitioner_algorithm Using a custom partitioner
        auto it = find(ex, p, v, 3000);
        if (it != v.end()) {
            assert(*it == 3000);
            std::ptrdiff_t pos = it - v.begin();
            assert(pos >= 0 && pos <= 50000);
        }
        //]
    }

    SECTION("Networking") {
        {
            //[enqueue asio::io_context as a task queue
            asio::io_context io;
            using io_executor = asio::io_context::executor_type;
            io_executor ex = io.get_executor();
            cfuture<int, io_executor> f = async(ex, []() { return 2; });
            //]

            //[pop asio::io_context as a task queue
            io.run();
            assert(f.get() == 2);
            //]
        }

        {
            /*
            //[push_networking Push a networking task
            asio::io_context io;
            asio::ip::tcp::acceptor acceptor(io,
            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 13));
            asio::ip::tcp::socket client_socket(io);
            acceptor.async_accept(client_socket, [](const asio::error_code&) {
                // handle connection
            });
            //]

            //[pop_networking Pop task
            io.run();
            //]
            */
        }
    }
}