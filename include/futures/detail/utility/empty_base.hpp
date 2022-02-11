//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_EMPTY_BASE_HPP
#define FUTURES_DETAIL_UTILITY_EMPTY_BASE_HPP

#include <type_traits>
#include <utility>

namespace futures::detail {
    /** \addtogroup futures Futures
     *  @{
     */

    /// \brief A convenience struct to refer to an empty type whenever we need
    /// one
    struct empty_value_type
    {};

    /// \brief A convenience struct to refer to an empty value whenever we need
    /// one
    inline constexpr empty_value_type empty_value = empty_value_type();

    /// \brief Represents a potentially empty base class for empty base class
    /// optimization
    ///
    /// We use the name maybe_empty for the base class that might be empty and
    /// empty_value_type / empty_value for the values we know to be empty.
    ///
    /// \tparam T The type represented by the base class
    /// \tparam BaseIndex An index to differentiate base classes in the same
    /// derived class. This might be important to ensure the same base class
    /// isn't inherited twice. \tparam E Indicates whether we should really
    /// instantiate the class (true when the class is not empty)
    template <class T, unsigned BaseIndex = 0, bool E = std::is_empty_v<T>>
    class maybe_empty
    {
    public:
        /// \brief The type this base class is effectively represent
        using value_type = T;

        /// \brief Initialize this potentially empty base with the specified
        /// values This will initialize the vlalue with the default constructor
        /// T(args...)
        template <class... Args>
        explicit maybe_empty(Args &&...args)
            : value_(std::forward<Args>(args)...) {}

        /// \brief Get the effective value this class represents
        /// This returns the underlying value represented here
        const T &
        get() const noexcept {
            return value_;
        }

        /// \brief Get the effective value this class represents
        /// This returns the underlying value represented here
        T &
        get() noexcept {
            return value_;
        }

    private:
        /// \brief The effective value representation when the value is not empty
        T value_;
    };

    /// \brief Represents a potentially empty base class, when it's effectively
    /// not empty
    ///
    /// \tparam T The type represented by the base class
    /// \tparam BaseIndex An index to differentiate base classes in the same
    /// derived class
    template <class T, unsigned BaseIndex>
    class maybe_empty<T, BaseIndex, true> : public T
    {
    public:
        /// \brief The type this base class is effectively represent
        using value_type = T;

        /// \brief Initialize this potentially empty base with the specified
        /// values This won't initialize any values but it will call the
        /// appropriate constructor T() in case we need its behaviour
        template <class... Args>
        explicit maybe_empty(Args &&...args) : T(std::forward<Args>(args)...) {}

        /// \brief Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to
        /// whatever it represents so we can access its underlying functions
        const T &
        get() const noexcept {
            return *this;
        }

        /// \brief Get the effective value this class represents
        /// Although the element takes no space, we can return a reference to
        /// whatever it represents so we can access its underlying functions
        T &
        get() noexcept {
            return *this;
        }
    };

    template <bool B, class T, unsigned BaseIndex = 0>
    using conditional_base
        = maybe_empty<std::conditional_t<B, T, empty_value_type>, BaseIndex>;

    /** @} */
} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_EMPTY_BASE_HPP
