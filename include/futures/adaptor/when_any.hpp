//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_ADAPTOR_WHEN_ANY_HPP
#define FUTURES_ADAPTOR_WHEN_ANY_HPP

/**
 *  @file adaptor/when_any.hpp
 *  @brief Disjunction adaptors
 *
 *  This file defines adaptors to create a new future representing the
 *  disjunction of other future objects.
 *
 *  It implements the when_any functionality for futures and executors
 *  The same rationale as `std::experimental::when_any` applies here
 *  @see
 * [`std::experimental::when_any`](https://en.cppreference.com/w/cpp/experimental/when_any)
 */

#include <futures/is_ready.hpp>
#include <futures/launch.hpp>
#include <futures/wait_for_any.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/detail/container/small_vector.hpp>
#include <futures/detail/utility/invoke.hpp>
#include <futures/adaptor/detail/lambda_to_future.hpp>
#include <futures/detail/deps/boost/mp11/function.hpp>
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
        /// Type used to represent the number of futures in the result
        using size_type = std::size_t;

        /// A sequence type with all the futures
        /**
         * This sequence might be a range or a tuple.
         */
        using sequence_type = Sequence;

        /// Index of the element whose result was ready first
        size_type index{ static_cast<size_type>(-1) };

        /// The sequence of future objects waited for
        sequence_type tasks;
    };

    /// Future object referring to the result of a disjunction of futures
    /**
     *  This class implements another future type to identify when one of a
     *  list of tasks is over.
     *
     *  As with @ref when_all, this class acts as a future that checks the
     * results of other futures to avoid creating a real disjunction of futures
     * that would need another thread for polling.
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
        static constexpr bool sequence_is_tuple = detail::
            mp_similar<std::tuple<>, sequence_type>::value;
        FUTURES_STATIC_ASSERT(sequence_is_range || sequence_is_tuple);

    public:
        /// Default constructor.
        /**
         *  Constructs a when_any_future with no shared state. After
         *  construction, valid() == false
         */
        when_any_future() = default;

        /// Move a sequence of futures into the when_any_future constructor.
        /**
         *  The sequence is moved into this future object and the
         *  objects from which the sequence was created get invalidated.
         *
         *  We immediately set up the notifiers for any input future that
         *  supports lazy continuations.
         */
        explicit when_any_future(sequence_type &&v) noexcept(
            std::is_nothrow_move_constructible<sequence_type>::value)
            : v(std::move(v)) {}

        /// Move constructor.
        /**
         *  Constructs a when_any_future with the shared state of other using
         *  move semantics. After construction, other.valid() == false
         *
         *  This is a class that controls resources, and their behavior needs to
         *  be moved. However, unlike a vector, some notifier resources cannot
         *  be moved and might need to be recreated, because they expect the
         *  underlying futures to be in a given address to work.
         *
         *  We cannot move the notifiers because these expect things to be
         *  notified at certain addresses. This means the notifiers in `other`
         *  have to be stopped and we have to be sure of that before its
         *  destructor gets called.
         *
         *  There are two in operations here.
         *  - Asking the notifiers to stop and waiting
         *    - This is what we need to do at the destructor because we can't
         *    destruct "this" until
         *      we are sure no notifiers are going to try to notify this object
         *  - Asking the notifiers to stop
         *    - This is what we need to do when moving, because we know we won't
         *    need these notifiers
         *      anymore. When the moved object gets destructed, it will ensure
         *      its notifiers are stopped and finish the task.
         */
        when_any_future(when_any_future &&other) noexcept(
            std::is_nothrow_move_constructible<sequence_type>::value)
            : v(std::move(other.v)) {}

        /// when_any_future is not CopyConstructible
        when_any_future(when_any_future const &other) = delete;

        /// Releases any shared state.
        /**
         *  - If the return object or provider holds the last reference to its
         *  shared state, the shared state is destroyed.
         *  - the return object or provider gives up its reference to its shared
         *  state
         *
         *  This means we just need to let the internal futures destroy
         *  themselves, but we have to stop notifiers if we have any, because
         *  these notifiers might later try to set tokens in a future that no
         *  longer exists.
         */
        ~when_any_future() = default;

        /// Assigns the contents of another future object.
        /**
         *  Releases any shared state and move-assigns the contents of other to
         *  *this.
         *
         *  After the assignment, other.valid() == false and this->valid() will
         *  yield the same value as other.valid() before the assignment.
         */
        when_any_future &
        operator=(when_any_future &&other) noexcept(
            std::is_nothrow_move_assignable<sequence_type>::value) {
            v = std::move(other.v);
        }

        /// Copy assigns the contents of another when_any_future object.
        /**
         *  @ref when_any_future is not copy assignable.
         */
        when_any_future &
        operator=(when_any_future const &other)
            = delete;

        /// Wait until any future has a valid result and retrieves it
        /**
         *  It effectively calls wait() in order to wait for the result.
         *  This avoids replicating the logic behind continuations, polling, and
         *  notifiers.
         *
         *  The behavior is undefined if valid() is false before the call to
         *  this function. Any shared state is released. valid() is false after
         *  a call to this method. The value v stored in the shared state, as
         *  std::move(v)
         *
         *  @return A @ref when_any_result holding the future objects
         */
        when_any_result<sequence_type>
        get() {
            // Check if the sequence is valid
            if (!valid()) {
                throw_exception(no_state{});
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
        /**
         *  This future is always valid() unless there are tasks and they are
         *  all invalid
         *
         *  @see
         * [`std::experimental::when_any`](https://en.cppreference.com/w/cpp/experimental/when_any)
         *
         *  @return Return `true` if underlying futures are valid
         */
        FUTURES_NODISCARD bool
        valid() const noexcept {
            return valid_impl(boost::mp11::mp_bool<sequence_is_range>{});
        }

    private:
        FUTURES_NODISCARD bool
        valid_impl(std::true_type /* sequence_is_range */) const noexcept {
            if (v.empty()) {
                return true;
            }
            return std::any_of(v.begin(), v.end(), [](auto &&f) {
                return f.valid();
            });
        }

        FUTURES_NODISCARD bool
        valid_impl(std::false_type /* sequence_is_range */) const noexcept {
            return valid_tuple_impl(
                boost::mp11::mp_bool<
                    std::tuple_size<sequence_type>::value == 0>{});
        }

        FUTURES_NODISCARD bool
        valid_tuple_impl(std::true_type /* is_empty */) const noexcept {
            return true;
        }

        FUTURES_NODISCARD bool
        valid_tuple_impl(std::false_type /* is_empty */) const noexcept {
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
        wait() {
            // Check if the sequence is valid
            if (!valid()) {
                throw_exception(no_state{});
            }
            // Reuse the logic from wait_for_any here
            wait_for_any(v);
        }

        /// Waits for the result to become available.
        /**
         *  Blocks until specified timeout_duration has elapsed or the result
         *  becomes available, whichever comes first. Not-polling is easier to
         *  emulate for future conjunctions (when_all) because we can sleep on
         *  each task until they are all done, since we need all of them anyway.
         *
         *  @param timeout_duration Time to wait
         *  @return Status of the future after the specified duration
         *
         *  @see https://en.m.wikipedia.org/wiki/Exponential_backoff
         */
        template <class Rep, class Period>
        future_status
        wait_for(std::chrono::duration<Rep, Period> const &timeout_duration) {
            if (size() == 0) {
                return future_status::ready;
            }
            wait_for_any_for(timeout_duration, v);
            if (get_ready_index() == std::size_t(-1)) {
                return future_status::timeout;
            } else {
                return future_status::ready;
            }
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
        wait_until(
            std::chrono::time_point<Clock, Duration> const &timeout_time) {
            if (size() == 0) {
                return future_status::ready;
            }
            wait_for_any_until(timeout_time, v);
            if (get_ready_index() == std::size_t(-1)) {
                return future_status::timeout;
            } else {
                return future_status::ready;
            }
        }

        /// Check if it's ready
        FUTURES_NODISCARD bool
        is_ready() const {
            auto idx = get_ready_index();
            return idx != static_cast<size_t>(-1) || (size() == 0);
        }

        /// Move the underlying sequence somewhere else
        /**
         *  The when_any_future is left empty and should now be considered
         *  invalid. This is useful for any algorithm that merges two
         *  wait_any_future objects without forcing encapsulation of the merge
         *  function.
         */
        sequence_type &&
        release() {
            return std::move(v);
        }

    private:
        // Get index of the first internal future that is ready
        // If no future is ready, this returns the sequence size as a sentinel
        template <class CheckLazyContinuables = std::true_type>
        FUTURES_NODISCARD size_t
        get_ready_index() const {
            auto const eq_comp = [](auto &&f) {
                FUTURES_IF_CONSTEXPR (
                    CheckLazyContinuables::value
                    || (!is_continuable_v<std::decay_t<decltype(f)>>) )
                {
                    return ::futures::is_ready(std::forward<decltype(f)>(f));
                } else {
                    return false;
                }
            };
            return get_ready_index_impl(
                boost::mp11::mp_bool<sequence_is_range>{},
                eq_comp);
        }

        template <class F>
        FUTURES_NODISCARD size_t
        get_ready_index_impl(std::true_type /* sequence_is_range */, F &eq_comp)
            const {
            auto it = std::find_if(v.begin(), v.end(), eq_comp);
            if (it != v.end()) {
                return it - v.begin();
            } else {
                return size_t(-1);
            }
        }

        template <class F>
        FUTURES_NODISCARD size_t
        get_ready_index_impl(
            std::false_type /* sequence_is_range */,
            F &eq_comp) const {
            constexpr auto n = std::tuple_size<sequence_type>();
            std::size_t ready_index(-1);
            detail::mp_for_each<detail::mp_iota_c<n>>([&](auto I) {
                if (ready_index == std::size_t(-1) && eq_comp(std::get<I>(v))) {
                    ready_index = I;
                }
            });
            return ready_index;
        }

        /// Get number of internal futures
        FUTURES_NODISCARD constexpr size_t
        size() const {
            return size_impl(boost::mp11::mp_bool<sequence_is_tuple>{});
        }

        FUTURES_NODISCARD constexpr size_t
        size_impl(std::true_type /* sequence_is_tuple */) const {
            return std::tuple_size<sequence_type>::value;
        }

        FUTURES_NODISCARD constexpr size_t
        size_impl(std::false_type /* sequence_is_tuple */) const {
            return v.size();
        }
    public:
        /// Get number of internal futures with lazy continuations
        FUTURES_NODISCARD constexpr size_t
        lazy_continuable_size() const {
            return lazy_continuable_size_impl(
                boost::mp11::mp_bool<sequence_is_tuple>{});
        }

    private:
        FUTURES_NODISCARD constexpr size_t
        lazy_continuable_size_impl(
            std::true_type /* sequence_is_tuple */) const {
            return std::tuple_size<sequence_type>::value;
        }

        FUTURES_NODISCARD constexpr size_t
        lazy_continuable_size_impl(
            std::false_type /* sequence_is_tuple */) const {
            if (is_continuable_v<
                    std::decay_t<typename sequence_type::value_type>>)
            {
                return v.size();
            } else {
                return 0;
            }
        }

    public:
        /// Check if all internal types are lazy continuable
        FUTURES_NODISCARD constexpr bool
        all_lazy_continuable() const {
            return lazy_continuable_size() == size();
        }

        /// Get size, if we know that at compile time
        FUTURES_NODISCARD static constexpr size_t
        compile_time_size() {
            return compile_time_size_impl(
                boost::mp11::mp_bool<sequence_is_tuple>{});
        }

    private:
        FUTURES_NODISCARD static constexpr size_t
        compile_time_size_impl(std::true_type /* sequence_is_tuple */) {
            return std::tuple_size<sequence_type>::value;
        }

        FUTURES_NODISCARD static constexpr size_t
        compile_time_size_impl(std::false_type /* sequence_is_tuple */) {
            return 0;
        }

    public:
        /// Check if the i-th future is ready
        FUTURES_NODISCARD bool
        is_ready(size_t index) const {
            return is_ready_impl(
                boost::mp11::mp_bool<sequence_is_tuple>{},
                index);
        }

    private:
        FUTURES_NODISCARD bool
        is_ready_impl(std::true_type /* sequence_is_tuple */, size_t index)
            const {
            return apply(
                       [](auto &&el) -> future_status {
                           return el.wait_for(std::chrono::seconds(0));
                       },
                       v,
                       index)
                   == future_status::ready;
        }

        FUTURES_NODISCARD bool
        is_ready_impl(std::false_type /* sequence_is_tuple */, size_t index)
            const {
            return v[index].wait_for(std::chrono::seconds(0))
                   == future_status::ready;
        }

    private:
        /// Internal wait_any_future state
        sequence_type v;
    };


    /// Create a future object that becomes ready when any of the futures
    /// in the range is ready
    /**
     *  @param first,last Range of futures
     *  @return @ref when_any_future with all future objects. The sequence type
     *  is a range object holding the futures.
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class InputIt>
    requires(
        is_future_like_v<std::decay_t<std::iter_value_t<InputIt>>>
        || detail::is_invocable_v<std::iter_value_t<InputIt>>)
#else
    template <
        class InputIt,
        std::enable_if_t<
            detail::disjunction_v<
                is_future_like<
                    std::decay_t<iter_value_t<InputIt>>>,
                detail::is_invocable<
                    iter_value_t<InputIt>>>,
            int> = 0>
#endif
    when_any_future<
        FUTURES_DETAIL(detail::small_vector<detail::lambda_to_future_t<
                           typename std::iterator_traits<InputIt>::
                               value_type>>)> when_any(InputIt first, InputIt last);

    /// @copybrief when_any
    /**
     *  This function does not participate in overload resolution unless every
     *  argument is future-like.
     *
     *  @param r Range of futures
     *  @return @ref when_any_future with all future objects
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <std::ranges::range Range>
#else
    template <
        class Range,
        std::enable_if_t<is_range_v<std::decay_t<Range>>, int> = 0>
#endif
    when_any_future<
        FUTURES_DETAIL(detail::small_vector<detail::lambda_to_future_t<
                           typename std::iterator_traits<typename std::decay_t<
                               Range>::iterator>::value_type>>)>
    when_any(Range &&r) {
        return when_any(
            std::begin(std::forward<Range>(r)),
            std::end(std::forward<Range>(r)));
    }


    /// @copybrief when_any
    /**
     *  This function does not participate in overload resolution unless every
     *  argument is either a (possibly cv-qualified) std::shared_future or a
     *  cv-unqualified std::future.
     *
     *  @param futures A sequence of future objects
     *  @return @ref when_any_future with all future objects
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class... Futures>
    requires detail::conjunction_v<detail::disjunction<
        is_future_like<std::decay_t<Futures>>,
        detail::is_invocable<std::decay_t<Futures>>>...>
#else
    template <
        class... Futures,
        std::enable_if_t<
            detail::conjunction_v<detail::disjunction<
                is_future_like<std::decay_t<Futures>>,
                detail::is_invocable<std::decay_t<Futures>>>...>,
            int>
        = 0>
#endif
    when_any_future<
        std::tuple<FUTURES_DETAIL(detail::lambda_to_future_t<Futures>...)>>
    when_any(Futures &&...futures);

    /// @copybrief when_any
    /**
     *  ready operator|| works for futures and functions (which are converted to
     *  futures with the default executor) If the future is a when_any_future
     *  itself, then it gets merged instead of becoming a child future of
     *  another when_any_future.
     *
     *  When the user asks for `f1 || f2 || f3`, we want that to return a single
     *  future that waits for `<f1 || f2 || f3>` rather than a future that wait
     *  for two futures `<f1 || <f2 || f3>>`.
     *
     *  This emulates the usual behavior we expect from other types with
     *  `operator||`.
     *
     *  Note that this default behaviour is different from `when_any(...), which
     *  doesn't merge the when_any_future objects by default, because they are
     *  variadic functions and this intention can be controlled explicitly:
     *  - `when_any(f1,f2,f3)` -> `<f1 || f2 || f3>`
     *  - `when_any(f1,when_any(f2,f3))` -> `<f1 || <f2 || f3>>`
     *
     *  @param lhs,rhs Future objects
     *  @return A @ref when_any_future holding all future types
     */
#ifdef FUTURES_HAS_CONCEPTS
    template <class T1, class T2>
    requires detail::disjunction_v<
                 is_future_like<std::decay_t<T1>>,
                 detail::is_invocable<std::decay_t<T1>>>
             && detail::disjunction_v<
                 is_future_like<std::decay_t<T2>>,
                 detail::is_invocable<std::decay_t<T2>>>
#else
    template <
        class T1,
        class T2,
        std::enable_if_t<
            detail::disjunction_v<
                is_future_like<std::decay_t<T1>>,
                detail::is_invocable<std::decay_t<T1>>>
                && detail::disjunction_v<
                    is_future_like<std::decay_t<T2>>,
                    detail::is_invocable<std::decay_t<T2>>>,
            int>
        = 0>
#endif
    FUTURES_DETAIL(auto)
    operator||(T1 &&lhs, T2 &&rhs);

    /** @} */
} // namespace futures

#include <futures/adaptor/impl/when_any.hpp>

#endif // FUTURES_ADAPTOR_WHEN_ANY_HPP
