# Stoppable Futures

<div class="mermaid">
graph LR
M[[Main Thread]] ==> |store|F[Future Value]
M -.-> |request stop!|F
E[[Executor]] --> |run|T[Task]
M -.-> |launch|T
subgraph Futures and Promises
F --> |read|S[(Shared State <br> + <br> Stop State)]
F -.-> |request stop!|S
T[Task] --> |write|S
T -.-> |should stop?|S
end
</div>


When calling [async] with a callable that can be called with a [stop_token] as its first argument, it returns [jcfuture], which contains a [stop_source]:  
 
```cpp
--8<-- "examples/future_types/stoppable.cpp"
```

--8<-- "docs/references.md"