# Waiting for futures

This library includes a number of helper functions to wait for the result of future tasks.

# Await

Usually use the functions `get()` or `wait()` to wait for the result of future types. This is especially important for deferred futures, which might not start executing until we start waiting for their results.

The free-function [await] can also be used to wait for results. This syntactic sugar makes waiting more similar to other common programming languages.

# Waiting for conjunction

The [await] can also be used for conjunctions. In this case, the function returns a tuple, which might be convenient with structured binding declarations.

We can also wait for a future conjunction with [wait_for_all]. This function waits for all results in a range or tuple of futures without owning them. This is convenient when we need to synchronously wait for a set of futures.

!!! warning "[wait_for_all] != [when_all]"

    [wait_for_all] should not be confused with the future adaptor [when_all], which generates a future type

    While [when_all] is an asynchronous operation in a task graph, [wait_for_all] is an indication that the task graph should end at that point as an alternative to `wait()`

# Waiting for disjunctions

We can also wait for a future disjunction with [wait_for_any]. This function waits for any result in a range or tuple of futures without owning them. 

!!! warning "[wait_for_any] != [when_any]"

    [wait_for_any] should not be confused with the future adaptor [when_any], which generates a future type

    While [when_any] is an asynchronous operation in a task graph, [wait_for_any] is an indication that the task graph should end at that point as an alternative to `wait()`

It's important to note that waiting for disjunctions is a little more complex than waiting for conjunctions of future. While we can wait for a conjunction by waiting for each of its elements, we need set up and wait for a notification from any of the futures in order to know which future has finished first in a sequence of futures. 

--8<-- "docs/references.md"