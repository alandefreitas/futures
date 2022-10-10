//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_UTILITY_COMPRESSED_TUPLE_HPP
#define FUTURES_DETAIL_UTILITY_COMPRESSED_TUPLE_HPP

#include <futures/config.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/mp11/list.hpp>
#include <tuple>

namespace futures::detail {
    // class that inherits from all classes in types in the list L
    template <class L>
    struct mp_list_inherit {};

    template <class... Types>
    class mp_list_inherit<mp_list<Types...>> : public Types... {
        using list_type = mp_list<Types...>;
        using size_type = mp_size<list_type>;
        static constexpr std::size_t size_value = size_type::value;

    public:
        mp_list_inherit() : Types(boost::empty_init)... {};

        template <class... UTypes>
        mp_list_inherit(UTypes&&... args)
            : Types(boost::empty_init, std::forward<UTypes>(args))... {}
    };

    // alias to a boost::empty_value
    template <class T, class S>
    using empty_value_t = boost::empty_value<T, S::value>;

    // list of boost::empty_values for the specified types
    template <class... Types>
    using empty_type_list = mp_transform<
        empty_value_t,
        mp_list<Types...>,
        mp_iota_c<sizeof...(Types)>>;

    template <class... Types>
    using tuple_base = mp_list_inherit<empty_type_list<Types...>>;

    // a compressed tuple that needs no storage for empty type
    template <class... Types>
    class compressed_tuple : public tuple_base<Types...> {
        // The empty tuple we actually store
        using empty_types = empty_type_list<Types...>;

        // The value types the tuple represents
        using value_types = mp_list<Types...>;

        // Get the i-th empty type
        template <std::size_t I>
        using empty_type = mp_at_c<empty_types, I>;

    public:
        // Get the i-th value type
        template <std::size_t I>
        using value_type = mp_at_c<value_types, I>;

        // Get index of a given type
        template <class T>
        static constexpr std::size_t index_of = mp_find<value_types, T>::value;

        // Check if this tuple holds the given alternative
        template <class T>
        static constexpr bool holds = index_of<T> != compressed_tuple::size();

        // Construct from one arg per param
        template <class... UTypes>
        constexpr compressed_tuple(UTypes&&... args)
            : tuple_base<Types...>(std::forward<UTypes>(args)...) {}

        // get i-th element
        template <std::size_t I>
        constexpr value_type<I>&
        get(mp_size_t<I> = {}) noexcept {
            return empty_type<I>::get();
        }

        template <std::size_t I>
        constexpr value_type<I> const&
        get(mp_size_t<I> = {}) const noexcept {
            return empty_type<I>::get();
        }

        // get element of type T
        template <class T>
        constexpr T&
        get(mp_identity<T> = {}) noexcept {
            return empty_type<index_of<T>>::get();
        }

        template <class T>
        constexpr T const&
        get(mp_identity<T> = {}) const noexcept {
            return empty_type<index_of<T>>::get();
        }

        // tuple size
        static constexpr std::size_t
        size() noexcept {
            return mp_size<empty_types>::value;
        }
    };

    // Make tuple
    template <class T>
    struct unwrap_refwrapper {
        using type = T;
    };

    template <class T>
    struct unwrap_refwrapper<std::reference_wrapper<T>> {
        using type = T&;
    };

    template <class T>
    using unwrap_decay_t = typename unwrap_refwrapper<
        typename std::decay<T>::type>::type;

    template <class... Types>
    constexpr compressed_tuple<unwrap_decay_t<Types>...>
    make_tuple(Types&&... args) {
        return compressed_tuple<unwrap_decay_t<Types>...>(
            std::forward<Types>(args)...);
    }

} // namespace futures::detail

#endif // FUTURES_DETAIL_UTILITY_COMPRESSED_TUPLE_HPP
