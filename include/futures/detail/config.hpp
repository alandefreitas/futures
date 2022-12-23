//
// Copyright (c) Alan de Freitas 11/18/21.
//

#ifndef FUTURES_DETAIL_CONFIG_HPP
#define FUTURES_DETAIL_CONFIG_HPP

/// @file
/// Private configuration macros
/**
 * This file defines private configuration macros. These are the macros
 * only the library implementation is allowed to use.
 */

#include <futures/detail/deps/boost/config.hpp>
#include <futures/detail/deps/boost/static_assert.hpp>

/*
 * Documentation related macros
 *
 * These macros can be used to bits of code that should look different in the
 * documentation.
 */
#ifndef FUTURES_DOXYGEN
#    define FUTURES_DETAIL(x) x
#else
#    define FUTURES_DETAIL(x) __see_below__
#endif

/*
 * Macros related to concepts and constraints
 *
 * These macros can be used to determine concept requirements of functions.
 * These requirements might use SFINAE or concepts.
 */

/*
 * Macros related to template declarations
 *
 * These macros can be used to declare templates using the components
 * above. FUTURES_TEMPLATE will be converted to template<..., enable_if<...>>
 * or template <...> requires ..., depending on whether concepts are available
 */
#if defined(FUTURES_DOXYGEN)
#    define FUTURES_EXPAND(...)
#    define FUTURES_TEMPLATE(...) \
        template <__VA_ARGS__>    \
        FUTURES_EXPAND
#    define FUTURES_TEMPLATE_IMPL(...) FUTURES_TEMPLATE(__VA_ARGS__)
#elif defined(__cpp_concepts) || defined(BOOST_HAS_CONCEPTS)
#    define FUTURES_EXPAND(...) __VA_ARGS__
#    define FUTURES_TEMPLATE(...) \
        template <__VA_ARGS__>    \
        FUTURES_EXPAND
#    define FUTURES_TEMPLATE_IMPL(...) FUTURES_TEMPLATE(__VA_ARGS__)
#else
// For SFINAE, we concatenate the requirements and put them inside
// std::enable_if<...>. An auxiliary macro helps us ignore the `requires`
// keyword and only include the rest of the expression in enable_if.
#    define FUTURES_CONCAT_HELPER(X, ...) X##__VA_ARGS__
#    define FUTURES_CONCAT(X, ...)        FUTURES_CONCAT_HELPER(X, __VA_ARGS__)
#    define FUTURES_REQUIRES_TO_SFINAE(...)                     \
        typename std::enable_if<                                \
            FUTURES_CONCAT(FUTURES_TEMPLATE_AUX_, __VA_ARGS__), \
            int>::type                                          \
            = 0 >
#    define FUTURES_TEMPLATE(...) \
        template <__VA_ARGS__, FUTURES_REQUIRES_TO_SFINAE
#    define FUTURES_REQUIRES_TO_SFINAE_IMPL(...)                \
        typename std::enable_if<                                \
            FUTURES_CONCAT(FUTURES_TEMPLATE_AUX_, __VA_ARGS__), \
            int>::type                                          \
            >
#    define FUTURES_TEMPLATE_IMPL(...) \
        template <__VA_ARGS__, FUTURES_REQUIRES_TO_SFINAE_IMPL
#    define FUTURES_TEMPLATE_AUX_requires
#endif

// Impose requirements on "this" class
// With SFINAE, this requires an extra special template parameter to
// express the condition because SFINAE doesn't work well with requirements
// on the class template parameters
#ifdef FUTURES_DOXYGEN
#    define FUTURES_SELF_REQUIRE(x)
#else
#    define FUTURES_SELF_REQUIRE(x)    \
        template <                     \
            bool SELF_CONDITION = (x), \
            std::enable_if_t<SELF_CONDITION && SELF_CONDITION == (x), int> = 0>
#endif

// Same as before, when there are other template parameters involved
// The previous macro replaces the whole template expression while this macro
// is supposed to be included among other template parameters
#ifdef FUTURES_DOXYGEN
#    define FUTURES_ALSO_SELF_REQUIRE(x)
#else
#    define FUTURES_ALSO_SELF_REQUIRE(x)                                   \
        , bool SELF_CONDITION = (x),                                       \
               std::enable_if_t < SELF_CONDITION && SELF_CONDITION == (x), \
               int > = 0
#endif

// Same as above, but without the "=0" for implementation files
#ifdef FUTURES_DOXYGEN
#    define FUTURES_ALSO_SELF_REQUIRE_IMPL(x)
#else
#    define FUTURES_ALSO_SELF_REQUIRE_IMPL(x) \
        , bool SELF_CONDITION,                \
            std::enable_if_t<SELF_CONDITION && SELF_CONDITION == (x), int>
#endif


/*
 * Source location
 */

#if defined(__cpp_lib_source_location) || defined(FUTURES_DOXYGEN)
#    define FUTURES_CURRENT_LOCATION std::source_location::current()
#else
#    define FUTURES_CURRENT_LOCATION BOOST_CURRENT_LOCATION
#endif

/*
 * Inline variables
 */
#if !defined(BOOST_NO_CXX17_INLINE_VARIABLES) || defined(FUTURES_DOXYGEN)
#    define FUTURES_INLINE_VAR inline
#    define FUTURES_INLINE_VARIABLE(type, name) \
        inline constexpr type name{};           \
        /**/
#else
namespace futures {
    namespace detail {
        template <typename T>
        struct static_const {
            static constexpr T value{};
        };

        template <typename T>
        constexpr T static_const<T>::value;
    } // namespace detail
} // namespace futures

#    define FUTURES_INLINE_VAR
#    define FUTURES_INLINE_VARIABLE(type, name)                    \
        namespace {                                                \
            constexpr auto& name = ::futures::detail<type>::value; \
        }
#endif

/*
 * STATIC_ASSERT
 */
#ifndef FUTURES_STATIC_ASSERT
#    define FUTURES_STATIC_ASSERT(...)     BOOST_STATIC_ASSERT(__VA_ARGS__)
#    define FUTURES_STATIC_ASSERT_MSG(...) BOOST_STATIC_ASSERT_MSG(__VA_ARGS__)
#endif

/*
 * STATIC_ASSERT
 */
#ifndef FUTURES_IF_CONSTEXPR
#    define FUTURES_IF_CONSTEXPR BOOST_IF_CONSTEXPR
#endif

/*
 * CONSTINIT
 */
#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
#    define FUTURES_CONST_INIT constinit
#elif __has_cpp_attribute(clang::require_constant_initialization)
#    define FUTURES_CONST_INIT [[clang::require_constant_initialization]]
#else
#    define FUTURES_CONST_INIT
#endif

/*
 * FUTURES_NODISCARD
 */
#ifndef FUTURES_NODISCARD
#    ifdef BOOST_ATTRIBUTE_NODISCARD
#        define FUTURES_NODISCARD BOOST_ATTRIBUTE_NODISCARD
#    else
#        define FUTURES_NODISCARD
#    endif
#endif

/*
 * constexpr
 */
// C++14 201304L supports relaxed constexpr with statements and non-const
// constexpr
#ifdef BOOST_NO_CXX14_CONSTEXPR
#    define FUTURES_CXX14_CONSTEXPR
#else
#    define FUTURES_CXX14_CONSTEXPR constexpr
#endif

// C++17 201603L supports constexpr lambdas
#if !defined(__cpp_constexpr) || (__cpp_constexpr < 201603L)
#    define FUTURES_CXX17_CONSTEXPR
#    define FUTURES_CXX17_CONSTEXPR_DECLARE inline
#else
#    define FUTURES_HAS_CXX17_CONSTEXPR
#    define FUTURES_CXX17_CONSTEXPR_DECLARE constexpr
#    define FUTURES_CXX17_CONSTEXPR         constexpr
#endif

#endif // FUTURES_DETAIL_CONFIG_HPP
