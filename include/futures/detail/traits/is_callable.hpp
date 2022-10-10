//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_TRAITS_IS_CALLABLE_HPP
#define FUTURES_DETAIL_TRAITS_IS_CALLABLE_HPP

namespace futures::detail {
    // Check if something is callable, regardless of its arguments
    /*
     * We have to recur to this trait because we need to attach callables to
     * executors before attaching them as continuations. This is part of the
     * expression template that will later join the antecedent future.
     *
     * We can only check if this is a valid continuation after the expression
     * is formed because we don't know to which future this will be attached.
     *
     * For this reason, we can only check if this has the operator() at this
     * point.
     *
     * @tparam T Callable type
     */
    template <class T>
    struct is_callable {
    private:
        typedef char (&yes)[1];
        typedef char (&no)[2];

        struct Fallback {
            void
            operator()();
        };
        struct Derived
            : T
            , Fallback {};

        template <class U, U>
        struct Check;

        template <class>
        static yes
        test(...);

        template <class C>
        static no
        test(Check<void (Fallback::*)(), &C::operator()> *);

    public:
        static bool const value = sizeof(test<Derived>(0)) == sizeof(yes);
    };

    template <class T>
    constexpr bool is_callable_v = is_callable<T>::value;

    /** @} */ // @addtogroup future-traits Future Traits
    /** @} */ // @addtogroup futures Futures
} // namespace futures::detail

#endif // FUTURES_DETAIL_TRAITS_IS_CALLABLE_HPP
