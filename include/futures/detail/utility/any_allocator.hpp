//
// Copyright (c) 2023 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_ANY_ALLOCATOR_HPP
#define FUTURES_DETAIL_UTILITY_ANY_ALLOCATOR_HPP

#include <futures/config.hpp>
#include <futures/throw.hpp>
#include <futures/detail/utility/sbo_ptr.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>

namespace futures {
    namespace detail {
        template <std::size_t x>
        struct log2_constant {
            static constexpr std::size_t value = 1 + log2_constant<x / 2>::value;
        };

        template <>
        struct log2_constant<1> {
            static constexpr std::size_t value = 1;
        };

        template <class X>
        struct pow2_constant {
            static constexpr std::size_t value
                = pow2_constant<mp_size_t<X::value - 1>>::value * 2;
        };

        template <>
        struct pow2_constant<mp_size_t<0>> {
            static constexpr std::size_t value = 1;
        };

        template <typename T>
        constexpr bool
        is_power_of_two(T x) {
            return (x != 0) && ((x & (x - 1)) == 0);
        }

        template <std::size_t Align>
        struct overaligned_byte_type {
            alignas(Align) unsigned char c;
        };

        struct allocator_interface {
            allocator_interface() = default;

            allocator_interface(allocator_interface const&) = default;

            virtual ~allocator_interface() = default;

            allocator_interface&
            operator=(allocator_interface const&)
                = default;

            FUTURES_NODISCARD void*
            allocate(
                std::size_t bytes,
                std::size_t alignment = alignof(std::max_align_t)) {
                return do_allocate(bytes, alignment);
            }

            void
            deallocate(
                void* p,
                std::size_t bytes,
                std::size_t alignment = alignof(std::max_align_t)) {
                do_deallocate(p, bytes, alignment);
            }

            bool
            is_equal(allocator_interface const& other) const noexcept {
                return do_is_equal(other);
            }

            virtual void*
            do_allocate(std::size_t bytes, std::size_t alignment)
                = 0;

            virtual void
            do_deallocate(void* p, std::size_t bytes, std::size_t alignment)
                = 0;

            virtual bool
            do_is_equal(allocator_interface const& other) const noexcept
                = 0;

            friend bool
            operator==(
                allocator_interface const& a,
                allocator_interface const& b) noexcept {
                return &a == &b || a.is_equal(b);
            }

            friend bool
            operator!=(
                allocator_interface const& a,
                allocator_interface const& b) noexcept {
                return !(a == b);
            }
        };

        template <class A>
        class allocator_interface_impl : public allocator_interface {
            /* Allocator type is always unsigned char by default */
            FUTURES_STATIC_ASSERT(
                detail::is_same_v<
                    typename boost::allocator_value_type<A>::type,
                    unsigned char>);

            A alloc_;

            using M = std::max_align_t;
            using max_align_idx = log2_constant<alignof(M)>;
            using alignment_idx = mp_iota_c<max_align_idx::value + 4>;
            using alignments = mp_transform<pow2_constant, alignment_idx>;

        public:
            allocator_interface_impl() = default;

            FUTURES_TEMPLATE(class U)
            (requires is_convertible_v<U, A>)
                allocator_interface_impl(U const& alloc)
                : alloc_(alloc) {}

            FUTURES_TEMPLATE(class U)
            (requires is_convertible_v<U, A>)
                allocator_interface_impl(A const& a)
                : alloc_(a) {}

            allocator_interface_impl(allocator_interface_impl const&) = default;

            allocator_interface_impl(allocator_interface_impl&&) = default;

            virtual ~allocator_interface_impl() = default;

            virtual void*
            do_allocate(std::size_t bytes, std::size_t alignment) {
                void* res = nullptr;
                mp_for_each<alignments>([&](auto I) {
                    if (res) {
                        return;
                    }
                    using N = decltype(I);
                    if (N::value >= alignment) {
                        using BN = overaligned_byte_type<N::value>;
                        auto a = boost::allocator_rebind_t<A, BN>(alloc_);
                        res = boost::allocator_allocate(a, bytes);
                    }
                });
                if (!res) {
                    using BN = overaligned_byte_type<
                        mp_back<alignments>::value * 2>;
                    auto a = boost::allocator_rebind_t<A, BN>(alloc_);
                    res = boost::allocator_allocate(a, bytes);
                }
                return res;
            }

            virtual void
            do_deallocate(void* p, std::size_t bytes, std::size_t alignment) {
                mp_for_each<alignments>([&](auto I) {
                    if (!p) {
                        return;
                    }
                    using N = decltype(I);
                    if (N::value >= alignment) {
                        using BN = overaligned_byte_type<N::value>;
                        auto a = boost::allocator_rebind_t<A, BN>(alloc_);
                        boost::allocator_deallocate(
                            a,
                            reinterpret_cast<BN*>(p),
                            bytes);
                        p = nullptr;
                    }
                });
                if (!p) {
                    using BN = overaligned_byte_type<
                        mp_back<alignments>::value * 2>;
                    auto a = boost::allocator_rebind_t<A, BN>(alloc_);
                    boost::allocator_deallocate(
                        a,
                        reinterpret_cast<BN*>(p),
                        bytes);
                }
            }

            virtual bool
            do_is_equal(allocator_interface const& other0) const noexcept {
                auto other_ptr = dynamic_cast<allocator_interface_impl const*>(
                    &other0);
                if (!other_ptr) {
                    return false;
                }
                FUTURES_IF_CONSTEXPR (boost::allocator_is_always_equal<
                                          A>::type::value)
                {
                    return true;
                } else {
                    return other_ptr->alloc_ == alloc_;
                }
            }
        };

        template <class T = unsigned char>
        class any_allocator {
            detail::sbo_ptr<allocator_interface> impl_;

            template <class>
            friend class any_allocator;
        public:
            using value_type = T;
            using pointer = T*;
            using const_pointer = T const*;
            using reference = T&;
            using const_reference = T const&;
            using size_type = std::size_t;
            using difference_type = std::ptrdiff_t;
            using propagate_on_container_move_assignment = std::false_type;
            template <class U>
            struct rebind {
                typedef any_allocator<U> other;
            };
            using is_always_equal = std::false_type;

            any_allocator() noexcept
                : impl_(
                    allocator_interface_impl<std::allocator<unsigned char>>()) {
            }

            any_allocator(any_allocator const& other) = default;

            template <class Allocator>
            any_allocator(Allocator alloc)
                : impl_(allocator_interface_impl<
                        boost::allocator_rebind_t<Allocator, unsigned char>>(
                    boost::allocator_rebind_t<Allocator, unsigned char>(
                        alloc))) {}

            template <class U>
            any_allocator(any_allocator<U> const& other) noexcept
                : impl_(other.impl_) {}

            any_allocator
            operator=(any_allocator const&) noexcept
                = delete;

            FUTURES_NODISCARD T*
            allocate(std::size_t n) {
                return reinterpret_cast<T*>(
                    impl_->allocate(sizeof(T) * n, alignof(T)));
            }

            T*
            allocate(std::size_t n, void const*) {
                return allocate(n);
            }

            void
            deallocate(T* p, std::size_t n) {
                impl_->deallocate(p, sizeof(T) * n, alignof(T));
            }

            template <class U, class... Args>
            void
            construct(U* p, Args&&... args) {
                ::new ((void*) p) U(std::forward<Args>(args)...);
            }

            template <class U>
            void
            destroy(U* p) {
                p->~U();
            }

            FUTURES_NODISCARD void*
            allocate_bytes(
                std::size_t nbytes,
                std::size_t alignment = alignof(std::max_align_t)) {
                return impl_->allocate(nbytes, alignment);
            }

            void
            deallocate_bytes(
                void* p,
                std::size_t nbytes,
                std::size_t alignment = alignof(std::max_align_t)) {
                impl_->deallocate(p, nbytes, alignment);
            }

            template <class U>
            FUTURES_NODISCARD U*
            allocate_object(std::size_t n = 1) {
                if (std::numeric_limits<std::size_t>::max() / sizeof(U) < n) {
                    throw_exception(std::bad_array_new_length{});
                }
                return static_cast<U*>(
                    allocate_bytes(n * sizeof(U), alignof(U)));
            }

            template <class U>
            void
            deallocate_object(U* p, std::size_t n = 1) {
                deallocate_bytes(p, n * sizeof(U), alignof(U));
            }

            template <class U, class... CtorArgs>
            FUTURES_NODISCARD U*
            new_object(CtorArgs&&... ctor_args) {
                U* p = allocate_object<U>();
                try {
                    construct(p, std::forward<CtorArgs>(ctor_args)...);
                    return p;
                }
                catch (...) {
                    deallocate_object(p);
                    throw;
                }
            }

            template <class U>
            void
            delete_object(U* p) {
                destroy(p);
                deallocate_object(p);
            }

            any_allocator
            select_on_container_copy_construction() const {
                // polymorphic_allocators do not propagate on container copy
                // construction. Returns a default-constructed
                // polymorphic_allocator object.
                return {};
            }

            friend bool
            operator==(
                any_allocator const& lhs,
                any_allocator const& rhs) noexcept {
                return *lhs.impl_ == *rhs.impl_;
            }

            friend bool
            operator!=(
                any_allocator const& lhs,
                any_allocator const& rhs) noexcept {
                return !(lhs == rhs);
            }
        };

        template <class T1, class T2>
        bool
        operator==(
            any_allocator<T1> const& lhs,
            any_allocator<T2> const& rhs) noexcept {
            return *lhs.impl_ == *rhs.impl_;
        }

        template <class T1, class T2>
        bool
        operator!=(
            any_allocator<T1> const& lhs,
            any_allocator<T2> const& rhs) noexcept {
            return !(lhs == rhs);
        }
    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_UTILITY_ANY_ALLOCATOR_HPP
