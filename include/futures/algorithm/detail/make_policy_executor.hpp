//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ALGORITHM_DETAIL_POLICIES_HPP
#define FUTURES_ALGORITHM_DETAIL_POLICIES_HPP

#include <futures/algorithm/policies.hpp>

namespace futures {
    namespace detail {
        template <class E>
        using policy_executor_type = std::conditional_t<
            !is_same_v<E, sequenced_policy>,
            default_execution_context_type::executor_type,
            inline_executor>;

        template <class E>
        constexpr detail::policy_executor_type<E>
        make_policy_executor_impl(std::true_type) {
            return make_default_executor();
        }

        template <class E>
        constexpr detail::policy_executor_type<E>
        make_policy_executor_impl(std::false_type) {
            return make_inline_executor();
        }

        // Make an executor appropriate to a given policy
        /*
         * The result type depends on the default executors we have available
         * for each policy. Sequenced policy will often use an inline executor
         * and other policies will use executors that will run the algorithms
         * in parallel.
         */
#ifdef FUTURES_HAS_CONCEPTS
        template <
            execution_policy E,
            std::input_iterator I,
            std::sentinel_for<I> S>
#else
        template <
            class E,
            class I,
            class S,
            std::enable_if_t<
                (!is_executor_v<E> && is_execution_policy_v<E>
                 && is_input_iterator_v<I> && is_sentinel_for_v<S, I>),
                int>
            = 0>
#endif
        constexpr detail::policy_executor_type<E>
        make_policy_executor() {
            return detail::make_policy_executor_impl<E>(
                boost::mp11::mp_bool<!detail::is_same_v<E, sequenced_policy>>{});
        }
    } // namespace detail
} // namespace futures

#endif // FUTURES_ALGORITHM_DETAIL_POLICIES_HPP
