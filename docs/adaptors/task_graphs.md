# Task graphs

It is quite common to compose task graphs with future adaptors. In many of these graphs, some continuation tasks might
need to recur to a task that has already been executed in the graph.

## Directed acyclic graphs

Say we want to execute the following task graph:

<div class="mermaid">
graph LR
subgraph Async
    B -.-> |false|D
    A --> B
    B -.-> |true|C
end
Main --> A
C --> End
D --> End
Main --> End
</div>

This is not an uncommon pattern in asynchronous applications:

<div class="mermaid">
graph LR
subgraph Async
    B[Attempt] -.-> |false|D[Handle error]
    A[Prepare] --> B
    B -.-> |true|C[Handle success]
end
</div>

We can combine future adaptors to directly express task graphs without cycles. In this case, our problem is that the
adaptor [then] only supports a single continuation. What we need is another continuation that defines which continuation
to execute. This is achieved by using the continuations to launch new tasks.

{{ code_snippet("adaptors/task_graph.cpp", "dag") }}

Task `A` can attach its continuation `B` as usual. However, task `B` needs to check a condition before launching `C` or
`D`. The process of checking this condition becomes the continuation to `B`: a continuation to decide which continuation
to launch.

However, note that we use an [inline_executor] for checking the condition. Deciding what task to launch is a cheap
operation, and we do not want to send another task to the underlying executor to do that. This inline execution
effectively makes the continuation behave as a light callback.

This pattern works because futures and promises enable representations equivalent to an implicit task queue. Any task we
launch is going to this implicit queue. Continuations make reference to earlier objects in the queue. Earlier tasks can
decide what to push to this queue.

## Rescheduling tasks

Say we want to execute the following task graph:

<div class="mermaid">
graph LR
subgraph Async
    B -.-> |false|B
    A --> B
    B -.-> |true|C
end
Main --> A
C --> End
Main --> End
</div>

This kind of pattern is not uncommon for tasks that might fail

<div class="mermaid">
graph LR
subgraph Async client
    B[Make request] -.-> |false|B
    A[Prepare request] --> B
    B -.-> |true|C[Handle response]
end
</div>

or tasks that should be split into smaller homogeneous tasks:

<div class="mermaid">
graph LR
subgraph Async client
    B[Read some] -.-> |true|B
    A[Handle response] --> B
    B -.-> |false|C[Write response]
end
</div>

The previous pattern will not work for this graph because `B` needs to have recursive access to the task that launches
itself. This simplest way to store these recursive functions is with a `struct` or `class`.

{{ code_snippet("adaptors/task_graph.cpp", "reschedule_struct") }}

The graph contains a single promise whose value we will set when the complete subgraph is executed. Note that we can't
simply wait for C outside the graph because its future instance is not valid until B succeeds. The future C will not
exist yet rather than simply not being ready. So we start the task graph by launching A.

{{ code_snippet("adaptors/task_graph.cpp", "reschedule_start") }}

A is launched as soon as we start. We use the [inline_executor] for light callbacks as in the previous example. We
detach the callback function to because we don't need to wait for this task. We only need to wait for the final promise.

The process of scheduling B is modularized into another function because we need to access it recursively. This works as
usual:

{{ code_snippet("adaptors/task_graph.cpp", "reschedule_schedule_B") }}

The light callback for B works as in the previous example for DAGs. The only difference here is `schedule_B` might need
to call itself when it fails. The `struct` makes this recursion easier to access.

If the operation is successful and we schedule task C, it simply handles the operation result and sets the promise we
created.

{{ code_snippet("adaptors/task_graph.cpp", "schedule_C") }}

Outside the graph, we can just wait for the promise to be set.

{{ code_snippet("adaptors/task_graph.cpp", "wait_for_graph") }}

In practice, we would probably attempt to reschedule B a number of times and a stop token could be attached to the graph
to allow us to request it to stop at any time. These scheduling functions are usually going to be interleaved with the
application logic. For instance, a web client would also use this object to store variables related to the state of the
request.

When we compare this model with implicit queues, the complete asynchronous operation as become a subgraph that
effectively represents a single subtask in this task queue. In fact, if we know the executor we are going to use is
modelled as an explicit task queue, such as `asio::io_context`, we don't even need the promise because we can just pop
tasks from the queue until there are no tasks left. At this point, we implicitly know task C has been executed.

The promise makes the subgraph itself behave as a single future in the implicit task queue. The graph members could be
encapsulated into a class, and the functions `get`/`wait` could be provided to request the value of the promise. In this
case, we would have one more complete future type matching the [is_future] concept. This type would be able to interact
with other futures through the library future adaptors.

## Loops in graphs

The pattern above can also be reused for asynchronous loops. Say we want to execute the following task graph:

<div class="mermaid">
graph LR
subgraph Async
    B -.-> |false|A
    A --> B
    B -.-> |true|C
end
Main --> A
C --> End
Main --> End
</div>

This kind of pattern is not uncommon for tasks that run continuously:

<div class="mermaid">
graph LR
subgraph Async server session
    B[Write] -.-> |completed|A[Read]
    A -.-> |completed|B
    B --> |disconnect|C[Close Session]
    A -.-> |read more|A
    B -.-> |write more|B
end
Listen -.-> |Connected|A
Listen -.-> |Connected|Listen
</div>

In this example, we have rescheduling because reading and writing longer message needs to be split into smaller tasks.
The server also needs to launch another listening task while it serves that client. We also have loops because after
writing a response we might need to read more requests or disconnect the client.

What we have here is a conditional continuation that might move backwards in case of failure. The logic for recursively
rescheduling `A` is the same as the logic for rescheduling `B`. We define this logic in a separate function and
recursively call it. The new `struct` would be:

{{ code_snippet("adaptors/task_graph.cpp", "loop_struct") }}

The logic to schedule A is now moved into another function because we need to reuse it. A schedules B as usual.

{{ code_snippet("adaptors/task_graph.cpp", "loop_schedule_A") }}

This time, in case of failure, task B moves back to task A instead of rescheduling.

{{ code_snippet("adaptors/task_graph.cpp", "loop_schedule_B") }}

Task C sets the promise as usual.

{{ code_snippet("adaptors/task_graph.cpp", "loop_schedule_C") }}

We also wait for the graph as usual.

{{ code_snippet("adaptors/task_graph.cpp", "loop_wait_for_graph") }}

Note that this pattern of implicit task graphs is easy enough to generalize as an explicit task graph object. An
explicit task graph needs to be aware of the tasks it might launch (its vertices) and the connections between tasks (its
edges).

However, there are many reasons not to reuse such a graph object. A graph object would make it more difficult to
interleave data related to the application logic with the tasks and the intermediary tasks usually have different types.
We could either recur to template instantiations for each possible graph combination or type erase these differences.
Both alternatives are more expensive and verbose than directly creating functions to recursively reschedule tasks.

!!! info "Task graphs in C++"

    Libraries such as [Taskflow] and [TTB] provide facilities to compose complete task graphs:
    
    === "Futures"
    
        ```cpp
        std::future A = std::async([] () { 
            std::cout << "TaskA\n"; 
        });
        
        // A runs before B and C
        std::future B = std::async([&A] () { 
            A.wait(); // Polling :( 
            std::cout << "TaskB\n";
        });
        
        std::future C = std::async([&A] () { 
            A.wait(); // Polling :( 
            std::cout << "TaskC\n"; 
        });
        
        // D runs after B and C
        std::future D = std::async([&B, &C] () { 
            B.wait(); // Polling :(
            C.wait(); // Polling :(
            std::cout << "TaskD\n";
        });
        
        D.wait(); 
        ```
    
    === "Continuable Futures"
    
        ```cpp
        std::future A = std::async([] () { 
            std::cout << "TaskA\n"; 
        });
        
        // A runs before B and C
        std::future B = A.then([] () { // Synchronization cost :( 
            std::cout << "TaskB\n"; // No polling :) 
        });
        
        std::future C = A.then([] () { // Synchronization cost :( 
            std::cout << "TaskC\n"; // No polling :)
        });
        
        // D runs after B and C
        std::future D = some_future_lib::when_all(B, C).then([] () { 
            std::cout << "TaskD\n"; 
        });
        
        D.wait(); 
        ```
    
    === "Taskflow"
    
        ```cpp
        tf::Executor executor;
        tf::Taskflow taskflow;
        
        auto [A, B, C, D] = g.emplace(
          [] () { 
            std::cout << "TaskA\n"; // No eager execution 
          },
          [] () { 
            std::cout << "TaskB\n"; // No eager execution 
          },
          [] () { 
            std::cout << "TaskC\n"; // No eager execution 
          },
          [] () { 
            std::cout << "TaskD\n"; // No eager execution 
          } 
        );
              
        A.precede(B, C); // No synchronization cost :)  
        D.succeed(B, C); // No synchronization cost :)
        
        executor.run(g).wait(); 
        ```
    
    === "TTB"
    
        ```cpp
        graph g;
        
        function_node<void> A( g, 1, [] () { 
            std::cout << "TaskA\n"; // No eager execution 
        } );
        
        function_node<void> B( g, 1, [] () { 
            std::cout << "TaskB\n"; // No eager execution 
        } );
        
        function_node<void> C( g, 1, [] () { 
            std::cout << "TaskC\n"; // No eager execution 
        } );
        
        function_node<void> D( g, 1, [] () { 
            std::cout << "TaskD\n"; // No eager execution 
        } );
        
        make_edge(A, B); // No synchronization cost :) 
        make_edge(A, C); // No synchronization cost :) 
        make_edge(B, D); // No synchronization cost :) 
        make_edge(C, D); // No synchronization cost :) 
        
        g.wait_for_all();
        ```
    
    Tasks in a task graph are **analogous to deferred futures** whose continuations are defined before the execution starts.
    However, we need to explicitly define all relationships between tasks before any execution starts, which might be
    inconvenient in some applications. Futures and async functions, on the other hand, allow us to 1) combine eager and lazy
    tasks, and 2) directly express their relationships in code without any explicit graph containers.
    
    On the other hand, [P1055](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1055r0.pdf) proposed the concept of
    **deferred work**, in opposition to eager futures, such as [std::future]. The idea is that a task related to a future should
    not start before its continuation is applied. This eliminates the race between the result and the continuation in eager
    futures. Futures with deferred work are also easier to implement ([example](https://godbolt.org/z/jWYno73nE)).




--8<-- "docs/references.md"