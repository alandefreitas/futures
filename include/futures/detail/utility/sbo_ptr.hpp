//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_SBO_PTR_HPP
#define FUTURES_DETAIL_UTILITY_SBO_PTR_HPP

#include <futures/config.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/deps/boost/core/allocator_access.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <utility>

namespace futures {
    namespace detail {
        /*
         * The idea comes from https://github.com/MiSo1289/sboptr, but we
         * have very different requirement. In particular, we need to allow
         * custom allocators and there's no need for pointers that don't
         * allow heap allocations.
         */
        using sbo_ptr_options = unsigned;
        FUTURES_INLINE_VAR constexpr auto no_options = sbo_ptr_options{};
        FUTURES_INLINE_VAR constexpr auto movable = sbo_ptr_options{ 1u << 0u };
        FUTURES_INLINE_VAR constexpr auto copyable = sbo_ptr_options{
            1u << 1u
        };
        FUTURES_INLINE_VAR constexpr auto allow_heap = sbo_ptr_options{
            1u << 2u
        };

        template <
            class Base,
            class Allocator,
            std::size_t sbo_size,
            bool enable_move,
            bool enable_copy>
        class sbo_ptr_base;

        template <class Base, class Allocator, std::size_t sbo_size>
        class sbo_ptr_base<
            Base,
            Allocator,
            sbo_size,
            /* enable_move */ false,
            /* enable_copy */ false>
            : protected boost::empty_value<Allocator, 0> {
        public:
            // Default allocator
            sbo_ptr_base() noexcept = default;

            // Custom allocator
            sbo_ptr_base(Allocator const& a) noexcept(
                is_nothrow_constructible_v<Allocator, Allocator const&>)
                : alloc_base(boost::empty_init, a) {}

            // Can't copy construct
            sbo_ptr_base(sbo_ptr_base const&) = delete;

            // Can't move construct
            sbo_ptr_base(sbo_ptr_base&&) noexcept = delete;

            // Can't copy assign
            sbo_ptr_base&
            operator=(sbo_ptr_base const&)
                = delete;

            // Can't move assign
            sbo_ptr_base&
            operator=(sbo_ptr_base&&) noexcept
                = delete;

            // Destroy if it's on heap
            ~sbo_ptr_base() noexcept {
                destroy();
            }

        protected:
            using alloc_base = boost::empty_value<Allocator, 0>;
            Base* ptr_ = nullptr;
            void (*heap_destroy_)(void* /* allocator */, void* /* ptr */)
                = nullptr;

            bool
            on_heap() const {
                return ptr_ != reinterpret_cast<Base const*>(sbo_buffer_);
            };

            template <class Derived, class... Args>
            using can_construct = conjunction<
                std::is_convertible<Derived*, Base*>,
                std::is_constructible<Derived, Args&&...>>;

            // clang-format off
            FUTURES_TEMPLATE(class Derived, class... Args)
            (requires can_construct<Derived, Args...>::value)
            void construct(Args&&... args)
                noexcept(
                    is_nothrow_constructible_v<Derived, Args&&...> &&
                    (sizeof(Derived) <= sbo_size))
            {
                // clang-format on
                FUTURES_IF_CONSTEXPR (sizeof(Derived) <= sbo_size) {
                    // construct on the buffer
                    ptr_ = reinterpret_cast<Base*>(&sbo_buffer_);
                    ::new ((void*) ptr_) Derived(std::forward<Args>(args)...);
                } else {
                    // allocate and construct
                    auto alloc = boost::allocator_rebind_t<Allocator, Derived>(
                        alloc_base::get());
                    ptr_ = boost::allocator_allocate(alloc, 1);
                    boost::allocator_construct(
                        alloc,
                        reinterpret_cast<Derived*>(ptr_),
                        std::forward<Args>(args)...);
                    // handle to destroy with the allocator
                    heap_destroy_ = {
                        [](void* alloc, void* from) {
                            FUTURES_ASSERT(alloc);
                            FUTURES_ASSERT(from);
                            auto a = boost::allocator_rebind_t<Allocator, Derived>(*reinterpret_cast<Allocator*>(alloc));
                            boost::allocator_destroy(a, reinterpret_cast<Derived*>(from));
                            boost::allocator_deallocate(a, reinterpret_cast<Derived*>(from), 1);
                        },
                    };
                }
            }

            void
            destroy() noexcept {
                if (ptr_) {
                    if (on_heap()) {
                        auto& a = alloc_base::get();
                        heap_destroy_(&a, ptr_);
                        heap_destroy_ = nullptr;
                    } else {
                        ptr_->~Base();
                    }
                    ptr_ = nullptr;
                }
            }

        private:
            alignas(
                alignof(std::max_align_t)) unsigned char sbo_buffer_[sbo_size];
        };

        template <class Base, class Allocator, std::size_t sbo_size>
        class sbo_ptr_base<
            Base,
            Allocator,
            sbo_size,
            /* enable_move */ true,
            /* enable_copy */ false>
            : protected boost::empty_value<Allocator, 0> {
        public:
            // Default allocator
            sbo_ptr_base() noexcept = default;

            // Custom allocator
            sbo_ptr_base(Allocator const& a) noexcept(
                is_nothrow_constructible_v<Allocator, Allocator const&>)
                : alloc_base(boost::empty_init, a) {}

            // Can't copy
            sbo_ptr_base(sbo_ptr_base const&) = delete;

            // Move constructor
            sbo_ptr_base(sbo_ptr_base&& other) noexcept
                : alloc_base(
                    boost::empty_init,
                    std::move(other.alloc_base::get())) {
                transfer_ownership_from(std::move(other));
            }

            // Can't copy assign
            sbo_ptr_base&
            operator=(sbo_ptr_base const&)
                = delete;

            // Move assignment
            sbo_ptr_base&
            operator=(sbo_ptr_base&& other) noexcept {
                if (this != &other) {
                    // destroy this now at the first step because the new
                    // allocator might not be able to
                    destroy();
                    // https://stackoverflow.com/questions/27471053/example-usage-of-propagate-on-container-move-assignment
                    static constexpr bool propagate = boost::
                        allocator_propagate_on_container_move_assignment<
                            Allocator>::type::value;
                    auto& this_alloc = alloc_base::get();
                    auto& other_alloc = other.alloc_base::get();
                    FUTURES_IF_CONSTEXPR (propagate) {
                        // move assign allocator
                        this_alloc = std::move(other_alloc);
                        // Transfer memory ownership. This new allocator should
                        // be able to later deallocate it.
                        transfer_ownership_from(std::move(other));
                    } else {
                        if (this_alloc == other_alloc) {
                            // allocators are the same, just move pointers
                            transfer_ownership_from(std::move(other));
                        } else {
                            // cannot transfer ownership of any memory
                            // we need to move construct the underlying object
                            construct_from(std::move(other));
                        }
                    }
                }
                return *this;
            }

            // Destroy if it's on the heap
            ~sbo_ptr_base() noexcept {
                destroy();
            }

        protected:
            using alloc_base = boost::empty_value<Allocator, 0>;
            Base* ptr_ = nullptr;

            // stores how to move this object
            Base* (*move_)(void* /* alloc */, void* /* from */, void* /* to */)
                = nullptr;
            void (*heap_destroy_)(void* /* allocator */, void* /* ptr */)
                = nullptr;

            bool
            on_heap() const {
                return ptr_ != reinterpret_cast<Base const*>(sbo_buffer_);
            };

            template <class Derived, class... Args>
            using can_construct = conjunction<
                std::is_convertible<Derived*, Base*>,
                std::is_constructible<Derived, Args&&...>,
                std::is_nothrow_move_constructible<Derived>>;

            // clang-format off
            FUTURES_TEMPLATE(class Derived, class... Args)
            (requires can_construct<Derived, Args...>::value)
            void
            construct(Args&&... args)
                noexcept(
                    is_nothrow_constructible_v<Derived, Args&&...> &&
                    (sizeof(Derived) <= sbo_size))
            {
                // clang-format on
                FUTURES_IF_CONSTEXPR (sizeof(Derived) <= sbo_size) {
                    // construct on buffer
                    ptr_ = reinterpret_cast<Base*>(&sbo_buffer_);
                    ::new ((void*) ptr_) Derived(std::forward<Args>(args)...);
                    move_ = {
                        [](void*, void* from, void* to) -> Base* {
                            FUTURES_ASSERT(from);
                            FUTURES_ASSERT(to);
                            new (to) Derived(
                                std::move(*reinterpret_cast<Derived*>(from)));
                            reinterpret_cast<Derived*>(from)->~Derived();
                            return reinterpret_cast<Base*>(to);
                        },
                    };
                } else {
                    // construct on heap
                    auto a = boost::allocator_rebind_t<Allocator, Derived>(
                        alloc_base::get());
                    ptr_ = boost::allocator_allocate(a, 1);
                    boost::allocator_construct(
                        a,
                        reinterpret_cast<Derived*>(ptr_),
                        std::forward<Args>(args)...);
                    move_ = {
                        [](void* alloc, void* from, void* to) -> Base* {
                            FUTURES_ASSERT(alloc);
                            FUTURES_ASSERT(from);
                            FUTURES_ASSERT(!to);
                            auto dalloc = boost::
                                allocator_rebind_t<Allocator, Derived>(
                                    *reinterpret_cast<Allocator*>(alloc));
                            to = boost::allocator_allocate(dalloc, 1);
                            boost::allocator_construct(
                                dalloc,
                                reinterpret_cast<Derived*>(to),
                                std::move(*reinterpret_cast<Derived*>(from)));
                            return reinterpret_cast<Base*>(to);
                        },
                    };
                    heap_destroy_ = {
                        [](void* alloc, void* from) {
                        FUTURES_ASSERT(alloc);
                        FUTURES_ASSERT(from);
                        auto a = boost::allocator_rebind_t<Allocator, Derived>(*reinterpret_cast<Allocator*>(alloc));
                        boost::allocator_destroy(a, reinterpret_cast<Derived*>(from));
                        boost::allocator_deallocate(a, reinterpret_cast<Derived*>(from), 1);
                        },
                    };
                }
            }

            void
            transfer_ownership_from(sbo_ptr_base&& other) noexcept {
                move_resources_from(std::true_type{}, std::move(other));
            }

            void
            construct_from(sbo_ptr_base&& other) noexcept {
                move_resources_from(std::false_type{}, std::move(other));
            }

            template <class StealPtrResource>
            void
            move_resources_from(
                StealPtrResource,
                sbo_ptr_base&& other) noexcept {
                FUTURES_STATIC_ASSERT(
                    std::is_same<StealPtrResource, std::true_type>::value
                    || std::is_same<StealPtrResource, std::false_type>::value);
                // This is only executed after destroy()
                FUTURES_ASSERT(!ptr_);
                if (other.ptr_) {
                    if (!other.on_heap()) {
                        // on buffer, we can't transfer ownership or just
                        // memmove because we are not sure if the underlying
                        // object is relocatable
                        ptr_ = other.move_(nullptr, other.ptr_, sbo_buffer_);
                        other.ptr_ = nullptr;
                    } else {
                        move_ptr_resource(StealPtrResource{}, std::move(other));
                    }
                    heap_destroy_ = std::exchange(other.heap_destroy_, nullptr);
                    move_ = std::exchange(other.move_, nullptr);
                }
            }

            void
            move_ptr_resource(std::true_type, sbo_ptr_base&& other) {
                ptr_ = std::exchange(other.ptr_, nullptr);
            }

            void
            move_ptr_resource(std::false_type, sbo_ptr_base&& other) {
                // we asked to construct because the allocators do not
                // propagate at move_construction and are different
                // This means this allocator cannot manage memory
                // allocated by other, and we need to move the object
                Allocator& a0 = alloc_base::get();
                ptr_ = other.move_(&a0, other.ptr_, nullptr);
                // Delete the moved from object from other
                Allocator& a1 = alloc_base::get();
                other.heap_destroy_(&a1, other.ptr_);
                other.ptr_ = nullptr;
            }

            void
            destroy() noexcept {
                if (ptr_) {
                    if (on_heap()) {
                        auto& a = alloc_base::get();
                        heap_destroy_(&a, ptr_);
                        heap_destroy_ = nullptr;
                    } else {
                        ptr_->~Base();
                        move_ = nullptr;
                    }
                    ptr_ = nullptr;
                }
            }

        private:
            alignas(
                alignof(std::max_align_t)) unsigned char sbo_buffer_[sbo_size];
        };

        template <class Base, class Allocator, std::size_t sbo_size>
        class sbo_ptr_base<
            Base,
            Allocator,
            sbo_size,
            /* enable_move */ true,
            /* enable_copy */ true>
            : protected boost::empty_value<Allocator, 0> {
        public:
            // Default allocator
            sbo_ptr_base() noexcept = default;

            // Custom allocator
            sbo_ptr_base(Allocator const& a) noexcept(
                is_nothrow_constructible_v<Allocator, Allocator const&>)
                : alloc_base(boost::empty_init, a) {}

            // Copy constructor
            sbo_ptr_base(sbo_ptr_base const& other)
                : alloc_base(
                    boost::empty_init,
                    boost::allocator_select_on_container_copy_construction(
                        other.alloc_base::get())) {
                construct_from(other);
            }

            // Move constructor
            sbo_ptr_base(sbo_ptr_base&& other) noexcept
                : alloc_base(
                    boost::empty_init,
                    std::move(other.alloc_base::get())) {
                transfer_ownership_from(std::move(other));
            }

            // Copy assignment
            sbo_ptr_base&
            operator=(sbo_ptr_base const& other) {
                if (this != &other) {
                    // Strong exception guarantee
                    sbo_ptr_base tmp(other);
                    destroy();
                    FUTURES_IF_CONSTEXPR (
                        boost::allocator_propagate_on_container_copy_assignment<
                            Allocator>::type ::value)
                    {
                        alloc_base::get() = other.alloc_base::get();
                    }
                    construct_from(std::move(tmp));
                }
                return *this;
            }

            // Move assignment
            sbo_ptr_base&
            operator=(sbo_ptr_base&& other) noexcept {
                if (this != &other) {
                    // destroy this now at the first step because the new
                    // allocator might not be able to
                    destroy();
                    // https://stackoverflow.com/questions/27471053/example-usage-of-propagate-on-container-move-assignment
                    static constexpr bool propagate = boost::
                        allocator_propagate_on_container_move_assignment<
                            Allocator>::type::value;
                    auto& this_alloc = alloc_base::get();
                    auto& other_alloc = other.alloc_base::get();
                    FUTURES_IF_CONSTEXPR (propagate) {
                        // move assign allocator
                        this_alloc = std::move(other_alloc);
                        // Transfer memory ownership. This new allocator should
                        // be able to later deallocate it.
                        transfer_ownership_from(std::move(other));
                    } else {
                        if (this_alloc == other_alloc) {
                            // allocators are the same, just move pointers
                            transfer_ownership_from(std::move(other));
                        } else {
                            // cannot transfer ownership of any memory
                            // we need to move construct the underlying object
                            construct_from(std::move(other));
                        }
                    }
                }
                return *this;
            }

            // Destructor
            ~sbo_ptr_base() noexcept {
                destroy();
            }

        protected:
            using alloc_base = boost::empty_value<Allocator, 0>;
            Base* ptr_ = nullptr;
            Base* (*move_)(void* /* alloc */, void* /* from */, void* /* to */)
                = nullptr;
            Base* (
                *copy_)(void* /* allocator */, void* /* from */, void* /* to */)
                = nullptr;
            void (*heap_destroy_)(void* /* allocator */, void* /* ptr */)
                = nullptr;

            bool
            on_heap() const {
                return ptr_ != reinterpret_cast<Base const*>(sbo_buffer_);
            };

            template <class Derived, class... Args>
            using can_construct = conjunction<
                std::is_convertible<Derived*, Base*>,
                std::is_constructible<Derived, Args&&...>,
                std::is_nothrow_move_constructible<Derived>,
                std::is_copy_constructible<Derived>>;

            // clang-format off
            FUTURES_TEMPLATE(class Derived, class... Args)
            (requires can_construct<Derived, Args...>::value)
            void
            construct(Args&&... args)
                noexcept(
                    is_nothrow_constructible_v<Derived, Args&&...> &&
                    (sizeof(Derived) <= sbo_size))
            {
                // clang-format on
                FUTURES_IF_CONSTEXPR (sizeof(Derived) <= sbo_size) {
                    ptr_ = reinterpret_cast<Base*>(&sbo_buffer_);
                    ::new ((void*) ptr_) Derived(std::forward<Args>(args)...);
                    move_ = {
                        [](void*, void* from, void* to) -> Base* {
                            FUTURES_ASSERT(from);
                            FUTURES_ASSERT(to);
                            new (to) Derived(
                                std::move(*reinterpret_cast<Derived*>(from)));
                            reinterpret_cast<Derived*>(from)->~Derived();
                            return reinterpret_cast<Base*>(to);
                        },
                    };
                    copy_ = {
                        [](void*, void* from, void* to) -> Base* {
                            FUTURES_ASSERT(from);
                            FUTURES_ASSERT(to);
                            return new (to) Derived(
                                *reinterpret_cast<Derived const*>(from));
                            return reinterpret_cast<Base*>(to);
                        },
                    };
                } else {
                    auto a = boost::allocator_rebind_t<Allocator, Derived>(
                        alloc_base::get());
                    ptr_ = boost::allocator_allocate(a, 1);
                    boost::allocator_construct(
                        a,
                        reinterpret_cast<Derived*>(ptr_),
                        std::forward<Args>(args)...);
                    move_ = {
                        [](void* alloc, void* from, void* to) -> Base* {
                            FUTURES_ASSERT(alloc);
                            FUTURES_ASSERT(from);
                            FUTURES_ASSERT(!to);
                            auto dalloc = boost::
                                allocator_rebind_t<Allocator, Derived>(
                                    *reinterpret_cast<Allocator*>(alloc));
                            to = boost::allocator_allocate(dalloc, 1);
                            boost::allocator_construct(
                                dalloc,
                                reinterpret_cast<Derived*>(to),
                                std::move(*reinterpret_cast<Derived*>(from)));
                            return reinterpret_cast<Base*>(to);
                        },
                    };
                    copy_ = {
                        [](void* alloc, void* from, void* to) -> Base* {
                            FUTURES_ASSERT(alloc);
                            FUTURES_ASSERT(from);
                            FUTURES_ASSERT(!to);
                            auto a = boost::
                                allocator_rebind_t<Allocator, Derived>(
                                    *reinterpret_cast<Allocator const*>(alloc));
                            to = boost::allocator_allocate(a, 1);
                            boost::allocator_construct(
                                a,
                                reinterpret_cast<Derived*>(to),
                                *reinterpret_cast<Derived const*>(from));
                            return reinterpret_cast<Base*>(to);
                        },
                    };
                    heap_destroy_ = {
                        [](void* alloc, void* from) {
                        FUTURES_ASSERT(alloc);
                        FUTURES_ASSERT(from);
                        auto a = boost::allocator_rebind_t<Allocator, Derived>(*reinterpret_cast<Allocator*>(alloc));
                        boost::allocator_destroy(a, reinterpret_cast<Derived*>(from));
                        boost::allocator_deallocate(a, reinterpret_cast<Derived*>(from), 1);
                        },
                    };
                }
            }

            void
            construct_from(sbo_ptr_base const& other) {
                FUTURES_ASSERT(!ptr_);
                if (other.ptr_) {
                    if (!other.on_heap()) {
                        ptr_ = reinterpret_cast<Base*>(sbo_buffer_);
                    }
                    auto a = alloc_base::get();
                    ptr_ = other.copy_(&a, other.ptr_, ptr_);
                    heap_destroy_ = other.heap_destroy_;
                    move_ = other.move_;
                    copy_ = other.copy_;
                    alloc_base::get() = other.alloc_base::get();
                }
            }

            void
            transfer_ownership_from(sbo_ptr_base&& other) noexcept {
                move_resources_from(std::true_type{}, std::move(other));
            }

            void
            construct_from(sbo_ptr_base&& other) noexcept {
                move_resources_from(std::false_type{}, std::move(other));
            }

            template <class StealPtrResource>
            void
            move_resources_from(
                StealPtrResource,
                sbo_ptr_base&& other) noexcept {
                FUTURES_STATIC_ASSERT(
                    std::is_same<StealPtrResource, std::true_type>::value
                    || std::is_same<StealPtrResource, std::false_type>::value);
                // This is only executed after destroy()
                FUTURES_ASSERT(!ptr_);
                if (other.ptr_) {
                    if (!other.on_heap()) {
                        // on buffer, we can't transfer ownership or just
                        // memmove because we are not sure if the underlying
                        // object is relocatable
                        ptr_ = other.move_(nullptr, other.ptr_, sbo_buffer_);
                        other.ptr_ = nullptr;
                    } else {
                        move_ptr_resource(StealPtrResource{}, std::move(other));
                    }
                    heap_destroy_ = std::exchange(other.heap_destroy_, nullptr);
                    move_ = std::exchange(other.move_, nullptr);
                    copy_ = std::exchange(other.copy_, nullptr);
                }
            }

            void
            move_ptr_resource(std::true_type, sbo_ptr_base&& other) {
                ptr_ = std::exchange(other.ptr_, nullptr);
            }

            void
            move_ptr_resource(std::false_type, sbo_ptr_base&& other) {
                // we asked to construct because the allocators do not
                // propagate at move_construction and are different
                // This means this allocator cannot manage memory
                // allocated by other, and we need to move the object
                Allocator& a0 = alloc_base::get();
                ptr_ = other.move_(&a0, other.ptr_, nullptr);
                // Delete the moved from object from other
                Allocator& a1 = alloc_base::get();
                other.heap_destroy_(&a1, other.ptr_);
                other.ptr_ = nullptr;
            }

            void
            destroy() noexcept {
                if (ptr_) {
                    if (on_heap()) {
                        auto& a = alloc_base::get();
                        heap_destroy_(&a, ptr_);
                        heap_destroy_ = nullptr;
                    } else {
                        ptr_->~Base();
                    }
                    move_ = nullptr;
                    copy_ = nullptr;
                    ptr_ = nullptr;
                }
            }

        private:
            alignas(
                alignof(std::max_align_t)) unsigned char sbo_buffer_[sbo_size];
        };

        template <
            class T,
            class Allocator,
            std::size_t sbo_size,
            sbo_ptr_options opts>
        class basic_sbo_ptr
            : private sbo_ptr_base<
                  T,
                  Allocator,
                  sbo_size,
                  static_cast<bool>(opts& movable),
                  static_cast<bool>(opts& copyable)> {
            using ptr_base = sbo_ptr_base<
                T,
                Allocator,
                sbo_size,
                static_cast<bool>(opts& movable),
                static_cast<bool>(opts& copyable)>;

            template <class Derived, class... Args>
            using can_construct = typename ptr_base::
                template can_construct<Derived, Args...>;

        public:
            using pointer = T*;
            using value_type = T;

            /*
             * Constructors
             */

            // Default constructor
            basic_sbo_ptr() noexcept = default;

            // Custom allocator
            basic_sbo_ptr(Allocator const& a) noexcept(
                is_nothrow_constructible_v<
                    typename ptr_base::alloc_base,
                    Allocator const&>)
                : ptr_base(a) {}

            // Copy constructor
            basic_sbo_ptr(basic_sbo_ptr const&) = default;

            // Move constructor
            basic_sbo_ptr(basic_sbo_ptr&&) noexcept = default;

            // Construct in-place with Args
            FUTURES_TEMPLATE(class U, class... Args)
            (requires can_construct<U, Args...>::value) explicit basic_sbo_ptr(
                in_place_type_t<U>,
                Args&&... args) noexcept(is_nothrow_constructible_v<U, Args&&...> && (sizeof(U) <= sbo_size)) {
                ptr_base::template construct<U>(std::forward<Args>(args)...);
            }

            // Construct copying an element of derived type U
            FUTURES_TEMPLATE(class U)
            (requires(
                !is_same_v<std::decay_t<U>, basic_sbo_ptr>
                && can_construct<std::decay_t<U>, U&&>::value))
                basic_sbo_ptr(U&& value) noexcept(
                    is_nothrow_constructible_v<std::decay_t<U>, U&&>
                    && (sizeof(U) <= sbo_size))
                : basic_sbo_ptr{ in_place_type<std::decay_t<U>>,
                                 std::forward<U>(value) } {}

            /*
             * Assignment
             */

            // Move assign
            FUTURES_TEMPLATE(class U)
            (requires(
                is_same_v<U, std::decay_t<U>> && !is_same_v<U, basic_sbo_ptr>
                && can_construct<std::decay_t<U>, U&&>::value)) basic_sbo_ptr&
            operator=(U&& value) noexcept {
                ptr_base::destroy();
                ptr_base::template construct<U>(std::forward<U>(value));
                return *this;
            }

            // Copy assign
            FUTURES_TEMPLATE(class U)
            (requires(
                !is_same_v<U, basic_sbo_ptr>
                && can_construct<U, U const&>::value)) basic_sbo_ptr&
            operator=(U const& other) noexcept(
                is_nothrow_constructible_v<U, U const&>
                && (sizeof(U) <= sbo_size)) {
                // Strong exception guarantee
                U u{ other };
                *this = std::move(u);
                return *this;
            }

            // Move assign
            basic_sbo_ptr&
            operator=(basic_sbo_ptr&& other) noexcept
                = default;

            // Copy assign
            basic_sbo_ptr&
            operator=(basic_sbo_ptr const& other)
                = default;

            // Leave pointer in an empty state
            basic_sbo_ptr&
            operator=(std::nullptr_t) noexcept {
                ptr_base::destroy();
                return *this;
            }

            // Emplace a value of type U with Args...
            FUTURES_TEMPLATE(class U, class... Args)
            (requires can_construct<U, Args&&...>::value) void emplace(Args&&... args) noexcept(
                is_nothrow_constructible_v<U, Args&&...>
                && (sizeof(U) <= sbo_size)) {
                // Emplace cannot provide strong exception guarantee
                ptr_base::destroy();
                ptr_base::template construct<U>(std::forward<Args>(args)...);
            }

            /*
             * Pointer ops
             */

            void
            reset() noexcept {
                ptr_base::destroy();
            }

            FUTURES_NODISCARD T*
            get() noexcept {
                return ptr_base::ptr_;
            }

            FUTURES_NODISCARD T const*
            get() const noexcept {
                return ptr_base::ptr_;
            }

            FUTURES_NODISCARD T&
            operator*() noexcept {
                return *get();
            }

            FUTURES_NODISCARD T const&
            operator*() const noexcept {
                return *get();
            }

            FUTURES_NODISCARD T*
            operator->() noexcept {
                return get();
            }

            FUTURES_NODISCARD T const*
            operator->() const noexcept {
                return get();
            }

            FUTURES_NODISCARD explicit operator bool() const noexcept {
                return *this != nullptr;
            }

            FUTURES_NODISCARD friend bool
            operator==(
                basic_sbo_ptr const& lhs,
                basic_sbo_ptr const& rhs) noexcept {
                return lhs.get() == rhs.get();
            }

            FUTURES_NODISCARD friend bool
            operator==(basic_sbo_ptr const& lhs, std::nullptr_t) noexcept {
                return lhs.ptr_ == nullptr;
            }

            FUTURES_NODISCARD friend bool
            operator==(std::nullptr_t, basic_sbo_ptr const& rhs) noexcept {
                return rhs.ptr_ == nullptr;
            }

            FUTURES_NODISCARD friend bool
            operator!=(
                basic_sbo_ptr const& lhs,
                basic_sbo_ptr const& rhs) noexcept {
                return !(lhs == rhs);
            }

            FUTURES_NODISCARD friend bool
            operator!=(basic_sbo_ptr const& lhs, std::nullptr_t) noexcept {
                return !(lhs == nullptr);
            }

            FUTURES_NODISCARD friend bool
            operator!=(std::nullptr_t, basic_sbo_ptr const& rhs) noexcept {
                return !(nullptr == rhs);
            }
        };

        // A pointer with SBO that can be moved but not copied
        template <
            class T,
            class A = std::allocator<T>,
            std::size_t sbo_size = sizeof(T)>
        using move_only_sbo_ptr = basic_sbo_ptr<T, A, sbo_size, movable>;

        // A pointer with SBO that can't be moved or copied
        template <
            class T,
            class A = std::allocator<T>,
            std::size_t sbo_size = sizeof(T)>
        using static_sbo_ptr = basic_sbo_ptr<T, A, sbo_size, no_options>;

        // A pointer with SBO that can be moved and copied
        template <
            class T,
            class A = std::allocator<T>,
            std::size_t sbo_size = sizeof(T)>
        using sbo_ptr = basic_sbo_ptr<T, A, sbo_size, movable | copyable>;

    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_UTILITY_SBO_PTR_HPP
