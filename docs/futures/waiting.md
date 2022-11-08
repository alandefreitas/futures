# Waiting for futures

## Waiting

The library includes a number of functions to wait for the result of future tasks. The simplest function we can use to
wait for a task is [basic_future::wait].

{{ code_snippet("tests/unit/snippets.cpp", "wait") }}

Although this is the function we have been using in most examples, [basic_future::wait] is very limited in practice
because we cannot usually wait indefinitely for tasks. Also, [basic_future::get] will already wait for the task to be
ready so [basic_future::wait] is often redundant.

## Specifying durations

In most applications, we launch long asynchronous tasks and cannot block another execution context indefinitely until
the task is ready. We might have a timeout after which the task should stop, or we might decide to do other work if the
task takes too long to get ready. We can limit the duration we will wait for with [basic_future::wait_for]:

{{ code_snippet("tests/unit/snippets.cpp", "wait_for") }}

Say we are making a network request. It would be impractical to allow this operation to continue indefinitely. It's
common practice to continuously read data from the socket until the operation is complete but also cancel the whole
operation if a specific timeout has been achieved. We can model this operation with a combination of a stoppable future
and [basic_future::wait_for].

{{ code_snippet("tests/unit/snippets.cpp", "wait_for_network") }}

In some applications, it might be easier to express how long we should wait through a time-point instead of a specific
duration. For instance, say you have an operation that should be canceled if it's not ready by 12:00AM. We can achieve
that with [basic_future::wait_until]:

{{ code_snippet("tests/unit/snippets.cpp", "wait_until") }}

## Busy waiting

The patterns described above are useful when we should block another execution context while the task is not ready. This
is useful when we have no other tasks to do while the future task is still running.

In other contexts, we might always have some other work to do even if our task graph is still running. This is common in
graphical interfaces, where we need to keep rendering the interface even while the task we have been waiting for is not
ready.

In these cases, when using a [basic_future] provided by the library, we can use the [basic_future::is_ready] member
function.

{{ code_snippet("tests/unit/snippets.cpp", "is_ready") }}

However, this member function is not defined for many other types, and we might need to make algorithms more generic. In
these cases, we can use the free function [is_ready], which works for any future type, including [std::future].

{{ code_snippet("tests/unit/snippets.cpp", "free_is_ready") }}

## Await

We typically use the functions [basic_future::get] or [basic_future::wait] to wait for the result of future types. This
is especially important for deferred futures, which might not start executing until we start waiting for their results.

The free-function [await] can also be used to wait for and get results. This is some syntactic sugar that makes waiting
more similar to other common programming languages, such as javascript.

{{ code_snippet("tests/unit/snippets.cpp", "await") }}

This is a free function that works for any future type. The [await] can also be used for conjunctions. If more than one
future object is provided, the result is returned as a [std::tuple].

{{ code_snippet("tests/unit/snippets.cpp", "await_tuple") }}

This is particularly convenient in C++17, where we can
use [structured bindings](https://en.cppreference.com/w/cpp/language/structured_binding):

{{ code_snippet("tests/unit/snippets.cpp", "await_tuple_bindings") }}

## Waiting for conjunctions

We can also wait for a future conjunction with [wait_for_all]. This function waits for all results in a range or tuple
of futures without owning them. This is convenient when we need to synchronously wait for a set of futures.

{{ code_snippet("tests/unit/snippets.cpp", "wait_for_all") }}

Unlike [await] and [basic_future::get], [wait_for_all] does not consume the results. The original future objects are
still valid and their values might be obtained directly from them.

The overloads of [wait_for_all_for] and [wait_for_all_until] are variants that might be used when we wish to specify a
time limit.

{{ code_snippet("tests/unit/snippets.cpp", "wait_for_all_for") }}

The overloads of these functions can accept ranges of futures, tuples of futures, or parameter packs. These functions
return an instance of [std::future_status] indicating whether all futures are ready.

!!! warning "[wait_for_all] != [when_all]"

    [wait_for_all] should not be confused with the future adaptor [when_all], which generates a future type.

    While [when_all] represents an asynchronous operation in a task graph, [wait_for_all] is an indication that the
    task graph should end at that point as an alternative to [basic_future::wait] for multiple tasks.

## Waiting for disjunctions

We can also wait for a future disjunction with [wait_for_any], [wait_for_any_for], and [wait_for_any_until]. These
functions wait for any result in a range or tuple of futures without owning them.

{{ code_snippet("tests/unit/snippets.cpp", "wait_for_any_for") }}

The overloads of these functions can accept ranges of futures, tuples of futures, or parameter packs. These functions
return an iterator to or the index of the first future to get ready. If no task gets ready in the specified duration, an
iterator to the last element or `std::size_t(-1)` is returned.

!!! warning "[wait_for_any] != [when_any]"

    [wait_for_any] should not be confused with the future adaptor [when_any], which generates a future type.

    While [when_any] is an asynchronous operation in a task graph, [wait_for_any] is an indication that the task graph 
    should end at that point as an alternative to [basic_future::wait] for multiple tasks.

It's important to note that waiting for disjunctions is more complex than waiting for conjunctions of future objects.
While we can wait for a conjunction by waiting for each of its elements, we need set up and wait for a notification from
any of the futures in order to know which future has finished first in a sequence of futures. When any of the future
objects do not support these external notifiers, we need to poll the tasks until they are completed.

## Adaptors

A number of futures adaptors are provided by the library to compose task graphs. These adaptors can also be used as a
more advanced form of waiting for futures implicitly in asynchronous task graphs.

--8<-- "docs/references.md"