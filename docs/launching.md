# Launching tasks

Like [std::async], [futures::async] is used to launch new tasks. However, [futures::async] implements a few improvements and extensions to [std::async]:

- Its [launch] policy might be replaced with a concrete [executor]  
- If the [launch] policy or [executor] is defined, a default executor (a thread pool) is used for the tasks, instead of always launching a new thread
- It returns a continuable [cfuture] instead of a [std::future]
- If the first task parameter is a [stop_source], it returns a [jcfuture] 

```cpp
--8<-- "examples/launching.cpp"
```
 
## Executors

The concept defined for an [executor] uses Asio executors. This model for executors has been evolving for over a decade and is widely adopted in C++. If the C++ standard eventually adopts a common vocabulary for executors, the [executor] concept can be easily adjusted to handle these new executors.

## Exceptions

Future objects defined in this library handle exceptions the same way [std::future] does. If the task throws an exception internally, the exception is rethrown when we attempt to retrieve the value from the future. 


--8<-- "docs/references.md"