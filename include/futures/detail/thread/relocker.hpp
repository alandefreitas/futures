//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_THREAD_RELOCKER_HPP
#define FUTURES_DETAIL_THREAD_RELOCKER_HPP

#include <mutex>

namespace futures {
    namespace detail {
        /** @addtogroup futures Futures
         *  @{
         */

        /// An object that temporarily unlocks a lock
        struct relocker {
            /// The underlying lock
            std::unique_lock<std::mutex> &lock_;

            /// Construct a relocker
            ///
            /// A relocker keeps a temporary reference to the lock and
            /// immediately unlocks it
            ///
            /// @param lk Reference to underlying lock
            explicit relocker(std::unique_lock<std::mutex> &lk) : lock_(lk) {
                lock_.unlock();
            }

            /// Copy constructor is deleted
            relocker(relocker const &) = delete;
            relocker(relocker &&other) noexcept = delete;

            /// Copy assignment is deleted
            relocker &
            operator=(relocker const &)
                = delete;
            relocker &
            operator=(relocker &&other) noexcept
                = delete;

            /// Destroy the relocker
            ///
            /// The relocker locks the underlying lock when it's done
            ~relocker() {
                if (!lock_.owns_lock()) {
                    lock_.lock();
                }
            }

            /// Lock the underlying lock
            void
            lock() {
                if (!lock_.owns_lock()) {
                    lock_.lock();
                }
            }
        };
        /** @} */
    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_THREAD_RELOCKER_HPP
