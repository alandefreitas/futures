//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_STOP_TOKEN_HPP
#define FUTURES_STOP_TOKEN_HPP

/**
 *  @file stop_token.hpp
 *  @brief Stop tokens
 *
 *  This header contains is an adapted version of std::stop_token for futures
 *  rather than threads.
 *
 *  The main difference in this implementation is 1) the reference counter does
 *  not distinguish between tokens and sources, and 2) there is no
 *  stop_callback.
 *
 *  The API was initially adapted from Baker Josuttis' reference implementation
 *  for C++20:
 *
 *  @see https://github.com/josuttis/jthread
 *
 *  Although the `std::future` class is obviously different from std::jthread,
 * this `stop_token` is not different from std::stop_token. The main goal here
 * is just to provide a stop source in C++17. In the future, we might replace
 * this with an alias to a C++20 std::stop_token.
 */


#include <atomic>
#include <memory>
#include <thread>
#include <utility>
#include <type_traits>

namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup cancellation Cancellation
     *
     * \brief Future cancellation primitives
     *
     *  @{
     */

    namespace detail {
        using shared_stop_state = std::shared_ptr<std::atomic<bool>>;
    } // namespace detail

    class stop_source;

    /// Empty struct to initialize a @ref stop_source without a shared
    /// stop state
    struct nostopstate_t {
        explicit nostopstate_t() = default;
    };

    /// Empty struct to initialize a @ref stop_source without a shared
    /// stop state
    inline constexpr nostopstate_t nostopstate{};

    /// Token to check if a stop request has been made
    /**
     *  The stop_token class provides the means to check if a stop request has
     *  been made or can be made, for its associated std::stop_source object. It
     *  is essentially a thread-safe "view" of the associated stop-state.
     */
    class stop_token {
    public:
        /// @name Constructors
        /// @{

        /// Constructs an empty stop_token with no associated stop-state
        /**
         *  @post stop_possible() and stop_requested() are both false
         */
        stop_token() noexcept = default;

        /// Copy constructor.
        /**
         *  Constructs a stop_token whose associated stop-state is the same as
         *  that of other.
         *
         *  @post *this and other share the same associated stop-state and
         *  compare equal
         *
         *  @param other another stop_token object to construct this stop_token
         *  object
         */
        stop_token(stop_token const &other) noexcept = default;

        /// Move constructor.
        /**
         *  Constructs a stop_token whose associated stop-state is the same as
         *  that of other; other is left empty
         *
         *  @post *this has other's previously associated stop-state, and
         *  other.stop_possible() is false
         *
         *  @param other another stop_token object to construct this stop_token
         *  object
         */
        stop_token(stop_token &&other) noexcept
            : shared_state_(std::exchange(other.shared_state_, nullptr)) {}

        /// Destroys the stop_token object.
        /**
         *  @post If *this has associated stop-state, releases ownership of it.
         */
        ~stop_token() = default;

        /// Copy-assigns the associated stop-state of other to that of
        /// *this
        /**
         *  Equivalent to stop_token(other).swap(*this)
         *
         *  @param other Another stop_token object to share the stop-state with
         *  to or acquire the stop-state from
         */
        stop_token &
        operator=(stop_token const &other) noexcept {
            if (shared_state_ != other.shared_state_) {
                stop_token tmp{ other };
                swap(tmp);
            }
            return *this;
        }

        /// Move-assigns the associated stop-state of other to that of
        /// *this
        /**
         *  After the assignment, *this contains the previous associated
         *  stop-state of other, and other has no associated stop-state
         *
         *  Equivalent to stop_token(std::move(other)).swap(*this)
         *
         *  @param other Another stop_token object to share the stop-state with
         *  to or acquire the stop-state from
         */
        stop_token &
        operator=(stop_token &&other) noexcept {
            if (this != &other) {
                stop_token tmp{ std::move(other) };
                swap(tmp);
            }
            return *this;
        }

        /// @}

        /// @name Modifiers
        /// @{

        /// Exchanges the associated stop-state of *this and other
        /**
         *  @param other stop_token to exchange the contents with
         */
        void
        swap(stop_token &other) noexcept {
            std::swap(shared_state_, other.shared_state_);
        }

        /// @}

        /// @name Observers
        /// @{

        /// Checks whether the associated stop-state has been requested
        /// to stop
        /**
         *  Checks if the stop_token object has associated stop-state and that
         *  state has received a stop request. A default constructed stop_token
         *  has no associated stop-state, and thus has not had stop requested
         *
         *  @return true if the stop_token object has associated stop-state and
         *  it received a stop request, false otherwise.
         */
        [[nodiscard]] bool
        stop_requested() const noexcept {
            return (shared_state_ != nullptr)
                   && shared_state_->load(std::memory_order_relaxed);
        }

        /// Checks whether associated stop-state can be requested to stop
        /**
         *  Checks if the stop_token object has associated stop-state, and that
         *  state either has already had a stop requested or it has associated
         *  std::stop_source object(s).
         *
         *  A default constructed stop_token has no associated `stop-state`, and
         *  thus cannot be stopped. the associated stop-state for which no
         *  std::stop_source object(s) exist can also not be stopped if such a
         *  request has not already been made.
         *
         *  @note If the stop_token object has associated stop-state and a stop
         *  request has already been made, this function still returns true.
         *
         *  @return false if the stop_token object has no associated stop-state,
         *  or it did not yet receive a stop request and there are no associated
         *  std::stop_source object(s); true otherwise
         */
        [[nodiscard]] bool
        stop_possible() const noexcept {
            return (shared_state_ != nullptr)
                   && (shared_state_->load(std::memory_order_relaxed)
                       || (shared_state_.use_count() > 1));
        }

        /// @}

        /// @name Non-member functions
        /// @{

        /// Compares two std::stop_token objects
        /**
         *  This function is not visible to ordinary unqualified or qualified
         *  lookup, and can only be found by argument-dependent lookup when
         *  std::stop_token is an associated class of the arguments.
         *
         *  @param a stop_tokens to compare
         *  @param b stop_tokens to compare
         *
         *  @return true if lhs and rhs have the same associated stop-state, or
         *  both have no associated stop-state, otherwise false
         */
        [[nodiscard]] friend bool
        operator==(stop_token const &a, stop_token const &b) noexcept {
            return a.shared_state_ == b.shared_state_;
        }

        /** Compares two std::stop_token objects for inequality
         *
         *  The != operator is synthesized from operator==
         *
         *  @param a stop_tokens to compare
         *  @param b stop_tokens to compare
         *
         *  @return true if lhs and rhs have different associated stop-states
         */
        [[nodiscard]] friend bool
        operator!=(stop_token const &a, stop_token const &b) noexcept {
            return a.shared_state_ != b.shared_state_;
        }

        /// @}

    private:
        friend class stop_source;

        // Constructor that allows the stop_source to construct the
        // stop_token directly from the stop state
        //
        // @param state State for the new token
        explicit stop_token(detail::shared_stop_state state) noexcept
            : shared_state_(std::move(state)) {}

        // Shared pointer to an atomic bool indicating if an external
        // procedure should stop
        detail::shared_stop_state shared_state_{ nullptr };
    };

    /// Object used to issue a stop request
    /**
     *  The stop_source class provides the means to issue a stop request, such
     *  as for std::jthread cancellation. A stop request made for one
     *  stop_source object is visible to all stop_sources and std::stop_tokens
     *  of the same associated stop-state; any std::stop_callback(s) registered
     *  for associated std::stop_token(s) will be invoked, and any
     *  std::condition_variable_any objects waiting on associated
     *  std::stop_token(s) will be awoken.
     */
    class stop_source {
    public:
        /// @name Constructors
        /// @{

        /// Constructs a stop_source with new stop-state
        /**
         *  @post stop_possible() is true and stop_requested() is false
         */
        stop_source()
            : shared_state_(std::make_shared<std::atomic_bool>(false)) {}

        /// Constructs an empty stop_source with no associated stop-state
        /**
         *  @post stop_possible() and stop_requested() are both false
         */
        explicit stop_source(nostopstate_t) noexcept {};

        /// Copy constructor
        /**
         *  Constructs a stop_source whose associated stop-state is the same as
         *  that of other.
         *
         *  @post *this and other share the same associated stop-state and
         *  compare equal
         *
         *  @param other another stop_source object to construct this
         *  stop_source object with
         */
        stop_source(stop_source const &other) noexcept = default;

        /// Move constructor
        /**
         *  Constructs a stop_source whose associated stop-state is the same as
         *  that of other; other is left empty
         *
         *  @post *this has other's previously associated stop-state, and
         *  other.stop_possible() is false
         *
         *  @param other another stop_source object to construct this
         *  stop_source object with
         */
        stop_source(stop_source &&other) noexcept
            : shared_state_(std::exchange(other.shared_state_, nullptr)) {}

        /// Destroys the stop_source object.
        /**
         *  If *this has associated stop-state, releases ownership of it.
         */
        ~stop_source() = default;

        /// Copy-assigns the stop-state of other
        /**
         *  Equivalent to stop_source(other).swap(*this)
         *
         *  @param other another stop_source object acquire the stop-state from
         */
        stop_source &
        operator=(stop_source &&other) noexcept {
            stop_source tmp{ std::move(other) };
            swap(tmp);
            return *this;
        }

        /// Move-assigns the stop-state of other
        /**
         *  Equivalent to stop_source(std::move(other)).swap(*this)
         *
         *  @post After the assignment, *this contains the previous stop-state
         *  of other, and other has no stop-state
         *
         *  @param other another stop_source object to share the stop-state with
         */
        stop_source &
        operator=(stop_source const &other) noexcept {
            if (shared_state_ != other.shared_state_) {
                stop_source tmp{ other };
                swap(tmp);
            }
            return *this;
        }

        /// @}

        /// @name Modifiers
        /// @{

        /// Makes a stop request for the associated stop-state, if any
        /**
         *  Issues a stop request to the stop-state, if the stop_source object
         *  has a stop-state, and it has not yet already had stop requested.
         *
         *  The determination is made atomically, and if stop was requested, the
         *  stop-state is atomically updated to avoid race conditions, such
         *  that:
         *
         *  - stop_requested() and stop_possible() can be concurrently invoked
         *  on other stop_tokens and stop_sources of the same stop-state
         *  - request_stop() can be concurrently invoked on other stop_source
         *  objects, and only one will actually perform the stop request.
         *
         *  @return true if the stop_source object has a stop-state and this
         *  invocation made a stop request (the underlying atomic value was
         *  successfully changed), otherwise false
         */
        bool
        request_stop() noexcept {
            if (shared_state_ != nullptr) {
                bool expected = false;
                return shared_state_->compare_exchange_strong(
                    expected,
                    true,
                    std::memory_order_relaxed);
            }
            return false;
        }

        /// Swaps two stop_source objects
        /**
         *  @param other stop_source to exchange the contents with
         */
        void
        swap(stop_source &other) noexcept {
            std::swap(shared_state_, other.shared_state_);
        }

        /// @}

        /// @name Non-member functions
        /// @{

        /// Returns a stop_token for the associated stop-state
        /**
         *  Returns a stop_token object associated with the stop_source's
         *  stop-state, if the stop_source has stop-state, otherwise returns a
         *  default-constructed (empty) stop_token.
         *
         *  @return A stop_token object, which will be empty if
         *  this->stop_possible() == false
         */
        [[nodiscard]] stop_token
        get_token() const noexcept {
            return stop_token{ shared_state_ };
        }

        /// Checks whether the associated stop-state has been requested
        /// to stop
        /**
         *  Checks if the stop_source object has a stop-state and that state has
         *  received a stop request.
         *
         *  @return true if the stop_token object has a stop-state, and it has
         *  received a stop request, false otherwise
         */
        [[nodiscard]] bool
        stop_requested() const noexcept {
            return (shared_state_ != nullptr)
                   && shared_state_->load(std::memory_order_relaxed);
        }

        /// Checks whether associated stop-state can be requested to stop
        /**
         *  Checks if the stop_source object has a stop-state.
         *
         *  @note If the stop_source object has a stop-state and a stop request
         *  has already been made, this function still returns true.
         *
         *  @return true if the stop_source object has a stop-state, otherwise
         *  false
         */
        [[nodiscard]] bool
        stop_possible() const noexcept {
            return shared_state_ != nullptr;
        }

        /// @}

        /// @name Non-member functions
        /// @{

        [[nodiscard]] friend bool
        operator==(stop_source const &a, stop_source const &b) noexcept {
            return a.shared_state_ == b.shared_state_;
        }
        [[nodiscard]] friend bool
        operator!=(stop_source const &a, stop_source const &b) noexcept {
            return a.shared_state_ != b.shared_state_;
        }

        /// @}

    private:
        // Shared pointer to an atomic bool indicating if an external
        // procedure should stop
        detail::shared_stop_state shared_state_{ nullptr };
    };

    /** @} */ // @addtogroup cancellation Cancellation
    /** @} */ // @addtogroup futures Futures
} // namespace futures

#endif // FUTURES_STOP_TOKEN_HPP
