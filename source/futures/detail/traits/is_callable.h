//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_IS_CALLABLE_H
#define FUTURES_IS_CALLABLE_H

namespace futures {
    /// Check if something is callable, regardless of the arguments
    template <typename T> struct is_callable {
      private:
        typedef char (&yes)[1];
        typedef char (&no)[2];

        struct Fallback {
            void operator()();
        };
        struct Derived : T, Fallback {};

        template <typename U, U> struct Check;

        template <typename> static yes test(...);

        template <typename C> static no test(Check<void (Fallback::*)(), &C::operator()> *);

      public:
        static const bool value = sizeof(test<Derived>(0)) == sizeof(yes);
    };

    template <typename T>
    constexpr bool is_callable_v = is_callable<T>::value;

} // namespace futures

#endif // FUTURES_IS_CALLABLE_H
