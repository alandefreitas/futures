//
// Copyright (c) alandefreitas 2/18/22.
// See accompanying file LICENSE
//

#ifndef FUTURES_DETAIL_UTILITY_IS_CONSTANT_EVALUATED_HPP
#define FUTURES_DETAIL_UTILITY_IS_CONSTANT_EVALUATED_HPP

#include <type_traits>
#ifdef __has_include
#    if __has_include(<version>)
#        include <version>
#    endif
#endif

namespace futures::detail {
#if (defined(_MSC_VER) && _MSC_VER >= 1925) \
    || (defined(__GNUC__) && __GNUC__ >= 9) \
    || (defined(__clang__) && __clang_major__ >= 9)
#    define FUTURES_HAS_BUILTIN_CONSTANT_EVALUATED
#endif

#if !defined(FUTURES_HAS_BUILTIN_CONSTANT_EVALUATED) && defined(__GNUC__) \
    && __GNUC__ >= 6
#    define FUTURES_HAS_GCC6_CONSTANT_EVALUATED
    namespace {
        unsigned char v{};
        // https://gist.github.com/schaumb/1dec6415d9ae18e94195c03964a662e9
        constexpr bool
        gcc6_is_constant_evaluated(unsigned char const* e = &v) {
            (void) v;
            return e - 0x6017d2; // memory dependent constant value
        }
    } // namespace
#endif

    /// Detects whether the call occurs within a constant-evaluated context
    /**
     * Detects whether the function call occurs within a constant-evaluated
     * context. Returns true if the evaluation of the call occurs within the
     * evaluation of an expression or conversion that is manifestly
     * constant-evaluated; otherwise returns false.
     *
     * While testing whether initializers of following variables are constant
     * initializers, compilers may first perform a trial constant evaluation
     * of the initializers:
     *
     * - variables with reference type or const-qualified integral or
     * enumeration type
     *     - if the initializers are constant expressions, the variables are
     *     usable in constant expressions
     * - static and thread local variables
     *     - if when all subexpressions of the initializers (including
     *      constructor calls and implicit conversions) are constant
     *      expressions, static initialization is performed, which can be
     *      asserted by constinit
     *
     * @return true if the evaluation of the call occurs within the evaluation
     * of an expression or conversion that is manifestly constant-evaluated;
     * otherwise false.
     */
    constexpr bool
    is_constant_evaluated() noexcept {
#if __cpp_lib_is_constant_evaluated >= 201811L
        return std::is_constant_evaluated();
#elif defined(FUTURES_HAS_BUILTIN_CONSTANT_EVALUATED)
        return __builtin_is_constant_evaluated();
#elif defined(FUTURES_HAS_GCC6_CONSTANT_EVALUATED)
        return gcc6_is_constant_evaluated();
#else
        return false;
#endif
    }

#if __cpp_lib_is_constant_evaluated >= 201811L         \
    || defined(FUTURES_HAS_BUILTIN_CONSTANT_EVALUATED) \
    || FUTURES_HAS_GCC6_CONSTANT_EVALUATED
#    define FUTURES_HAS_CONSTANT_EVALUATED
#endif

#ifdef __cpp_lib_constexpr_algorithms
#    if __cpp_lib_constexpr_algorithms >= 201806L
#        define FUTURES_STD_HAS_CONSTANT_EVALUATED_ALGORITHMS
#    endif
#endif


#if defined(FUTURES_HAS_CONSTANT_EVALUATED)
    /*
     * This means we have access to is_constant_evaluated even before C++20
     * so our algorithms can also be constexpr if we provide our own
     * implementations. We cannot, however, recur to the STL implementations
     * here.
     */
#    define FUTURES_CONSTANT_EVALUATED_CONSTEXPR constexpr
#else
#    define FUTURES_CONSTANT_EVALUATED_CONSTEXPR inline
#endif

#if defined(FUTURES_HAS_CONSTANT_EVALUATED) \
    && defined(FUTURES_STD_HAS_CONSTANT_EVALUATED_ALGORITHMS)
    /*
     * This means we have access to is_constant_evaluated AND the STL
     * algorithms are also constexpr. Thus, if algorithms can also be constexpr
     * even if we don't provide our own implementation because we can fallback
     * on the STL implementation.
     */
#    define FUTURES_STL_ALGORITHMS_CONSTEXPR constexpr
#else
#    define FUTURES_STL_ALGORITHMS_CONSTEXPR
#endif

#ifdef FUTURES_HAS_BUILTIN_CONSTANT_EVALUATED
#    undef FUTURES_HAS_BUILTIN_CONSTANT_EVALUATED
#endif

#ifdef FUTURES_HAS_GCC6_CONSTANT_EVALUATED
#    undef FUTURES_HAS_GCC6_CONSTANT_EVALUATED
#endif

} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_IS_CONSTANT_EVALUATED_HPP
