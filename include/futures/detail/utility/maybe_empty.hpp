//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_MAYBE_EMPTY_HPP
#define FUTURES_DETAIL_UTILITY_MAYBE_EMPTY_HPP

#include <utility>
#include <type_traits>

namespace futures::detail {
    /** @addtogroup futures Futures
     *  @{
     */

    /// A convenience struct to refer to an empty type whenever we need
    /// one
    struct empty_value_type
    {};

    /// A convenience struct to refer to an empty value whenever we need
    /// one
    inline constexpr empty_value_type empty_value = empty_value_type();

    /// Represents a potentially empty base class for empty base class
    /// optimization
    ///
    /// We use the name maybe_empty for the base class that might be empty and
    /// empty_value_type / empty_value for the values we know to be empty.
    ///
    /// @tparam T The type represented by the base class
    /// @tparam BaseIndex An index to differentiate base classes in the same
    /// derived class. This might be important to ensure the same base class
    /// isn't inherited twice. @tparam E Indicates whether we should really
    /// instantiate the class (true when the class is not empty)
    template <class T, unsigned BaseIndex = 0, bool E = std::is_empty_v<T>>
    class maybe_empty
    {
    public:
        /// The type this base class is effectively represent
        using value_type = T;

        /// Initialize this potentially empty base with the specified
        /// values This will initialize the vlalue with the default constructor
        /// T(args...)
        template <class... Args>
        explicit maybe_empty(Args &&...args)
            : value_(std::forward<Args>(args)...) {}

        /// Get the effective value this class represents
        /// This returns the underlying value represented here
        const T &
        get() const noexcept {
            return value_;
        }

        /// Get the effective value this class represents
        /// This returns the underlying value represented here
        T &
        get() noexcept {
            return value_;
        }

    private:
        /// The effective value representation when the value is not empty
        T value_;
    };

    /// Represents a potentially empty base class, when it's effectively
    /// not empty
    ///
    /// @tparam T The type represented by the base class
    /// @tparam BaseIndex An index to differentiate base classes in the same
    /// derived class
    template <class T, unsigned BaseIndex>
    class maybe_empty<T, BaseIndex, true> : public T
    {
    public:
        /// The type this base class is effectively represent
        using value_type = T;

        /// Initialize this potentially empty base with the specified
        /// values This won't initialize any values but it will call the
        /// appropriate constructor T() in case we need its behaviour
        template <class... Args>
        explicit maybe_empty(Args &&...args) : T(std::forward<Args>(args)...) {}

        /// Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to
        /// whatever it represents so we can access its underlying functions
        const T &
        get() const noexcept {
            return *this;
        }

        /// Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to
        /// whatever it represents so we can access its underlying functions
        T &
        get() noexcept {
            return *this;
        }
    };

    /// Conditional base class depending on a condition
    template <bool B, class T, unsigned BaseIndex = 0>
    using conditional_base
        = maybe_empty<std::conditional_t<B, T, empty_value_type>, BaseIndex>;


    // A macro to declare a new maybe_empty type with different
    // member names, so that we can better differentiate the
    // members and write pretty-printers
#define FUTURES_MAYBE_EMPTY_TYPE(MEMBER)                \
    template <class T, bool E = std::is_empty_v<T>>     \
    class maybe_empty_##MEMBER                          \
    {                                                   \
    public:                                             \
        using value_type = T;                           \
                                                        \
        template <class... Args>                        \
        explicit maybe_empty_##MEMBER(Args &&...args)   \
            : MEMBER##_(std::forward<Args>(args)...) {} \
                                                        \
        const T &get_##MEMBER() const noexcept {        \
            return MEMBER##_;                           \
        }                                               \
                                                        \
        T &get_##MEMBER() noexcept {                    \
            return MEMBER##_;                           \
        }                                               \
    private:                                            \
        T MEMBER##_;                                    \
    };                                                  \
                                                        \
    template <class T>                                  \
    class maybe_empty_##MEMBER<T, true> : public T      \
    {                                                   \
    public:                                             \
        using value_type = T;                           \
                                                        \
        template <class... Args>                        \
        explicit maybe_empty_##MEMBER(Args &&...args)   \
            : T(std::forward<Args>(args)...) {}         \
                                                        \
        const T &get_##MEMBER() const noexcept {        \
            return *this;                               \
        }                                               \
                                                        \
        T &get_##MEMBER() noexcept {                    \
            return *this;                               \
        }                                               \
    };                                                  \
                                                        \
    template <bool B, class T>                          \
    using conditional_##MEMBER = maybe_empty_##MEMBER<  \
        std::conditional_t<B, T, empty_value_type>>;


    /** @} */
} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_MAYBE_EMPTY_HPP
