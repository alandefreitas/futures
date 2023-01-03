//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_IS_READY_HPP
#define FUTURES_IMPL_IS_READY_HPP

#include <futures/config.hpp>
#include <futures/future_status.hpp>
#include <type_traits>

#ifndef FUTURES_DOXYGEN
namespace boost {
    namespace fibers {
        enum class future_status;
    } // namespace fibers
} // namespace boost

namespace boost {
    enum class future_status;
} // namespace boost
#endif

namespace futures {
#ifndef FUTURES_DOXYGEN
    namespace detail {
        // our own future status enum
        inline bool
        is_ready_status(future_status s) {
            return s == future_status::ready;
        }

        // boost::fibers::future_status::ready == 1
        inline bool
        is_ready_status(boost::fibers::future_status s) {
            return static_cast<int>(s) == 1;
        }

        // boost::future_status::ready == 0
        inline bool
        is_ready_status(boost::future_status s) {
            return static_cast<int>(s) == 0;
        }

        // std::future_status::ready == 0
        template <class E, std::enable_if_t<is_enum_v<E>, int> = 0>
        bool
        is_ready_status(E s) {
            // This will work for any std::future_status or equivalent that
            // defines future_status::ready as `0`
            return static_cast<int>(s) == 0;
        }

        template <class Future>
        bool
        is_ready_impl(std::true_type /* has_is_ready */, Future &&f) {
            return f.is_ready();
        }

        template <class Future>
        bool
        is_ready_impl(std::false_type /* has_is_ready */, Future &&f) {
            return detail::is_ready_status(f.wait_for(std::chrono::seconds(0)));
        }
    } // namespace detail
#endif

#ifdef FUTURES_HAS_CONCEPTS
    template <future_like Future>
#else
    template <
        class Future,
        std::enable_if_t<is_future_like_v<std::decay_t<Future>>, int>>
#endif
    bool
    is_ready(Future &&f) {
        assert(
            f.valid()
            && "Undefined behaviour. Checking if an invalid future is ready.");
        return detail::is_ready_impl(
            detail::has_is_ready<Future>{},
            std::forward<Future>(f));
    }
} // namespace futures

#endif // FUTURES_IMPL_IS_READY_HPP
