//
//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_PROMISE_H
#define FUTURES_PROMISE_H

#include <futures/futures/basic_future.hpp>
#include <futures/futures/detail/empty_base.hpp>
#include <futures/futures/detail/shared_state.hpp>
#include <futures/futures/detail/to_address.hpp>
#include <memory>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup shared_state Shared State
     *
     * \brief Shared state objects
     *
     *  @{
     */

    /// \brief Common members to promises of all types
    ///
    /// This includes a pointer to the corresponding shared_state for the future
    /// and the functions to manage the promise.
    ///
    /// The specific promise specialization will only differ by their set_value
    /// functions.
    ///
    template <typename R>
    class promise_base
    {
    public:
        /// \brief Create the base promise with std::allocator
        ///
        /// Use std::allocator_arg tag to dispatch and select allocator aware
        /// constructor
        promise_base()
            : promise_base{ std::allocator_arg,
                            std::allocator<promise_base>{} } {}

        /// \brief Create a base promise setting the shared state with the
        /// specified allocator
        ///
        /// This function allocates memory for and allocates an initial
        /// promise_shared_state (the future value) with the specified
        /// allocator. This object is stored in the internal intrusive pointer
        /// as the future shared state.
        template <typename Allocator>
        promise_base(std::allocator_arg_t, Allocator alloc)
            : shared_state_(
                std::allocate_shared<detail::shared_state<R>>(alloc)) {}

        /// \brief No copy constructor
        promise_base(promise_base const &) = delete;

        /// \brief Move constructor
        promise_base(promise_base &&other) noexcept
            : obtained_{ other.obtained_ },
              shared_state_{ std::move(other.shared_state_) } {
            other.obtained_ = false;
        }

        /// \brief No copy assignment
        promise_base &
        operator=(promise_base const &)
            = delete;

        /// \brief Move assignment
        promise_base &
        operator=(promise_base &&other) noexcept {
            if (this != &other) {
                promise_base tmp{ std::move(other) };
                swap(tmp);
            }
            return *this;
        }

        /// \brief Destructor
        ///
        /// This promise owns the shared state, so we need to warn the shared
        /// state when it's destroyed.
        virtual ~promise_base() {
            if (shared_state_ && obtained_) {
                shared_state_->signal_promise_destroyed();
            }
        }

        /// \brief Gets a future that shares its state with this promise
        ///
        /// This function constructs a future object that shares its state with
        /// this promise. Because this library handles more than a single future
        /// type, the future type we want is a template parameter.
        ///
        /// This function expects future type constructors to accept pointers to
        /// shared states.
        template <class Future = futures::cfuture<R>>
        Future
        get_future() {
            if (obtained_) {
                detail::throw_exception<future_already_retrieved>();
            }
            if (!shared_state_) {
                detail::throw_exception<promise_uninitialized>();
            }
            obtained_ = true;
            return Future{ shared_state_ };
        }

        /// \brief Set the promise result as an exception
        /// \note The set_value operation is only available at the concrete
        /// derived class, where we know the class type
        void
        set_exception(std::exception_ptr p) {
            if (!shared_state_) {
                detail::throw_exception<promise_uninitialized>();
            }
            shared_state_->set_exception(p);
        }

        /// \brief Set the promise result as an exception
        template <
            typename E
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<std::is_base_of_v<std::exception, E>, int> = 0
#endif
            >
        void
        set_exception(E e) {
            set_exception(std::make_exception_ptr(e));
        }

    protected:
        /// \brief Swap the value of two promises
        void
        swap(promise_base &other) noexcept {
            std::swap(obtained_, other.obtained_);
            shared_state_.swap(other.shared_state_);
        }

        /// \brief Intrusive pointer to the future corresponding to this promise
        constexpr std::shared_ptr<detail::shared_state<R>> &
        get_shared_state() {
            return shared_state_;
        };

    private:
        /// \brief True if the future has already obtained the promise
        bool obtained_{ false };

        /// \brief Pointer to the shared state for this promise
        std::shared_ptr<detail::shared_state<R>> shared_state_{};
    };

    /// \brief A shared state that will later be acquired by a future type
    ///
    /// The shared state is accessed by a future and a promise. The promise
    /// can write to the shared state while the future can read from it.
    ///
    /// The shared state is an implementation detail that takes advantages
    /// of the properties of futures and promises to avoid locking and
    /// wasteful memory allocations.
    ///
    /// \tparam R The shared state type
    template <typename R>
    class promise : public promise_base<R>
    {
    public:
        /// \brief Create the promise for type R
        using promise_base<R>::promise_base;

        /// \brief Set the promise value
        ///
        /// After this value is set, it can be obtained by the future object
        ///
        /// \param args arguments to set the promise
        template <class... Args>
        void
        set_value(Args &&...args) {
            if (!promise_base<R>::get_shared_state()) {
                detail::throw_exception<promise_uninitialized>();
            }
            promise_base<R>::get_shared_state()->set_value(
                std::forward<Args>(args)...);
        }

        /// \brief Swap the value of two promises
        void
        swap(promise &other) noexcept {
            promise_base<R>::swap(other);
        }
    };

    /// \brief Swap the value of two promises
    template <typename R>
    void
    swap(promise<R> &l, promise<R> &r) noexcept {
        l.swap(r);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PROMISE_H
