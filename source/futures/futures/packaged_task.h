//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_PACKAGED_TASK_H
#define FUTURES_PACKAGED_TASK_H

#include <futures/futures/detail/task.h>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup shared_state Shared State
     *  @{
     */

#ifndef FUTURES_DOXYGEN
    /// \brief Undefined packaged task class
    template <typename Signature> class packaged_task;
#endif

    /// \brief Packaged task
    /// A packaged task holds a task to be executed and a shared state for its result
    /// When the shared state becomes ready,
    template <typename R, typename... Args> class packaged_task<R(Args...)> {
      public:
        /// \brief Constructs a std::packaged_task object with no task and no shared state
        packaged_task() = default;

        /// \brief Construct a packaged task from a function
        /// \par Constraints
        /// This constructor only participates in overload resolution if Fn is not a packaged task
        /// \tparam Fn Function type
        /// \param fn The callable target to execute
        template <typename Fn
#ifndef FUTURES_DOXYGEN
                  ,
                  typename = std::enable_if_t<!std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
                  >
        explicit packaged_task(Fn &&fn)
            : packaged_task{std::allocator_arg, std::allocator<packaged_task>{}, std::forward<Fn>(fn)} {
        }

        /// \brief Constructs a std::packaged_task object with a shared state and a copy of the task
        /// This function constructs a std::packaged_task object with a shared state and a copy of the task, initialized
        /// with std::forward<Fn>(fn). It uses the provided allocator to allocate memory necessary to store the task.
        /// \par Constraits
        /// This constructor does not participate in overload resolution if std::decay<Fn>::type is the same type as
        /// std::packaged_task<R(ArgTypes...)>.
        /// \tparam Fn Function type
        /// \tparam Allocator Allocator type
        /// \param alloc The allocator to use when storing the task
        /// \param fn The callable target to execute
        template <typename Fn, typename Allocator
#ifndef FUTURES_DOXYGEN
                  ,
                  typename = std::enable_if_t<!std::is_base_of_v<packaged_task, typename std::decay_t<Fn>>>
#endif
                  >
        explicit packaged_task(std::allocator_arg_t, Allocator const &alloc, Fn &&fn) {
            using task_object_type = detail::task_object<typename std::decay_t<Fn>, Allocator, R, Args...>;
            using task_object_alloc_traits = std::allocator_traits<typename task_object_type::allocator_type>;
            using task_object_alloc_pointer = std::pointer_traits<typename task_object_alloc_traits::pointer>;

            typename task_object_type::allocator_type a{alloc};
            typename task_object_alloc_traits::pointer ptr{task_object_alloc_traits::allocate(a, 1)};
            typename task_object_alloc_pointer::element_type *p = to_address(ptr);
            try {
                task_object_alloc_traits::construct(a, p, a, std::forward<Fn>(fn));
            } catch (...) {
                task_object_alloc_traits::deallocate(a, ptr, 1);
                throw;
            }
            task_.reset(p);
        }

        /// \brief The copy constructor is deleted, std::packaged_task is move-only.
        packaged_task(packaged_task const &) = delete;

        /// \brief Constructs a std::packaged_task with the shared state and task formerly owned by other
        packaged_task(packaged_task &&other) noexcept : future_retrived_{other.future_retrived_}, task_{std::move(other.task_)} {
            other.future_retrived_ = false;
        }

        /// \brief Destructs the task object
        ~packaged_task() {
            if (task_ && future_retrived_) {
                task_->signal_owner_destroyed();
            }
        }

        /// \brief The copy assignment is deleted, std::packaged_task is move-only.
        packaged_task &operator=(packaged_task const &) = delete;

        /// \brief Assigns a std::packaged_task with the shared state and task formerly owned by other
        packaged_task &operator=(packaged_task &&other) noexcept {
            if (BOOST_LIKELY(this != &other)) {
                packaged_task tmp{std::move(other)};
                swap(tmp);
            }
            return *this;
        }

        /// \brief Checks if the task object has a valid function
        /// \return true if *this has a shared state, false otherwise
        [[nodiscard]] bool valid() const noexcept { return nullptr != task_.get(); }

        /// \brief Swaps two task objects
        /// This function exchanges the shared states and stored tasks of *this and other
        /// \param other packaged task whose state to swap with
        void swap(packaged_task &other) noexcept {
            std::swap(future_retrived_, other.future_retrived_);
            task_.swap(other.task_);
        }

        /// \brief Returns a future object associated with the promised result
        /// This function constructs a future object that shares its state with this promise
        /// Because this library handles more than a single future type, the future type we want is
        /// a template parameter. This function expects future type constructors to accept pointers
        /// to shared states.
        template <class Future> Future get_future() {
            if (future_retrived_) {
                throw future_already_retrieved{};
            }
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            future_retrived_ = true;
            return Future{detail::static_pointer_cast<detail::shared_state<R>>(task_)};
        }

        /// \brief Executes the function and set the shared state
        /// Calls the stored task with args as the arguments. The return value of the task or any exceptions thrown are
        /// stored in the shared state
        /// The shared state is made ready and any threads waiting for this are unblocked.
        /// \param args the parameters to pass on invocation of the stored task
        void operator()(Args... args) {
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            task_->run(std::forward<Args>(args)...);
        }

        /// \brief Resets the shared state abandoning any stored results of previous executions
        /// Resets the state abandoning the results of previous executions. New shared state is constructed.
        /// Equivalent to *this = packaged_task(std::move(f)), where f is the stored task.
        void reset() {
            if (!valid()) {
                throw packaged_task_uninitialized{};
            }
            packaged_task tmp;
            tmp.task_ = task_;
            task_ = tmp.task_->reset();
            future_retrived_ = false;
        }

      private:
        /// \brief Pointer to any kind of task class, return a value or void
        using ptr_type = typename detail::task_base<R, Args...>::ptr_type;

        /// \brief True if the corresponding future has already been retrieved
        bool future_retrived_{false};

        /// \brief The function this task should execute
        ptr_type task_{};
    };

    /// \brief Specializes the std::swap algorithm
    template <typename Signature> void swap(packaged_task<Signature> &l, packaged_task<Signature> &r) noexcept {
        l.swap(r);
    }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PACKAGED_TASK_H
