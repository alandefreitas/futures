//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_RELOCKER_H
#define FUTURES_RELOCKER_H

#include <mutex>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */
    /// \brief An object that temporarily unlocks a lock
    struct relocker
    {
        /// \brief The underlying lock
        std::unique_lock<std::mutex> &lock_;

        /// \brief Construct a relocker
        ///
        /// A relocker keeps a temporary reference to the lock and
        /// immediately unlocks it
        ///
        /// \param lk Reference to underlying lock
        explicit relocker(std::unique_lock<std::mutex> &lk) : lock_(lk) {
            lock_.unlock();
        }

        /// \brief Copy constructor is deleted
        relocker(const relocker &) = delete;
        relocker(relocker &&other) noexcept = delete;

        /// \brief Copy assignment is deleted
        relocker &
        operator=(const relocker &)
            = delete;
        relocker &
        operator=(relocker &&other) noexcept = delete;

        /// \brief Destroy the relocker
        ///
        /// The relocker locks the underlying lock when it's done
        ~relocker() {
            if (!lock_.owns_lock()) {
                lock_.lock();
            }
        }

        /// \brief Lock the underlying lock
        void
        lock() {
            if (!lock_.owns_lock()) {
                lock_.lock();
            }
        }
    };
    /** @} */
} // namespace futures::detail

#endif // FUTURES_RELOCKER_H
