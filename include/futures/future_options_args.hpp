//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURE_OPTIONS_ARGS_HPP
#define FUTURES_FUTURE_OPTIONS_ARGS_HPP

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup future-options Future options
     *  @{
     */

    /// Future option to identify the executor to be used by a future
    /**
     * This identifies the executor a deferred future will use to launch
     * the task and the executor where continuations will be launched
     * by default.
     *
     * @tparam Executor Executor type
     */
    template <class Executor>
    struct executor_opt {
        using type = Executor;
    };

    /// Future option to determine the future is continuable
    /**
     * The operation state of a continuable futures holds a list of
     * continuations to the task related to that future. The continuations
     * are executed as soon as the future main task is ready.
     */
    struct continuable_opt {};

    /// Future option to determine the future is stoppable
    /**
     * The operation state of a stoppable future holds a stop token we can
     * use to request the main operation to stop.
     */
    struct stoppable_opt {};

    /// Future option to determine the future is always_detached
    /**
     * A future with this option is considered to always be detached.
     * The detach() function does nothing and the future will not wait
     * for the promise to be set at destruction.
     */
    struct always_detached_opt {};

    /// Future option to determine the future is always_deferred
    /**
     * A future that is known to always be deferred can implement a number
     * of optimizations a regular future cannot. For instance,
     * - continuations can happen without the continuation list because the
     *   next future can simply hold the previous future.
     * - Continuations lists and the base operation state also don't need any
     *   synchronization because the task* is known to not have been launched
     *   when these primitives are being set.
     * - The operation state might be stored inline without any dynamic memory
     *   allocations because we can assume the calling thread will be locked
     *   when waiting for the future so the address of the operation state
     *   cannot change.
     */
    struct always_deferred_opt {};

    /// Type of the deferred function
    template <class Function>
    struct deferred_function_opt {
        using type = Function;
    };

    /// Future option to determine the future is shared
    /**
     * Shared futures refer to the same operation state. The result of the
     * future operation is not moved from the future so that other tasks
     * can depend on it.
     */
    struct shared_opt {};

    /** @} */
    /** @} */
} // namespace futures


#endif // FUTURES_FUTURE_OPTIONS_ARGS_HPP
