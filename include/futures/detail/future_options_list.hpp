//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_FUTURE_OPTIONS_LIST_HPP
#define FUTURES_DETAIL_FUTURE_OPTIONS_LIST_HPP

#include <futures/executor/default_executor.hpp>
#include <futures/detail/utility/invoke.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <type_traits>

namespace futures {
    namespace detail {
        /** @addtogroup futures Futures
         *  @{
         */
        /** @addtogroup future-options Future options
         *  @{
         */

        // Class used to define future extension at compile-time
        template <class... Args>
        struct future_options_list {
        private:
            static constexpr std::size_t N = sizeof...(Args);

            template <class T>
            struct is_executor_opt {
                static constexpr bool value = false;
            };

            template <class T>
            struct is_executor_opt<executor_opt<T>> {
                static constexpr bool value = true;
            };

            template <class TypeList>
            using get_executor_opt_type = typename mp_at<
                TypeList,
                mp_find_if<TypeList, is_executor_opt>>::type;

            template <class T>
            struct is_deferred_function_opt {
                static constexpr bool value = false;
            };

            template <class T>
            struct is_deferred_function_opt<deferred_function_opt<T>> {
                static constexpr bool value = true;
            };

            template <class TypeList>
            using get_deferred_function_opt_type = typename mp_at<
                TypeList,
                mp_find_if<TypeList, is_deferred_function_opt>>::type;

        public:
            /// Whether the future has an associated executor
            static constexpr bool has_executor
                = mp_find_if<mp_list<Args...>, is_executor_opt>::value
                  != sizeof...(Args);

            /// Executor used by the shared state
            /**
             *  This is the executor the shared state is using for the
             *  current task and the default executor it uses for
             *  potential continuations
             */
            using executor_t = mp_eval_or<
                default_executor_type,
                get_executor_opt_type,
                mp_list<Args...>>;

            /// Whether the future supports deferred continuations
            static constexpr bool is_continuable
                = mp_contains<mp_list<Args...>, continuable_opt>::value;

            /// Whether the future supports stop requests
            static constexpr bool is_stoppable
                = mp_contains<mp_list<Args...>, stoppable_opt>::value;

            /// Whether the future is always detached
            static constexpr bool is_always_detached
                = mp_contains<mp_list<Args...>, always_detached_opt>::value;

            /// Whether the future is always deferred
            /**
             *  Deferred futures are associated to a task that is only
             *  sent to the executor when we request or wait for the
             *  future value
             */
            static constexpr bool is_always_deferred
                = mp_contains<mp_list<Args...>, always_deferred_opt>::value;

            /// Whether the future holds an associated function with the task
            static constexpr bool has_deferred_function
                = mp_find_if<mp_list<Args...>, is_deferred_function_opt>::value
                  != N;

            /// Function used by the deferred shared state
            /**
             *  This is the function the deferred state will use when the task
             *  is launched
             */
            using function_t = mp_eval_or<
                move_only_function<void()>,
                get_deferred_function_opt_type,
                mp_list<Args...>>;

            /// Whether the future is shared
            /**
             *  The value of shared futures is not consumed when requested.
             *  Instead, it makes copies of the return value. On the other
             *  hand, simple unique future move their result from the
             *  shared state when their value is requested.
             */
            static constexpr bool is_shared
                = mp_contains<mp_list<Args...>, shared_opt>::value;

        private:
            FUTURES_STATIC_ASSERT(is_invocable_v<function_t>);

            // Identify args positions and ensure they are sorted
            // executor_opt < continuable_opt
            static constexpr std::size_t executor_idx
                = mp_find<mp_list<Args...>, executor_opt<executor_t>>::value;
            static constexpr std::size_t continuable_idx
                = mp_find<mp_list<Args...>, continuable_opt>::value;
            FUTURES_STATIC_ASSERT_MSG(
                executor_idx == N || executor_idx < continuable_idx,
                "The executor_opt tag should be defined before the "
                "continuable_opt "
                "tag");

            // continuable_opt < stoppable_opt
            static constexpr std::size_t stoppable_idx
                = mp_find<mp_list<Args...>, stoppable_opt>::value;
            FUTURES_STATIC_ASSERT_MSG(
                continuable_idx == N || continuable_idx < stoppable_idx,
                "The continuable_opt tag should be defined before the "
                "stoppable_opt tag");

            // stoppable_opt < always_detached_opt
            static constexpr std::size_t always_detached_idx
                = mp_find<mp_list<Args...>, always_detached_opt>::value;
            FUTURES_STATIC_ASSERT_MSG(
                stoppable_idx == N || stoppable_idx < always_detached_idx,
                "The stoppable_opt tag should be defined before the "
                "always_detached_opt tag");

            // always_detached_opt < always_deferred_opt
            static constexpr std::size_t always_deferred_idx
                = mp_find<mp_list<Args...>, always_deferred_opt>::value;
            FUTURES_STATIC_ASSERT_MSG(
                always_detached_idx == N
                    || always_detached_idx < always_deferred_idx,
                "The always_detached_opt tag should be defined before the "
                "always_deferred_opt tag");

            // always_deferred_opt < deferred_function_opt
            static constexpr std::size_t deferred_function_idx = mp_find<
                mp_list<Args...>,
                deferred_function_opt<function_t>>::value;
            FUTURES_STATIC_ASSERT_MSG(
                always_deferred_idx == N
                    || always_deferred_idx < deferred_function_idx,
                "The always_deferred_opt tag should be defined before the "
                "deferred_function_opt tag");

            // always_deferred_opt < shared_opt
            static constexpr std::size_t shared_idx
                = mp_find<mp_list<Args...>, shared_opt>::value;
            FUTURES_STATIC_ASSERT_MSG(
                always_deferred_idx == N || always_deferred_idx < shared_idx,
                "The always_deferred_opt tag should be defined before the "
                "shared_opt tag");
        };

        /** @} */
        /** @} */
    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_FUTURE_OPTIONS_LIST_HPP
