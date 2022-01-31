//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_SHARED_TASK_H
#define FUTURES_SHARED_TASK_H

#include <futures/executor/detail/allocator_rebind.hpp>
#include <futures/futures/detail/empty_base.hpp>
#include <futures/futures/detail/shared_state.hpp>
#include <futures/futures/detail/to_address.hpp>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Members common to shared tasks
    ///
    /// While the main purpose of shared_state_base is to differentiate the
    /// versions of `set_value`, the main purpose of this task base class is to
    /// nullify the function type and allocator from the concrete task
    /// implementation in the final packaged task.
    ///
    /// \tparam R Type returned by the task callable
    /// \tparam Args Argument types to run the task callable
    template <typename R, typename... Args>
    class shared_task_base : public shared_state<R>
    {
    public:
        /// \brief Virtual task destructor
        virtual ~shared_task_base() = default;

        /// \brief Virtual function to run the task with its Args
        /// \param args Arguments
        virtual void
        run(Args &&...args)
            = 0;

        /// \brief Reset the state
        ///
        /// This function returns a new pointer to this shared task where we
        /// reallocate everything
        ///
        /// \return New pointer to a shared_task
        virtual std::shared_ptr<shared_task_base>
        reset() = 0;
    };

    /// \brief A shared state that contains a task
    ///
    /// A shared_task extends the shared state with a task. A task is an
    /// extension of and analogous with shared states. The main difference is
    /// that tasks also define a function that specify how to create the state,
    /// with the `run` function.
    ///
    /// In practice, a shared_task is to a packaged_task what a shared_state is
    /// to a promise.
    ///
    /// \tparam R Type returned by the task callable
    /// \tparam Args Argument types to run the task callable
    template <typename Fn, typename Allocator, typename R, typename... Args>
    class shared_task
        : public shared_task_base<R, Args...>
#ifndef FUTURES_DOXYGEN
        , public maybe_empty<Fn>
        , public maybe_empty<allocator_rebind_t<
              Allocator,
              shared_task<Fn, Allocator, R, Args...>>>
#endif
    {
    public:
        /// \brief Allocator used to allocate this task object type
        using allocator_type = allocator_rebind_t<Allocator, shared_task>;

        /// \brief Construct a task object for the specified allocator and
        /// function, copying the function
        shared_task(const allocator_type &alloc, const Fn &fn)
            : shared_task_base<R, Args...>{}, maybe_empty<Fn>{ fn },
              maybe_empty<allocator_type>{ alloc } {}

        /// \brief Construct a task object for the specified allocator and
        /// function, moving the function
        shared_task(const allocator_type &alloc, Fn &&fn)
            : shared_task_base<R, Args...>{}, maybe_empty<Fn>{ std::move(fn) },
              maybe_empty<allocator_type>{ alloc } {}

        /// \brief No copy constructor
        shared_task(shared_task const &) = delete;

        /// \brief No copy assignment
        shared_task &
        operator=(shared_task const &)
            = delete;

        /// \brief Virtual shared task destructor
        virtual ~shared_task() = default;

        /// \brief Run the task function with the given arguments and use the
        /// result to set the shared state value \param args Arguments
        void
        run(Args &&...args) final {
            try {
                if constexpr (std::is_void_v<R>) {
                    std::apply(maybe_empty<Fn>::get(), std::make_tuple(args...));
                    this->set_value();
                } else {
                    this->set_value(std::apply(
                        maybe_empty<Fn>::get(),
                        std::make_tuple(args...)));
                }
            }
            catch (...) {
                this->set_exception(std::current_exception());
            }
        }

        /// \brief Reallocate and reconstruct a task object
        ///
        /// This constructs a task object of same type from scratch.
        typename std::shared_ptr<shared_task_base<R, Args...>>
        reset() final {
            return std::allocate_shared<shared_task>(
                maybe_empty<allocator_type>::get(),
                maybe_empty<allocator_type>::get(),
                std::move(maybe_empty<Fn>::get()));
        }

        /// @}
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_SHARED_TASK_H
