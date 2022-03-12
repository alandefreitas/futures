//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FUTURES_DETAIL_SHARED_TASK_HPP
#define FUTURES_FUTURES_DETAIL_SHARED_TASK_HPP

#include <futures/detail/allocator/allocator_rebind.hpp>
#include <futures/detail/utility/empty_base.hpp>
#include <futures/detail/utility/to_address.hpp>
#include <futures/futures/detail/operation_state.hpp>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    /// Members common to shared tasks
    ///
    /// While the main purpose of shared_state_base is to differentiate the
    /// versions of `set_value`, the main purpose of this task base class is to
    /// nullify the function type and allocator from the concrete task
    /// implementation in the final packaged task.
    ///
    /// @tparam R Type returned by the task callable
    /// @tparam Args Argument types to run the task callable
    template <class R, class Options, class... Args>
    class shared_task_base : public operation_state<R, Options>
    {
    public:
        /// Virtual task destructor
        virtual ~shared_task_base() = default;

        /// Virtual function to run the task with its Args
        /// @param args Arguments
        virtual void
        run(Args &&...args)
            = 0;

        /// Reset the state
        ///
        /// This function returns a new pointer to this shared task where we
        /// reallocate everything
        ///
        /// @return New pointer to a shared_task
        virtual std::shared_ptr<shared_task_base>
        reset() = 0;
    };

    /// A shared state that contains a task
    ///
    /// A shared_task extends the shared state with a task. A task is an
    /// extension of and analogous with shared states. The main difference is
    /// that tasks also define a function that specify how to create the state,
    /// with the `run` function.
    ///
    /// In practice, a shared_task is to a packaged_task what a shared_state is
    /// to a promise.
    ///
    /// @tparam R Type returned by the task callable
    /// @tparam Args Argument types to run the task callable
    template <
        typename Fn,
        typename Allocator,
        class Options,
        typename R,
        typename... Args>
    class shared_task
        : public shared_task_base<R, Options, Args...>
#ifndef FUTURES_DOXYGEN
        , public maybe_empty<Fn>
        , public maybe_empty<allocator_rebind_t<
              Allocator,
              shared_task<Fn, Allocator, Options, R, Args...>>>
#endif
    {
    private:
        using stop_source_base = detail::maybe_empty<
            std::conditional_t<
                Options::is_stoppable,
                stop_source,
                detail::empty_value_type>,
            0>;

    public:
        /// Allocator used to allocate this task object type
        using allocator_type = allocator_rebind_t<Allocator, shared_task>;

        /// Construct a task object for the specified allocator and
        /// function, copying the function
        shared_task(const allocator_type &alloc, const Fn &fn)
            : shared_task_base<R, Options, Args...>{}, maybe_empty<Fn>{ fn },
              maybe_empty<allocator_type>{ alloc } {}

        /// Construct a task object for the specified allocator and
        /// function, moving the function
        shared_task(const allocator_type &alloc, Fn &&fn)
            : shared_task_base<R, Options, Args...>{},
              maybe_empty<Fn>{ std::move(fn) },
              maybe_empty<allocator_type>{ alloc } {}

        /// No copy constructor
        shared_task(shared_task const &) = delete;

        /// No copy assignment
        shared_task &
        operator=(shared_task const &)
            = delete;

        /// Virtual shared task destructor
        virtual ~shared_task() = default;

        /// Run the task function with the given arguments and use
        /// the result to set the shared state value @param args Arguments
        void
        run(Args &&...args) final {
            if constexpr (!Options::is_stoppable) {
                apply(std::forward<Args>(args)...);
            } else {
                apply(
                    stop_source_base::get().get_token(),
                    std::forward<Args>(args)...);
            }
        }

        /// Reallocate and reconstruct a task object
        ///
        /// This constructs a task object of same type from scratch.
        typename std::shared_ptr<shared_task_base<R, Options, Args...>>
        reset() final {
            return std::allocate_shared<shared_task>(
                maybe_empty<allocator_type>::get(),
                maybe_empty<allocator_type>::get(),
                std::move(maybe_empty<Fn>::get()));
        }

        typename stop_source_base::value_type
        get_stop_source() {
            return stop_source_base::get();
        }
        /// @}
    private:
        template <class... UArgs>
        void
        apply(UArgs &&...args) {
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
    };

    /** @} */ // @addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_FUTURES_DETAIL_SHARED_TASK_HPP
