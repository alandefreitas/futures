//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_SHARED_TASK_HPP
#define FUTURES_DETAIL_SHARED_TASK_HPP

#include <futures/detail/operation_state.hpp>
#include <futures/detail/utility/compressed_tuple.hpp>
#include <futures/detail/utility/to_address.hpp>
#include <futures/detail/deps/boost/core/allocator_access.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>

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
    class shared_task_base : public operation_state<R, Options> {
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
    template <class Fn, class Allocator, class Options, class R, class... Args>
    class shared_task : public shared_task_base<R, Options, Args...> {
        using function_type = Fn;
        using allocator_type = boost::allocator_rebind_t<Allocator, shared_task>;

        compressed_tuple<function_type, allocator_type> values_;

        using stop_source_base =
            typename shared_task_base<R, Options, Args...>::stop_source_type;

        function_type &
        get_function() {
            return values_.get(mp_size_t<0>{});
        }

        allocator_type &
        get_allocator() {
            return values_.get(mp_size_t<1>{});
        }



    public:
        /// Construct a task object for the specified allocator and
        /// function, copying the function
        shared_task(allocator_type const &alloc, Fn const &fn)
            : shared_task_base<R, Options, Args...>{}
            , values_{ fn, alloc } {}

        /// Construct a task object for the specified allocator and
        /// function, moving the function
        shared_task(allocator_type const &alloc, Fn &&fn)
            : shared_task_base<R, Options, Args...>{}
            , values_{ std::move(fn), alloc } {}

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
                this->get_allocator(),
                this->get_allocator(),
                std::move(this->get_function()));
        }

        stop_source_base
        get_stop_source() {
            return stop_source_base::get();
        }
        /// @}
    private:
        template <class... UArgs>
        void
        apply(UArgs &&...args) {
            try {
                this->set_value(regular_void_invoke(
                    this->get_function(),
                    std::forward<UArgs>(args)...));
            }
            catch (...) {
                this->set_exception(std::current_exception());
            }
        }
    };

    /** @} */ // @addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_DETAIL_SHARED_TASK_HPP
