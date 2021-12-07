//
// Copyright (c) alandefreitas 12/3/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_AWAIT_H
#define FUTURES_AWAIT_H

#include <type_traits>

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

    /// \brief Very simple version syntax sugar for types that pass the Future concept: future.wait() / future.get()
    /// This syntax is most useful for cases where we are immediately requesting the future result
    template <typename Future
#ifndef FUTURES_DOXYGEN
              ,
              std::enable_if_t<is_future_v<Future>, int> = 0
#endif
              >
    decltype(auto) await(Future &&f) {
        return f.get();
    }

    /** @} */
    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_AWAIT_H
