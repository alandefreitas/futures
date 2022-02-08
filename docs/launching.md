# Launching tasks

## Futures tasks

The easiest way to create futures is by launching tasks.  

<div class="mermaid">
graph LR
M[[Main Thread]] ==> |store|F[Future Value]
E[[Executor]] --> |run|T[Task]
M -.-> |launch|T
subgraph Futures and Promises
F --> |read|S[(Shared State)]
T[Promised Task] --> |write|S
end
</div>

1. While an executor handles a task, the main thread holds a future value.
2. When the task is completed, it fulfills its promise by setting the shared state with its result. 
3. The future is considered ready and the main thread can obtain its value.

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

Note that the shared state is a private implementation detail with which the user does not interact.
This ensures all write operations will happen through the promised task and all read operations
will happen through the future. It also enables optimizations based on assumptions about
how specific future and promise types can access the shared state.

## Eager tasks

Like [std::async], [futures::async] is used to launch new tasks.  

```cpp
--8<-- "examples/future_types/launching.cpp"
```

However, [futures::async] implements a few improvements and extensions to [std::async]:

- Its launch policy is replaced with a concrete [executor].
- If the [executor] is not defined, a default executor (a thread pool) is used for the tasks, instead of always launching a new thread for the task
- It returns a continuable [cfuture] by default instead of a [std::future]
- If the first task parameter is a [stop_source], it returns a [jcfuture]

Note that eager tasks are allowed to start as soon as they are launched:

<div class="mermaid">
sequenceDiagram
    Main->>+Task: Launch
    Task->>Task: Do work
    Main->>Main: Do work
    Main->>Task: Wait
    Task->>-Main: Return
</div>


## Lazy tasks

The function [schedule] can be used to create lazy tasks.

```cpp
--8<-- "examples/future_types/schedule.cpp"
```

The difference between [async] and [schedule] is the latter only posts the task to the executor when we call wait
on the corresponding future. 

Note that lazy tasks are allowed to start only after we announce we are waiting for its results:

<div class="mermaid">
sequenceDiagram
    Main->>Task: Schedule
    Main->>+Task: Wait
    Task->>Task: Do work
    Task->>-Main: Return
</div>

Most of the time, the choice between eager and lazy futures is determined by the 
application.

When both eager and lazy futures are applicable, a few criteria might be considered. 

- Eager futures ✅: have the obvious benefit of allowing us to already start with the tasks we know about 
  before assembling the complete execution graph. This is especially useful when not all tasks are available at the same time. 
- Deferred futures ✅: On the other hand, lazy futures permit a few optimizations for functions operating on the shared state, 
  since we can assume there is nothing else we need to synchronize when a task is launched. This means:
    1. We can make extra assumptions about the condition of the shared state before waiting, and
    2. We don't have to handle a potential race with the task and its own continuations
- Eager futures ✅: The library implements the synchronization of eager futures using atomic operations to reduce 
  this synchronization cost. 
- Both futures ✅: For applications with reasonably long tasks, the difference between the two is likely to be negligible.  

!!! hint "Deferred Future Continuations"

    The library assumes continuations to deferred futures are always attached *before*
    the future starts executing. 

    This strategy uses deferred futures as an oportunity for an extra optimization where 
    we don't have to synchronize the access to continuations by the thread attaching the
    continuation and the main task. 

    If the deferred future is already executing in one thread, this means it is *not thread safe*
    to attempt to attach a continuation in a second thread.      

## Executors

The concept defined for an [executor] uses Asio executors. This model for executors has been evolving for over a 
decade and is widely adopted in C++. If the C++ standard eventually adopts a common vocabulary for executors, 
the [executor] concept can be easily adjusted to handle these new executors.

## Exceptions

Future objects defined in this library handle exceptions the same way [std::future] does. If the task throws an
exception internally, the exception is rethrown when we attempt to retrieve the value from the future. 

--8<-- "docs/references.md"