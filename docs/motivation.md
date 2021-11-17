# Motivation

## Summary

C++11 presented [std::future] as its model for asynchronicity. [Futures and promises](https://en.wikipedia.org/wiki/Futures_and_promises) are a common construct where an object represents a value that is still unknown. By composing with the future objects, this construct allows us to synchronize the program execution in concurrent programming. On the other hand, [many works](#literature-review) describe the specific C++11 [std::future] model as the wrong abstraction for asynchronous programming.

Since then, many proposals have been presented to extend this standard [std::future] model, such as future continuations, callbacks, lazy/eager execution, cancellation tokens, custom executors, custom allocators, and waiting on destruction. However, because any handle to a future value is a "future" object, it is unlikely that a single concrete future definition will be appropriate for most applications. 

This library attempts to [solve this problem](#the-futures-library) by defining generic algorithms for a common [is_future] concept, which includes 1) existing future types, such as [std::future], `boost::future`, `boost::fiber::future`; 2) library provided future types, such as [cfuture] and [jfuture]; and 3) custom future types.
 
The concepts allow reusable algorithms for all future types, an alternative to [std::async] based on executors, various efficient future types, many future composition algorithms, a syntax closer to other programming languages, and parallel variants of the STL algorithms.

## Standard Futures

C++11 [proposed](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2010/n3170.html) [std::future] as its intended model for asynchronicity. In the common use case, the user would call [std::async] to execute a task is parallel. This [std::async] function would then return a [std::future] which allows us to query the status of this asynchronous operation:

=== "`std::async`"

    ```cpp
    std::future<int> f = std::async([]{ return 8; });
    std::cout << "Result is: " << f.get() << '\n'; // 8
    ```

=== "`std::promise`"

    ```cpp
    std::promise<int> p;
    std::future<int> f = p.get_future();
    p.set_value(8); // any async operation can set this value
    std::cout << "Result is: " << f.get() << '\n'; // 8
    ```

One satisfactory thing about the combination of [std::future], [std::async], and [std::future::wait] is it makes C++ asynchronous programming somewhat similar to that of other languages, such as [Javascript](https://developer.mozilla.org/en-US/docs/Learn/JavaScript/Asynchronous/Choosing_the_right_approach) promises. However, because [std::future] relies on synchronization of a shared state, this initial model is incomplete, hard to use, inefficient, and lacks the usual generality of C++ algorithms.

## Previous work

### Executors

The first clear problem identified with [std::async] is we cannot define an executor (or scheduler). That is, we cannot define _where_ these tasks are executed. By default, every task is executed in a new thread in C++11, which is unacceptable to most applications. Common executors for these tasks would be thread pools, strands, or GPUs.

Many [models for standard executors](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html) have been proposed [over time](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p0443r14.html#appendix-executors-bibilography). In most current models, executors (or schedulers) are light handles representing a context _where_ work can be executed. The context is usually called an execution context or execution resource. Executors might be constructed directly from execution contexts or adapted from other executors.

### Lazy Continuations and Composability

The act of waiting for a [std::future] result is synchronous, which is not appropriate in communication-intensive code. In the original [std::future] model, if the continuation task `B` depends on the result of task `A`, we only have two options: waiting or polling for `A`. If we wait for task `A` to start its continuation `B`, the asynchronicity has no purpose. If we poll `A` for its results, we waste resources and an extra thread to repeatedly check the status of `A`. For this reason, the most common extension proposed for [std::future] is [continuations](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf), such as implemented in Microsoft's [PPL], [async++], [continuable]. 

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
    auto A = futures_lib::async([]() { return 2; });
    auto B = A.then([](int A_result) {
        std::cout << A_result << std::endl;
    });
    B.wait();
    ```

Lazy continuations allow a [std::experimental::future] to asynchronously register a second operation and pass data to it. So in the above example, task `B` is scheduled as soon as `A` is ready without consuming any polling threads. `B` could also have its continuations with `B.then()` and so on. The continuation avoids blocking waits and wasting threads pooling for the results of the antecedent task. A continuation is scheduled and executed only when the previous task completes. 

If the antecedent [std::future] throws an exception, some models allow the continuation to also catch this error. By default, the continuation usually inherits its executor from the antecedent future. The `then` function also includes an optional parameter for a custom executor or launch policy. Futures with continuations can also be used as components of [resumable functions](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3650.pdf).

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


Finally, continuations are the foundation for composing tasks, with operations such as [std::experimental::when_all] and [std::experimental::when_any]. These conjunction and disjunction operations depend on continuations so that previous tasks can inform the operation result when they are ready without polling.

### Lazy Futures and Callbacks

In [N3747 - A Universal Model for Asynchronous Operations](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3747.pdf), [Christopher Kohlhoff](https://github.com/chriskohlhoff) (2013) discusses some of these extensions proposed for [std::future], such as [continuations](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf), and compares them with callback functions as a low-overhead alternative to futures. This model has been used successfully for a long time in Christopher's well-known [Asio](https://github.com/chriskohlhoff/asio) library. Code in this callback model would typically look like this:

=== "Futures"

    ```cpp
    void schedule_write(std::size_t length) {
        // async_write returns some future type
        future_lib::async_write(socket_, asio::buffer(data_, length))
            .then(schedule_read_function).detach();
    }
    ``` 

=== "Callbacks"

    ```cpp
    void schedule_write(std::size_t length) {
        // async_write returns void
        asio::async_write(socket_, asio::buffer(data_, length), 
            schedule_read_function);
    }
    ``` 

In continuable futures, the antecedent future represents the result of the initiating function `async_write`. The antecedent can receive a continuation functor that creates a new future. Assuming this future type joins at destruction, we `detach` it because there's no reason to synchronously wait for the results of `async_write`, since the continuation `schedule_read_function` we need is already going to be scheduled. With callback functions, the continuation `schedule_read_function` is already provided by the time the `async_write` operation starts.

- Continuable futures: The calling function may attach the continuation functor *before or after* the antecedent future starts. 
- Callbacks function: The calling function provides the continuation functor *before* the antecedent future starts. 

This guarantee that the continuation functor is already present by the time the antecedent task finishes allows us to reduce synchronization costs, and this means callbacks 1) have *better performance* and 2) can be used as a building block for futures. This happens because in continuable futures the continuation functor may be attached *before or after* the initiating function is over. This requires the continuation attachment and continuation invocation to be coordinated: 1) we cannot call the continuation function before checking if a continuation is being attached, and 2) we attach a continuation before checking if the continuations are being called.  

With callback functions defined at construction, the continuation is known to always be available and there is no synchronization overhead. The callback model can also serve as the foundation for an alternative "futures" model in two ways: 

- [Completion Tokens](https://www.boost.org/doc/libs/1_77_0/doc/html/boost_asio/reference/asynchronous_operations.html): The calling function provides a [custom tag](https://cppalliance.org/richard/2021/10/10/RichardsOctoberUpdate.html#asio-and-the-power-of-completion-tokens) indicating the initiating function should return a future type. This effectively replaces the original callback function with a callback that sets the shared state of the future. 
- [Lazy futures](https://github.com/facebookexperimental/libunifex/blob/main/doc/concepts.md#starting-an-async-operation): Likewise, callback continuations can be implemented as a future type such that the continuation is guaranteed to be available when the initiating function is over. There are two alternatives to this: 
    - Callback futures: The future continuation is defined at construction. The initiating function *might* run eagerly.  
    - Lazy futures: The future continuation is defined at any time. The initiating function *needs* to run lazily.  

This is what these models would look like:

=== "Continuable Futures"

    ```cpp
    void schedule_write(std::size_t length) {
        // async_write returns some continuable future type
        // `then` incurs in a synchronization cost
        future_lib::async_write(socket_, asio::buffer(data_, length))
            .then(schedule_read_function).detach();
    }
    ``` 

=== "Callbacks"

    ```cpp
    void schedule_write(std::size_t length) {
        // async_write returns void
        // `async_write` has not started yet so no synchronization cost
        asio::async_write(socket_, asio::buffer(data_, length), 
            schedule_read_function);
    }
    ``` 

=== "Completion Token"

    ```cpp
    void schedule_write(std::size_t length) {
        // async_write returns some continuable future type defined by use_custom_future
        // synchronization cost depends on the custom future 
        asio::async_write(socket_, asio::buffer(data_, length), 
            use_custom_future).then(schedule_read_function).detach();
    }
    ``` 

=== "Callback Futures"

    ```cpp
    void schedule_write(std::size_t length) {
        // async_write returns some callback future type that always knows its continuation 
        // no synchronization cost because no extra continuations might be attached 
        future_lib::async_write(socket_, asio::buffer(data_, length), 
            schedule_read_function).detach();
    }
    ``` 

=== "Lazy Futures"

    ```cpp
    void schedule_write(std::size_t length) {
        // async_write returns some lazy future type that we know is not executing yet 
        // no synchronization cost because continuations always come before the task starts 
        future_lib::lazy_async_write(socket_, asio::buffer(data_, length)) 
            .then(schedule_read_function).detach();
    }
    ``` 

In other words, while this synchronization requirement of [std::future] is a problem with [std::future] and other [proposed](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3784.pdf) continuable futures, such as [std::experimental::future], this is not a problem with the concept of futures. This synchronization cost problem in fact relates to whether we can *guarantee* the execution of the previous function has not started at the moment a continuation is defined. Two common models that tackle this problem with this kind of lazy-futures/tasks are [tasks graphs](#task-graphs) and the [sender/receiver model](#senders-and-receivers).

### Task graphs

Libraries such as [Taskflow] and [TTB] provide facilities to compose task graphs. This is how code in this task graph models would typically compare with futures:

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
    std::future D = future_lib::when_all(B, C).then([] () { 
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

Both [Taskflow] and [TTB] rely on graph nodes that represent tasks. These nodes might be typed, to better support continuations. This also makes these tasks analogous to [lazy futures](#lazy-futures-and-callbacks), whose continuations are defined before the execution starts. Lazy tasks enable continuations to run without the synchronization overhead of eager futures. 

Task graphs require us to use lazy execution for all tasks. We explicitly define all relationships between tasks before any execution starts, which might be inconvenient in some applications. Futures and async functions, on the other hand, allow us to 1) combine eager and lazy tasks, and 2) directly express their relationships in code without any explicit graph containers. 

### Senders and Receivers

More recently, [std::execution](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2300r1.html) and [libunifex] describe a model based on "senders" and "receivers" for asynchronous operations. If we consider all extensions for futures described in previous sections, senders are very similar to [std::future] and its existing variants, while receivers are analogous with [std::promise]. Despite these unusual names, the main difference between [std::future]/[std::promise] and "senders/receivers" is the former are *types* while the latter are *concepts* that might match any type:

- Like [continuable futures](#lazy-continuations-and-composability), "senders" can directly encapsulate their work and "send" their results to their "lazy continuation senders".
- Like [lazy futures](#lazy-futures-and-callbacks), senders hold tasks that _might_ not have started executing yet. They might only represent work _to be done_, i.e. a node in a task graph.
- Like [lazy futures](#lazy-futures-and-callbacks), a sender identified as lazy solves the necessity of synchronization continuable futures have.
- Like [task graphs](#task-graphs), lazy senders form an execution graph that does not get submitted until the calling thread waits for the results. 
- However, unlike [Taskflow] or [TTB] task nodes, the graph is defined implicitly and node types are defined at compile-time.

The syntax of senders is analogous to that of continuable futures: 

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

Building on these continuations, some [std::execution] functions are also analogous to the adaptors found in common [continuable future](#lazy-continuations-and-composability) libraries:

| Creating senders                          |   Creating futures                  | 
| ----------------------------------------- | ----------------------------------- | 
| `unifex::schedule`                        | [`async`](https://en.cppreference.com/w/cpp/thread/async)                        | 
| `unifex::just`                            | [`make_ready_future`](https://en.cppreference.com/w/cpp/experimental/make_ready_future) | 
| `unifex::transfer_just`                   | [`make_ready_future`](https://en.cppreference.com/w/cpp/experimental/make_ready_future) + [`then`](https://en.cppreference.com/w/cpp/experimental/future/then)            | 

| Composing senders                         |   Composing analogous futures       | 
| ----------------------------------------- | ----------------------------------- | 
| `unifex::then`                            | [`then`](https://en.cppreference.com/w/cpp/experimental/future/then)                                | 
| `unifex::wait_all`                        | [`when_all`](https://en.cppreference.com/w/cpp/experimental/when_all)                            | 
| `unifex::wait_any`                        | [`when_any`](https://en.cppreference.com/w/cpp/experimental/when_all)                            |
| `unifex::bulk`                            | [`parallel_for_each`](https://docs.microsoft.com/en-us/cpp/parallel/concrt/how-to-write-a-parallel-for-each-loop?view=msvc-170)                            |
| `unifex::split`                           | [`share`](https://en.cppreference.com/w/cpp/thread/future/share)                               |
| `unifex::upon_done`                       | [`then`](https://en.cppreference.com/w/cpp/experimental/future/then)                                |
| `unifex::upon_error`                      | [`fail`](https://naios.github.io/continuable/tutorial-chaining-continuables.html#tutorial-chaining-continuables-fail)                               |

In parallel with [discussions](https://isocpp.org/files/papers/P2464R0.html) about the proper model for C++ executors, senders/receivers have received criticism for attempting to abruptly reformulate common practice in C++ asynchronous computing. 

1. Although the [ASIO] executor model based on [callbacks](#lazy-futures-and-callbacks) might have its drawbacks, [libunifex], on the other hand, is merely not completely implemented yet let alone existing practice. Most [libunifex algorithms](https://github.com/facebookexperimental/libunifex/blob/main/doc/overview.md#different-sets-of-algorithms) are currently labeled as "not yet implemented". Although simpler, the ASIO executor model supports a myriad of [continuation styles](https://isocpp.org/files/papers/P2469R0.pdf) that can implement all sorts of asynchronous patterns, including error handling.

2. A second problem often mentioned with the sender/receiver model is its semantics. Although the "sender"/"receiver" model intends to replace the simple and well-known concept of futures, promises, and tasks, a sender/receiver still represents a variant of the [futures and promises](https://en.wikipedia.org/wiki/Futures_and_promises) construct, which is a reasonably specific concept. Meanwhile, if the context is not provided, the "sender"/"receiver" concepts have such generic names that even functions that "return"/"receive" values could ambiguously be called "sender"/"receiver". 

3. This influences the semantics and syntax of the API, which does not match the asynchronous model of other programming languages and C++ historical constructs, which might be unnecessarily confusing. For instance, in a situation where the [std::async] construct works well, senders require extra intermediary `begin` senders whose semantics are not clear. 

4. This extra compile-time complexity of a sender might also involve more template metaprogramming than needed. This specific choice of constant time task graphs might be considered a problem in contexts where a more general standard model for asynchronous programming is required. 

Coming from other programming languages or even from the historical C++ constructs, it is hard to distinguish what `begin` semantically represents and why it is the same type as other tasks:   

=== "Futures and Promises"

    ```cpp
    auto f = async(ex, []{ 
        std::cout << "Hello world!"; 
        return 13; 
    }).then([](int arg) { 
        return arg + 42;
    });
    ```

=== "Senders and Receivers"

    ```cpp
    sender auto begin = schedule(sch);
    sender auto hello = then(begin, []{
        std::cout << "Hello world!";
        return 13;
    });
    sender auto f = libunifex::then(hello, [](int arg) { 
        return arg + 42; 
    });
    ```

Regarding the C++ standard, it creates an initial asynchronous model that might be too high level at a place where mistakes are costly, creates new vocabulary for constructs that already exist in other languages, and the lack of implementation and common practice makes bad practice likely to be included in the standard. Finally, before battle testing the concepts, the semantics might lead to a syntax that is confusing to users, who won't have an opportunity to evaluate it. Confusing patterns indicate code smells or that these solutions might not scale well:

```cpp
sender_of<dynamic_buffer> auto async_read_array(auto handle) {
  return just(dynamic_buffer{})
       | let_value([] (dynamic_buffer& buf) {
           return just(std::as_writeable_bytes(std::span(&buf.size, 1))
                | async_read(handle)
                | then(
                    [&] (std::size_t bytes_read) {
                      assert(bytes_read == sizeof(buf.size));
                      buf.data = std::make_unique(new std::byte[buf.size]);
                      return std::span(buf.data.get(), buf.size);
                    }
                | async_read(handle)
                | then(
                    [&] (std::size_t bytes_read) {
                      assert(bytes_read == buf.size);
                      return std::move(buf);
                    });
       });
}
```

- https://github.com/facebookexperimental/libunifex/blob/main/doc/concepts.md
- https://github.com/facebookexperimental/libunifex/blob/main/doc/overview.md

## The futures library

Given the pros and cons of each model described in our [review](#previous-work), this library goes in a different direction and models future types as an [is_future] concept. These are some features of this library:  

- Reusable algorithms work for [*all future types*](#standard-futures), including any new and existing future types, such as [std::future], `boost::future`, `boost::fiber::future`.
- Provides an alternative to [std::async] based on [ASIO] [executors](#executors), which is the model most people have been effectively using so far, while still adaptable to other scheduler concepts. 
- Implements *lots* of concrete future types, including continuable, cancellable, lazy/eager, unique/shared futures. 
- Future types support [lazy continuations](#lazy-continuations-and-composability) without polling, although the future adaptors still work for all future types by polling.  
- Future types might be [strictly lazy with callbacks](#lazy-futures-and-callbacks) or maybe eager, avoiding the synchronization costs of attaching continuations.
- Implements a large set of [composition operations](#senders-and-receivers) with a syntax closer to the existing future types users are used to
- Includes a large set of the STL algorithms in terms of futures and executors, with easy integration and no need for external libraries, like [common](https://link.springer.com/chapter/10.1007/978-1-4842-4398-5_4) with C++ execution policies.

--8<-- "docs/references.md"