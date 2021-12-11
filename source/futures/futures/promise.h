//
// Copyright (c) alandefreitas 11/30/21.
// See accompanying file LICENSE
//

#ifndef FUTURES_PROMISE_H
#define FUTURES_PROMISE_H

#include <memory>

#include <futures/futures/detail/empty_base.h>
#include <futures/futures/detail/shared_state.h>
#include <futures/futures/detail/to_address.h>

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup shared_state Shared State
     *  @{
     */

    namespace detail {
        /// \brief A concrete and allocator aware implementation of a shared state with its destructor
        /// This is the shared_state implementation used by promises
        /// Only the shared_state<R> overload needs allocators
        template <typename R, typename Allocator>
        class shared_state_object
            : public shared_state<R>,
              public maybe_empty<
                  /* maybe_empty<allocator_type> <- see below */
                  typename std::allocator_traits<Allocator>::template rebind_alloc<shared_state_object<R, Allocator>>> {
          public:
            /// \brief The allocator used for elements of type R
            using allocator_type =
                typename std::allocator_traits<Allocator>::template rebind_alloc<shared_state_object>;

            using maybe_empty_allocator_type = maybe_empty<allocator_type>;

            /// \brief The allocator traits for the allocator_type
            using allocator_traits_type = std::allocator_traits<allocator_type>;

            /// \brief Create promise shared state with the specified allocator
            /// The base class shared_state will create uninitialized aligned storage for R
            explicit shared_state_object(allocator_type const &rhs_alloc)
                : shared_state<R>{}, maybe_empty_allocator_type{rhs_alloc} {}

          protected:
            /// \brief Concrete function to deallocate the shared state
            /// It uses the underlying allocator to deallocate this very shared state
            void deallocate_future() noexcept final { destroy(alloc(), this); }

          private:
            /// \brief Allocator we use to create the object of type R
            /// \brief Get stop token from the base class as function for convenience
            const allocator_type &alloc() const { return maybe_empty<allocator_type>::get(); }

            /// \brief Get stop token from the base class as function for convenience
            allocator_type &alloc() { return maybe_empty<allocator_type>::get(); }

            /// \brief The destroy function implementation
            /// This function destroys the element with our custom allocator
            static void destroy(allocator_type const &alloc, shared_state_object *p) noexcept {
                allocator_type a{alloc};
                allocator_traits_type::destroy(a, p);
                allocator_traits_type::deallocate(a, p, 1);
            }
        };

        /// \brief Common members to promises of all types
        /// This includes a pointer to the corresponding shared_state for the future
        template <typename R> class promise_base {
          public:
            /// \brief Create the base promise with std::allocator
            /// Use std::allocator_arg tag to dispatch and select allocator aware constructor
            promise_base() : promise_base{std::allocator_arg, std::allocator<promise_base>{}} {}

            /// \brief Create a base promise setting the shared state with the specified allocator
            /// This function allocates memory for and allocates an initial shared_state_object (the future value)
            /// with the specified allocator. This object is stored in the internal intrusive pointer as the
            /// future shared state.
            template <typename Allocator> promise_base(std::allocator_arg_t, Allocator alloc) {
                using object_type = detail::shared_state_object<R, Allocator>;
                using traits_type = std::allocator_traits<typename object_type::allocator_type>;
                using pointer_traits_type = std::pointer_traits<typename traits_type::pointer>;

                typename object_type::allocator_type a{alloc};
                typename traits_type::pointer ptr{traits_type::allocate(a, 1)};
                typename pointer_traits_type::element_type *p = to_address(ptr);

                try {
                    traits_type::construct(a, p, a);
                } catch (...) {
                    traits_type::deallocate(a, ptr, 1);
                    throw;
                }
                shared_state_.reset(p);
            }

            /// \brief No copy constructor
            promise_base(promise_base const &) = delete;

            /// \brief Move constructor
            promise_base(promise_base &&other) noexcept
                : obtained_{other.obtained_}, shared_state_{std::move(other.shared_state_)} {
                other.obtained_ = false;
            }

            /// \brief No copy assignment
            promise_base &operator=(promise_base const &) = delete;

            /// \brief Move assignment
            promise_base &operator=(promise_base &&other) noexcept {
                if (this != &other) {
                    promise_base tmp{std::move(other)};
                    swap(tmp);
                }
                return *this;
            }

            /// \brief Destructor
            /// If the future has been set at some point and it has already been obtained by the respective
            /// future to this promise somewhere else, we warn the base shared_state
            ~promise_base() {
                if (shared_state_ && obtained_) {
                    shared_state_->signal_owner_destroyed();
                }
            }

            /// \brief Gets a future that shares its state with this promise
            /// This function constructs a future object that shares its state with this promise
            /// Because this library handles more than a single future type, the future type we want is
            /// a template parameter. This function expects future type constructors to accept pointers
            /// to shared states.
            template <class Future> Future get_future() {
                if (obtained_) {
                    throw future_already_retrieved{};
                }
                if (!shared_state_) {
                    throw promise_uninitialized{};
                }
                obtained_ = true;
                return Future{shared_state_};
            }

            /// \brief Swap the value of two promises
            void swap(promise_base &other) noexcept {
                std::swap(obtained_, other.obtained_);
                shared_state_.swap(other.shared_state_);
            }

            /// \brief Set the promise result as an exception
            /// \note The set_value operation is only available at the concrete derived class,
            /// where we know the class type
            void set_exception(std::exception_ptr p) {
                if (!shared_state_) {
                    throw promise_uninitialized{};
                }
                shared_state_->set_exception(p);
            }

            /// \brief Set the promise result as an exception
            template <typename E
#ifndef FUTURES_DOXYGEN
                      ,
                      std::enable_if_t<std::is_base_of_v<std::exception, E>, int> = 0
#endif
                      >
            void set_exception(E e) {
                set_exception(std::make_exception_ptr(e));
            }

            /// \brief True if the future has already obtained the shared state
            bool obtained_{false};

            /// \brief Intrusive pointer to the future corresponding to this promise
            typename shared_state<R>::ptr_type shared_state_{};
        };

    } // namespace detail

    /// \brief A shared state that will later be acquired by a future type
    template <typename R> class promise {
      public:
        /// \brief Create the promise for type R
        promise() = default;

        /// \brief Create the promise for type R with the specified allocator
        template <typename Allocator>
        promise(std::allocator_arg_t, Allocator alloc) : base_type{std::allocator_arg, alloc} {}

        /// \brief Delete copy constructor
        promise(promise const &) = delete;

        /// \brief Move construct the base promise contents
        promise(promise &&other) noexcept = default;

        /// \brief Deleted copy assignment
        promise &operator=(promise const &) = delete;

        /// \brief Move assign the base promise contents
        promise &operator=(promise &&other) noexcept = default;

        /// \brief Copy and set the promise value so it can be obtained by the future
        /// \param value lvalue reference to the shared state value
        void set_value(R const &value) {
            if (!base_promise_.shared_state_) {
                throw promise_uninitialized{};
            }
            base_promise_.shared_state_->set_value(value);
        }

        /// \brief Move and set the promise value so it can be obtained by the future
        /// \param value rvalue reference to the shared state value
        void set_value(R &&value) {
            if (!base_promise_.shared_state_) {
                throw promise_uninitialized{};
            }
            base_promise_.shared_state_->set_value(std::move(value));
        }

        /// \brief Swap the value of two promises
        void swap(promise &other) noexcept { base_promise_.swap(other.base_promise_); }

        /// \brief Gets a future that shares its state with this promise
        /// This function constructs a future object that shares its state with this promise
        /// Because this library handles more than a single future type, the future type we want is
        /// a template parameter. This function expects future type constructors to accept pointers
        /// to shared states.
        template <class Future> Future get_future() { return base_promise_.template get_future<Future>(); }

        /// \brief Set the promise result as an exception
        void set_exception(std::exception_ptr p) { base_promise_.set_exception(p); }

        /// \brief Set the promise result as an exception
        template <typename E
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<std::is_base_of_v<std::exception, E>, int> = 0
#endif
                  >
        void set_exception(E e) {
            base_promise_.set_exception(std::make_exception_ptr(e));
        }

      private:
        /// \brief Type of the promise contents
        using base_type = detail::promise_base<R>;

        /// \brief Base class with promise contents
        base_type base_promise_;
    };

    /// \brief A shared state that will later be acquired by a future type
    template <typename R> class promise<R &> {
      public:
        /// \brief Create the promise for type R
        promise() = default;

        /// \brief Create the promise for type R with the specified allocator
        template <typename Allocator>
        promise(std::allocator_arg_t, Allocator alloc) : base_type{std::allocator_arg, alloc} {}

        /// \brief Delete copy constructor
        promise(promise const &) = delete;

        /// \brief Move construct the base promise contents
        promise(promise &&other) noexcept = default;

        /// \brief Deleted copy assignment
        promise &operator=(promise const &) = delete;

        /// \brief Move assign the base promise contents
        promise &operator=(promise &&other) noexcept = default;

        /// \brief Set the promise value so it can be obtained by the future
        void set_value(R &value) {
            if (!base_promise_.shared_state_) {
                throw promise_uninitialized{};
            }
            base_promise_.shared_state_->set_value(value);
        }

        /// \brief Swap the value of two promises
        void swap(promise &other) noexcept { base_promise_.swap(other.base_promise_); }

        /// \brief Gets a future that shares its state with this promise
        /// This function constructs a future object that shares its state with this promise
        /// Because this library handles more than a single future type, the future type we want is
        /// a template parameter. This function expects future type constructors to accept pointers
        /// to shared states.
        template <class Future> Future get_future() { return base_promise_.template get_future<Future>(); }

        /// \brief Set the promise result as an exception
        void set_exception(std::exception_ptr p) { base_promise_.set_exception(p); }

        /// \brief Set the promise result as an exception
        template <typename E
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<std::is_base_of_v<std::exception, E>, int> = 0
#endif
                  >
        void set_exception(E e) {
            base_promise_.set_exception(std::make_exception_ptr(e));
        }

      private:
        /// \brief Type of the promise contents
        using base_type = detail::promise_base<R &>;

        /// \brief Base class with promise contents
        base_type base_promise_;
    };

    /// \brief A shared state that will later be acquired by a future type
    template <> class promise<void> {
      public:
        /// \brief Create the promise for type R
        promise() = default;

        /// \brief Create the promise for type R with the specified allocator
        template <typename Allocator>
        promise(std::allocator_arg_t, Allocator alloc) : base_type{std::allocator_arg, alloc} {}

        /// \brief Delete copy constructor
        promise(promise const &) = delete;

        /// \brief Move construct the base promise contents
        promise(promise &&other) noexcept = default;

        /// \brief Deleted copy assignment
        promise &operator=(promise const &) = delete;

        /// \brief Move assign the base promise contents
        promise &operator=(promise &&other) noexcept = default;

        /// \brief Set the promise value so it can be obtained by the future
        void set_value() { // NOLINT(readability-make-member-function-const)
            if (!base_promise_.shared_state_) {
                throw promise_uninitialized{};
            }
            base_promise_.shared_state_->set_value();
        }

        /// \brief Swap the value of two promises
        void swap(promise &other) noexcept { base_promise_.swap(other.base_promise_); }

        /// \brief Gets a future that shares its state with this promise
        /// This function constructs a future object that shares its state with this promise
        /// Because this library handles more than a single future type, the future type we want is
        /// a template parameter. This function expects future type constructors to accept pointers
        /// to shared states.
        template <class Future> Future get_future() { return base_promise_.template get_future<Future>(); }

        /// \brief Set the promise result as an exception
        void set_exception(std::exception_ptr p) { // NOLINT(performance-unnecessary-value-param)
            base_promise_.set_exception(p);
        }

        /// \brief Set the promise result as an exception
        template <typename E
#ifndef FUTURES_DOXYGEN
                  ,
                  std::enable_if_t<std::is_base_of_v<std::exception, E>, int> = 0
#endif
                  >
        void set_exception(E e) {
            base_promise_.set_exception(std::make_exception_ptr(e));
        }

      private:
        /// \brief Type of the promise contents
        using base_type = detail::promise_base<void>;

        /// \brief Base class with promise contents
        base_type base_promise_;
    };

    /// \brief Swap the value of two promises
    template <typename R> void swap(promise<R> &l, promise<R> &r) noexcept { l.swap(r); }

    /** @} */
    /** @} */
} // namespace futures

#endif // FUTURES_PROMISE_H
