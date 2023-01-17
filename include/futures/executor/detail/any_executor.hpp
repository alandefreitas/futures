//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_EXECUTOR_DETAIL_ANY_EXECUTOR_HPP
#define FUTURES_EXECUTOR_DETAIL_ANY_EXECUTOR_HPP

#include <futures/config.hpp>
#include <futures/executor/execute.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/utility/move_only_function.hpp>

namespace futures {
    namespace detail {
        struct executor_interface {
            virtual ~executor_interface() = default;

            virtual void
            execute(detail::move_only_function<void()>&& f) const
                = 0;
        };

        template <class E>
        class executor_interface_impl : public executor_interface {
            E ex_;
        public:
            virtual ~executor_interface_impl() = default;

            FUTURES_TEMPLATE(class U)
            (requires is_convertible_v<U, E>) executor_interface_impl(U&& e)
                : ex_(std::forward<U>(e)) {}

            virtual void
            execute(detail::move_only_function<void()>&& f) const {
                ::futures::execute(ex_, std::move(f));
            }
        };
    } // namespace detail
} // namespace futures

#endif // FUTURES_EXECUTOR_DETAIL_ANY_EXECUTOR_HPP
