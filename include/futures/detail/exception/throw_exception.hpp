//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_EXCEPTION_THROW_EXCEPTION_HPP
#define FUTURES_DETAIL_EXCEPTION_THROW_EXCEPTION_HPP

namespace futures::detail {
    /** @addtogroup futures::detail Futures
     *  @{
     */

    /// Throw an exception but terminate if we can't throw
    template <class Ex>
    [[noreturn]] void
    throw_exception(Ex &&ex) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        throw static_cast<Ex &&>(ex);
#else
        (void) ex;
        std::terminate();
#endif
    }

    /// Construct and throw an exception but terminate otherwise
    template <class Ex, class... Args>
    [[noreturn]] void
    throw_exception(Args &&...args) {
        throw_exception(Ex(std::forward<Args>(args)...));
    }

    /// Throw an exception but terminate if we can't throw
    template <class ThrowFn, class CatchFn>
    void
    catch_exception(ThrowFn &&thrower, CatchFn &&catcher) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        try {
            return static_cast<ThrowFn &&>(thrower)();
        }
        catch (std::exception &) {
            return static_cast<CatchFn &&>(catcher)();
        }
#else
        return static_cast<ThrowFn &&>(thrower)();
#endif
    }

} // namespace futures::detail
#endif // FUTURES_DETAIL_EXCEPTION_THROW_EXCEPTION_HPP
