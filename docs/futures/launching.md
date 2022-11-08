# Launching tasks

## Future tasks

The easiest way to create futures is by launching tasks:

{{ code_snippet("tests/unit/snippets.cpp", "no_param") }}

This is what happens under the hood:

<div class="mermaid">
graph LR
M[[Main Thread]] ==> |store|F[Future Value]
E[[Executor]] --> |run|T[Task]
M -.-> |launch|T
subgraph Futures and Promises
F --> |read|S[(Operation State)]
T[Promised Task] --> |write|S
end
</div>

1. An executor handles a task. Any data is stored in a private operation state.
2. While the executor handles a task, the main thread holds a future value.
3. When the task is completed, it fulfills its promise by setting the operation state with its result.
4. The future is considered ready and the main thread can obtain its value.

This is how these three steps might happen:

<div class="mermaid">
sequenceDiagram
    rect rgb(191, 200, 255)
    Main->>Task: Launch
    Task->>State: Create
    activate Task
    State->>Future: Create
    Task->>Task: Executor runs task
    Future->>Main: Store
    end
    Main->>Main: Do work before waiting
    rect rgb(191, 223, 200)
    Main->>Future: Wait
    Future->>State: Wait
    Task->>State: Write
    deactivate Task
    Future->>State: Read
    end
    rect rgb(150, 223, 255)
    Future->>Main: Return
    end
</div>

!!! hint "The Operation State"

    The operation state is a private implementation detail with 
    which the user does not interact. 

    This encapsulation ensures all write operations will happen
    through the promised task and all read operations will happen
    through the future.

    It also enables optimizations based on assumptions about how
    specific future and promise types can access the operation 
    state.

!!! hint "Inline Operation States"

    If these future and promise objects are stable during the execution
    of the task, the operation state can be stored inline. This is 
    an optimization that avoids dynamic memory allocations.    

## Eager tasks

Like [std::async], [futures::async] is used to launch new tasks in parallel.

{{ code_snippet("tests/unit/snippets.cpp", "no_param") }}

In this example, this function returns a continuable [cfuture] by default instead of a [std::future]. If the first task
parameter is a [stop_source], it returns a [jcfuture] we can stop from the main thread with `request_stop`.

{{ code_snippet("tests/unit/snippets.cpp", "no_param_jcfuture") }}

If the task accepts parameters, we can provide them directly to [async].

{{ code_snippet("tests/unit/snippets.cpp", "with_params") }}

Unlike [std::async], which uses launch policies, [futures::async] can use any concrete [executor] specifying details
about how the task should be executed.

{{ code_snippet("tests/unit/snippets.cpp", "with_executor") }}

If the [executor] is not defined, the default executor is used for the tasks. This executor ensures we do not launch a
new thread for each task.

To block execution of the main thread until the tasks are complete, we can use the functions [basic_future::wait]
and [basic_future::get].

{{ code_snippet("tests/unit/snippets.cpp", "waiting") }}

The function [basic_future::wait] will only block execution until the task is ready, while [basic_future::get] can be
used to wait for and retrieve the final value.

Note that eager tasks are allowed to start as soon as they are launched:

<div class="mermaid">
sequenceDiagram
    Main->>+Task: Launch
    Task->>Task: Do work
    Main->>Main: Do work
    Main->>Task: Wait
    Task->>-Main: Return
</div>

!!! info "Callbacks and Eager Futures in C++"

    In [N3747](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3747.pdf)
    , [Christopher Kohlhoff](https://github.com/chriskohlhoff) compares the model of eager continuations, such as
    in [N3784](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf) to the model of **callback functions** used in
    the [Asio](https://github.com/chriskohlhoff/asio) library:
    
    === "Futures"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // the antecedent future represents the future result of `async_write`
            some_future_lib::async_write(socket_, asio::buffer(data_, length))
                // The continuation function creates a new future
                .then(schedule_read_function);
        }
        ``` 
    
    === "Callbacks"
    
        ```cpp
        void schedule_write(std::size_t length) {
            // async_write returns void
            asio::async_write(socket_, asio::buffer(data_, length), 
                // the continuation is provided when `async_write` schedules the first task
                schedule_read_function);
        }
        ``` 
    
    Eager continuable futures may attach the continuation *before or after* the antecedent future starts. With
    callbacks functions the calling function provides the continuation *before* the antecedent task starts, which
    **avoids the synchronization cost** in a race between the result and its continuation.



## Deferred tasks

The function [schedule] can be used to create lazy tasks.

{{ code_snippet("tests/unit/snippets.cpp", "schedule") }}

The main difference between [async] and [schedule] is the latter only posts the task to the executor when we
call [basic_future::wait] or [basic_future::get] on the corresponding future:

<div class="mermaid">
sequenceDiagram
    Main->>Task: Schedule
    Main->>+Task: Wait
    Task->>Task: Do work
    Task->>-Main: Return
</div>

Most of the time, the choice between eager and lazy futures is determined by the application. When both eager and lazy
futures are applicable, a few criteria might be considered.

- Eager futures ✅: have the obvious benefit of allowing us to already start with the tasks we know about before
  assembling the complete execution graph. This is especially useful when not all tasks are available at the same time.
- Deferred futures ✅: On the other hand, lazy futures permit a few optimizations for functions operating on the shared
  state, since we can assume there is nothing else we need to synchronize when a task is launched. This means:
    1. We can make extra assumptions about the condition of the operation state before waiting, and
    2. We don't have to handle a potential race with the task and its own continuations
- Eager futures ✅: The library implements the synchronization of eager futures using atomic operations to reduce this
  synchronization cost.
- Both futures ✅: For applications with reasonably long tasks, the difference between the two is likely to be
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

{{ code_snippet("tests/unit/snippets.cpp", "ready_future") }}

The function returns a [vfuture], which represents a [basic_future] with no associated operation state extensions.

## Executors

The concept defined for an [executor] uses Asio executors as defined
by [P1393r0](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1393r0.html). This model for executors has been
evolving for over a decade and is widely adopted in C++. If the C++ standard eventually adopts a common vocabulary for
executors, the [executor] concept can be easily adjusted to handle these new executors types.

!!! info "Executors in C++"

    The first clear problem identified with [std::async] is we **cannot define its executor** (or scheduler). That is, we cannot
    define _where_ these tasks are executed. By default, every task is executed in a new thread in C++11, which is
    unacceptable to most applications. Common executors for these tasks would be thread pools, strands, or GPUs.
    
    Many [models for standard executors](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html) have been
    proposed [over time](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html#appendix-executors-bibilography)
    . In most current models, executors (or schedulers) are **light handles** representing an execution context _where_ work can
    be executed. The context is usually called an execution context or execution resource. Executors might be constructed
    directly from execution contexts or adapted from other executors.

!!! info "Better future executors"

    - Support for custom executors
        - Its traits are currently a subset of existing Asio executors.
    - Customization points can make other executor types work with the library types
    - Provides an alternative to [std::async] **based on [executors](#executors)**
        - Still adaptable to other executor or scheduler concepts, including senders and receivers.
        - The default executor does NOT create a new thread for each task



## Exceptions

Future objects defined in this library handle exceptions the same way [std::future] does. If the task throws an
exception internally, the exception is rethrown when we attempt to retrieve the value from the future
with [basic_future::get].

{{ code_snippet("tests/unit/snippets.cpp", "throw_exception") }}

When working without exceptions, we can avoid terminating the process by querying the state of the future before
attempting to get its value.

{{ code_snippet("tests/unit/snippets.cpp", "query_exception") }}

## Allocations

The operation state is a private implementation detail with which the user does not interact. To ensure stability,
it is usually implemented as a shared pointer to the concrete Operation State, which is where the task will store its
result.

<div class="mermaid">
graph LR
F[Future] --> |read|S1[Shared State Pointer]
T[Promise] --> |write|S2[Shared State Pointer]
subgraph Shared State
S1 --> O[Operation State]
S2 --> O[Operation State]
end
</div>

In the general case, the operation state needs a stable address so that futures and promises can access it. In turn,
this requires dynamic memory allocations of this shared state. For smaller tasks, the cost of this allocation might
dominate the time spent by the parallel task.

For this reason, functions used for launching futures allow custom memory allocators for eager tasks. When no
allocator is provided for launching tasks, an optimized memory pool allocator is provided for the operation state.

<div class="mermaid">
graph LR
F[Future] --> |read|S1[Shared State Pointer]
T[Promise] --> |write|S2[Shared State Pointer]
subgraph Shared State
S1 --> O[Operation State 1]
S2 --> O[Operation State 1]
end
P[Memory Pool] -.-> |owns|O[Operation State 1]
P[Memory Pool] -.-> |owns|O2[Operation State 2]
P[Memory Pool] -.-> |owns|O3[Operation State 3]
P[Memory Pool] -.-> |owns|Odots[...]
P[Memory Pool] -.-> |owns|ON[Operation State N]
</div>

The memory pool allows fast dynamic memory allocation and provides better cache locality for accessing the tasks
and their results. 

However, in some circumstances, the library implements a few optimizations to avoid allocations altogether. In the
following example, we have a deferred future where no allocations are required.

{{ code_snippet("tests/unit/snippets.cpp", "no_alloc") }}

In this example, the operation state might be stored inline with the future:

<div class="mermaid">
graph RL
F[Future<br>+<br>Operation State] --> |read|F
T[Promise] --> |write|F
</div>

With this optimization, the operation state is stored in the future, so the future can read the results from its own
operation state. The promise can use the address of the operation state in the future to write its result. In this case,
the promise is **connected** with the future and can store results directly in its operation state.

This optimization is only applicable if the address of the future will not change, i.e.: will not be moved or destroyed,
during task execution, i.e.: (i) after the task to set the operation state is launched and (ii) before the operation
state is set. This is easiest to achieve in deferred futures, because the thread waiting for its result is blocked when
we call [basic_future::wait] and the future can only be moved or destroyed after the underlying operation state is set.

Note that this optimization is only possible if we can ensure the future cannot be moved or destroyed during task
execution:

- Functions such as [basic_future::wait_for] will disable this optimization for deferred futures because the underlying
  object might be moved after [basic_future::wait_for] times out and during task execution.
- This optimization can be enabled for eager futures that we know should not be moved or destroyed during task
  execution. This is common in applications represented as classes that store their future instances. If we mistakenly
  attempt to move or destroy the future during execution, it blocks the calling thread until the operation state is set.
- Similarly, this optimization can be enabled for shared futures if the future object from where the task is launched is
  not moved or destroyed during execution.

--8<-- "docs/references.md"