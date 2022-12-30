//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_WHEN_ALL_HPP
#define FUTURES_ADAPTOR_WHEN_ALL_HPP

/**
 *  @file adaptor/when_all.hpp
 *  @brief Conjunction adaptors
 *
 *  This file defines adaptors to create a new future representing the
 *  conjunction of other future objects.
 *
 *  Because all tasks need to be done to achieve the result, the algorithm
 *  doesn't depend much on the properties of the underlying futures. The thread
 *  that is awaiting just needs sleep and await for each of the internal
 *  futures.
 *
 *  The usual approach, without our future concepts, like in returning another
 *  std::future, is to start another polling thread, which sets a promise when
 *  all other futures are ready. If the futures support lazy continuations,
 *  these promises can be set from the previous objects. However, this has an
 *  obvious cost for such a trivial operation, given that the solutions is
 *  already available in the underlying futures.
 *
 *  Instead, we implement one more future type `when_all_future` that can query
 *  if the futures are ready and waits for them to be ready whenever get() is
 *  called. This proxy object can then be converted to a regular future if the
 *  user needs to.
 *
 *  This has a disadvantage over futures with lazy continuations because we
 *  might need to schedule another task if we need notifications from this
 *  future. However, we avoid scheduling another task right now, so this is, at
 *  worst, as good as the common approach of wrapping it into another existing
 *  future type.
 *
 *  If the input futures are not shared, they are moved into `when_all_future`
 *  and are invalidated, as usual. The `when_all_future` cannot be shared.
 */

#include <futures/config.hpp>
#include <futures/launch.hpp>
#include <futures/throw.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/traits/std_type_traits.hpp>
#include <futures/detail/utility/invoke.hpp>
#include <futures/adaptor/detail/lambda_to_future.hpp>
#include <futures/detail/deps/boost/mp11/algorithm.hpp>
#include <futures/detail/deps/boost/mp11/function.hpp>
#include <futures/detail/deps/boost/mp11/tuple.hpp>

namespace futures {
    /** @addtogroup adaptors Adaptors
     *  @{
     */

    /// Proxy future class referring to a conjunction of futures from @ref
    /// when_all
    /**
     *  This class implements the behavior of the `when_all` operation as
     *  another future type, which can handle heterogeneous future objects.
     *
     *  This future type logically checks the results of other futures in place
     *  to avoid creating a real conjunction of futures that would need to be
     *  polling (or be a lazy continuation) on another thread.
     *
     *  If the user does want to poll on another thread, then this can be
     *  converted into a cfuture as usual with async. If the other future holds
     *  the when_all_state as part of its state, then it can become another
     *  future.
     */
    template <class Sequence>
    class when_all_future {
    private:
        using sequence_type = Sequence;
        static constexpr bool sequence_is_range = is_range_v<sequence_type>;
        static constexpr bool sequence_is_tuple = detail::
            mp_similar<std::tuple<>, sequence_type>::value;
        FUTURES_STATIC_ASSERT(sequence_is_range || sequence_is_tuple);

    public:
        /// Constructor.
        /**
         *  Constructs a when_all_future with no shared state. After
         *  construction, valid() == false
         */
        when_all_future() = default;

        /// Move a sequence of futures into the when_all_future
        /**
         *  The sequence is moved into this future object and the
         *  objects from which the sequence was created get invalidated
         */
        explicit when_all_future(sequence_type &&v) noexcept(
            std::is_nothrow_move_assignable<sequence_type>::value)
            : v(std::move(v)) {}

        /// Move constructor.
        /**
         *  Constructs a when_all_future with the shared state of other using
         *  move semantics. After construction, other.valid() == false
         */
        when_all_future(when_all_future &&other) noexcept(
            std::is_nothrow_move_assignable<sequence_type>::value)
            : v(std::move(other.v)) {}

        /// when_all_future is not CopyConstructible
        when_all_future(when_all_future const &other) = delete;

        /// Releases any shared state.
        /**
         *  - If the return object or provider holds the last reference to its
         *  shared state, the shared state is destroyed
         *  - the return object or provider gives up its reference to its shared
         *  state This means we just need to let the internal futures destroy
         *  themselves
         */
        ~when_all_future() = default;

        /// Assigns the contents of another future object.
        /**
         *  Releases any shared state and move-assigns the contents of other to
         *  *this. After the assignment, other.valid() == false and
         *  this->valid() will yield the same value as other.valid() before the
         *  assignment.
         */
        when_all_future &
        operator=(when_all_future &&other) noexcept(
            std::is_nothrow_move_assignable<sequence_type>::value) {
            v = std::move(other.v);
        }

        /// when_all_future is not CopyAssignable.
        when_all_future &
        operator=(when_all_future const &other)
            = delete;

        /// Wait until all futures have a valid result and retrieves it
        /**
         *  It effectively calls wait() in order to wait for the result.
         *  The behavior is undefined if valid() is false before the call to
         *  this function. Any shared state is released. valid() is false after
         *  a call to this method. The value v stored in the shared state, as
         *  std::move(v)
         */
        sequence_type
        get() {
            // Check if the sequence is valid
            if (!valid()) {
                throw_exception(no_state{});
            }
            // Wait for the complete sequence to be ready
            wait();
            // Move results
            return std::move(v);
        }

        /// Checks if the future refers to a shared state
        FUTURES_NODISCARD bool
        valid() const noexcept {
            return valid_impl(boost::mp11::mp_bool<sequence_is_range>{});
        }

    private:
        FUTURES_NODISCARD bool
        valid_impl(std::true_type /* sequence_is_range */) const noexcept {
            return std::all_of(v.begin(), v.end(), [](auto &&f) {
                return f.valid();
            });
        }

        FUTURES_NODISCARD bool
        valid_impl(std::false_type /* sequence_is_range */) const noexcept {
            bool r = true;
            detail::tuple_for_each(v, [&r](auto &&f) { r = r && f.valid(); });
            return r;
        }
    public:
        /// Blocks until the result becomes available.
        /**
         *  valid() == true after the call.
         *  The behavior is undefined if valid() == false before the call to
         *  this function
         */
        void
        wait() const {
            // Check if the sequence is valid
            if (!valid()) {
                throw_exception(no_state{});
            }
            wait_impl(boost::mp11::mp_bool<sequence_is_range>{});
        }

    private:
        void
        wait_impl(std::true_type /* sequence_is_range */) const {
            std::for_each(v.begin(), v.end(), [](auto &&f) { f.wait(); });
        }

        void
        wait_impl(std::false_type /* sequence_is_range */) const {
            detail::tuple_for_each(v, [](auto &&f) { f.wait(); });
        }

        constexpr bool
        is_empty_impl(std::true_type /* sequence is tuple */) const {
            return std::tuple_size<sequence_type>::value == 0;
        }

        constexpr bool
        is_empty_impl(std::false_type /* sequence is tuple */) const {
            return v.empty();
        }

        constexpr bool
        is_empty() const {
            return is_empty_impl(boost::mp11::mp_bool<sequence_is_tuple>{});
        }

        template <class F>
        FUTURES_NODISCARD future_status
        wait_for_impl(std::true_type /* sequence_is_range */, F &equal_fn)
            const {
            // Use a hack to "break" for_each loops with find_if
            auto it = std::find_if(v.begin(), v.end(), equal_fn);
            return (it == v.end()) ?
                       future_status::ready :
                       it->wait_for(std::chrono::seconds(0));
        }

        template <class F>
        FUTURES_NODISCARD future_status
        wait_for_tuple_impl(std::true_type /* is_empty */, F &) const {
            return future_status::ready;
        }

        template <class F>
        FUTURES_NODISCARD future_status
        wait_for_tuple_impl(std::false_type /* is_empty */, F &equal_fn) const {
            constexpr auto n = std::tuple_size<sequence_type>::value;
            auto idx = n;
            detail::mp_for_each<detail::mp_iota_c<n>>([&](auto I) {
                if (idx == n && equal_fn(std::get<I>(v))) {
                    idx = decltype(I)::value;
                }
            });
            if (idx == n) {
                return future_status::ready;
            } else {
                return detail::mp_with_index<n>(idx, [&](auto I) {
                    return std::get<I>(v).wait_for(std::chrono::seconds(0));
                });
            }
        }

        template <class F>
        FUTURES_NODISCARD future_status
        wait_for_impl(std::false_type /* sequence_is_range */, F &equal_fn)
            const {
            constexpr auto n = std::tuple_size<sequence_type>::value;
            return wait_for_tuple_impl(boost::mp11::mp_bool<n == 0>{}, equal_fn);
        }

        template <class Rep, class Period>
        FUTURES_NODISCARD future_status
        wait_for_impl(
            std::chrono::duration<Rep, Period> const &timeout_duration) const {
            // Duration spent waiting
            using duration_type = std::chrono::duration<Rep, Period>;
            using namespace std::chrono;
            auto start_time = system_clock::now();
            duration_type total_elapsed = duration_cast<duration_type>(
                nanoseconds(0));

            // Look for a future that's not ready.
            // - Wait for i-th future and discount from duration
            // - Return if no duration left, or we found a future is not ready
            auto equal_fn = [&](auto &&f) {
                future_status s = f.wait_for(timeout_duration - total_elapsed);
                total_elapsed = duration_cast<duration_type>(
                    system_clock::now() - start_time);
                const bool when_all_impossible = s != future_status::ready;
                return when_all_impossible || total_elapsed > timeout_duration;
            };

            return wait_for_impl(
                boost::mp11::mp_bool<sequence_is_range>{},
                equal_fn);
        }

    public:
        /// Waits for the result to become available.
        /**
         *  Blocks until specified timeout_duration has elapsed or the result
         *  becomes available, whichever comes first.
         *
         *  @param timeout_duration Time to wait
         *  @return Status of the future after the specified duration
         */
        template <class Rep, class Period>
        FUTURES_NODISCARD future_status
        wait_for(
            std::chrono::duration<Rep, Period> const &timeout_duration) const {
            if (is_empty()) {
                return future_status::ready;
            }

            // Check if the sequence is valid
            if (!valid()) {
                throw_exception(no_state{});
            }

            return wait_for_impl(timeout_duration);
        }

        /// Waits for a result to become available.
        /**
         *  It blocks until specified timeout_time has been reached or the
         *  result becomes available, whichever comes first
         *
         *  @param timeout_time The timepoint to wait until
         *  @return Status of the future after the specified duration
         */
        template <class Clock, class Duration>
        future_status
        wait_until(std::chrono::time_point<Clock, Duration> const &timeout_time)
            const {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ?
                       wait_for(std::chrono::seconds(0)) :
                       wait_for(
                           timeout_time - std::chrono::system_clock::now());
        }

        /// Allow move the underlying sequence somewhere else
        /**
         *  The when_all_future is left empty and should now be considered
         *  invalid. This is useful for the algorithm that merges two
         *  wait_all_future objects without forcing encapsulation of the merge
         *  function.
         */
        sequence_type &&
        release() {
            return std::move(v);
        }

        /// Request the stoppable futures to stop
        bool
        request_stop() noexcept {
            bool any_request = false;
            auto f_request_stop = [&](auto &&f) {
                any_request = any_request || f.request_stop();
            };
            FUTURES_IF_CONSTEXPR (sequence_is_range) {
                std::for_each(v.begin(), v.end(), f_request_stop);
            } else {
                tuple_for_each(v, f_request_stop);
            }
            return any_request;
        }

    private:
        /// Internal wait_all_future state
        sequence_type v;
    };

    /// Create a future object that becomes ready when the range of input
    /// futures becomes ready
    /**
     *  This function does not participate in overload resolution unless
     *  InputIt's value type (i.e., typename
     *  std::iterator_traits<InputIt>::value_type) is a std::future or
     *  std::shared_future.
     *
     *  This overload uses a small vector for avoid further allocations for such
     *  a simple operation.
     *
     *  @param first,last Range of futures
     *  @return Future object of type @ref when_all_future
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class InputIt>
    requires detail::disjunction_v<
        is_future<
            std::decay_t<typename std::iterator_traits<InputIt>::value_type>>,
        detail::is_invocable<
            std::decay_t<typename std::iterator_traits<InputIt>::value_type>>>
#else
    template <
        class InputIt,
        std::enable_if_t<
            detail::disjunction_v<
                is_future<std::decay_t<
                    typename std::iterator_traits<InputIt>::value_type>>,
                detail::is_invocable<std::decay_t<
                    typename std::iterator_traits<InputIt>::value_type>>>,
            int>
        = 0>
#endif
    when_all_future<
        FUTURES_DETAIL(detail::small_vector<detail::lambda_to_future_t<
                           typename std::iterator_traits<InputIt>::value_type>>)>
    when_all(InputIt first, InputIt last);

    /// @copybrief when_all
    /**
     *  This function does not participate in overload resolution unless the
     *  range type @ref is_future.
     *
     *  @param r Range of futures
     *  @return Future object of type @ref when_all_future
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class Range>
    requires is_range_v<std::decay_t<Range>>
#else
    template <
        class Range,
        std::enable_if_t<is_range_v<std::decay_t<Range>>, int> = 0>
#endif
    when_all_future<
        FUTURES_DETAIL(detail::small_vector<detail::lambda_to_future_t<
                           typename std::iterator_traits<typename std::decay_t<
                               Range>::iterator>::value_type>>)>
    when_all(Range &&r) {
        using std::begin;
        using std::end;
        return when_all(
            begin(std::forward<Range>(r)),
            end(std::forward<Range>(r)));
    }

    /// @copybrief when_all
    /**
     *  This function does not participate in overload resolution unless every
     *  argument is either a (possibly cv-qualified) shared_future or a
     *  cv-unqualified future, as defined by the trait @ref is_future.
     *
     *  @param futures Instances of future objects
     *  @return Future object of type @ref when_all_future
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class... Futures>
    requires detail::conjunction_v<detail::disjunction<
        is_future<std::decay_t<Futures>>,
        detail::is_invocable<std::decay_t<Futures>>>...>
#else
    template <
        class... Futures,
        std::enable_if_t<
            detail::conjunction_v<detail::disjunction<
                is_future<std::decay_t<Futures>>,
                detail::is_invocable<std::decay_t<Futures>>>...>,
            int>
        = 0>
#endif
    when_all_future<
        std::tuple<FUTURES_DETAIL(detail::lambda_to_future_t<Futures>...)>>
    when_all(Futures &&...futures);

    /// @copybrief when_all
    /**
     *  Operator&& works for futures and functions (which are converted to
     *  futures with the default executor) If the future is a when_all_future
     *  itself, then it gets merged instead of becoming a child future of
     *  another when_all_future.
     *
     *  When the user asks for `f1 && f2 && f3`, we want that to return a single
     *  future that waits for `<f1,f2,f3>` rather than a future that wait for
     * two futures `<f1,<f2,f3>>`.
     *
     *  This emulates the usual behavior we expect from other types with
     *  operator&&.
     *
     *  Note that this default behaviour is different from `when_all(...)`,
     * which doesn't merge the when_all_future objects by default, because they
     * are variadic functions and this intention can be controlled explicitly:
     *  - `when_all(f1,f2,f3)` -> `<f1,f2,f3>`
     *  - `when_all(f1,when_all(f2,f3))` -> `<f1,<f2,f3>>`
     *
     *  @param lhs,rhs Future objects or callables
     *  @return @ref when_all_future object that concatenates all futures
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class T1, class T2>
    requires detail::disjunction_v<
                 is_future<std::decay_t<T1>>,
                 detail::is_invocable<std::decay_t<T1>>>
             && detail::disjunction_v<
                 is_future<std::decay_t<T2>>,
                 detail::is_invocable<std::decay_t<T2>>>
#else
    template <
        class T1,
        class T2,
        std::enable_if_t<
            detail::disjunction_v<
                is_future<std::decay_t<T1>>,
                detail::is_invocable<std::decay_t<T1>>>
                && detail::disjunction_v<
                    is_future<std::decay_t<T2>>,
                    detail::is_invocable<std::decay_t<T2>>>,
            int>
        = 0>
#endif
    FUTURES_DETAIL(auto)
    operator&&(T1 &&lhs, T2 &&rhs);

    /** @} */
} // namespace futures

#include <futures/adaptor/impl/when_all.hpp>

#endif // FUTURES_ADAPTOR_WHEN_ALL_HPP
