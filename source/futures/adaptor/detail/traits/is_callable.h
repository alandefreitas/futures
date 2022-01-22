//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IS_CALLABLE_H
#define FUTURES_IS_CALLABLE_H

namespace futures {
    /** \addtogroup futures Futures
     *  @{
     */
    /** \addtogroup future-traits Future Traits
     *  @{
     */

    /// \brief Check if something is callable, regardless of the arguments
    template <typename T>
    struct is_callable
    {
    private:
        typedef char (&yes)[1];
        typedef char (&no)[2];

        struct Fallback
        {
            void
            operator()();
        };
        struct Derived
            : T
            , Fallback
        {};

        template <typename U, U>
        struct Check;

        template <typename>
        static yes
        test(...);

        template <typename C>
        static no
        test(Check<void (Fallback::*)(), &C::operator()> *);

    public:
        static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
    };

    template <typename T>
    constexpr bool is_callable_v = is_callable<T>::value;

    /** @} */ // \addtogroup future-traits Future Traits
    /** @} */ // \addtogroup futures Futures
} // namespace futures

#endif // FUTURES_IS_CALLABLE_H
