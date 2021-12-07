//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TRY_ASYNC_H
#define FUTURES_TRY_ASYNC_H

#include <futures/futures/async.h>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Attempts to schedule a function
    ///
    /// This function attempts to schedule a function, and returns 3 objects:
    /// - The future for the task itself
    /// - A future that indicates if the task got scheduled yet
    /// - A token for canceling the task
    ///
    /// This is mostly useful for recursive tasks, where there might not be room in the executor for
    /// a new task, as depending on recursive tasks for which there is no room is the executor might
    /// block execution.
    ///
    /// Although this is a general solution to allow any executor in the algorithms, executor traits
    /// to identify capacity in executor are much more desirable.
    ///
    template <typename Executor, typename Function, typename... Args
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<detail::is_valid_async_input_v<Executor, Function, Args...>, int> = 0
#endif
              >
    decltype(auto) try_async(const Executor &ex, Function &&f, Args &&...args) {
        // Communication flags
        std::promise<void> started_token;
        std::future<void> started = started_token.get_future();
        stop_source cancel_source;

        // Wrap the task in a lambda that sets and checks the flags
        auto do_task = [p = std::move(started_token), cancel_token = cancel_source.get_token(),
                        f](Args &&...args) mutable {
            p.set_value();
            if (cancel_token.stop_requested()) {
                small::throw_exception<std::runtime_error>("task cancelled");
            }
            return std::invoke(f, std::forward<Args>(args)...);
        };

        // Make it copy constructable
        auto do_task_ptr = std::make_shared<decltype(do_task)>(std::move(do_task));
        auto do_task_handle = [do_task_ptr](Args &&...args) { return (*do_task_ptr)(std::forward<Args>(args)...); };

        // Launch async
        using internal_result_type = std::decay_t<decltype(std::invoke(f, std::forward<Args>(args)...))>;
        cfuture<internal_result_type> rhs = async(ex, do_task_handle, std::forward<Args>(args)...);

        // Return future and tokens
        return std::make_tuple(std::move(rhs), std::move(started), cancel_source);
    }


    /** @} */
}


#endif // FUTURES_TRY_ASYNC_H
