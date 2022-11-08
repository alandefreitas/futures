# Continuable Futures

Continuations are the most common extension to future objects. A continuable shared state provides the future instance
with write access to attach continuations to a task. When the task sets the value of the shared state, all continuations
are run.

<div class="mermaid">
graph LR
M[[Main Thread]] ==> |store|F[Future Value]
M -.-> |attach continuation|F
E[[Executor]] --> |run|T[Task]
M -.-> |launch|T
subgraph Futures and Promises
F --> |read|S[(Shared State <br> + <br> Continuations)]
F -.-> |attach continuation|S
T[Task] --> |set and continue|S
end
</div>

## Motivation

### Non-continuable tasks

Consider what happens when we launch a task with C++11 [std::async]:

{{ code_snippet("tests/unit/snippets.cpp", "std_async") }}

When we only need a single parallel task with future types, the main thread is allowed to wait or do some other work
while the task is running.

<div class="mermaid">
sequenceDiagram
    Main->>+Task: Launch
    Main->>Main: Do work
    Main->>Task: Wait
    activate Main
    Note left of Main: Time spent waiting
    Task->>-Main: Return
    deactivate Main
</div>

In this example, the main thread spent some time waiting but this is often OK, as long as it had nothing better to do
but to wait for the asynchronous task. This is common in user interfaces that need to be refreshed while a longer
background task is running.

Now let's say we want to execute a sequence of asynchronous tasks as simple as:

<div class="mermaid">
graph LR
subgraph Async
A --> B --> C
end
Main --> A
C --> End
Main --> End
</div>

As we shall see, [std::async] does not provide the mechanisms to make this happen properly. The first alternative that
usually comes to mind is waiting for one task after launching the next.

{{ code_snippet("tests/unit/snippets.cpp", "wait_for_next") }}

The code might look reasonable but, in that case, we would have:

<div class="mermaid">
sequenceDiagram
    Main->>+A: Launch
    Main->>A: Wait
    activate Main
    A->>-Main: Return
    deactivate Main
    Main->>+B: Launch
    Main->>B: Wait
    activate Main
    B->>-Main: Return
    deactivate Main
    Main->>+C: Launch
    Main->>C: Wait
    activate Main
    C->>-Main: Return
    deactivate Main
</div>

We have a number of problems here. The more tasks we have, and the shorter the tasks, the less time the main thread has
to do any useful work before waiting and the more time it spends waiting for tasks. Even worse, we already know how
these tasks should continue: we are just waiting to attach this continuation. At a certain point, it might not even be
worth using asynchronous code at all.

### Polling

The second alternative to solve this problem is polling. In this case, we would make task B wait for A before doing its
work. The same for task B and C.

{{ code_snippet("tests/unit/snippets.cpp", "polling") }}

And now we have:

<div class="mermaid">
sequenceDiagram
    Main->>+A: Launch
    Main->>+B: Launch
    Main->>+C: Launch
    B->>A: Wait
    C->>B: Wait
    Main->>Main: Do work
    A->>-B: Return
    B->>-C: Return
    Main->>C: Wait
    C->>-Main: Return
</div>

This might look more reasonable from the perspective of the main thread. We are not waiting for so long inline, and we
have more time to do work in parallel. However, this outsources the cost of waiting to other threads even though we know
the initial task is not ready. Note for how long the tasks A, B, and C are active in this example.

Thus, the biggest problem with this strategy is it cannot scale properly. For every task in our application, we would
need one idle thread waiting for the previous task. In an application with 2000 tasks, we would need 1999 threads for
polling antecedent tasks and only one thread would to execute real work.

!!! info "Continuations in C++ libraries"

    The act of waiting for a [std::future] **result is synchronous**, which is not appropriate in communication-intensive code.
    In the original [std::future] model, if a continuation task `B` depends on the result of the first task `A`, we only
    have two options:
    
    - waiting for the first task synchronously
    - polling the first task asynchronously.
    
    If we always wait for the first task to start its continuation, the asynchronicity has **no purpose**. If we always poll for
    the first task, we **waste resources** and an extra thread to repeatedly check the status of the first task.
    
    For this reason, the most common **extension** proposed for [std::future]
    is [continuations](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf), such as implemented in
    Microsoft's [PPL], [async++], [continuable].
    
    === "Waiting"
    
        ```cpp
        std::future A = std::async([]() { return 2; });
        int A_result = A.get();
        std::cout << A_result << std::endl;
        ```
    
    === "Polling"
    
        ```cpp
        std::future A = std::async([]() { return 2; });
        std::future B = std::async([&]() {
            int A_result = A.get();
            std::cout << A_result << std::endl;
        });
        B.wait();
        ```
    
    === "Continuations"
    
        ```cpp
        auto A = std::experimental::async([]() { return 2; });
        auto B = A.then([](int A_result) {
            std::cout << A_result << std::endl;
        });
        B.wait();
        ```

    Continuations are the foundation for composing task graphs, with operations such
    as [std::experimental::when_all] and [std::experimental::when_any]. These conjunction and disjunction
    operations depend on continuations so that previous tasks can inform the operation result when they
    are ready without polling.

## Continuable futures

Continuable futures allow us to launch a second task as a continuation to the first task, instead of an independent
task.

{{ code_snippet("tests/unit/snippets.cpp", "continuables") }}

Note we can use both the member function [basic_future::then] or the free function [then]. [basic_future::then] allows
chaining while the free function [then] allows interoperability between future types.

{{ code_snippet("tests/unit/snippets.cpp", "chaining") }}

In these examples,

- it's up to the continuation to wait for the previous task, and
- it's up to the previous task to launch its own continuations

In other words, task B does not have to pool task A because task A is launching task B. Task B knows A is ready and can
just take it from there.

<div class="mermaid">
sequenceDiagram
    Main->>A: Launch
    activate A
    Main-->>B: Attach to A
    Main-->>C: Attach to B
    A->>B: Launch
    deactivate A
    activate B
    B->>C: Launch
    deactivate B
    activate C
    Main->>Main: Do work
    Main->>C : Wait
    activate Main
    C->>Main: Return
    deactivate Main
    deactivate C
</div>

In this solution, the main thread has more time to do useful work, such as scheduling other tasks, and the processing
time we spend on waiting is minimized.

!!! hint "Continuation Unwrapping"

    You might notice that continuation functions also unwrap the previous future value. A continuation function for
    `future<A>` might have a parameter `future<A>` or `A`. More patterns of continuation unwrapping are described in
    Section [adaptors/continuations](/futures/adaptors/continuations). 

For this reason, continuations are one of the most common proposed extensions for [std::future], including the original
model presented by
the [Microsoft PPL Library](https://docs.microsoft.com/en-us/cpp/parallel/concrt/parallel-patterns-library-ppl?redirectedfrom=MSDN&view=msvc-160)
which inspired the [C++ Extensions for Concurrency].

!!! info "Eager Future Continuations in C++"

    _Eager_ futures with _eager_ continuation chaining, such as in [std::experimental::future], allow us to **asynchronously**
    register a second operation and pass data to it. The first task **might already be running** eagerly.
    
    - The continuation is attached after the first task is scheduled
    - The continuation is scheduled as soon as, but not before, the first task is ready
    
    The process does not consume any polling threads. The continuation can also have its continuations and so on.
    In this scenario, attaching a continuation has its own synchronization cost.


## Deferred continuations

The process of attaching continuations to a future whose main task is potentially executing has a synchronization cost.
When attaching a continuation, we need to check if the future is not currently attempting to run the continuations and
vice-versa. This synchronization cost was identified
in [N3747](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3747.pdf).

The library implements this procedure with [atomic](https://en.cppreference.com/w/cpp/atomic/atomic) operations to avoid
this cost. However, in some contexts, the cost of continuations can be further minimized
by [launching deferred futures](/futures/launching/).

{{ code_snippet("tests/unit/snippets.cpp", "deferred_continuables") }}

The continuation to a deferred shared state created with [schedule] is also deferred by default. When the task related
to any shared state is deferred, we have a different sequence of events:

<div class="mermaid">
sequenceDiagram
    Main->>A: Schedule
    Main-->>B: Attach to A
    Main-->>C: Attach to B
    Main->>C : Wait
    activate Main
    C->>B : Request start
    B->>A : Request start
    activate A
    B->>A: Wait
    A->>B: Return
    deactivate A
    activate B
    C->>B: Wait
    B->>C: Return
    deactivate B
    activate C
    Main->>Main: Do work
    C->>Main: Return
    deactivate C
    deactivate Main
</div>

In this case, the synchronization cost can be completely removed because the task will only be sent to the executor once
its continuations have already been attached to it. Notice how tasks B and C are never waiting at the same time.

Deferred futures can also avoid the list of continuations all together. When requesting a deferred continuation to
start, it can simply wait for the previous task inline before sending its own task to the executor.

While an eager future stores its continuations:

<div class="mermaid">
graph LR
subgraph Eager futures
A --> |store|B --> |store|C
end
</div>

A deferred continuation can store the previous task:

<div class="mermaid">
graph LR
subgraph Deferred futures
C --> |store|D --> |store|A
end
</div>

Thus, deferred futures without explicit continuation lists can still have lazy continuations, as the continuation task
will store its previous task, forming a chain of tasks in the shared state of these objects. This allows deferred
futures to have the member function [basic_future::then] defined even we no continuation list is available.

In fact, this is safer than continuation lists for deferred futures. Let A and B be deferred tasks. Because they are not
eager, task A will _not be launched before task B_ and then post task B to the executor. When we wait for task B, it
will be launched, which would take room in the executor. Task B, already running, will need to wait for task A. At the
point, however, the executor might not have the enough capacity for launching task A because B is already running and
polling A:

<div class="mermaid">
sequenceDiagram
    Main ->> A: Create
    Main ->> B: Create
    A ->> B: Store as continuation
    Main ->> B: Wait
    B ->> B: Post task
    B ->> A: Wait (using executor)
    A ->> A: Do work (might fail)
    A ->> B: Return
    B ->> B: Do work
</div>

The library solves this problem by checking for previous continuations of A before starting B. But deferred
continuations can solve the problem natively:

<div class="mermaid">
sequenceDiagram
    Main ->> A: Create
    Main ->> B: Create
    B ->> A: Stores inline
    Main ->> B: Wait
    B ->> A: Wait inline
    A ->> A: Do work (executor is free)
    A ->> B: Return
    B ->> B: Post task
    B ->> B: Do work
</div>

For this reason, by default, [then] attaches the previous future to its deferred continuation instead of attaching the
continuation to the antecedent future.

!!! info "Lazy future continuations in C++"

    _Lazy_ futures with _lazy_ continuation chaining store the continuation in the shared state of the
    first task **before** the task is scheduled.
    
    - The continuation is attached before the first task starts to execute
    - The continuation is scheduled as soon as, but not before, the first task is ready
    
    As usual, all futures are programmed to run its internal continuations when they finish their
    main task. This also avoids blocking waits and wasting threads pooling for the results of the
    antecedent task. In this scenario, attaching a continuation has no synchronization cost.
    
    === "Continuations"
    
        ```cpp
        auto A = std::experimental::async([]() { return 2; });
        auto B = A.then([](int A_result) {
            // This task is not scheduled until A completes
            std::cout << A_result << std::endl;
        });
        B.wait();
        ```

!!! info "Exceptions and continuations in C++"

    If the antecedent future throws an exception, attempting to retrieve the result usually rethrows the error.
    Some models besides [C++ Extensions for Concurrency], such as [Continuable](https://github.com/Naios/continuable),
    allow the continuation to also catch this error:
    
    === "Catching errors"
    
        ```cpp
        async([]{ /* operation that might throw an error */ })
          .then([] {
            throw std::exception("Some error");
          })
          .fail([] (std::exception_ptr ptr) {
              try {
                std::rethrow_exception(ptr);
              } catch(std::exception const& e) {
                // Handle the exception or error code here
              }
          });
        ```

!!! info "Continuations and Executors in C++"

    By default, the first task usually includes an executor handle and the continuation inherits it unless some other
    executor is requested for the continuation. Futures with continuations can also be used as components
    of [resumable functions](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3650.pdf).


--8<-- "docs/references.md"