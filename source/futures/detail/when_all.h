//
// Created by Alan Freitas on 8/18/21.
//

#ifndef FUTURES_WHEN_ALL_H
#define FUTURES_WHEN_ALL_H

/// \file Implement the when_all functionality for futures and executors
///
/// Because all tasks need to be done to achieve the result, the algorithm doesn't depend much
/// on the properties of the underlying futures. The thread that is awaiting just needs sleep
/// and await for each of the internal futures.
///
/// The usual approach, without our future concepts, like in returning another std::future, is
/// to start another polling thread, which sets a promise when all other futures are ready.
/// If the futures support lazy continuations, these promises can be set from the previous
/// objects. However, this has an obvious cost for such a trivial operation, given that the
/// solutions is already available in the underlying futures.
///
/// Instead, we implement one more future type `when_all_future` that can query if the
/// futures are ready and waits for them to be ready whenever get() is called.
/// This proxy object can then be converted to a regular future if the user needs to.
///
/// This has a disadvantage over futures with lazy continuations because we might need
/// to schedule another task if we need notifications from this future. However,
/// we avoid scheduling another task right now, so this is, at worst, as good as
/// the common approach of wrapping it into another existing future type.
///
/// If the input futures are not shared, they are moved into `when_all_future` and are invalidated,
/// as usual. The `when_all_future` cannot be shared.

#include <small/vector.h>

#include <range/v3/range/concepts.hpp>

#include "traits/is_tuple.h"
#include "traits/to_future.h"
#include "tuple_algorithm.h"

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */


    /// \brief Proxy future class referring to the result of a conjunction of futures from @ref when_all
    ///
    /// This class implements the behavior of the `when_all` operation as another future type,
    /// which can handle heterogeneous future objects.
    ///
    /// This future type logically checks the results of other futures in place to avoid creating a
    /// real conjunction of futures that would need to be polling (or be a lazy continuation)
    /// on another thread.
    ///
    /// If the user does want to poll on another thread, then this can be converted into a cfuture
    /// as usual with async. If the other future holds the when_all_state as part of its state,
    /// then it can become another future.
    template <class Sequence> class when_all_future {
      private:
        using sequence_type = Sequence;
        using corresponding_future_type = std::future<sequence_type>;
        static constexpr bool sequence_is_range = ranges::range<sequence_type>;
        static constexpr bool sequence_is_tuple = is_tuple_v<sequence_type>;
        static_assert(sequence_is_range || sequence_is_tuple);

      public:
        /// \brief Default constructor.
        /// Constructs a when_all_future with no shared state. After construction, valid() == false
        when_all_future() noexcept = default;

        /// \brief Move a sequence of futures into the when_all_future constructor.
        /// The sequence is moved into this future object and the objects from which the
        /// sequence was created get invalidated
        explicit when_all_future(sequence_type &&v) noexcept : v(std::move(v)) {}

        /// \brief Move constructor.
        /// Constructs a when_all_future with the shared state of other using move semantics.
        /// After construction, other.valid() == false
        when_all_future(when_all_future &&other) noexcept : v(std::move(other.v)) {}

        /// \brief when_all_future is not CopyConstructible
        when_all_future(const when_all_future &other) = delete;

        /// \brief Releases any shared state.
        /// - If the return object or provider holds the last reference to its shared state, the shared state is
        /// destroyed
        /// - the return object or provider gives up its reference to its shared state
        /// This means we just need to let the internal futures destroy themselves
        ~when_all_future() = default;

        /// \brief Assigns the contents of another future object.
        /// Releases any shared state and move-assigns the contents of other to *this.
        /// After the assignment, other.valid() == false and this->valid() will yield the same value as
        /// other.valid() before the assignment.
        when_all_future &operator=(when_all_future &&other) noexcept { v = std::move(other.v); }

        /// \brief Assigns the contents of another future object.
        /// when_all_future is not CopyAssignable.
        when_all_future &operator=(const when_all_future &other) = delete;

        /// \brief Wait until all futures have a valid result and retrieves it
        /// It effectively calls wait() in order to wait for the result.
        /// The behavior is undefined if valid() is false before the call to this function.
        /// Any shared state is released. valid() is false after a call to this method.
        /// The value v stored in the shared state, as std::move(v)
        sequence_type get() {
            // Check if the sequence is valid
            if (not valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            // Wait for the complete sequence to be ready
            wait();
            // Move results
            return std::move(v);
        }

        /// \brief Checks if the future refers to a shared state
        [[nodiscard]] bool valid() const noexcept {
            if constexpr (sequence_is_range) {
                return std::all_of(v.begin(), v.end(), [](auto &&f) { return f.valid(); });
            } else {
                return tuple_all_of(v, [](auto &&f) { return f.valid(); });
            }
        }

        /// \brief Blocks until the result becomes available.
        /// valid() == true after the call.
        /// The behavior is undefined if valid() == false before the call to this function
        void wait() const {
            // Check if the sequence is valid
            if (not valid()) {
                throw std::future_error(std::future_errc::no_state);
            }
            if constexpr (sequence_is_range) {
                std::for_each(v.begin(), v.end(), [](auto &&f) { f.wait(); });
            } else {
                tuple_for_each(v, [](auto &&f) { f.wait(); });
            }
        }

        /// \brief Waits for the result to become available.
        /// Blocks until specified timeout_duration has elapsed or the result becomes available, whichever comes
        /// first.
        template <class Rep, class Period>
        [[nodiscard]] std::future_status wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) const {
            constexpr bool is_compile_time_empty = []() {
                if constexpr (sequence_is_tuple) {
                    return std::tuple_size_v<sequence_type> == 0;
                } else {
                    return false;
                }
            }();
            if constexpr (sequence_is_tuple && is_compile_time_empty) {
                return std::future_status::ready;
            } else {
                if constexpr (sequence_is_range) {
                    if (v.empty()) {
                        return std::future_status::ready;
                    }
                }

                // Check if the sequence is valid
                if (not valid()) {
                    throw std::future_error(std::future_errc::no_state);
                }
                using duration_type = std::chrono::duration<Rep, Period>;
                using namespace std::chrono;
                auto start_time = system_clock::now();
                duration_type total_elapsed = duration_cast<duration_type>(nanoseconds(0));
                auto equal_fn = [&](auto &&f) {
                    std::future_status s = f.wait_for(timeout_duration - total_elapsed);
                    total_elapsed = duration_cast<duration_type>(system_clock::now() - start_time);
                    const bool when_all_impossible = s != std::future_status::ready;
                    return when_all_impossible || total_elapsed > timeout_duration;
                };
                if constexpr (sequence_is_range) {
                    // Use a hack to "break" for_each loops with find_if
                    auto it = std::find_if(v.begin(), v.end(), equal_fn);
                    return (it == v.end()) ? std::future_status::ready : it->wait_for(seconds(0));
                } else {
                    auto idx = tuple_find_if(v, equal_fn);
                    if (idx == std::tuple_size<sequence_type>()) {
                        return std::future_status::ready;
                    } else {
                        std::future_status s =
                            apply([](auto &&el) -> std::future_status { return el.wait_for(seconds(0)); }, v, idx);
                        return s;
                    }
                }
            }
        }

        /// \brief wait_until waits for a result to become available.
        /// It blocks until specified timeout_time has been reached or the result becomes available, whichever comes
        /// first
        template <class Clock, class Duration>
        std::future_status wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time) const {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ? wait_for(std::chrono::seconds(0))
                                           : wait_for(timeout_time - std::chrono::system_clock::now());
        }

        /// \brief Allow move the underlying sequence somewhere else
        /// The when_all_future is left empty and should now be considered invalid.
        /// This is useful for the algorithm that merges two wait_all_future objects without
        /// forcing encapsulation of the merge function.
        sequence_type &&release() { return std::move(v); }

        /// \brief Request the stoppable futures to stop
        bool request_stop() noexcept {
            bool any_request = false;
            auto f_request_stop = [&](auto &&f) { any_request = any_request || f.request_stop(); };
            if constexpr (sequence_is_range) {
                std::for_each(v.begin(), v.end(), f_request_stop);
            } else {
                tuple_for_each(v, f_request_stop);
            }
            return any_request;
        }

      private:
        /// \brief Internal wait_all_future state
        sequence_type v;
    };

    /// \section Define when_all_future as a kind of future

    /// Specialization explicitly setting when_all_future<T> as a type of future
    template <typename T> struct is_future<when_all_future<T>> : std::true_type {};

    /// Specialization explicitly setting when_all_future<T> & as a type of future
    template <typename T> struct is_future<when_all_future<T> &> : std::true_type {};

    /// Specialization explicitly setting when_all_future<T> && as a type of future
    template <typename T> struct is_future<when_all_future<T> &&> : std::true_type {};

    /// Specialization explicitly setting const when_all_future<T> as a type of future
    template <typename T> struct is_future<const when_all_future<T>> : std::true_type {};

    /// Specialization explicitly setting const when_all_future<T> & as a type of future
    template <typename T> struct is_future<const when_all_future<T> &> : std::true_type {};

    namespace detail {
        /// \section Useful traits for when all future

        /// \brief Check if type is a when_all_future as a type
        template <typename> struct is_when_all_future : std::false_type {};
        template <typename Sequence> struct is_when_all_future<when_all_future<Sequence>> : std::true_type {};
        template <typename Sequence> struct is_when_all_future<const when_all_future<Sequence>> : std::true_type {};
        template <typename Sequence> struct is_when_all_future<when_all_future<Sequence> &> : std::true_type {};
        template <typename Sequence> struct is_when_all_future<when_all_future<Sequence> &&> : std::true_type {};
        template <typename Sequence> struct is_when_all_future<const when_all_future<Sequence> &> : std::true_type {};

        /// \brief Check if type is a when_all_future as constant bool
        template <class T> constexpr bool is_when_all_future_v = is_when_all_future<T>::value;

        /// \brief Check if a type can be used in a future conjunction (when_all or operator&& for futures)
        template <class T> using is_valid_when_all_argument = std::disjunction<is_future<T>, std::is_invocable<T>>;
        template <class T> constexpr bool is_valid_when_all_argument_v = is_valid_when_all_argument<T>::value;

        /// \brief Trait to identify valid when_all inputs
        template <class...> struct are_valid_when_all_arguments : std::true_type {};
        template <class B1> struct are_valid_when_all_arguments<B1> : is_valid_when_all_argument<B1> {};
        template <class B1, class... Bn>
        struct are_valid_when_all_arguments<B1, Bn...>
            : std::conditional_t<is_valid_when_all_argument_v<B1>, are_valid_when_all_arguments<Bn...>,
                                 std::false_type> {};
        template <class... Args>
        constexpr bool are_valid_when_all_arguments_v = are_valid_when_all_arguments<Args...>::value;

        /// \subsection Helpers and traits for operator&& on futures, functions and when_all futures

        /// \brief Check if type is a when_all_future with tuples as a sequence type
        template <typename T, class Enable = void> struct is_when_all_tuple_future : std::false_type {};
        template <typename Sequence>
        struct is_when_all_tuple_future<when_all_future<Sequence>, std::enable_if_t<is_tuple_v<Sequence>>>
            : std::true_type {};
        template <class T> constexpr bool is_when_all_tuple_future_v = is_when_all_tuple_future<T>::value;

        /// \brief Check if all template parameters are when_all_future with tuples as a sequence type
        template <class...> struct are_when_all_tuple_futures : std::true_type {};
        template <class B1> struct are_when_all_tuple_futures<B1> : is_when_all_tuple_future<std::decay_t<B1>> {};
        template <class B1, class... Bn>
        struct are_when_all_tuple_futures<B1, Bn...>
            : std::conditional_t<is_when_all_tuple_future_v<std::decay_t<B1>>, are_when_all_tuple_futures<Bn...>,
                                 std::false_type> {};
        template <class... Args>
        constexpr bool are_when_all_tuple_futures_v = are_when_all_tuple_futures<Args...>::value;

        /// \brief Check if type is a when_all_future with a range as a sequence type
        template <typename T, class Enable = void> struct is_when_all_range_future : std::false_type {};
        template <typename Sequence>
        struct is_when_all_range_future<when_all_future<Sequence>, std::enable_if_t<ranges::range<Sequence>>>
            : std::true_type {};
        template <class T> constexpr bool is_when_all_range_future_v = is_when_all_range_future<T>::value;

        /// \brief Check if all template parameters are when_all_future with tuples as a sequence type
        template <class...> struct are_when_all_range_futures : std::true_type {};
        template <class B1> struct are_when_all_range_futures<B1> : is_when_all_range_future<B1> {};
        template <class B1, class... Bn>
        struct are_when_all_range_futures<B1, Bn...>
            : std::conditional_t<is_when_all_range_future_v<B1>, are_when_all_range_futures<Bn...>, std::false_type> {};
        template <class... Args>
        constexpr bool are_when_all_range_futures_v = are_when_all_range_futures<Args...>::value;

        /// \brief Constructs a when_all_future that is a concatenation of all when_all_futures in args
        /// It's important to be able to merge when_all_future objects because of operator&&
        /// When the user asks for f1 && f2 && f3, we want that to return a single future that
        /// waits for <f1,f2,f3> rather than a future that wait for two futures <f1,<f2,f3>>
        /// \note This function only participates in overload resolution if all types in
        /// std::decay_t<WhenAllFutures>... are specializations of when_all_future with a tuple sequence type
        /// \overload "Merging" a single when_all_future of tuples. Overload provided for symmetry.
        template <class WhenAllFuture, std::enable_if_t<is_when_all_tuple_future_v<WhenAllFuture>, int> = 0>
        decltype(auto) when_all_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        /// \overload Merging a two when_all_future objects of tuples
        template <class WhenAllFuture1, class WhenAllFuture2,
                  std::enable_if_t<are_when_all_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>, int> = 0>
        decltype(auto) when_all_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_all_future(std::move(s));
        }

        /// \overload Merging two+ when_all_future of tuples
        template <class WhenAllFuture1, class... WhenAllFutures,
                  std::enable_if_t<are_when_all_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>, int> = 0>
        decltype(auto) when_all_future_cat(WhenAllFuture1 &&arg0, WhenAllFutures &&...args) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(when_all_future_cat(std::forward<WhenAllFutures>(args)...).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_all_future(std::move(s));
        }
    } // namespace detail

    /// \brief Create a future object that becomes ready when the range of input futures becomes ready
    ///
    /// This function does not participate in overload resolution unless InputIt's value type (i.e.,
    /// typename std::iterator_traits<InputIt>::value_type) is a std::future or
    /// std::shared_future.
    ///
    /// This overload uses a small vector for avoid further allocations for such a simple operation.
    ///
    /// \return Future object of type @ref when_all_future
    template <class InputIt,
              std::enable_if_t<detail::is_valid_when_all_argument_v<typename std::iterator_traits<InputIt>::value_type>,
                               int> = 0>
    when_all_future<small::vector<detail::to_future_t<typename std::iterator_traits<InputIt>::value_type>>>
    when_all(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = std::is_invocable_v<input_type>;
        static_assert(input_is_future || input_is_invocable);
        using output_future_type = detail::to_future_t<input_type>;
        using sequence_type = small::vector<output_future_type>;
        constexpr bool output_is_shared = is_shared_future_v<output_future_type>;

        // Create sequence
        sequence_type v;
        v.reserve(std::distance(first, last));

        // Move or copy the future objects
        if constexpr (input_is_future) {
            if constexpr (output_is_shared) {
                std::copy(first, last, std::back_inserter(v));
            } else /* if constexpr (not output_is_shared) */ {
                std::move(first, last, std::back_inserter(v));
            }
        } else /* if constexpr (input_is_invocable) */ {
            static_assert(input_is_invocable);
            std::transform(first, last, std::back_inserter(v), [](auto &&f) {
                return std::move(asio::post(make_default_executor(), asio::use_future(std::forward<decltype(f)>(f))));
            });
        }

        return when_all_future<sequence_type>(std::move(v));
    }

    /// \brief Create a future object that becomes ready when the range of input futures becomes ready
    ///
    /// This function does not participate in overload resolution unless the range type @ref is_future.
    ///
    /// \return Future object of type @ref when_all_future
    template <class Range, std::enable_if_t<ranges::range<std::decay_t<Range>>, int> = 0>
    when_all_future<small::vector<
        detail::to_future_t<typename std::iterator_traits<typename std::decay_t<Range>::iterator>::value_type>>>
    when_all(Range &&r) {
        return when_all(std::begin(std::forward<Range>(r)), std::end(std::forward<Range>(r)));
    };

    /// \brief Create a future object that becomes ready when all of the input futures become ready
    ///
    /// This function does not participate in overload resolution unless every argument is either a (possibly
    /// cv-qualified) shared_future or a cv-unqualified future, as defined by the trait @ref is_future.
    ///
    /// \return Future object of type @ref when_all_future
    template <class... Futures, std::enable_if_t<detail::are_valid_when_all_arguments_v<Futures...>, int> = 0>
    when_all_future<std::tuple<detail::to_future_t<Futures>...>> when_all(Futures &&...futures) {
        // Infer sequence type
        using sequence_type = std::tuple<detail::to_future_t<Futures>...>;

        // When making the tuple for when_all_future:
        // - futures need to be moved
        // - shared futures need to be copied
        // - lambdas need to be posted
        constexpr auto move_share_or_post = [](auto &&f) {
            if constexpr (is_shared_future_v<decltype(f)>) {
                return std::forward<decltype(f)>(f);
            } else if constexpr (is_future_v<decltype(f)>) {
                return std::move(std::forward<decltype(f)>(f));
            } else /* if constexpr (std::is_invocable_v<decltype(f)>) */ {
                return asio::post(make_default_executor(), asio::use_future(std::forward<decltype(f)>(f)));
            }
        };

        // Create sequence (and infer types as we go)
        sequence_type v = std::make_tuple((detail::to_future_t<Futures>(move_share_or_post(futures)))...);

        return when_all_future<sequence_type>(std::move(v));
    };

    /// \brief Operator to create a future object that becomes ready when all of the input futures are ready
    ///
    /// Cperator&& works for futures and functions (which are converted to futures with the default executor)
    /// If the future is a when_all_future itself, then it gets merged instead of becoming a child future
    /// of another when_all_future.
    ///
    /// When the user asks for f1 && f2 && f3, we want that to return a single future that
    /// waits for <f1,f2,f3> rather than a future that wait for two futures <f1,<f2,f3>>.
    ///
    /// This emulates the usual behavior we expect from other types with operator&&.
    ///
    /// Note that this default behaviour is different from when_all(...), which doesn't merge
    /// the when_all_future objects by default, because they are variadic functions and
    /// this intention can be controlled explicitly:
    /// - when_all(f1,f2,f3) -> <f1,f2,f3>
    /// - when_all(f1,when_all(f2,f3)) -> <f1,<f2,f3>>
    ///
    /// \return @ref when_all_future object that concatenates all futures
    template <
        class T1, class T2,
        std::enable_if_t<detail::is_valid_when_all_argument_v<T1> && detail::is_valid_when_all_argument_v<T2>, int> = 0>
    auto operator&&(T1 &&lhs, T2 &&rhs) {
        constexpr bool first_is_when_all = detail::is_when_all_future_v<T1>;
        constexpr bool second_is_when_all = detail::is_when_all_future_v<T2>;
        constexpr bool both_are_when_all = first_is_when_all && second_is_when_all;
        if constexpr (both_are_when_all) {
            // Merge when all futures with operator&&
            return detail::when_all_future_cat(std::forward<T1>(lhs), std::forward<T2>(rhs));
        } else {
            // At least one of the arguments is not a when_all_future.
            // Any such argument might be another future or a function which needs to become a future.
            // Thus, we need a function to maybe convert these functions to futures.
            auto maybe_make_future = [](auto &&f) {
                if constexpr (std::is_invocable_v<decltype(f)> && not is_future_v<decltype(f)>) {
                    // Convert to future with the default executor if not a future yet
                    return asio::post(make_default_executor(), asio::use_future(std::forward<decltype(f)>(f)));
                } else {
                    if constexpr (is_shared_future_v<decltype(f)>) {
                        return std::forward<decltype(f)>(f);
                    } else {
                        return std::move(std::forward<decltype(f)>(f));
                    }
                }
            };
            // Simplest case, join futures in a new when_all_future
            constexpr bool none_are_when_all = not first_is_when_all && not second_is_when_all;
            if constexpr (none_are_when_all) {
                return when_all(maybe_make_future(std::forward<T1>(lhs)), maybe_make_future(std::forward<T2>(rhs)));
            } else if constexpr (first_is_when_all) {
                // If one of them is a when_all_future, then we need to concatenate the results
                // rather than creating a child in the sequence. To concatenate them, the
                // one that is not a when_all_future needs to become one.
                return detail::when_all_future_cat(lhs, when_all(maybe_make_future(std::forward<T2>(rhs))));
            } else /* if constexpr (second_is_when_all) */ {
                return detail::when_all_future_cat(when_all(maybe_make_future(std::forward<T1>(lhs))), rhs);
            }
        }
    }

    /** @} */  // \addtogroup adaptors Adaptors
} // namespace futures

#endif // FUTURES_WHEN_ALL_H
