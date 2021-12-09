/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef FUTURES_RANGES_RANGE_ACCESS_HPP
#define FUTURES_RANGES_RANGE_ACCESS_HPP

#include <functional> // for reference_wrapper (whose use with begin/end is deprecated)
#include <initializer_list>
#include <iterator>
#include <limits>
#include <utility>

#ifdef __has_include
#if __has_include(<span>)
namespace std {
    template <class T, std::size_t Extent> class span;
}
#endif
#if __has_include(<string_view>)
#include <string_view>
#endif
#endif

#include <futures/algorithm/detail/traits/range/range_fwd.h>

#include <futures/algorithm/detail/traits/range/iterator/concepts.h>
#include <futures/algorithm/detail/traits/range/iterator/reverse_iterator.h>
#include <futures/algorithm/detail/traits/range/iterator/traits.h>
#include <futures/algorithm/detail/traits/range/utility/static_const.h>

#include <futures/algorithm/detail/traits/range/detail/prologue.h>

namespace futures::detail {
#if defined(__cpp_lib_string_view) && __cpp_lib_string_view > 0
    template <class CharT, class Traits>
    RANGES_INLINE_VAR constexpr bool enable_borrowed_range<std::basic_string_view<CharT, Traits>> = true;
#endif

#if defined(__cpp_lib_span) && __cpp_lib_span > 0
    template <class T, std::size_t N> RANGES_INLINE_VAR constexpr bool enable_borrowed_range<std::span<T, N>> = true;
#endif

    namespace ranges_detail {
        template <typename T> RANGES_INLINE_VAR constexpr bool _borrowed_range = enable_borrowed_range<uncvref_t<T>>;

        template <typename T> RANGES_INLINE_VAR constexpr bool _borrowed_range<T &> = true;
    } // namespace ranges_detail

    /// \cond
    namespace _begin_ {
        // Poison pill for std::begin. (See the detailed discussion at
        // https://github.com/ericniebler/stl2/issues/139)
        template <typename T> void begin(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void begin();
#endif

        template <typename T> void begin(std::initializer_list<T>) = delete;

        template(typename I)(
            /// \pre
            requires input_or_output_iterator<I>) void is_iterator(I);

        // clang-format off
        template<typename T>
        CPP_requires(has_member_begin_,
            requires(T & t) //
            (
                _begin_::is_iterator(t.begin())
            ));
        template<typename T>
        CPP_concept has_member_begin =
            CPP_requires_ref(_begin_::has_member_begin_, T);

        template<typename T>
        CPP_requires(has_non_member_begin_,
            requires(T & t) //
            (
                _begin_::is_iterator(begin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_begin =
            CPP_requires_ref(_begin_::has_non_member_begin_, T);
        // clang-format on

        struct fn {
          private:
            template <bool> struct impl_ {
                // has_member_begin == true
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.begin())) {
                    return r.begin();
                }
            };

            template <typename R> using impl = impl_<has_member_begin<R>>;

          public:
            template <typename R, std::size_t N> void operator()(R(&&)[N]) const = delete;

            template <typename R, std::size_t N> constexpr R *operator()(R (&array)[N]) const noexcept { return array; }

            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_begin<R> || has_non_member_begin<R>)) constexpr auto
            operator()(R &&r) const //
                noexcept(noexcept(impl<R>::invoke(r))) {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };

        template <> struct fn::impl_<false> {
            // has_member_begin == false
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(begin(r))) { return begin(r); }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _begin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return \c r, if \c r is an array. Otherwise, `r.begin()` if that expression is
    ///   well-formed and returns an input_or_output_iterator. Otherwise, `begin(r)` if
    ///   that expression returns an input_or_output_iterator.
    RANGES_DEFINE_CPO(_begin_::fn, begin)

    /// \cond
    namespace _end_ {
        // Poison pill for std::end. (See the detailed discussion at
        // https://github.com/ericniebler/stl2/issues/139)
        template <typename T> void end(T &&) = delete;

#ifdef RANGES_WORKAROUND_MSVC_895622
        void end();
#endif

        template <typename T> void end(std::initializer_list<T>) = delete;

        template(typename I, typename S)(
            /// \pre
            requires sentinel_for<S, I>) void _is_sentinel(S, I);

        // clang-format off
        template<typename T>
        CPP_requires(has_member_end_,
            requires(T & t) //
            (
                _end_::_is_sentinel(t.end(), futures::detail::begin(t))
            ));
        template<typename T>
        CPP_concept has_member_end =
            CPP_requires_ref(_end_::has_member_end_, T);

        template<typename T>
        CPP_requires(has_non_member_end_,
            requires(T & t) //
            (
                _end_::_is_sentinel(end(t), futures::detail::begin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_end =
            CPP_requires_ref(_end_::has_non_member_end_, T);
        // clang-format on

        struct fn {
          private:
            template <bool> struct impl_ {
                // has_member_end == true
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.end())) {
                    return r.end();
                }
            };

            template <typename Int>
            using iter_diff_t = futures::detail::meta::_t<futures::detail::meta::conditional_t<std::is_integral<Int>::value,
                                                             std::make_signed<Int>, //
                                                             futures::detail::meta::id<Int>>>;

            template <typename R> using impl = impl_<has_member_end<R>>;

          public:
            template <typename R, std::size_t N> void operator()(R(&&)[N]) const = delete;

            template <typename R, std::size_t N> constexpr R *operator()(R (&array)[N]) const noexcept {
                return array + N;
            }

            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_end<R> || has_non_member_end<R>)) constexpr auto
            operator()(R &&r) const                    //
                noexcept(noexcept(impl<R>::invoke(r))) //
            {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get())))
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template(typename Int)(
                /// \pre
                requires ranges_detail::integer_like_<Int>) auto
            operator-(Int dist) const -> ranges_detail::from_end_<iter_diff_t<Int>> {
                using SInt = iter_diff_t<Int>;
                RANGES_EXPECT(0 <= dist);
                RANGES_EXPECT(dist <= static_cast<Int>((std::numeric_limits<SInt>::max)()));
                return ranges_detail::from_end_<SInt>{-static_cast<SInt>(dist)};
            }
        };

        // has_member_end == false
        template <> struct fn::impl_<false> {
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(end(r))) { return end(r); }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _end_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return \c r+size(r), if \c r is an array. Otherwise, `r.end()` if that expression
    /// is
    ///   well-formed and returns an input_or_output_iterator. Otherwise, `end(r)` if that
    ///   expression returns an input_or_output_iterator.
    RANGES_DEFINE_CPO(_end_::fn, end)

    /// \cond
    namespace _cbegin_ {
        struct fn {
            template <typename R>
            constexpr _begin_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::begin(ranges_detail::as_const(r)))) {
                return futures::detail::begin(ranges_detail::as_const(r));
            }
        };
    } // namespace _cbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::begin` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_cbegin_::fn, cbegin)

    /// \cond
    namespace _cend_ {
        struct fn {
            template <typename R>
            constexpr _end_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::end(ranges_detail::as_const(r)))) {
                return futures::detail::end(ranges_detail::as_const(r));
            }
        };
    } // namespace _cend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::end` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_cend_::fn, cend)

    /// \cond
    namespace _rbegin_ {
        template <typename R> void rbegin(R &&) = delete;
        // Non-standard, to keep unqualified rbegin(r) from finding std::rbegin
        // and returning a std::reverse_iterator.
        template <typename T> void rbegin(std::initializer_list<T>) = delete;
        template <typename T, std::size_t N> void rbegin(T (&)[N]) = delete;

        // clang-format off
        template<typename T>
        CPP_requires(has_member_rbegin_,
            requires(T & t) //
            (
                _begin_::is_iterator(t.rbegin())
            ));
        template<typename T>
        CPP_concept has_member_rbegin =
            CPP_requires_ref(_rbegin_::has_member_rbegin_, T);

        template<typename T>
        CPP_requires(has_non_member_rbegin_,
            requires(T & t) //
            (
                _begin_::is_iterator(rbegin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_rbegin =
            CPP_requires_ref(_rbegin_::has_non_member_rbegin_, T);

        template<typename I>
        void _same_type(I, I);

        template<typename T>
        CPP_requires(can_reverse_end_,
            requires(T & t) //
            (
                // make_reverse_iterator is constrained with
                // bidirectional_iterator.
                futures::detail::make_reverse_iterator(futures::detail::end(t)),
                _rbegin_::_same_type(futures::detail::begin(t), futures::detail::end(t))
            ));
        template<typename T>
        CPP_concept can_reverse_end =
            CPP_requires_ref(_rbegin_::can_reverse_end_, T);
        // clang-format on

        struct fn {
          private:
            // has_member_rbegin == true
            template <int> struct impl_ {
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.rbegin())) {
                    return r.rbegin();
                }
            };

            template <typename R> using impl = impl_<has_member_rbegin<R> ? 0 : has_non_member_rbegin<R> ? 1 : 2>;

          public:
            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_rbegin<R> || has_non_member_rbegin<R> ||
                                                        can_reverse_end<R>)) //
                constexpr auto
                operator()(R &&r) const                //
                noexcept(noexcept(impl<R>::invoke(r))) //
            {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const //
                noexcept(noexcept(Fn{}(ref.get())))         //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get()))) //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };

        // has_non_member_rbegin == true
        template <> struct fn::impl_<1> {
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(rbegin(r))) {
                return rbegin(r);
            }
        };

        // can_reverse_end
        template <> struct fn::impl_<2> {
            template <typename R>
            static constexpr auto invoke(R &&r) noexcept(noexcept(futures::detail::make_reverse_iterator(futures::detail::end(r)))) {
                return futures::detail::make_reverse_iterator(futures::detail::end(r));
            }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _rbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return `make_reverse_iterator(r + futures::detail::size(r))` if r is an array. Otherwise,
    ///   `r.rbegin()` if that expression is well-formed and returns an
    ///   input_or_output_iterator. Otherwise, `make_reverse_iterator(futures::detail::end(r))` if
    ///   `futures::detail::begin(r)` and `futures::detail::end(r)` are both well-formed and have the same
    ///   type that satisfies `bidirectional_iterator`.
    RANGES_DEFINE_CPO(_rbegin_::fn, rbegin)

    /// \cond
    namespace _rend_ {
        template <typename R> void rend(R &&) = delete;
        // Non-standard, to keep unqualified rend(r) from finding std::rend
        // and returning a std::reverse_iterator.
        template <typename T> void rend(std::initializer_list<T>) = delete;
        template <typename T, std::size_t N> void rend(T (&)[N]) = delete;

        // clang-format off
        template<typename T>
        CPP_requires(has_member_rend_,
            requires(T & t) //
            (
                _end_::_is_sentinel(t.rend(), futures::detail::rbegin(t))
            ));
        template<typename T>
        CPP_concept has_member_rend =
            CPP_requires_ref(_rend_::has_member_rend_, T);

        template<typename T>
        CPP_requires(has_non_member_rend_,
            requires(T & t) //
            (
                _end_::_is_sentinel(rend(t), futures::detail::rbegin(t))
            ));
        template<typename T>
        CPP_concept has_non_member_rend =
            CPP_requires_ref(_rend_::has_non_member_rend_, T);

        template<typename T>
        CPP_requires(can_reverse_begin_,
            requires(T & t) //
            (
                // make_reverse_iterator is constrained with
                // bidirectional_iterator.
                futures::detail::make_reverse_iterator(futures::detail::begin(t)),
                _rbegin_::_same_type(futures::detail::begin(t), futures::detail::end(t))
            ));
        template<typename T>
        CPP_concept can_reverse_begin =
            CPP_requires_ref(_rend_::can_reverse_begin_, T);
        // clang-format on

        struct fn {
          private:
            // has_member_rbegin == true
            template <int> struct impl_ {
                template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(r.rend())) {
                    return r.rend();
                }
            };

            template <typename R> using impl = impl_<has_member_rend<R> ? 0 : has_non_member_rend<R> ? 1 : 2>;

          public:
            template(typename R)(
                /// \pre
                requires ranges_detail::_borrowed_range<R> AND(has_member_rend<R> ||     //
                                                        has_non_member_rend<R> || //
                                                        can_reverse_begin<R>))    //
                constexpr auto
                operator()(R &&r) const noexcept(noexcept(impl<R>::invoke(r))) //
            {
                return impl<R>::invoke(r);
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(std::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get()))) //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }

            template <typename T, typename Fn = fn>
            RANGES_DEPRECATED("Using a reference_wrapper as a range is deprecated. Use views::ref "
                              "instead.")
            constexpr auto
            operator()(futures::detail::reference_wrapper<T> ref) const noexcept(noexcept(Fn{}(ref.get()))) //
                -> decltype(Fn{}(ref.get())) {
                return Fn{}(ref.get());
            }
        };

        // has_non_member_rend == true
        template <> struct fn::impl_<1> {
            template <typename R> static constexpr auto invoke(R &&r) noexcept(noexcept(rend(r))) { return rend(r); }
        };

        // can_reverse_begin
        template <> struct fn::impl_<2> {
            template <typename R>
            static constexpr auto invoke(R &&r) noexcept(noexcept(futures::detail::make_reverse_iterator(futures::detail::begin(r)))) {
                return futures::detail::make_reverse_iterator(futures::detail::begin(r));
            }
        };

        template <typename R> using _t = decltype(fn{}(std::declval<R>()));
    } // namespace _rend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return `make_reverse_iterator(r)` if `r` is an array. Otherwise,
    ///   `r.rend()` if that expression is well-formed and returns a type that
    ///   satisfies `sentinel_for<S, I>` where `I` is the type of `futures::detail::rbegin(r)`.
    ///   Otherwise, `make_reverse_iterator(futures::detail::begin(r))` if `futures::detail::begin(r)`
    ///   and `futures::detail::end(r)` are both well-formed and have the same type that
    ///   satisfies `bidirectional_iterator`.
    RANGES_DEFINE_CPO(_rend_::fn, rend)

    /// \cond
    namespace _crbegin_ {
        struct fn {
            template <typename R>
            constexpr _rbegin_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::rbegin(ranges_detail::as_const(r)))) {
                return futures::detail::rbegin(ranges_detail::as_const(r));
            }
        };
    } // namespace _crbegin_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::rbegin` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_crbegin_::fn, crbegin)

    /// \cond
    namespace _crend_ {
        struct fn {
            template <typename R>
            constexpr _rend_::_t<ranges_detail::as_const_t<R>> operator()(R &&r) const
                noexcept(noexcept(futures::detail::rend(ranges_detail::as_const(r)))) {
                return futures::detail::rend(ranges_detail::as_const(r));
            }
        };
    } // namespace _crend_
    /// \endcond

    /// \ingroup group-range
    /// \param r
    /// \return The result of calling `futures::detail::rend` with a const-qualified
    ///    reference to r.
    RANGES_INLINE_VARIABLE(_crend_::fn, crend)

    template <typename Rng> using iterator_t = decltype(begin(std::declval<Rng &>()));

    template <typename Rng> using sentinel_t = decltype(end(std::declval<Rng &>()));

    namespace cpp20 {
        using futures::detail::begin;
        using futures::detail::cbegin;
        using futures::detail::cend;
        using futures::detail::crbegin;
        using futures::detail::crend;
        using futures::detail::end;
        using futures::detail::rbegin;
        using futures::detail::rend;

        using futures::detail::iterator_t;
        using futures::detail::sentinel_t;

        using futures::detail::enable_borrowed_range;
    } // namespace cpp20
} // namespace futures::detail

#include <futures/algorithm/detail/traits/range/detail/epilogue.h>

#endif
