# Motivation

## Summary

**C++11** presented [std::future] as its model for
asynchronicity. [Futures and promises](https://en.wikipedia.org/wiki/Futures_and_promises)
are [a common construct from the 70s](https://iucat.iu.edu/iub/1810628) where an object represents a value that is still
unknown. By composing with the future objects, this construct allows us to synchronize the program execution in
concurrent programming. On the other hand, [many works](#literature-review) describe the specific C++11 [std::future]
model as the wrong abstraction for asynchronous programming.

Since then, many **proposals** have been presented to extend this standard [std::future] model, such as future
continuations, callbacks, lazy/eager execution, cancellation tokens, custom executors, custom allocators, and waiting on
destruction. However, because any handle to a future value is a "future" object, it is unlikely that a single concrete
future definition will be appropriate for most applications.

This library attempts to [solve this problem](#the-futures-library) by defining generic algorithms for **a
common [is_future] concept**, which includes 1) existing future types, such as [std::future], `boost::future`
, `boost::fiber::future`; 2) library provided future types, such as [cfuture] and [jfuture]; and 3) custom future types.

The concepts allow reusable algorithms for all future types, an alternative to [std::async] based on executors, various
efficient future types, many future composition algorithms, a syntax closer to other programming languages, and parallel
variants of the STL algorithms.

## Outline

Our motivation is organized as follows:

- [An introduction](#standard-futures) to the model of futures and promises in C++11
- [An overview](#previous-work) of all related work presented since then
- [A brief description](#the-futures-library) of how this library relates this previous work

Alternatively, you can move straight to [our guides](futures/future_types.md) and see how the library works in practice.
Be welcome.

## Standard Futures

### In Computer Science

A future _or_ promise is a value that might not be available yet. They are also called "deferred", "delay", or simply
"task" objects in some contexts. This storage for this value might be provided by the future object, the promise
object, _or_ by some form shared state between objects related to task execution. A shared state would usually be
accessed by the set setting the future value.

- 1977: The first mention of Futures was by [Baker and Hewitt](https://dl.acm.org/doi/10.1145/872736.806932). These
  futures would contain a process, a memory location for the result, and a list of continuations.
- 1978: The term Promises is used by [Daniel P. Friedman and David Wise](https://ieeexplore.ieee.org/document/1675100)
  for the same concept.
- 1985: [Multilisp](https://dl.acm.org/doi/10.1145/4472.4478) provided the future and delay annotations for values that
  might not be available yet. A variable with the delay annotation would only be calculated when its value was
  requested.
- 1988: The term Promises is used by [Liskov and Shrira](https://dl.acm.org/doi/10.1145/960116.54016) for a similar
  construct in Argus. It also proposed "call-streams" to represent directed acyclic graphs (DAGs) of computation with
  promises.
- 1996: The term "eventual" is used
  by [Tribble, Miller, Hardy, & Krieger](http://www.erights.org/history/joule/MANUAL.BK2.pdf) to represent promises of
  "eventual" send value into a variable.
- 2002: The Python [Twisted](https://github.com/twisted/twisted) library presents Deferred objects for results of
  operations that might still be incomplete.
- 2009: The Javascript [CommonJS Promises/A spec](http://wiki.commonjs.org/wiki/Promises/A) is proposed by Kris Zyp.

The formulation implies the future value might always be in completed or incomplete _atomic_ states. As all the
formulations presented above, a language might have a single construct called future _or_ promise for eventual values.
Javascript defines the single `Promise` that acts like a read-only future value. When they are distinct, like in C++11,
it's common to have two constructs: futures _and_ promises, where one of them is a read-only reference to the expected
value.

Common asynchronous applications of futures are servers, user input, long-running computations, database queries, remote
procedure calls, and timeouts. A number of variations of this pattern have emerged with slightly different meaning for
the terms. These variations are used as a model of asynchronous operations in many languages such as JavaScript, Scala,
Java, and C++.

### In Modern C++

C++11 proposed, in [N3170](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3170.html), [std::future] as its
intended model of [futures and promises](https://en.wikipedia.org/wiki/Futures_and_promises) for asynchronicity. A
C++ [std::future] represents a read-only proxy for a result that might not been set by a write-only [std::promise] yet.

When the operation is complete, the result is stored in a shared state, to which both the future and the promise have
access. The future is a read-only view of this shared state while a promise allows an external operation to set its
value.

<div class="mermaid">
graph LR
F[Future] --> |read|S[(Shared State)]
T[Promised Task] --> |write|S
</div>

In the most common use case, this promise is hidden. The user would call [std::async] to execute a task is parallel and
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
asynchronous programming somewhat similar and often even simpler to that of other programming languages, such
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
use, inefficient, and lacks the usual generality of C++ algorithms.

## Previous work

### Executors

The first clear problem identified with [std::async] is we cannot define its executor (or scheduler). That is, we cannot
define _where_ these tasks are executed. By default, every task is executed in a new thread in C++11, which is
unacceptable to most applications. Common executors for these tasks would be thread pools, strands, or GPUs.

Many [models for standard executors](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html) have been
proposed [over time](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html#appendix-executors-bibilography)
. In most current models, executors (or schedulers) are light handles representing an execution context _where_ work can
be executed. The context is usually called an execution context or execution resource. Executors might be constructed
directly from execution contexts or adapted from other executors.

### Continuations

The act of waiting for a [std::future] result is synchronous, which is not appropriate in communication-intensive code.
In the original [std::future] model, if a continuation task `B` depends on the result of the first task `A`, we only
have two options:

- waiting for the first task synchronously
- polling the first task asynchronously.

If we always wait for the first task to start its continuation, the asynchronicity has no purpose. If we always poll for
the first task, we waste resources and an extra thread to repeatedly check the status of the first task.

For this reason, the most common extension proposed for [std::future]
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

### Lazy Continuations

Lazy continuations, such as in [std::experimental::future], allow us to asynchronously register a second operation and
pass data to it. So in the above example, the continuation is scheduled as soon as, but not before, the first task is
ready without consuming any polling threads. The continuation can also have its continuations and so on.

A lazy continuation stored in the shared state of the first task and all futures are programmed to run its internal
continuations when they finish the main task. This avoids blocking waits and wasting threads pooling for the results of
the antecedent task. A continuation is scheduled and executed only when the previous task completes.

=== "Lazy Continuations"

    ```cpp
    auto A = std::experimental::async([]() { return 2; });
    auto B = A.then([](int A_result) {
        // This task is not scheduled until A completes
        std::cout << A_result << std::endl;
    });
    B.wait();
    ```

If the antecedent future throws an exception, some models besides [C++ Extensions for Concurrency], such as
[Continuable](https://github.com/Naios/continuable), allow the continuation to also catch this error:

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

By default, the first task usually includes an executor handle and the continuation inherits it unless some other
executor is requested for the continuation. Futures with continuations can also be used as components
of [resumable functions](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3650.pdf).

Finally, continuations are the foundation for composing task graphs, with operations such
as [std::experimental::when_all] and [std::experimental::when_any]. These conjunction and disjunction operations depend
on continuations so that previous tasks can inform the operation result when they are ready without polling.

### Task Callbacks

In [N3747](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3747.pdf)
, [Christopher Kohlhoff](https://github.com/chriskohlhoff) compares the model of lazy continuations, such as
in [N3784](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf) to the model of callback functions used in
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

Continuable futures may attach the continuation *before or after* the antecedent future starts. With callbacks functions
the calling function provides the continuation *before* the antecedent task starts, which avoids the synchronization
cost in a race between the result and its continuation.

### Deferred futures

The advantage of callbacks functions is that calling function provides the continuation *before* the antecedent task
starts. Thus, it's easy to see this model can serve as the foundation for an alternative "futures" model in two ways
which would also avoid the synchronization overhead for continuations:

- [Completion Tokens](https://www.boost.org/doc/libs/1_77_0/doc/html/boost_asio/reference/asynchronous_operations.html):
  The calling function in the callback model provides
  a [custom tag](https://cppalliance.org/richard/2021/10/10/RichardsOctoberUpdate.html#asio-and-the-power-of-completion-tokens)
  indicating the initiating function should return a future type representing the result of the second operation.
- [Lazy futures](https://github.com/facebookexperimental/libunifex/blob/main/doc/concepts.md#starting-an-async-operation):
  Likewise, lock-free continuations can be implemented as a future type such that the continuation is guaranteed to be
  available when the first task starts.

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
as [std::experimental::future], this is not a problem with the concept of futures. This synchronization cost is only a
problem if we cannot *guarantee* the execution of the previous function has not started when the continuation is
attached.

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

Tasks in a task graph are analogous to deferred futures whose continuations are defined before the execution starts.
However, we need to explicitly define all relationships between tasks before any execution starts, which might be
inconvenient in some applications. Futures and async functions, on the other hand, allow us to 1) combine eager and lazy
tasks, and 2) directly express their relationships in code without any explicit graph containers.

On the other hand, [P1055](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1055r0.pdf) proposed the concept of
deferred work, in opposition to eager futures, such as [std::future]. The idea is that a task related to a future should
not start before its continuation is applied. This eliminates the race between the result and the continuation in eager
futures. Futures with deferred work are lock-free and easier to implement ([example](https://godbolt.org/z/jWYno73nE)).

### Senders and Receivers

To differentiate this "Future" concept proposed in P1055 from a [std::future],
[P1194](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1194r0.html) proposes the name "senders" to represent
the "deferred" concept defined in [P1055](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1055r0.pdf). The
paper also proposes to rename the function `then` to `submit` to suggest the possibility that it may in fact submit the
task for execution.

More recently, [P2300](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2300r1.html) and [libunifex] propose a
model based on "senders" and "receivers" for asynchronous operations. From
a [computer science perspective](#in-computer-science), senders and receivers are constraints for futures and promises.
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
senders/receivers have received criticism for attempting to abruptly reformulate common practice in C++ asynchronous
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

## The futures library

Given the pros and cons of each model described in our [review](#previous-work), this library models future types as a
simple [is_future] concept, which supports all the features we have discussed, such as executors, continuations and
deferred work. We implement optimizations possible to each future types while sticking to existing practice.

- Futures and Promises
    - This library maintains the common model most programmers are familiar with
    - Whenever possible, new features always use language familiar to C++ and other common programming languages
- Future concept
    - By modeling futures as a concept, futures can be optionally instantiated with any combination of custom extensions
      such as continuations, stop tokens, and deferred work
    - Reusable algorithms **work for** [**all future types**](#standard-futures), including any new and existing future
      types, such as [std::future], `boost::future`, `boost::fiber::future`.
    - Futures can still match and work with whatever constraints C++ eventually imposes on future types
- Executors
    - Custom executors can be defined and its traits are a subset of existing Asio executors.
    - Customization points can make other executor types work with the library types
    - Provides an alternative to [std::async] **based on [executors](#executors)** while still adaptable to other
      executor or scheduler concepts, including senders and receivers.
- Extensions
    - [basic_future] implements a **vast number of concrete future types**, including continuable, cancellable,
      eager/deferred, and unique/shared futures.
    - Future types support [lazy **continuations**](#lazy-continuations) without polling for existing types
    - The future adaptors still work for existing future types, such as [std::future] by polling.
    - Integrations with Asio are provided, such as completion tokens and async IO operations.
- Data races
    - Both eager and lazy futures are lock-free through atomic operations.
    - Future types might indicate [**deferred** work](#deferred-futures) at compile-time or be eager, avoiding the
      synchronization costs of attaching continuations and other optimizations.
    - Eager futures use lock-free operations and data structures to minimize synchronization costs.
- Adaptors
    - Implements a large set of [**composition** operations](#senders-and-receivers) with a **syntax closer to the
      existing future types** users are used to, such as [when_all] and [when_any]
    - Adaptors are also provided to facilitate the creation of cyclic task graphs
- Algorithms
    - Includes a large set of the **STL-like algorithms in terms of futures and executors**
    - Easy access to async algorithms based on executors without requiring external libraries,
      as [common](https://link.springer.com/chapter/10.1007/978-1-4842-4398-5_4) with C++ execution policies, such
      as [TTB].

--8<-- "docs/references.md"