//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_WHEN_ANY_HPP
#define FUTURES_ADAPTOR_WHEN_ANY_HPP

/// @file
/// Implement the when_any functionality for futures and executors
/// The same rationale as when_all applies here
/// @see https://en.cppreference.com/w/cpp/experimental/when_any

#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/wait_for_any.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/traits/is_tuple.hpp>
#include <futures/adaptor/detail/lambda_to_future.hpp>
#include <array>
#include <optional>
#include <condition_variable>
#include <shared_mutex>

namespace futures {
    /** @addtogroup adaptors Adaptors
     *  @{
     */

    /// Result type for when_any_future objects
    /**
     *  This is defined in a separate file because many other concepts depend on
     *  this definition, especially the inferences for unwrapping `then`
     *  continuations, regardless of the when_any algorithm.
     */
    template <class Sequence>
    struct when_any_result {
        using size_type = std::size_t;
        using sequence_type = Sequence;

        size_type index{ static_cast<size_type>(-1) };
        sequence_type tasks;
    };


    /// Proxy future class referring to the result of a disjunction of
    /// futures from @ref when_any
    /**
     *  This class implements another future type to identify when one of the
     *  tasks is over.
     *
     *  As with `when_all`, this class acts as a future that checks the results
     *  of other futures to avoid creating a real disjunction of futures that
     *  would need another thread for polling.
     *
     *  Not-polling is easier to emulate for future conjunctions (when_all)
     *  because we can sleep on each task until they are all done, since we need
     *  all of them anyway.
     */
    template <class Sequence>
    class when_any_future {
    private:
        using sequence_type = Sequence;
        static constexpr bool sequence_is_range = is_range_v<sequence_type>;
        static constexpr bool sequence_is_tuple = detail::is_tuple_v<
            sequence_type>;
        static_assert(sequence_is_range || sequence_is_tuple);

    public:
        /// Default constructor.
        ///
        /// Constructs a when_any_future with no shared state. After
        /// construction, valid() == false
        when_any_future() noexcept = default;

        /// Move a sequence of futures into the when_any_future
        /// constructor. The sequence is moved into this future object and the
        /// objects from which the sequence was created get invalidated.
        ///
        /// We immediately set up the notifiers for any input future that
        /// supports lazy continuations.
        explicit when_any_future(sequence_type &&v) noexcept
            : v(std::move(v)) {}

        /// Move constructor.
        ///
        /// Constructs a when_any_future with the shared state of other using
        /// move semantics. After construction, other.valid() == false
        ///
        /// This is a class that controls resources, and their behavior needs to
        /// be moved. However, unlike a vector, some notifier resources cannot
        /// be moved and might need to be recreated, because they expect the
        /// underlying futures to be in a given address to work.
        ///
        /// We cannot move the notifiers because these expect things to be
        /// notified at certain addresses. This means the notifiers in `other`
        /// have to be stopped and we have to be sure of that before its
        /// destructor gets called.
        ///
        /// There are two in operations here.
        /// - Asking the notifiers to stop and waiting
        ///   - This is what we need to do at the destructor because we can't
        ///   destruct "this" until
        ///     we are sure no notifiers are going to try to notify this object
        /// - Asking the notifiers to stop
        ///   - This is what we need to do when moving, because we know we won't
        ///   need these notifiers
        ///     anymore. When the moved object gets destructed, it will ensure
        ///     its notifiers are stopped and finish the task.
        when_any_future(when_any_future &&other) noexcept
            : v(std::move(other.v)) {}

        /// when_any_future is not CopyConstructible
        when_any_future(when_any_future const &other) = delete;

        /// Releases any shared state.
        ///
        /// - If the return object or provider holds the last reference to its
        /// shared state, the shared state is destroyed.
        /// - the return object or provider gives up its reference to its shared
        /// state
        ///
        /// This means we just need to let the internal futures destroy
        /// themselves, but we have to stop notifiers if we have any, because
        /// these notifiers might later try to set tokens in a future that no
        /// longer exists.
        ///
        ~when_any_future() = default;

        /// Assigns the contents of another future object.
        ///
        /// Releases any shared state and move-assigns the contents of other to
        /// *this.
        ///
        /// After the assignment, other.valid() == false and this->valid() will
        /// yield the same value as other.valid() before the assignment.
        ///
        when_any_future &
        operator=(when_any_future &&other) noexcept {
            v = std::move(other.v);
        }

        /// Copy assigns the contents of another when_any_future object.
        ///
        /// when_any_future is not copy assignable.
        when_any_future &
        operator=(when_any_future const &other)
            = delete;

        /// Wait until any future has a valid result and retrieves it
        ///
        /// It effectively calls wait() in order to wait for the result.
        /// This avoids replicating the logic behind continuations, polling, and
        /// notifiers.
        ///
        /// The behavior is undefined if valid() is false before the call to
        /// this function. Any shared state is released. valid() is false after
        /// a call to this method. The value v stored in the shared state, as
        /// std::move(v)
        when_any_result<sequence_type>
        get() {
            // Check if the sequence is valid
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }
            // Wait for the complete sequence to be ready
            wait();
            // Set up a `when_any_result` and move results to it.
            when_any_result<sequence_type> r;
            r.index = get_ready_index();
            r.tasks = std::move(v);
            return r;
        }

        /// Checks if the future refers to a shared state
        ///
        /// This future is always valid() unless there are tasks and they are
        /// all invalid
        ///
        /// @see https://en.cppreference.com/w/cpp/experimental/when_any
        [[nodiscard]] bool
        valid() const noexcept {
            if constexpr (sequence_is_range) {
                if (v.empty()) {
                    return true;
                }
                return std::any_of(v.begin(), v.end(), [](auto &&f) {
                    return f.valid();
                });
            } else {
                if constexpr (std::tuple_size_v<sequence_type> == 0) {
                    return true;
                } else {
                    bool r = true;
                    detail::tuple_for_each(v, [&r](auto &&f) {
                        r = r && f.valid();
                    });
                    return r;
                }
            }
        }

        /// Blocks until the result becomes available.
        /// valid() == true after the call.
        /// The behavior is undefined if valid() == false before the call to
        /// this function
        void
        wait() {
            // Check if the sequence is valid
            if (!valid()) {
                detail::throw_exception<std::future_error>(
                    std::future_errc::no_state);
            }
            // Reuse the logic from wait_for_any here
            wait_for_any(v);
        }

        /// Waits for the result to become available.
        ///
        /// Blocks until specified timeout_duration has elapsed or the result
        /// becomes available, whichever comes first. Not-polling is easier to
        /// emulate for future conjunctions (when_all) because we can sleep on
        /// each task until they are all done, since we need all of them anyway.
        ///
        /// @see https://en.m.wikipedia.org/wiki/Exponential_backoff
        template <class Rep, class Period>
        std::future_status
        wait_for(std::chrono::duration<Rep, Period> const &timeout_duration) {
            if (size() == 0) {
                return std::future_status::ready;
            }
            wait_for_any_for(timeout_duration, v);
            if (get_ready_index() == std::size_t(-1)) {
                return std::future_status::timeout;
            } else {
                return std::future_status::ready;
            }
        }

        /// wait_until waits for a result to become available.
        /// It blocks until specified timeout_time has been reached or the
        /// result becomes available, whichever comes first
        template <class Clock, class Duration>
        std::future_status
        wait_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time) {
            if (size() == 0) {
                return std::future_status::ready;
            }
            wait_for_any_until(timeout_time, v);
            if (get_ready_index() == std::size_t(-1)) {
                return std::future_status::timeout;
            } else {
                return std::future_status::ready;
            }
        }

        /// Check if it's ready
        [[nodiscard]] bool
        is_ready() const {
            auto idx = get_ready_index();
            return idx != static_cast<size_t>(-1) || (size() == 0);
        }

        /// Move the underlying sequence somewhere else
        ///
        /// The when_any_future is left empty and should now be considered
        /// invalid. This is useful for any algorithm that merges two
        /// wait_any_future objects without forcing encapsulation of the merge
        /// function.
        ///
        sequence_type &&
        release() {
            return std::move(v);
        }

    private:
        /// Get index of the first internal future that is ready
        ///
        /// If no future is ready, this returns the sequence size as a sentinel
        template <class CheckLazyContinuables = std::true_type>
        [[nodiscard]] size_t
        get_ready_index() const {
            auto const eq_comp = [](auto &&f) {
                if constexpr (
                    CheckLazyContinuables::value
                    || (!is_continuable_v<std::decay_t<decltype(f)>>) )
                {
                    return ::futures::is_ready(std::forward<decltype(f)>(f));
                } else {
                    return false;
                }
            };
            size_t ready_index(-1);
            if constexpr (sequence_is_range) {
                auto it = std::find_if(v.begin(), v.end(), eq_comp);
                ready_index = it - v.begin();
            } else {
                constexpr auto n = std::tuple_size<sequence_type>();
                ready_index = n;
                detail::mp_for_each<detail::mp_iota_c<n>>([&](auto I) {
                    if (ready_index == n && eq_comp(std::get<I>(v)))
                        ready_index = I;
                });
            }
            if (ready_index == size()) {
                return static_cast<size_t>(-1);
            } else {
                return ready_index;
            }
        }

        /// Get number of internal futures
        [[nodiscard]] constexpr size_t
        size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return v.size();
            }
        }

        /// Get number of internal futures with lazy continuations
        [[nodiscard]] constexpr size_t
        lazy_continuable_size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                if (is_continuable_v<
                        std::decay_t<typename sequence_type::value_type>>)
                {
                    return v.size();
                } else {
                    return 0;
                }
            }
        }

        /// Check if all internal types are lazy continuable
        [[nodiscard]] constexpr bool
        all_lazy_continuable() const {
            return lazy_continuable_size() == size();
        }

        /// Get size, if we know that at compile time
        [[nodiscard]] static constexpr size_t
        compile_time_size() {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return 0;
            }
        }

        /// Check if the i-th future is ready
        [[nodiscard]] bool
        is_ready(size_t index) const {
            if constexpr (!sequence_is_range) {
                return apply(
                           [](auto &&el) -> std::future_status {
                               return el.wait_for(std::chrono::seconds(0));
                           },
                           v,
                           index)
                       == std::future_status::ready;
            } else {
                return v[index].wait_for(std::chrono::seconds(0))
                       == std::future_status::ready;
            }
        }

    private:
        /// Internal wait_any_future state
        sequence_type v;
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// @name Define when_any_future as a kind of future
    /// @{
    /// Specialization explicitly setting when_any_future<T> as a type of future
    template <class T>
    struct is_future<when_any_future<T>> : std::true_type {};
#endif

    namespace detail {
        /// @name Useful traits for when_any
        /// @{

        /// Check if type is a when_any_future as a type
        template <class>
        struct is_when_any_future : std::false_type {};
        template <class Sequence>
        struct is_when_any_future<when_any_future<Sequence>>
            : std::true_type {};

        /// Check if type is a when_any_future as constant bool
        template <class T>
        constexpr bool is_when_any_future_v = is_when_any_future<T>::value;

        /// Check if a type can be used in a future disjunction (when_any
        /// or operator|| for futures)
        template <class T>
        using is_valid_when_any_argument = std::
            disjunction<is_future<std::decay_t<T>>, std::is_invocable<T>>;

        template <class T>
        constexpr bool is_valid_when_any_argument_v
            = is_valid_when_any_argument<T>::value;

        /// Trait to identify valid when_any inputs
        template <class...>
        struct are_valid_when_any_arguments : std::true_type {};
        template <class B1>
        struct are_valid_when_any_arguments<B1>
            : is_valid_when_any_argument<B1> {};
        template <class B1, class... Bn>
        struct are_valid_when_any_arguments<B1, Bn...>
            : std::conditional_t<
                  is_valid_when_any_argument_v<B1>,
                  are_valid_when_any_arguments<Bn...>,
                  std::false_type> {};
        template <class... Args>
        constexpr bool are_valid_when_any_arguments_v
            = are_valid_when_any_arguments<Args...>::value;

        /// \name Helpers for operator|| on futures, functions and
        /// when_any futures

        /// Check if type is a when_any_future with tuples as a sequence
        /// type
        template <class T, class Enable = void>
        struct is_when_any_tuple_future : std::false_type {};
        template <class Sequence>
        struct is_when_any_tuple_future<
            when_any_future<Sequence>,
            std::enable_if_t<detail::is_tuple_v<Sequence>>> : std::true_type {};
        template <class T>
        constexpr bool is_when_any_tuple_future_v = is_when_any_tuple_future<
            T>::value;

        /// Check if all template parameters are when_any_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_any_tuple_futures : std::true_type {};
        template <class B1>
        struct are_when_any_tuple_futures<B1>
            : is_when_any_tuple_future<std::decay_t<B1>> {};
        template <class B1, class... Bn>
        struct are_when_any_tuple_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_tuple_future_v<std::decay_t<B1>>,
                  are_when_any_tuple_futures<Bn...>,
                  std::false_type> {};
        template <class... Args>
        constexpr bool are_when_any_tuple_futures_v
            = are_when_any_tuple_futures<Args...>::value;

        /// Check if type is a when_any_future with a range as a sequence
        /// type
        template <class T, class Enable = void>
        struct is_when_any_range_future : std::false_type {};
        template <class Sequence>
        struct is_when_any_range_future<
            when_any_future<Sequence>,
            std::enable_if_t<is_range_v<Sequence>>> : std::true_type {};
        template <class T>
        constexpr bool is_when_any_range_future_v = is_when_any_range_future<
            T>::value;

        /// Check if all template parameters are when_any_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_any_range_futures : std::true_type {};
        template <class B1>
        struct are_when_any_range_futures<B1> : is_when_any_range_future<B1> {};
        template <class B1, class... Bn>
        struct are_when_any_range_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_range_future_v<B1>,
                  are_when_any_range_futures<Bn...>,
                  std::false_type> {};
        template <class... Args>
        constexpr bool are_when_any_range_futures_v
            = are_when_any_range_futures<Args...>::value;

        /// Constructs a when_any_future that is a concatenation of all
        /// when_any_futures in args It's important to be able to merge
        /// when_any_future objects because of operator|| When the user asks for
        /// f1 && f2 && f3, we want that to return a single future that waits
        /// for <f1,f2,f3> rather than a future that wait for two futures
        /// <f1,<f2,f3>> @note This function only participates in overload
        /// resolution if all types in std::decay_t<WhenAllFutures>... are
        /// specializations of when_any_future with a tuple sequence type
        ///
        /// @note "Merging" a single when_any_future of tuples. Overload
        /// provided for symmetry.
        ///
        template <
            class WhenAllFuture,
            std::enable_if_t<is_when_any_tuple_future_v<WhenAllFuture>, int> = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        /// @overload Merging a two when_any_future objects of tuples
        template <
            class WhenAllFuture1,
            class WhenAllFuture2,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>,
                int>
            = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future(std::move(s));
        }

        /// @overload Merging two+ when_any_future of tuples
        template <
            class WhenAllFuture1,
            class... WhenAllFutures,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>,
                int>
            = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFutures &&...args) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(
                when_any_future_cat(std::forward<WhenAllFutures>(args)...)
                    .release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future(std::move(s));
        }

        /// @}
    } // namespace detail

    /// Create a future object that becomes ready when any of the futures
    /// in the range is ready
    ///
    /// This function does not participate in overload resolution unless
    /// InputIt's value type (i.e., typename
    /// std::iterator_traits<InputIt>::value_type) @ref is_future .
    ///
    /// This overload uses a small vector to avoid further allocations for such
    /// a simple operation.
    ///
    /// @return @ref when_any_future with all future objects
    template <
        class InputIt
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            detail::is_valid_when_any_argument_v<
                typename std::iterator_traits<InputIt>::value_type>,
            // clang-format on
            int>
        = 0
#endif
        >
    when_any_future<detail::small_vector<detail::lambda_to_future_t<
        typename std::iterator_traits<InputIt>::value_type>>>
    when_any(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<
            typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = std::is_invocable_v<input_type>;
        static_assert(input_is_future || input_is_invocable);
        using output_future_type = detail::lambda_to_future_t<input_type>;
        using sequence_type = detail::small_vector<output_future_type>;
        constexpr bool output_is_shared = is_shared_future_v<output_future_type>;

        // Create sequence
        sequence_type v;
        v.reserve(std::distance(first, last));

        // Move or copy the future objects
        if constexpr (input_is_future) {
            if constexpr (output_is_shared) {
                std::copy(first, last, std::back_inserter(v));
            } else /* if constexpr (input_is_future) */ {
                std::move(first, last, std::back_inserter(v));
            }
        } else /* if constexpr (input_is_invocable) */ {
            static_assert(input_is_invocable);
            std::transform(first, last, std::back_inserter(v), [](auto &&f) {
                return ::futures::async(std::forward<decltype(f)>(f));
            });
        }

        return when_any_future<sequence_type>(std::move(v));
    }

    /// Create a future object that becomes ready when any of the futures
    /// in the range is ready
    ///
    /// This function does not participate in overload resolution unless every
    /// argument is either a (possibly cv-qualified) std::shared_future or a
    /// cv-unqualified std::future.
    ///
    /// @return @ref when_any_future with all future objects
    template <
        class Range
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_range_v<std::decay_t<Range>>, int> = 0
#endif
        >
    when_any_future<
        detail::small_vector<detail::lambda_to_future_t<typename std::iterator_traits<
            typename std::decay_t<Range>::iterator>::value_type>>>
    when_any(Range &&r) {
        return when_any(
            std::begin(std::forward<Range>(r)),
            std::end(std::forward<Range>(r)));
    }

    /// Create a future object that becomes ready when any of the input
    /// futures is ready
    ///
    /// This function does not participate in overload resolution unless every
    /// argument is either a (possibly cv-qualified) std::shared_future or a
    /// cv-unqualified std::future.
    ///
    /// @return @ref when_any_future with all future objects
    template <
        class... Futures
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            // clang-format off
            detail::are_valid_when_any_arguments_v<Futures...>,
            // clang-format on
            int>
        = 0
#endif
        >
    when_any_future<std::tuple<detail::lambda_to_future_t<Futures>...>>
    when_any(Futures &&...futures) {
        // Infer sequence type
        using sequence_type = std::tuple<detail::lambda_to_future_t<Futures>...>;

        // When making the tuple for when_any_future:
        // - futures need to be moved
        // - shared futures need to be copied
        // - lambdas need to be posted
        [[maybe_unused]] constexpr auto move_share_or_post = [](auto &&f) {
            if constexpr (is_shared_future_v<std::decay_t<decltype(f)>>) {
                return std::forward<decltype(f)>(f);
            } else if constexpr (is_future_v<std::decay_t<decltype(f)>>) {
                return std::move(std::forward<decltype(f)>(f));
            } else /* if constexpr (std::is_invocable_v<decltype(f)>) */ {
                return ::futures::async(std::forward<decltype(f)>(f));
            }
        };

        // Create sequence (and infer types as we go)
        sequence_type v = std::make_tuple((move_share_or_post(futures))...);

        return when_any_future<sequence_type>(std::move(v));
    }

    /// Operator to create a future object that becomes ready when any of
    /// the input futures is ready
    ///
    /// ready operator|| works for futures and functions (which are converted to
    /// futures with the default executor) If the future is a when_any_future
    /// itself, then it gets merged instead of becoming a child future of
    /// another when_any_future.
    ///
    /// When the user asks for f1 || f2 || f3, we want that to return a single
    /// future that waits for <f1 || f2 || f3> rather than a future that wait
    /// for two futures <f1 || <f2 || f3>>.
    ///
    /// This emulates the usual behavior we expect from other types with
    /// operator||.
    ///
    /// Note that this default behaviour is different from when_any(...), which
    /// doesn't merge the when_any_future objects by default, because they are
    /// variadic functions and this intention can be controlled explicitly:
    /// - when_any(f1,f2,f3) -> <f1 || f2 || f3>
    /// - when_any(f1,when_any(f2,f3)) -> <f1 || <f2 || f3>>
    template <
        class T1,
        class T2
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_when_any_argument_v<T1>
                && detail::is_valid_when_any_argument_v<T2>,
            int>
        = 0
#endif
        >
    auto
    operator||(T1 &&lhs, T2 &&rhs) {
        constexpr bool first_is_when_any = detail::is_when_any_future_v<T1>;
        constexpr bool second_is_when_any = detail::is_when_any_future_v<T2>;
        constexpr bool both_are_when_any = first_is_when_any
                                           && second_is_when_any;
        if constexpr (both_are_when_any) {
            // Merge when all futures with operator||
            return detail::when_any_future_cat(
                std::forward<T1>(lhs),
                std::forward<T2>(rhs));
        } else {
            // At least one of the arguments is not a when_any_future.
            // Any such argument might be another future or a function which
            // needs to become a future. Thus, we need a function to maybe
            // convert these functions to futures.
            auto maybe_make_future = [](auto &&f) {
                if constexpr (
                    std::is_invocable_v<decltype(f)>
                    && (!is_future_v<decltype(f)>) )
                {
                    // Convert to future with the default executor if not a
                    // future yet
                    return async(f);
                } else {
                    if constexpr (is_shared_future_v<decltype(f)>) {
                        return std::forward<decltype(f)>(f);
                    } else {
                        return std::move(std::forward<decltype(f)>(f));
                    }
                }
            };
            // Simplest case, join futures in a new when_any_future
            constexpr bool none_are_when_any = !first_is_when_any
                                               && !second_is_when_any;
            if constexpr (none_are_when_any) {
                return when_any(
                    maybe_make_future(std::forward<T1>(lhs)),
                    maybe_make_future(std::forward<T2>(rhs)));
            } else if constexpr (first_is_when_any) {
                // If one of them is a when_any_future, then we need to
                // concatenate the results rather than creating a child in the
                // sequence. To concatenate them, the one that is not a
                // when_any_future needs to become one.
                return detail::when_any_future_cat(
                    lhs,
                    when_any(maybe_make_future(std::forward<T2>(rhs))));
            } else /* if constexpr (second_is_when_any) */ {
                return detail::when_any_future_cat(
                    when_any(maybe_make_future(std::forward<T1>(lhs))),
                    rhs);
            }
        }
    }

    /** @} */
} // namespace futures
#endif // FUTURES_ADAPTOR_WHEN_ANY_HPP
