/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_RANGE_PRIMITIVES_HPP
#define FUTURES_RANGES_RANGE_PRIMITIVES_HPP

#include <futures/algorithm/detail/traits/range/concepts/concepts.hpp>

#include <futures/algorithm/detail/traits/range/range_fwd.hpp>

#include <futures/algorithm/detail/traits/range/iterator/concepts.hpp>
#include <futures/algorithm/detail/traits/range/range/access.hpp>
#include <futures/algorithm/detail/traits/range/utility/addressof.hpp>
#include <futures/algorithm/detail/traits/range/utility/static_const.hpp>

#include <futures/algorithm/detail/traits/range/detail/prologue.hpp>

namespace futures::detail {
    /// \addtogroup group-range
    // Specialize this if the default is wrong.
    template <typename T> RANGES_INLINE_VAR constexpr bool disable_sized_range = false;

    /// \cond
    namespace _size_ {
        template <typename T> void size(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void size();
#endif

        struct fn {
          private:
            template <typename R> using member_size_t = decltype(+(std::declval<R>()).size());
            template <typename R> using non_member_size_t = decltype(+size(std::declval<R>()));

            template <typename R, std::size_t N> static constexpr std::size_t impl_(R (&)[N], int) noexcept {
                return N;
            }

            template <typename R, std::size_t N> static constexpr std::size_t impl_(R(&&)[N], int) noexcept {
                return N;
            }

            // Prefer member if it returns integral.
            template(typename R)(
                /// \pre
                requires integral<member_size_t<R>> AND(!disable_sized_range<uncvref_t<R>>)) //
                static constexpr member_size_t<R> impl_(R &&r, int)                          //
                noexcept(noexcept(((R &&) r).size())) {
                return ((R &&) r).size();
            }

            // Use ADL if it returns integral.
            template(typename R)(
                /// \pre
                requires integral<non_member_size_t<R>> AND(!disable_sized_range<uncvref_t<R>>)) //
                static constexpr non_member_size_t<R> impl_(R &&r, long)                         //
                noexcept(noexcept(size((R &&) r))) {
                return size((R &&) r);
            }

            template(typename R)(
                /// \pre
                requires forward_iterator<_begin_::_t<R>> AND
                    sized_sentinel_for<_end_::_t<R>, _begin_::_t<R>>) static constexpr auto impl_(R &&r, ...)
                -> ranges_detail::iter_size_t<_begin_::_t<R>> {
                using size_type = ranges_detail::iter_size_t<_begin_::_t<R>>;
                return static_cast<size_type>(futures::detail::end((R &&) r) - futures::detail::begin((R &&) r));
            }

          public:
            template <typename R>
            constexpr auto operator()(R &&r) const noexcept(noexcept(fn::impl_((R &&) r, 0)))
                -> decltype(fn::impl_((R &&) r, 0)) {
                return fn::impl_((R &&) r, 0);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };
    } // namespace _size_
    /// \endcond

    /// \ingroup group-range
    /// \return For a given expression `E` of type `T`, `futures::detail::size(E)` is equivalent
    /// to:
    ///   * `+extent_v<T>` if `T` is an array type.
    ///   * Otherwise, `+E.size()` if it is a valid expression and its type `I` models
    ///     `integral` and `disable_sized_range<std::remove_cvref_t<T>>` is false.
    ///   * Otherwise, `+size(E)` if it is a valid expression and its type `I` models
    ///     `integral` with overload resolution performed in a context that includes the
    ///     declaration:
    ///     \code
    ///     template<class T> void size(T&&) = delete;
    ///     \endcode
    ///     and does not include a declaration of `futures::detail::size`, and
    ///     `disable_sized_range<std::remove_cvref_t<T>>` is false.
    ///   * Otherwise, `static_cast<U>(futures::detail::end(E) - futures::detail::begin(E))` where `U` is
    ///     `std::make_unsigned_t<iter_difference_t<iterator_t<T>>>` if
    ///     `iter_difference_t<iterator_t<T>>` satisfies `integral` and
    ///     `iter_difference_t<iterator_t<T>>` otherwise; except that `E` is
    ///     evaluated once, if it is a valid expression and the types `I` and `S` of
    ///     `futures::detail::begin(E)` and `futures::detail::end(E)` model `sized_sentinel_for<S, I>` and
    ///     `forward_iterator<I>`.
    ///   * Otherwise, `futures::detail::size(E)` is ill-formed.
    RANGES_DEFINE_CPO(_size_::fn, size)

    // Customization point data
    /// \cond
    namespace _data_ {
        struct fn {
          private:
            template <typename R> using member_data_t = ranges_detail::decay_t<decltype(std::declval<R>().data())>;

            template(typename R)(
                /// \pre
                requires std::is_pointer<member_data_t<R &>>::value) //
                static constexpr member_data_t<R &> impl_(R &r, ranges_detail::priority_tag<2>) noexcept(noexcept(r.data())) {
                return r.data();
            }
            template(typename R)(
                /// \pre
                requires std::is_pointer<_begin_::_t<R>>::value) //
                static constexpr _begin_::_t<R> impl_(R &&r, ranges_detail::priority_tag<1>) noexcept(
                    noexcept(futures::detail::begin((R &&) r))) {
                return futures::detail::begin((R &&) r);
            }
            template(typename R)(
                /// \pre
                requires contiguous_iterator<
                    _begin_::_t<R>>) static constexpr auto impl_(R &&r,
                                                                 ranges_detail::priority_tag<
                                                                     0>) noexcept(noexcept(futures::detail::begin((R &&) r) ==
                                                                                                   futures::detail::end((R &&) r)
                                                                                               ? nullptr
                                                                                               : ranges_detail::addressof(
                                                                                                     *futures::detail::begin(
                                                                                                         (R &&) r))))
                -> decltype(ranges_detail::addressof(*futures::detail::begin((R &&) r))) {
                return futures::detail::begin((R &&) r) == futures::detail::end((R &&) r) ? nullptr
                                                                        : ranges_detail::addressof(*futures::detail::begin((R &&) r));
            }

          public:
            template <typename charT, typename Traits, typename Alloc>
            constexpr charT *operator()(std::basic_string<charT, Traits, Alloc> &s) const noexcept {
                // string doesn't have non-const data before C++17
                return const_cast<charT *>(ranges_detail::as_const(s).data());
            }

            template <typename R>
            constexpr auto operator()(R &&r) const noexcept(noexcept(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})))
                -> decltype(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})) {
                return fn::impl_((R &&) r, ranges_detail::priority_tag<2>{});
            }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _data_
    /// \endcond

    RANGES_INLINE_VARIABLE(_data_::fn, data)

    /// \cond
    namespace _cdata_ {
        struct fn {
            template <typename R>
            constexpr _data_::_t<R const &> operator()(R const &r) const noexcept(noexcept(futures::detail::data(r))) {
                return futures::detail::data(r);
            }
            template <typename R>
            constexpr _data_::_t<R const> operator()(R const &&r) const
                noexcept(noexcept(futures::detail::data((R const &&)r))) {
                return futures::detail::data((R const &&)r);
            }
        };
    } // namespace _cdata_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::data` with a const-qualified
    ///    (lvalue or rvalue) reference to `r`.
    RANGES_INLINE_VARIABLE(_cdata_::fn, cdata)

    /// \cond
    namespace _empty_ {
        struct fn {
          private:
            // Prefer member if it is valid.
            template <typename R>
            static constexpr auto impl_(R &&r, ranges_detail::priority_tag<2>) noexcept(noexcept(bool(((R &&) r).empty())))
                -> decltype(bool(((R &&) r).empty())) {
                return bool(((R &&) r).empty());
            }

            // Fall back to size == 0.
            template <typename R>
            static constexpr auto impl_(R &&r,
                                        ranges_detail::priority_tag<1>) noexcept(noexcept(bool(futures::detail::size((R &&) r) == 0)))
                -> decltype(bool(futures::detail::size((R &&) r) == 0)) {
                return bool(futures::detail::size((R &&) r) == 0);
            }

            // Fall further back to begin == end.
            template(typename R)(
                /// \pre
                requires forward_iterator<
                    _begin_::_t<R>>) static constexpr auto impl_(R &&r,
                                                                 ranges_detail::priority_tag<
                                                                     0>) noexcept(noexcept(bool(futures::detail::begin((R &&)
                                                                                                                  r) ==
                                                                                                futures::detail::end((R &&) r))))
                -> decltype(bool(futures::detail::begin((R &&) r) == futures::detail::end((R &&) r))) {
                return bool(futures::detail::begin((R &&) r) == futures::detail::end((R &&) r));
            }

          public:
            template <typename R>
            constexpr auto operator()(R &&r) const noexcept(noexcept(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})))
                -> decltype(fn::impl_((R &&) r, ranges_detail::priority_tag<2>{})) {
                return fn::impl_((R &&) r, ranges_detail::priority_tag<2>{});
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a Range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };
    } // namespace _empty_
    /// \endcond

    /// \ingroup group-range
    /// \return true if and only if range contains no elements.
    RANGES_INLINE_VARIABLE(_empty_::fn, empty)

    namespace cpp20 {
        // Specialize this is namespace futures::detail::
        using futures::detail::cdata;
        using futures::detail::data;
        using futures::detail::disable_sized_range;
        using futures::detail::empty;
        using futures::detail::size;
    } // namespace cpp20
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.hpp>

#endif
