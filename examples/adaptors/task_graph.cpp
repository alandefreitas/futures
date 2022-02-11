#include <futures/algorithm.hpp>
#include <futures/futures.hpp>
#include <iostream>

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

int
handle_error() {
    return 1;
}

int
main() {
    using namespace futures;

    {
        //[dag Direct Acyclic Task Graph
        cfuture<int> A = async([]() { return 2; });

        cfuture<bool> B = then(A, [](int a) { return try_operation(a); });

        inline_executor ex = make_inline_executor();
        auto C_or_D = then(ex, B, [](bool ok) {
            return ok ? async(handle_success) : async(handle_error);
        });

        std::cout << "Op failed: " << C_or_D.get().get() << '\n';
        //]
    }

    {
        //[reschedule_struct Structure to reschedule operation
        struct graph_launcher
        {
            promise<int> C_completed;
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
                return C_completed.get_future();
            }
            // ...
            //]

            //[reschedule_schedule_B Scheduling or rescheduling B
            // struct graph_launcher {
            // ...
            void
            schedule_B(int a) {
                cfuture<bool>
                    B = async([](int ra) { return try_operation(ra); }, a);
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
                    C_completed.set_value(1);
                    return r;
                }).detach();
            }
        };
        //]

        //[wait_for_graph Waiting for the graph
        graph_launcher g;
        cfuture<int> f = g.start();
        std::cout << "Op failed: " << f.get() << '\n';
        //]
    }

    {
        //[loop_struct Structure to reschedule operation
        struct graph_launcher
        {
            promise<int> C_completed;

            cfuture<int>
            start() {
                schedule_A();
                return C_completed.get_future();
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
                cfuture<bool>
                    B = async([](int ra) { return try_operation(ra); }, a);
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
                    C_completed.set_value(1);
                    return r;
                }).detach();
            }
        };
        //]

        //[loop_wait_for_graph Waiting for the graph
        graph_launcher g;
        cfuture<int> f = g.start();
        std::cout << "Op failed: " << f.get() << '\n';
        //]
    }

    return 0;
}