//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_DETAIL_INTERNAL_THEN_FUNCTOR_HPP
#define FUTURES_ADAPTOR_DETAIL_INTERNAL_THEN_FUNCTOR_HPP

#include <futures/future.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/traits/future_value.hpp>
#include <futures/traits/is_future.hpp>
#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/move_if_not_shared.hpp>
#include <futures/detail/traits/is_callable.hpp>
#include <futures/adaptor/detail/continue.hpp>
#include <futures/adaptor/detail/make_continuation_state.hpp>
#include <futures/detail/deps/boost/mp11/function.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    // Wrap implementation in empty struct to facilitate friends
    struct internal_then_functor {
        /// Maybe copy the previous continuations source
        template <class Future>
        static continuations_source<is_always_deferred_v<Future>>
        copy_continuations_source(Future const &before) {
            if constexpr (is_continuable_v<std::decay_t<Future>>) {
                return before.state_->get_continuations_source();
            } else {
                return continuations_source<is_always_deferred_v<Future>>(
                    nocontinuationsstate);
            }
        }


        FUTURES_TEMPLATE(class Executor, class Function, class Future)
        (requires(
            is_executor_v<std::decay_t<Executor>>
            && !is_executor_v<std::decay_t<Function>>
            && !is_executor_v<std::decay_t<Future>>
            && is_future_v<std::decay_t<Future>>
            && next_future_traits<
                Executor,
                std::decay_t<Function>,
                std::decay_t<Future>>::is_valid)) FUTURES_DETAIL(decltype(auto))
        operator()(Executor const &ex, Future &&before, Function &&after)
            const {
            // Determine next future options
            using traits
                = next_future_traits<Executor, Function, std::decay_t<Future>>;
            using next_value_type = typename traits::next_value_type;
            using next_future_options = typename traits::next_future_options;
            using next_future_type
                = basic_future<next_value_type, next_future_options>;
            if constexpr (is_continuable_v<std::decay_t<Future>>) {
                // If future is continuable, just use the then function
                return std::forward<Future>(before)
                    .then(ex, std::forward<Function>(after));
            } else if constexpr (is_always_deferred_v<next_future_type>) {
                // Previous is not continuable or both are deferred, so we don't
                // need the continuations because next will wait for prev in
                // a task graph.
                future_continue_task<
                    std::decay_t<Future>,
                    std::decay_t<Function>>
                    task{ move_if_not_shared(before),
                          std::forward<Function>(after) };
                static_assert(!is_shared_future_v<next_future_type>);
                using operation_state_t = detail::deferred_operation_state<
                    next_value_type,
                    next_future_options>;
                operation_state_t state(ex, std::move(task));
                next_future_type fut(std::move(state));
                return fut;
            } else {
                // Create a shared version of the previous future, because
                // multiple handles will need to access this shared state
                // now Create task for the continuation future
                future_continue_task<
                    std::decay_t<Future>,
                    std::decay_t<Function>>
                    task{ move_if_not_shared(before),
                          std::forward<Function>(after) };

                // Create shared state for the next future
                auto state = detail::make_continuation_shared_state<
                    next_value_type,
                    next_future_options>(ex, std::move(task));
                next_future_type fut(state);

                // Before not continuable -> both futures are eager
                // - post a task that starts polling
                auto poll_and_set_value =
                    [state = std::move(state),
                     task = std::move(task)]() mutable {
                    state->apply(std::move(task));
                };
                asio::post(ex, std::move(poll_and_set_value));
                return fut;
            }
        }
    };

    constexpr internal_then_functor internal_then;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_ADAPTOR_DETAIL_INTERNAL_THEN_FUNCTOR_HPP
