//
// Copyright (c) alandefreitas 12/1/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_TASK_H
#define FUTURES_TASK_H

#include <futures/futures/detail/intrusive_ptr.h>
#include <futures/futures/detail/to_address.h>
#include <futures/futures/detail/shared_state.h>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief Abstract class with the members common to all task objects regardless of type
    ///
    /// A task is an extension of and analogous with shared states. The main difference is that tasks also store and
    /// wraps a function to create the state besides the state information.
    ///
    /// Tasks are to packaged_tasks what shared states are to promises.
    ///
    /// \tparam R Type returned by the task callable
    /// \tparam Args Argument types to run the task callable
    template <typename R, typename... Args> class task_base : public shared_state<R> {
      public:
        /// \brief Type of pointer to this task and its shared state
        using ptr_type = intrusive_ptr<task_base>;

        /// \brief Virtual task destructor
        virtual ~task_base() = default;

        /// \brief Virtual function to run the task with its Args
        /// \param args Arguments
        virtual void run(Args &&...args) = 0;

        /// \brief Reset the state
        virtual ptr_type reset() = 0;
    };

    /// \brief A task object storing a function of any type, an allocator for the shared state and the state
    template <typename Fn, typename Allocator, typename R, typename... Args>
    class task_object : public task_base<R, Args...> {
      public:
        /// \brief Allocator used to allocate this task object type
        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<task_object>;

        /// \brief Construct a task object for the specified allocator and function, copying the function
        task_object(allocator_type const &alloc, Fn const &fn) : base_type{}, fn_{fn}, alloc_{alloc} {}

        /// \brief Construct a task object for the specified allocator and function, moving the function
        task_object(allocator_type const &alloc, Fn &&fn) : base_type{}, fn_{std::move(fn)}, alloc_{alloc} {}

        /// \brief Run the task function with the given arguments and use the result to set the shared state value
        void run(Args &&...args) final {
            try {
                this->set_value(std::apply(fn_, std::make_tuple(std::forward<Args>(args)...)));
            } catch (...) {
                this->set_exception(std::current_exception());
            }
        }

        /// \brief Allocate and construct a task object
        typename task_base<R, Args...>::ptr_type reset() final {
            using task_object_alloc_traits = std::allocator_traits<allocator_type>;
            using task_object_alloc_ptr = typename task_object_alloc_traits::pointer;
            using task_object_alloc_ptr_traits = std::pointer_traits<task_object_alloc_ptr>;

            typename task_object_alloc_traits::pointer ptr{task_object_alloc_traits::allocate(alloc_, 1)};
            typename task_object_alloc_ptr_traits::element_type *p = to_address(ptr);
            try {
                task_object_alloc_traits::construct(alloc_, p, alloc_, std::move(fn_));
            } catch (...) {
                task_object_alloc_traits::deallocate(alloc_, ptr, 1);
                throw;
            }
            return {p};
        }

      protected:
        /// \brief Deallocate this task object
        void deallocate_future() noexcept final { destroy(alloc_, this); }

      private:
        /// \brief Base task type defining the abstract function to run the task and the shared state
        using base_type = task_base<R, Args...>;

        /// \brief Internal function representing the function
        Fn fn_;

        /// \brief Allocator for the shared state
        allocator_type alloc_;

        /// \brief Destroy a task object with a given allocator
        /// This function destroys and deallocates the specified task_object
        static void destroy(allocator_type const &alloc, task_object *p) noexcept {
            allocator_type a{alloc};
            using traity_type = std::allocator_traits<allocator_type>;
            traity_type::destroy(a, p);
            traity_type::deallocate(a, p, 1);
        }
    };

    /// \brief A task object storing a function of any type, an allocator for the shared state and the state
    /// This overload is used for tasks that return void
    template <typename Fn, typename Allocator, typename... Args>
    class task_object<Fn, Allocator, void, Args...> : public task_base<void, Args...> {
      public:
        /// \brief Allocator used to allocate this task object type
        using allocator_type =
            typename std::allocator_traits<Allocator>::template rebind_alloc<task_object<Fn, Allocator, void, Args...>>;

        /// \brief Construct a task object for the specified allocator and function, copying the function
        task_object(allocator_type const &alloc, Fn const &fn) : base_type{}, fn_{fn}, alloc_{alloc} {}

        /// \brief Construct a task object for the specified allocator and function, moving the function
        task_object(allocator_type const &alloc, Fn &&fn) : base_type{}, fn_{std::move(fn)}, alloc_{alloc} {}

        /// \brief Run the task function with the given arguments and use the result to set the shared state as done
        void run(Args &&...args) final {
            try {
                std::apply(fn_, std::make_tuple(std::forward<Args>(args)...));
                this->set_value();
            } catch (...) {
                this->set_exception(std::current_exception());
            }
        }

        /// \brief Allocate and construct a task object
        typename task_base<void, Args...>::ptr_type reset() final {
            using task_object_alloc_traits = std::allocator_traits<allocator_type>;
            using task_object_alloc_ptr_traits = std::pointer_traits<typename task_object_alloc_traits::pointer>;

            typename task_object_alloc_traits::pointer ptr{task_object_alloc_traits::allocate(alloc_, 1)};
            typename task_object_alloc_ptr_traits::element_type *p = to_address(ptr);
            try {
                task_object_alloc_traits::construct(alloc_, p, alloc_, std::move(fn_));
            } catch (...) {
                task_object_alloc_traits::deallocate(alloc_, ptr, 1);
                throw;
            }
            return {p};
        }

      protected:
        /// \brief Deallocate this task object
        void deallocate_future() noexcept final { destroy(alloc_, this); }

      private:
        /// \brief Base task type defining the abstract function to run the task and the shared state
        using base_type = task_base<void, Args...>;

        /// \brief Internal function representing the function
        Fn fn_;

        /// \brief Allocator for the shared state
        allocator_type alloc_;

        /// \brief Destroy a task object with a given allocator
        /// This function destroys and deallocates the specified task_object
        static void destroy(allocator_type const &alloc, task_object *p) noexcept {
            allocator_type a{alloc};
            using task_object_alloc_traits = std::allocator_traits<allocator_type>;
            task_object_alloc_traits::destroy(a, p);
            task_object_alloc_traits::deallocate(a, p, 1);
        }
    };

    /** @} */ // \addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_TASK_H
