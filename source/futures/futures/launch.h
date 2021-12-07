//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_LAUNCH_H
#define FUTURES_LAUNCH_H

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup launch Launch
     *  @{
     */
    /** \addtogroup launch-policies Launch Policies
     *  @{
     */

    /// \brief Specifies the launch policy for a task executed by the @ref futures::async function
    ///
    /// std::async creates a new thread for each asynchronous operation, which usually entails in only
    /// two execution policies: new thread or inline. Because futures::async use executors, there are
    /// many more policies and ways to use these executors beyond yes/no.
    ///
    /// Most of the time, we want the executor/post policy for executors. So as the @ref async function
    /// also accepts executors directly, this option can often be ignored, and is here mostly here for
    /// compatibility with the std::async.
    ///
    /// When only the policy is provided, async will try to generate the proper executor for that policy.
    /// When the executor and the policy is provided, we might only have some conflict for the deferred
    /// policy, which does not use an executor in std::launch. In the context of executors, the deferred
    /// policy means the function is only posted to the executor when its result is requested.
    enum class launch {
        /// no policy
        none = 0b0000'0000,
        /// execute on a new thread regardless of executors (same as std::async::async)
        new_thread = 0b0000'0001,
        async = 0b0000'0001,
        /// execute on the calling thread when result is requested (same as std::async::deferred)
        deferred = 0b0000'0010,
        lazy = 0b0000'0010,
        /// inherit from context
        inherit = 0b0000'0100,
        /// execute on the calling thread now (uses inline executor)
        inline_now = 0b0000'1000,
        sync = 0b0000'1000,
        /// enqueue task in the executor
        post = 0b0001'0000,
        executor = 0b0001'0000,
        /// run immediately if inside the executor
        dispatch = 0b0010'0000,
        executor_now = 0b0010'0000,
        /// enqueue task for later in the executor
        executor_later = 0b0100'0000,
        defer = 0b0100'0000,
        any = async | deferred
    };

    /// \brief operator & for launch policies
    constexpr launch operator&(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) & static_cast<int>(y));
    }

    /// \brief operator | for launch policies
    constexpr launch operator|(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) | static_cast<int>(y));
    }

    /// \brief operator ^ for launch policies
    constexpr launch operator^(launch x, launch y) {
        return static_cast<launch>(static_cast<int>(x) ^ static_cast<int>(y));
    }

    /// \brief operator ~ for launch policies
    constexpr launch operator~(launch x) { return static_cast<launch>(~static_cast<int>(x)); }

    /// \brief operator &= for launch policies
    inline launch &operator&=(launch &x, launch y) { return x = x & y; }

    /// \brief operator |= for launch policies
    inline launch &operator|=(launch &x, launch y) { return x = x | y; }

    /// \brief operator ^= for launch policies
    inline launch &operator^=(launch &x, launch y) { return x = x ^ y; }
    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_LAUNCH_H
