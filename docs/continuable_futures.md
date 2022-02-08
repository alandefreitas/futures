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

## Non-continuable tasks

When we only need a single parallel task with future types, the main thread is allowed to wait or do
some other work while the task is running.

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

In this example, the main thread spent some time waiting but this is often OK, as long as it had nothing better
to do but to wait for the asynchronous task. This is common in user interfaces that need to be refreshed
while a longer task is running.

Now say we want to execute a sequence of asynchronous tasks as simple as:

<div class="mermaid">
graph LR
subgraph Async
A --> B --> C
end
Main --> A
C --> End
Main --> End
</div>

As we shall see, [std::async] does not provide the mechanisms to make this happen properly. The first alternative
that comes to mind is obviously waiting for one task after launching the next, where we would have:

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

We have a number of problems here. The more tasks we have, and the shorter the tasks, the less time the main thread
has to do any useful work before waiting and the more time it spends waiting for tasks. Even worse, it's waiting for tasks
we already know how they should continue. At a certain point, it's not even worth using asynchronous code at all.

## Polling

The second alternative to solve this problem is polling. In this case, we would make task B wait for A before doing
its work. The same for task B and C.

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

This looks more reasonable. The main thread is not waiting for so long, and it has more time to do work.
However, this outsources the cost of waiting to other threads while we know the initial task is not ready.
Note for how long the tasks A, B, and C are active in this example.

This, the biggest problem with this strategy is that cannot scale properly. For every task in our application,
we would need one idle thread waiting for the previous task. For instance, in an application with 2000 tasks,
we would need to create 1999 tasks for polling the previous task and only one thread would be executing the 
current task. 

## Continuable futures

Continuable futures allow us to launch a second task as a continuation to the first task, instead of an independent
task.
 
```cpp
--8<-- "examples/future_types/continuable.cpp"
```

In this example, it's to up to the continuation to wait for the previous task. It's up to the previous task
to launch its own continuations. In other words, task B does not have to wait for task A because task A is
launching task B, which can just take it from there.

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

In this solution, the main thread has more time to do useful work, such as scheduling other tasks, and 
the processing time we spend on waiting is minimized. 

For this reason, continuations are one of the most common proposed extensions for [std::future], including
the original model presented by the [Microsoft PPL Library](https://docs.microsoft.com/en-us/cpp/parallel/concrt/parallel-patterns-library-ppl?redirectedfrom=MSDN&view=msvc-160).

## Deferred continuations

The process of attaching continuations to a future whose main task is potentially executing 
has a synchronization cost. The process attaching the continuation needs to check the future is not
attempting to run the continuations and vice-versa. This synchronization cost was identified
in [N3747](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3747.pdf).

The library implements this procedure with atomic queues to avoid this cost. However, in some contexts, 
the cost of continuations can be further minimized by [launching deferred futures](/futures/launching/).

<div class="mermaid">
sequenceDiagram
    Main->>A: Schedule
    Main-->>B: Attach to A
    Main-->>C: Attach to B
    Main->>C : Wait
    activate Main
    C->>A : Request
    activate A
    A->>B: Launch
    deactivate A
    activate B
    B->>C: Launch
    deactivate B
    activate C
    Main->>Main: Do work
    C->>Main: Return
    deactivate C
    deactivate Main
</div>

In this case, the synchronization cost can be completely removed because the task will
only be sent to the executor once its continuations have already been attached to it. 

--8<-- "docs/references.md"