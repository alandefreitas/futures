#ifndef FUTURES_INTRUSIVE_PTR_H
#define FUTURES_INTRUSIVE_PTR_H

/// \file
/// This is a small subset adapted from Boost intrusive pointer
/// Intrusive ptr is a smart point with a custom reference counter

//
//  intrusive_ptr.hpp
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/ for documentation.
//

#include <type_traits>

namespace futures::detail {

    template <class T> class intrusive_ptr {
      private:
        typedef intrusive_ptr this_type;

      public:
        typedef T element_type;

        constexpr intrusive_ptr() noexcept : px(0) {}

        intrusive_ptr(T *p, bool add_ref = true) : px(p) {
            if (px != 0 && add_ref)
                intrusive_ptr_add_ref(px);
        }

        template <class U, typename std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        intrusive_ptr(intrusive_ptr<U> const &rhs) : px(rhs.get()) {
            if (px != 0)
                intrusive_ptr_add_ref(px);
        }

        intrusive_ptr(intrusive_ptr const &rhs) : px(rhs.px) {
            if (px != 0)
                intrusive_ptr_add_ref(px);
        }

        ~intrusive_ptr() {
            if (px != 0)
                intrusive_ptr_release(px);
        }

        template <class U> intrusive_ptr &operator=(intrusive_ptr<U> const &rhs) {
            this_type(rhs).swap(*this);
            return *this;
        }

        // Move support

        intrusive_ptr(intrusive_ptr &&rhs) noexcept : px(rhs.px) { rhs.px = 0; }

        intrusive_ptr &operator=(intrusive_ptr &&rhs) noexcept {
            this_type(static_cast<intrusive_ptr &&>(rhs)).swap(*this);
            return *this;
        }

        template <class U> friend class intrusive_ptr;

        template <class U, typename std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
        intrusive_ptr(intrusive_ptr<U> &&rhs) : px(rhs.px) {
            rhs.px = 0;
        }

        template <class U> intrusive_ptr &operator=(intrusive_ptr<U> &&rhs) noexcept {
            this_type(static_cast<intrusive_ptr<U> &&>(rhs)).swap(*this);
            return *this;
        }

        intrusive_ptr &operator=(intrusive_ptr const &rhs) {
            this_type(rhs).swap(*this);
            return *this;
        }

        intrusive_ptr &operator=(T *rhs) {
            this_type(rhs).swap(*this);
            return *this;
        }

        void reset() { this_type().swap(*this); }

        void reset(T *rhs) { this_type(rhs).swap(*this); }

        void reset(T *rhs, bool add_ref) { this_type(rhs, add_ref).swap(*this); }

        T *get() const noexcept { return px; }

        T *detach() noexcept {
            T *ret = px;
            px = 0;
            return ret;
        }

        T &operator*() const noexcept {
            assert(px != 0);
            return *px;
        }

        T *operator->() const noexcept {
            assert(px != 0);
            return px;
        }

        explicit operator bool() const noexcept { return px != 0; }

        void swap(intrusive_ptr &rhs) noexcept {
            T *tmp = px;
            px = rhs.px;
            rhs.px = tmp;
        }

      private:
        T *px;
    };

    template <class T, class U> inline bool operator==(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
        return a.get() == b.get();
    }

    template <class T, class U> inline bool operator!=(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
        return a.get() != b.get();
    }

    template <class T, class U> inline bool operator==(intrusive_ptr<T> const &a, U *b) noexcept {
        return a.get() == b;
    }

    template <class T, class U> inline bool operator!=(intrusive_ptr<T> const &a, U *b) noexcept {
        return a.get() != b;
    }

    template <class T, class U> inline bool operator==(T *a, intrusive_ptr<U> const &b) noexcept {
        return a == b.get();
    }

    template <class T, class U> inline bool operator!=(T *a, intrusive_ptr<U> const &b) noexcept {
        return a != b.get();
    }

    template <class T> inline bool operator==(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
        return p.get() == 0;
    }

    template <class T> inline bool operator==(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
        return p.get() == 0;
    }

    template <class T> inline bool operator!=(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
        return p.get() != 0;
    }

    template <class T> inline bool operator!=(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
        return p.get() != 0;
    }

    template <class T> inline bool operator<(intrusive_ptr<T> const &a, intrusive_ptr<T> const &b) noexcept {
        return std::less<T *>()(a.get(), b.get());
    }

    template <class T> void swap(intrusive_ptr<T> &lhs, intrusive_ptr<T> &rhs) noexcept { lhs.swap(rhs); }

    // mem_fn support

    template <class T> T *get_pointer(intrusive_ptr<T> const &p) noexcept { return p.get(); }

    // pointer casts

    template <class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const &p) {
        return static_cast<T *>(p.get());
    }

    template <class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const &p) {
        return const_cast<T *>(p.get());
    }

    template <class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const &p) {
        return dynamic_cast<T *>(p.get());
    }

    template <class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> &&p) noexcept {
        return intrusive_ptr<T>(static_cast<T *>(p.detach()), false);
    }

    template <class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> &&p) noexcept {
        return intrusive_ptr<T>(const_cast<T *>(p.detach()), false);
    }

    template <class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> &&p) noexcept {
        T *p2 = dynamic_cast<T *>(p.get());

        intrusive_ptr<T> r(p2, false);

        if (p2)
            p.detach();

        return r;
    }

} // namespace futures::detail

#endif // #ifndef FUTURES_INTRUSIVE_PTR_H
