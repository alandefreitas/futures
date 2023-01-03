# Futures types

## Concrete types

This library implements a number of future types as a *concept* rather than a single concrete object.
For instance, consider a task we are launching with C++11 [std::async]:

{{ code_snippet("test/unit/snippets.cpp", "interop_std_async") }}

We could do something similar with the library function [futures::async]:

{{ code_snippet("test/unit/snippets.cpp", "interop_cfuture") }}

Notice how [async] returns a [cfuture] by default, which is a future to which we can attach continuations.
The function [async] does not always return [cfuture]. For instance, suppose we call [async] with a
function that accepts a [stop_token]:

{{ code_snippet("test/unit/snippets.cpp", "interop_jcfuture") }}

It now returns [jcfuture] by default, which is a future type that supports stop tokens.
We can ask the task to stop through the future at any time with:

{{ code_snippet("test/unit/snippets.cpp", "interop_jcfuture_stop") }}

We now have three types of future objects. Because all of these types are [future_like],
the library functions allow them to interoperate:

{{ code_snippet("test/unit/snippets.cpp", "types_wait_for_all") }}

!!! info "Futures and Promises in Computer Science" 

    A future _or_ promise is a value that might not be available yet. They are also called "deferred", "delay", or simply
    "task" objects in some contexts. This storage for this value might be provided by the future object, the promise
    object, _or_ by some form shared state between objects related to task execution. A shared state would usually be
    accessed by the set setting the future value.
    
    - **1977 - Futures**: The first mention of Futures was by [Baker and Hewitt](https://dl.acm.org/doi/10.1145/872736.806932). These
      futures would contain a process, a memory location for the result, and a list of continuations.
    - **1978 - Promises**: The term Promises is used by [Daniel P. Friedman and David Wise](https://ieeexplore.ieee.org/document/1675100)
      for the same concept.
    - **1985 - Multilisp**: [Multilisp](https://dl.acm.org/doi/10.1145/4472.4478) provided the future and delay annotations for values that
      might not be available yet. A variable with the delay annotation would only be calculated when its value was
      requested.
    - **1988 - Argus**: The term Promises is used by [Liskov and Shrira](https://dl.acm.org/doi/10.1145/960116.54016) for a similar
      construct in Argus. It also proposed "call-streams" to represent directed acyclic graphs (DAGs) of computation with
      promises.
    - **1996 - Eventual**: The term "eventual" is used
      by [Tribble, Miller, Hardy, & Krieger](http://www.erights.org/history/joule/MANUAL.BK2.pdf) to represent promises of
      "eventual" send value into a variable.
    - **2002 - Python**: The Python [Twisted](https://github.com/twisted/twisted) library presents Deferred objects for results of
      operations that might still be incomplete.
    - **2009 - Javascript**: The Javascript [CommonJS Promises/A spec](http://wiki.commonjs.org/wiki/Promises/A) is proposed by Kris Zyp.
    
    The formulation implies the future value might always be in completed or incomplete **_atomic_ states**. As all the
    formulations presented above, a language might have a single construct called future _or_ promise for eventual values.
    Javascript defines the single `Promise` that acts like a read-only future value. When they are distinct, like in C++11,
    it's common to have two constructs: futures _and_ promises, where one of them is a read-only reference to the expected
    value.
    
    Common asynchronous **applications** of futures are servers, user input, long-running computations, database queries, remote
    procedure calls, and timeouts. A number of variations of this pattern have emerged with slightly different meaning for
    the terms. These variations are used as a model of asynchronous operations in many languages such as JavaScript, Scala,
    Java, and C++.

## Summary

For instance, these are some concrete future classes defined in this library and their comparison with [std::future]:

| Class             | Lazy Continuations | Stoppable | Shared |
|-------------------|--------------------|-----------|--------|
| [future]          | ❌                  | ❌         | ❌     |
| [shared_future]   | ❌                  | ❌         | ✅     |
| [jfuture]         | ❌                  | ✅         | ❌     |
| [shared_jfuture]  | ❌                  | ✅         | ✅     |
| [cfuture]         | ✅                  | ❌         | ❌     |
| [shared_cfuture]  | ✅                  | ❌         | ✅     |
| [jcfuture]        | ✅                  | ✅         | ❌     |
| [shared_jcfuture] | ✅                  | ✅         | ✅     |

- Like [std::shared_future], multiple threads are allowed to wait for the same shared state of all shared future types.
- Future types with lazy continuations allow new functions to be appended to the end of the current function being
  executed. This allows these continuations to run without launching a new task to the executor. If the continuation
  should run in a new executor, it allows the continuation to be scheduled only once the current task is completed.
- Stoppable future types contain an internal [stop_source], similar to [std::stop_source]. This allows the future to
  directly receive stop requests, while the internal future task can identify a stop request has been made through
  a [stop_token], similar to [std::stop_token]. This replicates the model of [std::jthread] for futures.

!!! info "What problem does this library solve?"

    - **The problem**: **C++11** presented [std::future] as its model for
      asynchronicity. [Futures and promises](https://en.wikipedia.org/wiki/Futures_and_promises)
      are [a common construct from the 70s](https://iucat.iu.edu/iub/1810628) where an object represents a value that is still
      unknown. By composing with the future objects, this construct allows us to synchronize the program execution in
      concurrent programming. On the other hand, [many works](#literature-review) describe the specific C++11 [std::future]
      model as the wrong abstraction for asynchronous programming.
    - **Solutions**: Since then, many **proposals** have been presented to extend this standard [std::future] model, such as future
      continuations, callbacks, lazy/eager execution, cancellation tokens, custom executors, custom allocators, and waiting on
      destruction. However, because any handle to a future value is a "future" object, it is unlikely that a single concrete
      future definition will be appropriate for most applications.
    - **This library**: This library attempts to [solve this problem](#the-futures-library) by defining generic algorithms for **a
      common [future_like] concept**, which includes 1) existing future types, such as [std::future], `boost::future`
      , `boost::fiber::future`; 2) library provided future types, such as [cfuture] and [jfuture]; and 3) custom future types.
    - **Results**: The concepts allow reusable algorithms for all future types, an alternative to [std::async] based on executors, various
      efficient future types, many future composition algorithms, a syntax closer to other programming languages, and parallel
      variants of the STL algorithms.

## Adaptor types

Some other future types are:

| Class             | Description                                   |
| ----------------- |-----------------------------------------------|
| **Value future**  |                                               | 
| [vfuture]         | Hold a single value as a future               |
| **Adaptors**      |                                               | 
| [when_all_future] | Represent the conjunction of other futures    | 
| [when_any_future] | Represent the disjunction of other futures    | 

Unlike other future types, [when_all_future] and [when_any_future] are proxy future classes to hold the results of
the [when_all] and [when_all] functions. This proxy allows different future types to interoperate and save resources on
new tasks.

## Custom future types

Any custom type satisfying the [future_like] requirements can interoperate with other future type. These classes might
represent any process for which a result will only be available in the future, such as child processes and network
requests with third-party libraries.

A simpler alternative for custom future types is through specific template instantiations of [basic_future], which can
be configured at compile-time with [future_options]. Many future types provided by the library as aliases [basic_future]
with specific [future_options].

!!! info "Futures in C++11"

    C++11 proposed, in [N3170](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3170.html), [std::future] as its
    intended model of [futures and promises](https://en.wikipedia.org/wiki/Futures_and_promises) for asynchronicity. A
    C++ [std::future] represents a **read-only proxy** for a result that might not been set by a write-only [std::promise] yet.
    
    When the operation is complete, the result is stored in a **shared state**, to which both the future and the promise have
    access. The future is a read-only view of this shared state while a promise allows an external operation to set its
    value.
     
    In the most common use case, this **promise** is hidden. The user would call [std::async] to execute a task is parallel and
    set the promise at the end. After the task is scheduled, [std::async] would then return a [std::future] which allows us
    to query the status of this asynchronous operation:
    
    === "`std::async`"
    
        ```cpp
        std::future<int> f = std::async([]{ return 8; });
        std::cout << "Result is: " << f.get() << '\n'; // 8
        ```
    
    === "`std::promise`"
    
        ```cpp
        std::promise<int> p;
        std::future<int> f = p.get_future();
        // any async operation, such as std::async, can set this value
        p.set_value(8); 
        std::cout << "Result is: " << f.get() << '\n'; // 8
        ```
    
    One satisfactory thing about the combination of [std::future], [std::async], and [std::future::wait] is it makes C++
    asynchronous programming somewhat **similar** and often even **simpler** to that of other programming languages, such
    as [Javascript](https://developer.mozilla.org/en-US/docs/Learn/JavaScript/Asynchronous/Choosing_the_right_approach)
    futures and promises:
    
    === "C++ Futures and Promises"
    
        ```cpp
        std::future<int> f = std::async([]{ return 8; });
        std::cout << "Result is: " << f.get() << '\n'; // 8
        ```
    
    === "JS Futures and Promises"
    
        ```js
        // std::future<int> f = std::async([]{ return 8; });
        let f = new Promise(function(resolve, reject) { resolve(8) });
        // std::cout << "Result is: " << f.get() << '\n'; // 8
        f.then( function(value) { console.log('Result is: ' + value) } )
        ```
    
    === "JS Async / Await"
    
        ```js
        // std::future<int> f = std::async([]{ return 8; });
        // `async` makes my_function `return Promise.resolve(8)`
        async function my_function() { return 8 };
        let f = my_function()
        // std::cout << "Result is: " << f.get() << '\n'; // 8
        let value = await f
        console.log('Result is: ' + value)
        ```
    
    However, because [std::future] relies on synchronization of a shared state, this initial model is incomplete, hard to
    use, **inefficient**, and lacks the usual generality of C++ algorithms. Most implementations could be used for
    useful for a background threads but are unusable in any context where performance is a requirement. The usual
    [std::future] will create one thread per task, which either doesn't scale at all or kills the machine.

<div class="mermaid">
graph LR
F[Future] --> |read|S[(Shared State)]
T[Promised Task] --> |write|S
</div>

!!! info "Better future types"

    This documentation includes historical notes about many models proposed for asynchronicity in C++.
    Given the pros and cons of each model described in this review, this library models future types as a
    simple [future_like] concept/trait, which supports all the features we have discussed in this overview, 
    such as executors, continuations, and deferred work. We implement optimizations possible to each 
    future types while sticking to existing practice.

    - This library maintains the common model most programmers are familiar with
    - Whenever possible, new features always use language familiar to C++ and other common programming languages
    - Whenever possible, we reuse the syntax of [std::future]/[std::promise]

!!! info "Better future concepts"

    - Reusable algorithms **work for** [**all future types**](#standard-futures),
        - [std::future]
        - `boost::future`
        - `boost::fiber::future`
        - Any new and existing future type
    - Concrete futures can be optionally instantiated with any combination of custom extensions, such as:
        - Eager/deferred work
        - Unique/shared
        - Continuations
        - Cancellation
        - Allocators
    - All optimizations possible for "senders" are also implemented for "deferred futures".
    - Synchronization costs are reduced with deferred futures, atomic operations, and atomic data structures
    - Futures can still match and work with whatever constraints C++ eventually imposes on future types



--8<-- "docs/references.md"
