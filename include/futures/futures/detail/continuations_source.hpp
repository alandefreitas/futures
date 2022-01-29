//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_CONTINUATIONS_SOURCE_H
#define FUTURES_CONTINUATIONS_SOURCE_H

#include <futures/futures/detail/small_vector.hpp>
#include <memory>
#include <shared_mutex>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief The continuation state as a small thread safe container that
    /// holds continuation functions for a future
    ///
    /// The whole logic here is very similar to that of stop_tokens. There is a
    /// source, a state, and a token.
    ///
    /// This is very limited as a container because there are not many
    /// operations we need to do with the continuation state. We need to be able
    /// to attach continuations (then), and run all continuations with a single
    /// shared lock.
    ///
    /// Like the stop_state, a continuation state might be shared between shared
    /// futures. Once one of the futures has run the continuations, the state is
    /// considered done.
    ///
    /// The continuation state needs to be atomic because it's also a shared
    /// state. Especially when the future is shared, many threads might be
    /// trying to attach new continuations to this future type, and the main
    /// future callback needs to wait for it.
    class continuations_state
    {
    public:
        /// \name Public Types
        /// @{

        /// \brief Type of a continuation callback
        /// This is a callback function that posts the next task to an executor.
        /// We cannot ensure the tasks go to the same executor.
        /// This needs to be type erased because there are many types of
        /// callables that might become a continuation here.
        using continuation_type = std::function<void()>;

        /// \brief Continuation ptr
        /// The callbacks are stored pointers because their addresses cannot
        /// lose stability when the future is moved or shared
        using continuation_ptr = std::unique_ptr<continuation_type>;

        /// \brief The continuation vector
        /// We use a small vector because of the common case when there few
        /// continuations per task
        using continuation_vector = detail::small_vector<continuation_ptr>;

        /// @}

        /// \name Constructors
        /// @{

        /// \brief Default constructor
        continuations_state() = default;

        /// \brief Copy constructor
        continuations_state(const continuations_state &) = delete;

        /// \brief Destructor - Run continuations if they have not run yet
        ~continuations_state() {
            request_run();
        }

        /// \brief Copy assignment
        continuations_state &
        operator=(const continuations_state &)
            = delete;

        /// @}

        /// \name Non-modifying
        /// @{

        /// \brief Get number of continuations
        [[nodiscard]] size_t
        size() const {
            std::shared_lock lock(continuations_mutex_);
            return continuations_.size();
        }

        /// \brief Get the i-th continuation
        /// The return reference is safe (in context) because the continuation
        /// vector has stability
        continuation_type &
        operator[](size_t index) const {
            std::shared_lock lock(continuations_mutex_);
            return continuations_.at(index).operator*();
        }
        /// @}

        /// \name Modifying
        /// @{

        /// \brief Emplace a new continuation
        /// Use executor ex if more continuations are not possible
        template <class Executor>
        bool
        emplace_back(const Executor &ex, continuation_type &&fn) {
            std::unique_lock lock(continuations_mutex_);
            if (is_run_possible()) {
                continuations_.emplace_back(
                    std::make_unique<continuation_type>(std::move(fn)));
                return true;
            } else {
                // When the shared state currently associated with *this is
                // ready, the continuation is called on an unspecified thread of
                // execution
                asio::post(ex, asio::use_future(std::move(fn)));
                return false;
            }
        }

        /// \brief Check if some source asked already asked for the
        /// continuations to run
        bool
        is_run_requested() const {
            std::shared_lock lock(run_requested_mutex_);
            return run_requested_;
        }

        /// \brief Check if some source asked already asked for the
        /// continuations to run
        bool
        is_run_possible() const {
            return !is_run_requested();
        }

        /// \brief Run all continuations
        bool
        request_run() {
            {
                // Check or update in a single lock
                std::unique_lock lock(run_requested_mutex_);
                if (run_requested_) {
                    return false;
                } else {
                    run_requested_ = true;
                }
            }
            std::unique_lock lock(continuations_mutex_);
            for (auto &continuation: continuations_) {
                (*continuation)();
            }
            continuations_.clear();
            return true;
        }
        /// @}

    private:
        /// \brief The actual pointers to the continuation functions
        /// This is encapsulated so we can't break anything
        continuation_vector continuations_;
        bool run_requested_{ false };
        mutable std::shared_mutex continuations_mutex_;
        mutable std::shared_mutex run_requested_mutex_;
    };

    /// Unit type intended for use as a placeholder in continuations_source
    /// non-default constructor
    struct nocontinuationsstate_t
    {
        explicit nocontinuationsstate_t() = default;
    };

    /// This is a constant object instance of stdnocontinuationsstate_t for use
    /// in constructing an empty continuations_source, as a placeholder value in
    /// the non-default constructor
    inline constexpr nocontinuationsstate_t nocontinuationsstate{};

    /// \brief Token the future object uses to emplace continuations
    class continuations_token
    {
    public:
        /// \brief Constructs an empty continuations_token with no associated
        /// continuations-state
        continuations_token() noexcept : state_(nullptr) {}

        /// \brief Constructs a continuations_token whose associated
        /// continuations-state is the same as that of other
        continuations_token(
            const continuations_token &other) noexcept = default;

        /// \brief Constructs a continuations_token whose associated
        /// continuations-state is the same as that of other; other is left empty
        continuations_token(continuations_token &&other) noexcept = default;

        /// \brief Copy-assigns the associated continuations-state of other to
        /// that of *this
        continuations_token &
        operator=(const continuations_token &other) noexcept = default;

        /// \brief Move-assigns the associated continuations-state of other to
        /// that of *this
        continuations_token &
        operator=(continuations_token &&other) noexcept = default;

        /// \brief Exchanges the associated continuations-state of *this and
        /// other.
        void
        swap(continuations_token &other) noexcept {
            std::swap(state_, other.state_);
        }

        /// \brief Checks if the continuations_token object has associated
        /// continuations-state and that state has received a run request
        [[nodiscard]] bool
        run_requested() const noexcept {
            return (state_ != nullptr) && state_->is_run_requested();
        }

        /// \brief Checks if the continuations_token object has associated
        /// continuations-state, and that state either has already had a run
        /// requested or it has associated continuations_source object(s)
        [[nodiscard]] bool
        run_possible() const noexcept {
            return (state_ != nullptr) && (!state_->is_run_requested());
        }

        /// \brief compares two std::run_token objects
        [[nodiscard]] friend bool
        operator==(
            const continuations_token &lhs,
            const continuations_token &rhs) noexcept {
            return lhs.state_ == rhs.state_;
        }

        [[nodiscard]] friend bool
        operator!=(
            const continuations_token &lhs,
            const continuations_token &rhs) noexcept {
            return lhs.state_ != rhs.state_;
        }

    private:
        friend class continuations_source;

        /// \brief Create token from state
        explicit continuations_token(
            std::shared_ptr<continuations_state> state) noexcept
            : state_(std::move(state)) {}

        /// \brief The state
        std::shared_ptr<continuations_state> state_;
    };

    /// \brief The continuations_source class provides the means to issue a
    /// request to run the future continuations
    class continuations_source
    {
    public:
        /// \brief Constructs a continuations_source with new continuations-state
        continuations_source()
            : state_(std::make_shared<continuations_state>()){};

        /// \brief Constructs an empty continuations_source with no associated
        /// continuations-state.
        explicit continuations_source(nocontinuationsstate_t) noexcept
            : state_{ nullptr } {}

        /// \brief Copy constructor.
        /// Constructs a continuations_source whose associated
        /// continuations-state is the same as that of other.
        continuations_source(
            const continuations_source &other) noexcept = default;

        /// \brief Move constructor.
        /// Constructs a continuations_source whose associated
        /// continuations-state is the same as that of other; other is left
        /// empty.
        continuations_source(continuations_source &&other) noexcept = default;

        /// \brief Copy-assigns the continuations-state of other to that of *this
        continuations_source &
        operator=(const continuations_source &other) noexcept = default;

        /// \brief Move-assigns the continuations-state of other to that of *this
        continuations_source &
        operator=(continuations_source &&other) noexcept = default;

        /// \brief Run all continuations
        /// The return reference is safe because the continuation vector has
        /// stability
        bool
        request_run() const {
            if (state_ != nullptr) {
                return state_->request_run();
            }
            return false;
        }

        /// \brief Run all continuations
        /// The return reference is safe because the continuation vector has
        /// stability
        template <class Executor>
        bool
        emplace_continuation(
            const Executor &ex,
            continuations_state::continuation_type &&fn) {
            if (state_ != nullptr) {
                return state_->emplace_back(ex, std::move(fn));
            }
            return false;
        }

        /// \brief Exchanges the continuations-state of *this and other.
        void
        swap(continuations_source &other) noexcept {
            std::swap(state_, other.state_);
        }

        /// \brief Get a token to this object
        /// Returns a continuations_token object associated with the
        /// continuations_source's continuations-state, if the
        /// continuations_source has continuations-state; otherwise returns a
        /// default-constructed (empty) continuations_token.
        [[nodiscard]] continuations_token
        get_token() const noexcept {
            return continuations_token(state_);
        }

        /// \brief Checks if the continuations_source object has a
        /// continuations-state and that state has received a run request.
        [[nodiscard]] bool
        run_requested() const noexcept {
            return state_ != nullptr && state_->is_run_requested();
        }

        /// \brief Checks if the continuations_source object has a
        /// continuations-state.
        [[nodiscard]] bool
        run_possible() const noexcept {
            return state_ != nullptr;
        }

        /// \brief Compares two continuations_source values
        [[nodiscard]] friend bool
        operator==(
            const continuations_source &a,
            const continuations_source &b) noexcept {
            return a.state_ == b.state_;
        }

        /// \brief Compares two continuations_source values
        [[nodiscard]] friend bool
        operator!=(
            const continuations_source &a,
            const continuations_source &b) noexcept {
            return a.state_ != b.state_;
        }

    private:
        std::shared_ptr<continuations_state> state_;
    };

    /** @} */
} // namespace futures::detail

#endif // FUTURES_CONTINUATIONS_SOURCE_H
