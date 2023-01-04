# Deferred tasks

The function [schedule] can be used to create lazy tasks.

{{ code_snippet("test/unit/snippets.cpp", "schedule") }}

The main difference between [async] and [schedule] is the latter only posts the task to the executor when we
call [basic_future::wait] or [basic_future::get] on the corresponding future:

<div class="mermaid">
sequenceDiagram
    Main->>Task: Schedule
    Main->>+Task: Wait
    Task->>Task: Do work
    Task->>-Main: Return
</div>

Deferred tasks allow a number of optimizations, such as storing the shared
state inline and no synchronization cost to attach continuations.

!!! hint "Benchmark: Launching Tasks"

    The library includes a number of optimizations to ensure the allocation and
    synchronization costs of eager tasks are mitigated. Here's a small benchmark
    comparing eager and deferred futures for short tasks:
    
    ```vegalite
    
    --8<-- "docs/benchmarks/eager_vs_deferred.json"
    
    ```
    
    - The library achieves a similar overhead for eager and deferred futures, with 
      deferred futures being 1.00719 faster.
    - Most overhead comes from the executor, which is a thread pool in this example
    - Invoking the task directly or through the inline executor has a similar cost

Most of the time, the choice between eager and lazy futures is determined by the application. When both eager and lazy
futures are applicable, a few criteria might be considered.

- Eager futures:
    - ✅ They have the obvious benefit of allowing us to already start with the tasks we know about before
      assembling the complete execution graph. This is especially useful when not all tasks are available at the same
      time.
    - ✅ The library implements the synchronization of eager futures using atomic operations and
      custom allocators to reduce this synchronization cost.
- Deferred futures:
    - ✅ Lazy futures allow a few optimizations for functions operating on the shared
      state, since we can assume there is nothing else we need to synchronize when a task is launched.
      This includes no dynamic allocations for the shared state and less synchronization overhead.
    - ✅ If the application allows, there's no reason not to use them.
- Both futures type:
    - ✅ For applications with reasonably long tasks, the difference between the two is likely to be
      negligible.

!!! info "Deferred futures in C++"

    The advantage of callbacks functions is that the calling function provides the continuation *before*
    the antecedent task starts. Thus, it's easy to see this model can serve as the foundation for an
    alternative model of futures in two ways which would also avoid the synchronization overhead of
    continuations:
    
    - [Completion Tokens](https://www.boost.org/doc/libs/1_77_0/doc/html/boost_asio/reference/asynchronous_operations.html):
      The calling function in the callback model provides
      a [custom tag](https://cppalliance.org/richard/2021/10/10/RichardsOctoberUpdate.html#asio-and-the-power-of-completion-tokens)
      indicating the initiating function should return a future type representing the result of the second operation.
    - [Lazy futures](https://github.com/facebookexperimental/libunifex/blob/main/doc/concepts.md#starting-an-async-operation):
      Likewise, lock-free continuations can be implemented as a (deferred) future type such that the continuation is
      guaranteed to be available when the first task starts.
    
    This is what these models would look like:
    
    === "Continuable Futures"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // async_write returns some continuable future type
            some_future_lib::async_write(socket_, asio::buffer(data_, length))
                // synchronization cost: check if the current continuation is being read 
                .then(schedule_read_function);
        }
        ``` 
    
    === "Callbacks"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // async_write returns void
            asio::async_write(socket_, asio::buffer(data_, length), 
                // no synchronization cost: `async_write` has not started yet 
                schedule_read_function);
        }
        ``` 
    
    === "Completion Token"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // async_write returns some continuable future type defined by use_custom_future
            asio::async_write(socket_, asio::buffer(data_, length), 
                // synchronization cost: dependent on custom future type 
                use_custom_future).then(schedule_read_function);
        }
        ``` 
    
    === "Callback Futures"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // async_write returns some callback future type that always knows its continuation 
            some_future_lib::async_write(socket_, asio::buffer(data_, length), 
                // no synchronization cost: no extra continuations might be attached 
                schedule_read_function);
        }
        ``` 
    
    === "Lazy Futures"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // async_write returns some deferred future type that we know is not executing yet 
            some_future_lib::deferred_async_write(socket_, asio::buffer(data_, length)) 
                // no synchronization cost: async_write starts after the continuation is attached 
                .then(schedule_read_function).detach();
        }
        ``` 
    
    In other words, while this synchronization requirement of [std::future] is a problem with [std::future] and
    other [proposed](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf) continuable futures, such
    as [std::experimental::future], this is **not a problem** with the concept of futures. This synchronization cost is only an
    issue if we cannot *guarantee* the execution of the previous function has not started when the continuation is
    attached.

!!! hint "Deferred Future Continuations"

    The library assumes continuations to deferred futures are always attached *before*
    the future starts executing. 

    This strategy uses deferred futures as an oportunity for an extra optimization where 
    we don't have to synchronize the access to continuations by the thread attaching the
    continuation and the main task. 

    If the deferred future is already executing in one thread, this means it is *not thread safe*
    to attempt to attach a continuation in a second thread.      

!!! info "Senders and Receivers"

    To differentiate the "Future" concept proposed in P1055 from a [std::future],
    [P1194](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1194r0.html) proposes the name "**senders**" to represent
    the "**Deferred**" concept defined in [P1055](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1055r0.pdf). The
    paper also proposes to rename the function `then` to `submit` to suggest the possibility that it may in fact submit the
    task for execution.
    
    More recently, [P2300](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2300r1.html) and [libunifex] propose a
    model based on "senders" and "receivers" for asynchronous operations. From
    a [computer science perspective](#in-computer-science), senders and receivers are constraints for **futures and promises**.
    From a C++ perspective, these are defined as [concepts](https://en.cppreference.com/w/cpp/language/constraints) rather
    than object types. For this reason, the syntax of sender types is still analogous to futures:
    
    === "Futures and Promises"
    
        ```cpp
        auto ex = get_thread_pool().executor();
        
        auto f = async(ex, []{
            std::cout << "Hello world! Have an int.";
            return 13;
        }).then([](int arg) { return arg + 42; });                                                                           
        
        int i = f.get();
        ```
    
    === "Senders and Receivers"
    
        ```cpp
        scheduler auto sch = get_thread_pool().scheduler();
        
        sender auto begin = schedule(sch);
        sender auto hi_again = then(begin, []{
            std::cout << "Hello world! Have an int.";
            return 13;
        });
        sender auto add_42 = then(hi_again, [](int arg) { return arg + 42; });
        
        auto [i] = this_thread::sync_wait(add_42).value();
        ```
    
    In parallel with [discussions](https://isocpp.org/files/papers/P2464R0.html) about the proper model for C++ executors,
    senders/receivers have received **criticism** for attempting to abruptly reformulate common practice in C++ asynchronous
    computing. The most common objections:
    
    1. _Lack of existing practice_:
       Most [libunifex algorithms](https://github.com/facebookexperimental/libunifex/blob/main/doc/overview.md#different-sets-of-algorithms)
       are currently labeled as "not yet implemented".
    2. _Unnecessarily reinventing the wheel_: a [sender/receive](https://www.google.com/search?q=senders+and+receivers)
       still represents a [future/promise](https://www.google.com/search?q=futures+and+promises) from a computer science
       perspective.
    3. _Unnecessary deviation from common patterns_: it does not match the asynchronous model of other programming languages
       and C++ historical constructs, which might be unnecessarily confusing.
    4. _Unnecessary complexity_: it is difficult to foresee what problems we are going to have with this model
       before [libunifex] is completely implemented, and the C++ standard is not a good place for experimentation.
    
    Coming from other programming languages or even from the historical C++ constructs, it is hard to distinguish
    what `begin` semantically represents and why it is the same type as other tasks in constructs such as:
    
    === "Futures and Promises"
    
        ```cpp
        auto f = async(ex, []{ 
            std::cout << "Hello world!"; 
            return 13; 
        }).then([](int arg) { 
            return arg + 42;
        });
        ```
    
    === "Senders and Receivers v1"
    
        ```cpp
        sender auto begin = libunifex::schedule(sch);
        sender auto hello = libunifex::then(begin, []{
            std::cout << "Hello world!";
            return 13;
        });
        sender auto f = libunifex::then(hello, [](int arg) { 
            return arg + 42; 
        });
        ```
    
    === "Senders and Receivers v2"
    
        ```cpp
        sender auto f = schedule(sch) 
                        | libunifex::then([]{
                             std::cout << "Hello world!";
                             return 13;
                          }) 
                        | libunifex::then([](int arg) { 
                             return arg + 42; 
                          });
        ```

!!! info "Senders and Futures"

    Although senders are still (deferred) futures from a Computer Science perspective, 
    [P2300 Section 1.10.1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2300r4.html#intro-prior-art-futures)
    briefly describes two reasons for preferring senders over futures ("as traditionally realized"):
    
    - futures require the dynamic allocation and management of a shared state
    - futures require type-erasure of work and continuation
    
    As it should be clear from this review, from a [computer science perspective](#in-computer-science), senders _are_
    futures, even though they don't represent what was "traditionally realized" in [std::future]. All optimizations possible
    for "single-shot senders" are also possible in "unique deferred futures", which are also provided by this library.

## Ready tasks

When assembling task graphs, it's often useful to include constant values for which we already know the result but
behave like a future type. This can be achieved through [make_ready_future]:

{{ code_snippet("test/unit/snippets.cpp", "ready_future") }}

The function returns a [vfuture], which represents a [basic_future] with no associated operation state extensions.

--8<-- "docs/references.md"