//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_PACKAGED_TASK_H
#define FUTURES_PACKAGED_TASK_H

#include <futures/futures/basic_future.hpp>
#include <futures/futures/detail/shared_task.hpp>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup shared_state Shared State
     *  @{
     */

#ifndef FUTURES_DOXYGEN
    /// \brief Undefined packaged task class
    template <typename Signature>
    class packaged_task;
#endif

    /// \brief A packaged task that sets a shared state when done
    ///
    /// A packaged task holds a task to be executed and a shared state for its
    /// result.
    ///
    /// It's very similar to a promise where the shared state is replaced by a
    /// shared task.
    ///
    /// \tparam R Return type
    /// \tparam Args Task arguments
#ifndef FUTURES_DOXYGEN
    template <typename R, typename... Args>
#else
    template <typename Signature>
#endif
    class packaged_task<R(Args...)>
    {
    public:
        /// \brief Constructs a std::packaged_task object with no task and no
        /// shared state
        packaged_task() = default;

        /// \brief Construct a packaged task from a function with the default
        /// std allocator
        ///
        /// \par Constraints
        /// This constructor only participates in overload resolution if Fn is
        /// not a packaged task itself.
        ///
        /// \tparam Fn Function type
        /// \param fn The callable target to execute
        template <
            typename Fn
#ifndef FUTURES_DOXYGEN
            ,
            typename = std::enable_if_t<
                !std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
            >
        explicit packaged_task(Fn &&fn)
            : packaged_task{ std::allocator_arg,
                             std::allocator<packaged_task>{},
                             std::forward<Fn>(fn) } {
        }

        /// \brief Constructs a std::packaged_task object with a shared state
        /// and a copy of the task
        ///
        /// This function constructs a std::packaged_task object with a shared
        /// state and a copy of the task, initialized with std::forward<Fn>(fn).
        /// It uses the provided allocator to allocate memory necessary to store
        /// the task.
        ///
        /// \par Constraints
        /// This constructor does not participate in overload resolution if
        /// std::decay<Fn>::type is the same type as
        /// std::packaged_task<R(ArgTypes...)>.
        ///
        /// \tparam Fn Function type
        /// \tparam Allocator Allocator type
        /// \param alloc The allocator to use when storing the task
        /// \param fn The callable target to execute
        template <
            typename Fn,
            typename Allocator
#ifndef FUTURES_DOXYGEN
            ,
            typename = std::enable_if_t<
                !std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
            >
        explicit packaged_task(
            std::allocator_arg_t,
            const Allocator &alloc,
            Fn &&fn) {
            task_ = std::allocate_shared<
                detail::shared_task<std::decay_t<Fn>, Allocator, R, Args...>>(
                alloc,
                alloc,
                std::forward<Fn>(fn));
        }

        /// \brief The copy constructor is deleted, std::packaged_task is
        /// move-only.
        packaged_task(packaged_task const &) = delete;

        /// \brief Constructs a std::packaged_task with the shared state and
        /// task formerly owned by other
        packaged_task(packaged_task &&other) noexcept
            : future_retrieved_{ other.future_retrieved_ },
              task_{ std::move(other.task_) } {
            other.future_retrieved_ = false;
        }

        /// \brief The copy assignment is deleted, std::packaged_task is
        /// move-only.
        packaged_task &
        operator=(packaged_task const &)
            = delete;

        /// \brief Assigns a std::packaged_task with the shared state and task
        /// formerly owned by other
        packaged_task &
        operator=(packaged_task &&other) noexcept {
            if (this != &other) {
                packaged_task tmp{ std::move(other) };
                swap(tmp);
            }
            return *this;
        }

        /// \brief Destructs the task object
        ~packaged_task() {
            if (task_ && future_retrieved_) {
                task_->signal_promise_destroyed();
            }
        }

        /// \brief Checks if the task object has a valid function
        ///
        /// \return true if *this has a shared state, false otherwise
        [[nodiscard]] bool
        valid() const noexcept {
            return task_ != nullptr;
        }

        /// \brief Swaps two task objects
        ///
        /// This function exchanges the shared states and stored tasks of *this
        /// and other
        ///
        /// \param other packaged task whose state to swap with
        void
        swap(packaged_task &other) noexcept {
            std::swap(future_retrieved_, other.future_retrieved_);
            task_.swap(other.task_);
        }

        /// \brief Returns a future object associated with the promised result
        ///
        /// This function constructs a future object that shares its state with
        /// this promise Because this library handles more than a single future
        /// type, the future type we want is a template parameter. This function
        /// expects future type constructors to accept pointers to shared states.
        template <class Future = cfuture<R>>
        Future
        get_future() {
            if (future_retrieved_) {
                detail::throw_exception<future_already_retrieved>();
            }
            if (!valid()) {
                detail::throw_exception<packaged_task_uninitialized>();
            }
            future_retrieved_ = true;
            return Future{ std::static_pointer_cast<shared_state<R>>(task_) };
        }

        /// \brief Executes the function and set the shared state
        ///
        /// Calls the stored task with args as the arguments. The return value
        /// of the task or any exceptions thrown are stored in the shared state
        /// The shared state is made ready and any threads waiting for this are
        /// unblocked.
        ///
        /// \param args the parameters to pass on invocation of the stored task
        void
        operator()(Args... args) {
            if (!valid()) {
                detail::throw_exception<packaged_task_uninitialized>();
            }
            task_->run(std::forward<Args>(args)...);
        }

        /// \brief Resets the shared state abandoning any stored results of
        /// previous executions
        ///
        /// Resets the state abandoning the results of previous executions. A
        /// new shared state is constructed. Equivalent to *this =
        /// packaged_task(std::move(f)), where f is the stored task.
        void
        reset() {
            if (!valid()) {
                detail::throw_exception<packaged_task_uninitialized>();
            }
            task_ = task_->reset();
            future_retrieved_ = false;
        }

    private:
        /// \brief True if the corresponding future has already been retrieved
        bool future_retrieved_{ false };

        /// \brief The function this task should execute
        std::shared_ptr<detail::shared_task_base<R, Args...>> task_{};
    };

    /// \brief Specializes the std::swap algorithm
    template <typename Signature>
    void
    swap(packaged_task<Signature> &l, packaged_task<Signature> &r) noexcept {
        l.swap(r);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PACKAGED_TASK_H
