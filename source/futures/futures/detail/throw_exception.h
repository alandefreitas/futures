//
// Copyright (c) alandefreitas 12/7/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_THROW_EXCEPTION_H
#define FUTURES_THROW_EXCEPTION_H

namespace futures::detail {
    /** \addtogroup futures::detail Futures
     *  @{
     */

    /// \brief Throw an exception but terminate if we can't throw
    template <typename Ex> [[noreturn]] void throw_exception(Ex &&ex) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        throw static_cast<Ex &&>(ex);
#else
        (void)ex;
        std::terminate();
#endif
    }

    /// \brief Construct and throw an exception but terminate otherwise
    template <typename Ex, typename... Args> [[noreturn]] void throw_exception(Args &&...args) {
        throw_exception(Ex(std::forward<Args>(args)...));
    }

    /// \brief Throw an exception but terminate if we can't throw
    template <typename ThrowFn, typename CatchFn> void catch_exception(ThrowFn &&thrower, CatchFn &&catcher) {
#ifndef FUTURES_DISABLE_EXCEPTIONS
        try {
            return static_cast<ThrowFn &&>(thrower)();
        } catch (std::exception &) {
            return static_cast<CatchFn &&>(catcher)();
        }
#else
        return static_cast<ThrowFn &&>(thrower)();
#endif
    }

} // namespace futures::detail
#endif // FUTURES_THROW_EXCEPTION_H
