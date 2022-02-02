//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_CONTINUATION_UNWRAP_H
#define FUTURES_CONTINUATION_UNWRAP_H

#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/range_value.hpp>
#include <futures/futures/basic_future.hpp>
#include <futures/futures/traits/future_value.hpp>
#include <futures/futures/traits/is_future.hpp>
#include <futures/adaptor/detail/move_or_copy.hpp>
#include <futures/adaptor/detail/traits/is_callable.hpp>
#include <futures/adaptor/detail/traits/is_single_type_tuple.hpp>
#include <futures/adaptor/detail/traits/is_tuple.hpp>
#include <futures/adaptor/detail/traits/is_tuple_invocable.hpp>
#include <futures/adaptor/detail/traits/is_when_any_result.hpp>
#include <futures/adaptor/detail/traits/tuple_type_all_of.hpp>
#include <futures/adaptor/detail/traits/tuple_type_concat.hpp>
#include <futures/adaptor/detail/traits/tuple_type_transform.hpp>
#include <futures/adaptor/detail/traits/type_member_or.hpp>
#include <futures/adaptor/detail/tuple_algorithm.hpp>
#include <futures/adaptor/detail/unwrap_and_continue.hpp>
#include <futures/futures/detail/small_vector.hpp>
#include <futures/futures/detail/traits/type_member_or_void.hpp>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief A trait to validate whether a Function can be a continuation to a
    /// future
    template <class Function, class Future>
    using is_valid_continuation = std::bool_constant<
        continuation_traits_helper<Function, Future>::is_valid>;

    template <class Function, class Future>
    constexpr bool is_valid_continuation_v
        = is_valid_continuation<Function, Future>::value;

    // Wrap implementation in empty struct to facilitate friends
    struct internal_then_functor
    {
        template <class Future, class Function>
        struct unwrap_and_continue_task
        {
            Future before_;
            Function after_;

            decltype(auto)
            operator()() {
                return unwrap_and_continue(
                    std::move(before_),
                    std::move(after_));
            }

            decltype(auto)
            operator()(stop_token st) {
                return unwrap_and_continue(
                    std::move(before_),
                    std::move(after_),
                    st);
            }
        };

        template <
            class value_type,
            class future_options,
            class Executor,
            class Function>
        std::shared_ptr<shared_state<value_type, future_options>>
        make_initial_shared_state(const Executor &ex, Function &&f) const {
            if constexpr (!future_options::is_deferred) {
                using shared_state_t = shared_state<value_type, future_options>;
                (void) f;
                return std::make_shared<shared_state_t>(ex);
            } else {
                using shared_state_t = deferred_shared_state<
                    value_type,
                    future_options,
                    Function>;
                return std::make_shared<
                    shared_state_t>(ex, std::forward<Function>(f));
            }
        }

        /// \brief Maybe copy the previous continuations source
        template <class Future>
        static continuations_source
        copy_continuations_source(const Future &before) {
            if constexpr (is_continuable_v<std::decay_t<Future>>) {
                return before.state_->get_continuations_source();
            } else {
                return continuations_source(nocontinuationsstate);
            }
        }


        template <
            typename Executor,
            typename Function,
            class Future
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                // clang-format off
                is_executor_v<std::decay_t<Executor>> &&
                !is_executor_v<std::decay_t<Function>> &&
                !is_executor_v<std::decay_t<Future>> &&
                is_future_v<std::decay_t<Future>> &&
                is_valid_continuation_v<std::decay_t<Function>, std::decay_t<Future>>,
                // clang-format on
                int> = 0
#endif
            >
        decltype(auto)
        operator()(const Executor &ex, Future &&before, Function &&after)
            const {
            // Determiner next future options
            using traits = continuation_traits<Function, std::decay_t<Future>>;

            using future_options_base
                = future_options<executor_opt<Executor>, continuable_opt>;
            using future_options_stop = conditional_append_future_option_t<
                traits::continuation_expects_stop_token,
                stoppable_opt,
                future_options_base>;
            using future_options = conditional_append_future_option_t<
                is_deferred_v<Future>,
                deferred_opt,
                future_options_stop>;
            using value_type = typename traits::value_type;
            using future_type = basic_future<value_type, future_options>;

            // Create task for continuation future
            continuations_source cs_backup = copy_continuations_source(before);
            unwrap_and_continue_task<Future, Function> task{
                std::forward<Future>(before),
                std::forward<Function>(after)
            };

            // Create shared state for next task
            auto state = make_initial_shared_state<value_type, future_options>(
                ex,
                std::move(task));
            basic_future<value_type, future_options> fut(state);

            // Attach or launch the future
            if constexpr (is_deferred_v<future_type>) {
                // continuation future is lazy
                // - after.wait() needs to invoke before.wait() inline
                // - this might start polling if `before` has no continuations
                // - after.get() will already post the continuation task
                state->set_wait_callback([&before = task.before_]() mutable {
                    before.wait();
                });
            } else if constexpr (is_continuable_v<std::decay_t<Future>>) {
                // before is continuable / futures are eager
                // - attach as continuation to before
                auto apply_fn =
                    [state = std::move(state),
                     task = std::move(task)]() mutable {
                    state->apply(std::move(task));
                }; // make task copyable because `before` is not copyable
                auto fn_shared_ptr = std::make_shared<decltype(apply_fn)>(
                    std::move(apply_fn));
                auto copyable_handle = [fn_shared_ptr]() {
                    (*fn_shared_ptr)();
                };
                cs_backup.emplace_continuation(ex, copyable_handle);
            } else {
                // before not continuable / futures are eager
                // - start polling now
                auto poll_and_set_value =
                    [state = std::move(state), f = std::move(task)]() mutable {
                    state->apply(std::move(f));
                };
                asio::post(ex, std::move(poll_and_set_value));
            }
            return fut;
        }
    };

    constexpr internal_then_functor internal_then;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_CONTINUATION_UNWRAP_H
