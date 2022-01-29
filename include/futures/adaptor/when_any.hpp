//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_WHEN_ANY_H
#define FUTURES_WHEN_ANY_H

/// \file Implement the when_any functionality for futures and executors
/// The same rationale as when_all applies here
/// \see https://en.cppreference.com/w/cpp/experimental/when_any

#include <futures/adaptor/when_any_result.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/futures/async.hpp>
#include <futures/futures/traits/to_future.hpp>
#include <futures/adaptor/detail/traits/is_tuple.hpp>
#include <futures/adaptor/detail/tuple_algorithm.hpp>
#include <futures/futures/detail/small_vector.hpp>
#include <array>
#include <optional>
#include <condition_variable>
#include <shared_mutex>

namespace futures {
    /** \addtogroup adaptors Adaptors
     *  @{
     */

    /// \brief Proxy future class referring to the result of a disjunction of
    /// futures from @ref when_any
    ///
    /// This class implements another future type to identify when one of the
    /// tasks is over.
    ///
    /// As with `when_all`, this class acts as a future that checks the results
    /// of other futures to avoid creating a real disjunction of futures that
    /// would need to be polling on another thread.
    ///
    /// If the user does want to poll on another thread, then this can be
    /// converted into a regular future type with async or std::async.
    ///
    /// Not-polling is easier to emulate for future conjunctions (when_all)
    /// because we can sleep on each task until they are all done, since we need
    /// all of them anyway.
    ///
    /// For disjunctions, we have few options:
    /// - If the input futures have lazy continuations:
    ///   - Attach continuations to notify when a task is over
    /// - If the input futures do not have lazy continuations:
    ///   - Polling in a busy loop until one of the futures is ready
    ///   - Polling with exponential backoffs until one of the futures is ready
    ///   - Launching n continuation tasks that set a promise when one of the
    ///   futures is ready
    ///   - Hybrids, usually polling for short tasks and launching threads for
    ///   other tasks
    /// - If the input futures are mixed in regards to lazy continuations:
    ///   - Mix the strategies above, depending on each input future
    ///
    /// If the thresholds for these strategies are reasonable, this should be
    /// efficient for futures with or without lazy continuations.
    ///
    template <class Sequence>
    class when_any_future
    {
    private:
        using sequence_type = Sequence;
        static constexpr bool sequence_is_range = is_range_v<sequence_type>;
        static constexpr bool sequence_is_tuple = is_tuple_v<sequence_type>;
        static_assert(sequence_is_range || sequence_is_tuple);

    public:
        /// \brief Default constructor.
        /// Constructs a when_any_future with no shared state. After
        /// construction, valid() == false
        when_any_future() noexcept = default;

        /// \brief Move a sequence of futures into the when_any_future
        /// constructor. The sequence is moved into this future object and the
        /// objects from which the sequence was created get invalidated.
        ///
        /// We immediately set up the notifiers for any input future that
        /// supports lazy continuations.
        explicit when_any_future(sequence_type &&v) noexcept
            : v(std::move(v)), thread_notifiers_set(false),
              ready_notified(false) {
            maybe_set_up_lazy_notifiers();
        }

        /// \brief Move constructor.
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
            : thread_notifiers_set(false), ready_notified(false) {
            other.request_notifiers_stop();
            // we can only move v after stopping the notifiers, or they will
            // keep trying to access invalid future address before they stop
            v = std::move(other.v);
            // Set up our own lazy notifiers
            maybe_set_up_lazy_notifiers();
        }

        /// \brief when_any_future is not CopyConstructible
        when_any_future(const when_any_future &other) = delete;

        /// \brief Releases any shared state.
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
        ~when_any_future() {
            request_notifiers_stop_and_wait();
        };

        /// \brief Assigns the contents of another future object.
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
            other.request_notifiers_stop();
            // Set up our own lazy notifiers
            maybe_set_up_lazy_notifiers();
        }

        /// \brief Copy assigns the contents of another when_any_future object.
        ///
        /// when_any_future is not copy assignable.
        when_any_future &
        operator=(const when_any_future &other)
            = delete;

        /// \brief Wait until any future has a valid result and retrieves it
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
                detail::throw_exception<std::future_error>(std::future_errc::no_state);
            }
            // Wait for the complete sequence to be ready
            wait();
            // Set up a `when_any_result` and move results to it.
            when_any_result<sequence_type> r;
            r.index = get_ready_index();
            request_notifiers_stop_and_wait();
            r.tasks = std::move(v);
            return r;
        }

        /// \brief Checks if the future refers to a shared state
        ///
        /// This future is always valid() unless there are tasks and they are
        /// all invalid
        ///
        /// \see https://en.cppreference.com/w/cpp/experimental/when_any
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
                    return tuple_any_of(v, [](auto &&f) { return f.valid(); });
                }
            }
        }

        /// \brief Blocks until the result becomes available.
        /// valid() == true after the call.
        /// The behavior is undefined if valid() == false before the call to
        /// this function
        void
        wait() const {
            // Check if the sequence is valid
            if (!valid()) {
                detail::throw_exception<std::future_error>(std::future_errc::no_state);
            }
            // Reuse the logic from wait_for here
            using const_version = std::true_type;
            using timeout_version = std::false_type;
            wait_for_common<const_version, timeout_version>(
                *this,
                std::chrono::seconds(0));
        }

        /// \overload mutable version which allows setting up notifiers which
        /// might not have been set yet
        void
        wait() {
            // Check if the sequence is valid
            if (!valid()) {
                detail::throw_exception<std::future_error>(std::future_errc::no_state);
            }
            // Reuse the logic from wait_for here
            using const_version = std::false_type;
            using timeout_version = std::false_type;
            wait_for_common<const_version, timeout_version>(
                *this,
                std::chrono::seconds(0));
        }

        /// \brief Waits for the result to become available.
        /// Blocks until specified timeout_duration has elapsed or the result
        /// becomes available, whichever comes first. Not-polling is easier to
        /// emulate for future conjunctions (when_all) because we can sleep on
        /// each task until they are all done, since we need all of them anyway.
        /// For disjunctions, we have few options:
        /// - Polling in a busy loop until one of the futures is ready
        /// - Polling in a less busy loop with exponential backoffs
        /// - Launching n continuation tasks that set a promise when one of the
        /// futures is ready
        /// - Hybrids, usually polling for short tasks and launching threads for
        /// other tasks If these parameters are reasonable, this should not be
        /// less efficient that futures with continuations, because we save on
        /// the creation of new tasks. However, the relationship between the
        /// parameters depend on:
        /// - The number of tasks (n)
        /// - The estimated time of completion for each task (assumes a time
        /// distribution)
        /// - The probably a given task is the first task to finish (>=1/n)
        /// - The cost of launching continuation tasks
        /// Because we don't have access to information about the estimated time
        /// for a given task to finish, we can ignore a less-busy loop as a
        /// general solution. Thus, we can come up with a hybrid algorithm for
        /// all cases:
        /// - If there's only one task, behave as when_all
        /// - If there are more tasks:
        ///   1) Initially poll in a busy loop for a while, because tasks might
        ///   finish very sooner than we would need
        ///      to create a continuations.
        ///   2) Create continuation tasks after a threshold time
        /// \see https://en.m.wikipedia.org/wiki/Exponential_backoff
        template <class Rep, class Period>
        std::future_status
        wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration) const {
            using const_version = std::true_type;
            using timeout_version = std::true_type;
            return wait_for_common<const_version, timeout_version>(
                *this,
                timeout_duration);
        }

        /// \overload wait for might need to be mutable so we can set up the
        /// notifiers \note the const version will only wait for notifiers if
        /// these have already been set
        template <class Rep, class Period>
        std::future_status
        wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
            using const_version = std::false_type;
            using timeout_version = std::true_type;
            return wait_for_common<const_version, timeout_version>(
                *this,
                timeout_duration);
        }

        /// \brief wait_until waits for a result to become available.
        /// It blocks until specified timeout_time has been reached or the
        /// result becomes available, whichever comes first
        template <class Clock, class Duration>
        std::future_status
        wait_until(const std::chrono::time_point<Clock, Duration> &timeout_time)
            const {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ?
                       wait_for(std::chrono::seconds(0)) :
                       wait_for(
                           timeout_time - std::chrono::system_clock::now());
        }

        /// \overload mutable version that allows setting notifiers
        template <class Clock, class Duration>
        std::future_status
        wait_until(
            const std::chrono::time_point<Clock, Duration> &timeout_time) {
            auto now_time = std::chrono::system_clock::now();
            return now_time > timeout_time ?
                       wait_for(std::chrono::seconds(0)) :
                       wait_for(
                           timeout_time - std::chrono::system_clock::now());
        }

        /// \brief Check if it's ready
        [[nodiscard]] bool
        is_ready() const {
            auto idx = get_ready_index();
            return idx != static_cast<size_t>(-1) || (size() == 0);
        }

        /// \brief Allow move the underlying sequence somewhere else
        /// The when_any_future is left empty and should now be considered
        /// invalid. This is useful for the algorithm that merges two
        /// wait_any_future objects without forcing encapsulation of the merge
        /// function.
        sequence_type &&
        release() {
            request_notifiers_stop();
            return std::move(v);
        }

    private:
        /// \brief Get index of the first internal future that is ready
        /// If no future is ready, this returns the sequence size as a sentinel
        template <class CheckLazyContinuables = std::true_type>
        [[nodiscard]] size_t
        get_ready_index() const {
            const auto eq_comp = [](auto &&f) {
                if constexpr (
                    CheckLazyContinuables::value
                    || (!is_lazy_continuable_v<std::decay_t<decltype(f)>>) )
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
                ready_index = tuple_find_if(v, eq_comp);
            }
            if (ready_index == size()) {
                return static_cast<size_t>(-1);
            } else {
                return ready_index;
            }
        }

        /// \brief To common paths to wait for a future
        /// \tparam const_version std::true_type or std::false_type for version
        /// with or without setting up new notifiers \tparam timeout_version
        /// std::true_type or std::false_type for version with or without
        /// timeout \tparam Rep Common std::chrono Rep time representation
        /// \tparam Period Common std::chrono Period time representation \param
        /// f when_any_future on which we want to wait \param timeout_duration
        /// Max time we should wait for a result (if timeout version is
        /// std::true_type) \return Status of the future
        /// (std::future_status::ready if any is ready)
        template <
            class const_version,
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        static std::future_status
        wait_for_common(
            std::conditional_t<
                const_version::value,
                std::add_const<when_any_future>,
                when_any_future> &f,
            const std::chrono::duration<Rep, Period> &timeout_duration) {
            constexpr bool is_trivial_tuple = sequence_is_tuple
                                              && (compile_time_size() < 2);
            if constexpr (is_trivial_tuple) {
                if constexpr (0 == compile_time_size()) {
                    // Trivial tuple: empty -> ready()
                    return std::future_status::ready;
                } else /* if constexpr (1 == compile_time_size()) */ {
                    // Trivial tuple: one element -> get()
                    if constexpr (timeout_version::value) {
                        return std::get<0>(f.v).wait_for(timeout_duration);
                    } else {
                        std::get<0>(f.v).wait();
                        return std::future_status::ready;
                    }
                }
            } else {
                if constexpr (sequence_is_range) {
                    if (f.v.empty()) {
                        // Trivial range: empty -> ready()
                        return std::future_status::ready;
                    } else if (f.v.size() == 1) {
                        // Trivial range: one element -> get()
                        if constexpr (timeout_version::value) {
                            return f.v.begin()->wait_for(timeout_duration);
                        } else {
                            f.v.begin()->wait();
                            return std::future_status::ready;
                        }
                    }
                }

                // General case: we already know it's ready
                if (f.is_ready()) {
                    return std::future_status::ready;
                }

                // All future types have their own notifiers as continuations
                // created when this object starts. Don't busy wait if thread
                // notifiers are already set anyway.
                if (f.all_lazy_continuable() || f.thread_notifiers_set) {
                    return f.template notifier_wait_for<timeout_version>(
                        timeout_duration);
                }

                // Choose busy or future wait for, depending on how much time we
                // have to wait and whether the notifiers have been set in a
                // previous function call
                if constexpr (const_version::value) {
                    // We cannot set up notifiers in the busy version of this
                    // function even though this is encapsulated. Maybe the
                    // notifiers should be always mutable, like we usually do
                    // with mutexes, but better safe than sorry. This has a
                    // small impact on continuable futures through.
                    return f.template busy_wait_for<timeout_version>(
                        timeout_duration);
                } else /* if not const_version::value */ {
                    // - Don't busy wait forever, even though it implements an
                    // exponential backoff
                    // - Don't create notifiers for very short tasks either
                    const std::chrono::seconds max_busy_time(5);
                    const bool no_time_to_setup_notifiers = timeout_duration
                                                            < max_busy_time;
                    // - Don't create notifiers if there are more tasks than the
                    // hardware limit already:
                    //   - If there are more tasks, the probability of a ready
                    //   task increases while the cost
                    //     of notifiers is higher.
                    const bool too_many_threads_already
                        = f.size() >= hardware_concurrency();
                    const bool busy_wait_only = no_time_to_setup_notifiers
                                                || too_many_threads_already;
                    if (busy_wait_only) {
                        return f.template busy_wait_for<timeout_version>(
                            timeout_duration);
                    } else {
                        std::future_status s = f.template busy_wait_for<
                            timeout_version>(max_busy_time);
                        if (s != std::future_status::ready) {
                            f.maybe_set_up_thread_notifiers();
                            return f
                                .template notifier_wait_for<timeout_version>(
                                    timeout_duration - max_busy_time);
                        } else {
                            return s;
                        }
                    }
                }
            }
        }

        /// \brief Get number of internal futures
        [[nodiscard]] constexpr size_t
        size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return v.size();
            }
        }

        /// \brief Get number of internal futures with lazy continuations
        [[nodiscard]] constexpr size_t
        lazy_continuable_size() const {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
                size_t count = 0;
                tuple_for_each(v, [&count](auto &&el) {
                    if constexpr (is_lazy_continuable_v<
                                      std::decay_t<decltype(el)>>) {
                        ++count;
                    }
                });
                return count;
            } else {
                if (is_lazy_continuable_v<
                        std::decay_t<typename sequence_type::value_type>>) {
                    return v.size();
                } else {
                    return 0;
                }
            }
        }

        /// \brief Check if all internal types are lazy continuable
        [[nodiscard]] constexpr bool
        all_lazy_continuable() const {
            return lazy_continuable_size() == size();
        }

        /// \brief Get size, if we know that at compile time
        [[nodiscard]] static constexpr size_t
        compile_time_size() {
            if constexpr (sequence_is_tuple) {
                return std::tuple_size_v<sequence_type>;
            } else {
                return 0;
            }
        }

        /// \brief Check if the i-th future is ready
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

        /// \brief Busy wait for a certain amount of time
        /// \see https://en.m.wikipedia.org/wiki/Exponential_backoff
        template <
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        std::future_status
        busy_wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration
            = std::chrono::seconds(0)) const {
            // Check if the sequence is valid
            if (!valid()) {
                detail::throw_exception<std::future_error>(std::future_errc::no_state);
            }
            // Wait for on each thread, increasingly accounting for the time we
            // waited from the total
            using duration_type = std::chrono::duration<Rep, Period>;
            using namespace std::chrono;
            // 1) Total time we've waited so far
            auto start_time = system_clock::now();
            duration_type total_elapsed = duration_cast<duration_type>(
                nanoseconds(0));
            // 2) The time we are currently waiting on each thread (something
            // like a minimum slot time)
            nanoseconds each_wait_for(1);
            // 3) After how long we should start increasing the each_wait_for
            // Wait longer in the busy pool if we have more tasks to account for
            // the lower individual end in less-busy
            const auto full_busy_timeout_duration = milliseconds(100) * size();
            // 4) The increase factor of each_wait_for (20%)
            using each_wait_for_increase_factor = std::ratio<5, 4>;
            // 5) The max time we should wait on each thread (at most the
            // estimated time to create a thread) Assumes 1000 threads can be
            // created per second and split that with the number of tasks so
            // that finding the task would never take longer than creating a
            // thread
            auto max_wait_for = duration_cast<nanoseconds>(microseconds(20))
                                / size();
            // 6) Index of the future we found to be ready
            auto ready_in_the_meanwhile_idx = size_t(-1);
            // 7) Function to identify if a given task is ready
            // We will later keep looping on this function until we run out of
            // time
            auto equal_fn = [&](auto &&f) {
                // a) Don't wait on the ones with lazy continuations
                // We are going to wait on all of them at once later
                if constexpr (is_lazy_continuable_v<std::decay_t<decltype(f)>>)
                {
                    return false;
                }
                // b) update parameters of our heuristic _before_ checking the
                // future
                const bool use_backoffs = total_elapsed
                                          > full_busy_timeout_duration;
                if (use_backoffs) {
                    if (each_wait_for > max_wait_for) {
                        each_wait_for = max_wait_for;
                    } else if (each_wait_for < max_wait_for) {
                        each_wait_for *= each_wait_for_increase_factor::num;
                        each_wait_for /= each_wait_for_increase_factor::den;
                        each_wait_for += nanoseconds(1);
                    }
                }
                // c) effectively check if this future is ready
                std::future_status s = f.wait_for(each_wait_for);
                // d) update time spent on futures
                auto total_elapsed = system_clock::now() - start_time;
                // e) if this one wasn't ready, and we are already using
                // exponential backoff
                if (s != std::future_status::ready && use_backoffs) {
                    // do a single pass in other futures to make sure no other
                    // future got ready in the meanwhile
                    size_t ready_in_the_meanwhile_idx = get_ready_index();
                    if (ready_in_the_meanwhile_idx != size_t(-1)) {
                        return true;
                    }
                }
                // f) request function to break the loop if we found something
                // that's ready or we timed out
                return s == std::future_status::ready
                       || (timeout_version::value
                           && total_elapsed > timeout_duration);
            };
            // 8) Loop through the futures trying to find a ready future with
            // the function above
            do {
                // Check for a signal from lazy continuable futures all at once
                // The notifiers for lazy continuable futures are always set up
                if (lazy_continuable_size() != 0 && notifiers_started()) {
                    std::future_status s = wait_for_ready_notification<
                        timeout_version>(each_wait_for);
                    if (s == std::future_status::ready) {
                        return s;
                    }
                }
                // Check if other futures aren't ready
                // Use a hack to "break" "for_each" algorithms with find_if
                auto idx = size_t(-1);
                if constexpr (sequence_is_range) {
                    idx = std::find_if(v.begin(), v.end(), equal_fn)
                          - v.begin();
                } else {
                    idx = tuple_find_if(v, equal_fn);
                }
                const bool found_ready_future = idx != size();
                if (found_ready_future) {
                    size_t ready_found_idx
                        = ready_in_the_meanwhile_idx != size_t(-1) ?
                              ready_in_the_meanwhile_idx :
                              idx;
                    if constexpr (sequence_is_range) {
                        return (v.begin() + ready_found_idx)
                            ->wait_for(seconds(0));
                    } else {
                        return apply(
                            [](auto &&el) -> std::future_status {
                                return el.wait_for(seconds(0));
                            },
                            v,
                            ready_found_idx);
                    }
                }
            }
            while (!timeout_version::value || total_elapsed < timeout_duration);
            return std::future_status::timeout;
        }

        /// \brief Check if the notifiers have started wait on futures
        [[nodiscard]] bool
        notifiers_started() const {
            if constexpr (sequence_is_range) {
                return std::any_of(
                    notifiers.begin(),
                    notifiers.end(),
                    [](const notifier &n) { return n.start_token.load(); });
            } else {
                return tuple_any_of(v, [](auto &&f) { return f.valid(); });
            }
        };

        /// \brief Launch continuation threads and wait for any of them instead
        ///
        /// This is the second alternative to busy waiting, but once we used
        /// this alternative, we already have paid the price to create these
        /// continuation futures and we shouldn't busy wait ever again, even in
        /// a new call to wait_for. Thus, the wait_any_future needs to keep
        /// track of these continuation tasks to ensure this doesn't happen.
        ///
        /// Once the notifiers are set, if using an unlimited number of threads,
        /// we would only need to wait without a more specific timeout here.
        ///
        /// However, if the notifiers could not be launched for some reason,
        /// this can lock our process in the somewhat rare condition that (i)
        /// the last function is running in the last available pool thread, and
        /// (ii) none of the notifiers got a chance to get into the pool. We are
        /// then waiting for notifiers that don't exist yet and we don't have
        /// access to that information.
        ///
        /// To avoid that from happening, we (i) perform a single busy pass in
        /// the underlying futures from time to time to ensure they are really
        /// still running, regardless of the notifiers, and (ii) create a start
        /// token, besides the cancel token, for the notifiers to indicate that
        /// they have really started waiting. Thus, we can always check
        /// condition (i) to ensure we don't already have the results before we
        /// start waiting for them, and (ii) to ensure we don't wait for
        /// notifiers that are not running yet.
        template <
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        std::future_status
        notifier_wait_for(
            const std::chrono::duration<Rep, Period> &timeout_duration
            = std::chrono::seconds(0)) {
            // Check if that have started yet and do some busy waiting while
            // they haven't
            if (!notifiers_started()) {
                std::chrono::microseconds current_busy_wait(20);
                std::chrono::seconds max_busy_wait(1);
                do {
                    std::future_status s;
                    if (timeout_duration < current_busy_wait) {
                        return busy_wait_for<timeout_version>(timeout_duration);
                    } else {
                        s = busy_wait_for<timeout_version>(current_busy_wait);
                        if (s == std::future_status::ready) {
                            return s;
                        } else {
                            current_busy_wait *= 3;
                            current_busy_wait /= 2;
                            if (current_busy_wait > max_busy_wait) {
                                current_busy_wait = max_busy_wait;
                            }
                        }
                    }
                }
                while (!notifiers_started());
            }

            // wait for ready_notified to be set to true by a notifier task
            return wait_for_ready_notification<timeout_version>(
                timeout_duration);
        }

        /// \brief Sleep and wait for ready_notified to be set to true by a
        /// notifier task
        template <
            class timeout_version,
            class Rep = std::chrono::seconds::rep,
            class Period = std::chrono::seconds::period>
        std::future_status
        wait_for_ready_notification(
            const std::chrono::duration<Rep, Period> &timeout_duration
            = std::chrono::seconds(0)) const {
            // Create lock to be able to read/wait_for "ready_notified" with the
            // condition variable
            std::unique_lock<std::mutex> lock(ready_notified_mutex);

            // Check if it isn't true yet. Already notified.
            if (ready_notified) {
                return std::future_status::ready;
            }

            // Wait to be notified by another thread
            if constexpr (timeout_version::value) {
                ready_notified_cv.wait_for(lock, timeout_duration, [this]() {
                    return ready_notified;
                });
            } else {
                while (!ready_notified_cv
                            .wait_for(lock, std::chrono::seconds(1), [this]() {
                                return ready_notified;
                            }))
                {
                    if (is_ready()) {
                        return std::future_status::ready;
                    }
                }
            }

            // convert what we got into a future status
            return ready_notified ? std::future_status::ready :
                                    std::future_status::timeout;
        }

        /// \brief Stop the notifiers
        /// We might need to stop the notifiers if the when_any_future is being
        /// destroyed, or they might try to set a condition variable that no
        /// longer exists.
        ///
        /// This is something we also have to check when merging two
        /// when_any_future objects, because this creates a new future with a
        /// single notifier that needs to replace the old notifiers.
        ///
        /// In practice, all of that should happen very rarely, but things get
        /// ugly when it happens.
        ///
        void
        request_notifiers_stop_and_wait() {
            // Check if we have notifiers
            if (!thread_notifiers_set && !lazy_notifiers_set) {
                return;
            }

            // Set each cancel token to true (separately to cancel as soon as
            // possible)
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) {
                n.cancel_token.store(true);
            });

            // Wait for each future<void> notifier
            // - We have to wait for them even if they haven't started, because
            // we don't have that kind of control
            //   over the executor queue. They need to start running, see the
            //   cancel token and just give up.
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) {
                if (n.task.valid()) {
                    n.task.wait();
                }
            });

            thread_notifiers_set = false;
        }

        /// \brief Request stop but don't wait
        /// This is useful when moving the object, because we know we won't need
        /// the notifiers anymore but we don't want to waste time waiting for
        /// them yet.
        void
        request_notifiers_stop() {
            // Check if we have notifiers
            if (!thread_notifiers_set && !lazy_notifiers_set) {
                return;
            }

            // Set each cancel token to true (separately to cancel as soon as
            // possible)
            std::for_each(notifiers.begin(), notifiers.end(), [](auto &&n) {
                n.cancel_token.store(true);
            });
        }

        void
        maybe_set_up_lazy_notifiers() {
            maybe_set_up_notifiers_common<std::true_type>();
        }

        void
        maybe_set_up_thread_notifiers() {
            maybe_set_up_notifiers_common<std::false_type>();
        }

        /// \brief Common functionality to setup notifiers
        ///
        /// The logic for setting notifiers for futures with and without lazy
        /// continuations is almost the same.
        ///
        /// The task is the same but the difference is:
        /// 1) the notification task is a continuation if the future supports
        /// continuations, and 2) the notification task goes into a new new
        /// thread if the future does not support continuations.
        ///
        /// @note Unfortunately, we need a new thread an not only a new task in
        /// some executor whenever the task doesn't support continuations
        /// because we cannot be sure there's room in the executor for the
        /// notification task.
        ///
        /// This might be counter intuitive, as one could assume there's going
        /// to be room for the notifications as soon as the ongoing tasks are
        /// running. However, there are a few situations where this might
        /// happen:
        ///
        /// 1) The current tasks we are waiting for have not been launched yet
        /// and the executor is busy with
        ///    tasks that need cancellation to stop
        /// 2) Some of the tasks we are waiting for are running and some are
        /// enqueued. The running tasks finish
        ///    but we don't hear about it because the enqueued tasks come before
        ///    the notification.
        /// 3) All tasks we are waiting for have no support for continuations.
        /// The executor has no room for the
        ///    notifier because of some parallel tasks happening in the executor
        ///    and we never hear about a future getting ready.
        ///
        /// So although this is an edge case, we cannot assume there's room for
        /// the notifications in the executor.
        ///
        template <class SettingLazyContinuables>
        void
        maybe_set_up_notifiers_common() {
            constexpr bool setting_notifiers_as_continuations
                = SettingLazyContinuables::value;
            constexpr bool setting_notifiers_as_new_threads
                = !SettingLazyContinuables::value;

            // Never do that more than once. Also check
            if constexpr (setting_notifiers_as_new_threads) {
                if (thread_notifiers_set) {
                    return;
                }
                thread_notifiers_set = true;
            } else {
                if (lazy_notifiers_set) {
                    return;
                }
                lazy_notifiers_set = true;
            }

            // Initialize the variable the notifiers need to set
            // Any of the notifiers will set the same variable
            const bool init_ready = (setting_notifiers_as_continuations
                                     && (!thread_notifiers_set))
                                    || (setting_notifiers_as_new_threads
                                        && (!lazy_notifiers_set));
            if (init_ready) {
                ready_notified = false;
            }

            // Check if there are threads to set up
            const bool no_compatible_futures
                = (setting_notifiers_as_new_threads && all_lazy_continuable())
                  || (setting_notifiers_as_continuations
                      && lazy_continuable_size() == 0);
            if (no_compatible_futures) {
                return;
            }

            // Function that posts a notifier task to update our common variable
            // that indicates if the task is ready
            auto launch_notifier_task =
                [&](auto &&future,
                    std::atomic_bool &cancel_token,
                    std::atomic_bool &start_token) {
                // Launch a task with access to the underlying when_any_future
                // and a cancel token that asks it to stop These threads need to
                // be independent of any executor because the whole system might
                // crash if the executor has no room for them. So this is either
                // a real continuation or a new thread. Direct access to the
                // when_any_future avoid another level of indirection, but makes
                // things a little harder to get right.

                // Create promise the notifier needs to fulfill
                // All we need to know is whether it's over
                promise<void> p;
                auto std_future = p.get_future<futures::future<void>>();
                auto notifier_task =
                    [p = std::move(p),
                     &future,
                     &cancel_token,
                     &start_token,
                     this]() mutable {
                    // The very first thing we need to do is set the start
                    // token, so we never wait for notifiers that aren't running
                    start_token.store(true);

                    // Check if we haven't started too late
                    if (cancel_token.load()) {
                        p.set_value();
                        return;
                    }

                    // If future is ready at the start, just ensure
                    // wait_any_future knows about it already before setting up
                    // more state data for this task:
                    // - `is_ready` shouldn't fail in this case because the
                    // future is ready, but we haven't
                    //   called `get()` yet, so its state is valid or the whole
                    //   when_any_future would be invalid already.
                    // - We also have to ensure it's valid in case we've moved
                    // the when_any_future, and we
                    //   requested this to stop before we even got to this
                    //   point. In this case, this task is accessing the correct
                    //   location, but we invalidated the future when moving it
                    //   we did so correctly, to avoid blocking unnecessarily
                    //   when moving.
                    if (!future.valid() || ::futures::is_ready(future)) {
                        std::lock_guard lk(ready_notified_mutex);
                        if (!ready_notified) {
                            ready_notified = true;
                            ready_notified_cv.notify_one();
                        }
                        p.set_value();
                        return;
                    }

                    // Set the max waiting time for each wait_for operation
                    // This task might need to be cancelled, so we cannot wait
                    // on the future forever. So we `wait_for(max_waiting_time)`
                    // rather than `wait()` forever. There are two reasons for
                    // this:
                    // - The main when_any object is being destroyed when we
                    // found nothing yet.
                    // - Other tasks might have found the ready value, so this
                    // task can stop running. A number of heuristics can be used
                    // to adjust this time, but both conditions are supposed to
                    // be relatively rare.
                    const std::chrono::seconds max_waiting_time(1);

                    // Waits for the future to be ready, sleeping most of the
                    // time
                    while (future.wait_for(max_waiting_time)
                           != std::future_status::ready) {
                        // But once in a while, check if:
                        // - the main when_any_future has requested this
                        // operation to stop
                        //   because it's being destructed (cheaper condition)
                        if (cancel_token.load()) {
                            p.set_value();
                            return;
                        }
                        // - any other tasks haven't set the ready condition
                        // yet, so we can terminate a continuation task we no
                        // longer need
                        std::lock_guard lk(ready_notified_mutex);
                        if (ready_notified) {
                            p.set_value();
                            return;
                        }
                    }

                    // We found out about a future that's ready: notify the
                    // when_any_future object
                    std::lock_guard lk(ready_notified_mutex);
                    if (!ready_notified) {
                        ready_notified = true;
                        // Notify any thread that might be waiting for this event
                        ready_notified_cv.notify_one();
                    }
                    p.set_value();
                };

                // Create a copiable handle for the notifier task
                auto notifier_task_ptr = std::make_shared<
                    decltype(notifier_task)>(std::move(notifier_task));
                auto executor_handle = [notifier_task_ptr]() {
                    (*notifier_task_ptr)();
                };

                // Post the task appropriately
                using future_type = std::decay_t<decltype(future)>;
                // MSVC hack
                constexpr bool internal_lazy_continuable
                    = is_lazy_continuable_v<future_type>;
                constexpr bool internal_setting_lazy = SettingLazyContinuables::
                    value;
                constexpr bool internal_setting_thread
                    = !SettingLazyContinuables::value;
                if constexpr (internal_setting_lazy && internal_lazy_continuable)
                {
                    // Execute notifier task inline whenever `future` is done
                    future.then(make_inline_executor(), executor_handle);
                } else if constexpr (
                    internal_setting_thread && !internal_lazy_continuable) {
                    // Execute notifier task in a new thread because we don't
                    // have the executor context to be sure. We detach it here
                    // but can still control the cancel_token and the future.
                    // This is basically the same as calling std::async and
                    // ignoring its std::future because we already have one set
                    // up.
                    std::thread(executor_handle).detach();
                }
                // Return a future we can use to wait for the notifier and
                // ensure it's done
                return std_future;
            };

            // Launch the notification task for each future
            if constexpr (sequence_is_range) {
                if constexpr (
                    is_lazy_continuable_v<
                        typename sequence_type::
                            value_type> && setting_notifiers_as_new_threads)
                {
                    return;
                } else if constexpr (
                    !is_lazy_continuable_v<
                        typename sequence_type::
                            value_type> && setting_notifiers_as_continuations)
                {
                    return;
                } else {
                    // Ensure we have one notifier allocated for each task
                    notifiers.resize(size());
                    // For each future in v
                    for (size_t i = 0; i < size(); ++i) {
                        notifiers[i].cancel_token.store(false);
                        notifiers[i].start_token.store(false);
                        // Launch task with reference to this future and its
                        // tokens
                        notifiers[i].task = std::move(launch_notifier_task(
                            v[i],
                            notifiers[i].cancel_token,
                            notifiers[i].start_token));
                    }
                }
            } else {
                for_each_paired(
                    v,
                    notifiers,
                    [&](auto &this_future, notifier &n) {
                    using future_type = std::decay_t<decltype(this_future)>;
                    constexpr bool current_is_lazy_continuable_v
                        = is_lazy_continuable_v<future_type>;
                    constexpr bool internal_setting_thread
                        = !SettingLazyContinuables::value;
                    constexpr bool internal_setting_lazy
                        = SettingLazyContinuables::value;
                    if constexpr (
                        current_is_lazy_continuable_v
                        && internal_setting_thread) {
                        return;
                    } else if constexpr (
                        (!current_is_lazy_continuable_v)
                        && internal_setting_lazy) {
                        return;
                    } else {
                        n.cancel_token.store(false);
                        n.start_token.store(false);
                        future<void> tmp = launch_notifier_task(
                            this_future,
                            n.cancel_token,
                            n.start_token);
                        n.task = std::move(tmp);
                    }
                    });
            }
        }

    private:
        /// \name Helpers to infer the type for the notifiers
        ///
        /// The array of futures comes with an array of tokens that also allows
        /// us to cancel the notifiers We shouldn't need these tokens because we
        /// do expect the notifiers to deliver their promise before object
        /// destruction and we don't usually expect to merge when_any_futures
        /// for which we have started notifiers. This is still a requirement to
        /// make sure the notifier model works. We could probably use stop
        /// tokens instead of atomic_bool in C++20
        /// @{

        /// \brief Type that defines an internal when_any notifier task
        ///
        /// A notifier task notifies the when_any_future of any internal future
        /// that is ready.
        ///
        /// We use this notifier type instead of a std::pair because futures
        /// need to be moved and the atomic bools do not, but std::pair
        /// conservatively deletes the move constructor because of atomic_bool.
        struct notifier
        {
            /// A simple task that notifies us whenever the task is ready
            future<void> task{ make_ready_future() };

            /// Cancel the notification task
            std::atomic_bool cancel_token{ false };

            /// Notifies this task the notification task has started
            std::atomic_bool start_token{ false };

            /// Construct a ready notifier
            notifier() = default;

            /// Construct a notifier from an existing future
            notifier(future<void> &&f, bool c, bool s)
                : task(std::move(f)), cancel_token(c), start_token(s) {}

            /// Move a notifier
            notifier(notifier &&rhs) noexcept
                : task(std::move(rhs.task)),
                  cancel_token(rhs.cancel_token.load()),
                  start_token(rhs.start_token.load()) {}
        };

#ifdef FUTURES_USE_SMALL_VECTOR
        using notifier_vector = ::futures::small_vector<notifier>;
#else
        // Whenever small::vector in unavailable we use std::vector because
        // boost small_vector couldn't handle move-only notifiers
        using notifier_vector = ::std::vector<notifier>;
#endif
        using notifier_array = std::array<notifier, compile_time_size()>;

        using notifier_sequence_type = std::
            conditional_t<sequence_is_range, notifier_vector, notifier_array>;
        /// @}

    private:
        /// \brief Internal wait_any_future state
        sequence_type v;

        /// \name Variables for the notifiers to indicate if the future is ready
        /// They indicate if any underlying future has been identified as ready
        /// by an auxiliary thread or as a lazy continuation to an existing
        /// future type.
        /// @{
        notifier_sequence_type notifiers;
        bool thread_notifiers_set{ false };
        bool lazy_notifiers_set{ false };
        bool ready_notified{ false };
        mutable std::mutex ready_notified_mutex;
        mutable std::condition_variable ready_notified_cv;
        /// @}
    };

#ifndef FUTURES_DOXYGEN
    /// \name Define when_any_future as a kind of future
    /// @{
    /// Specialization explicitly setting when_any_future<T> as a type of future
    template <typename T>
    struct is_future<when_any_future<T>> : std::true_type
    {};

    /// Specialization explicitly setting when_any_future<T> & as a type of
    /// future
    template <typename T>
    struct is_future<when_any_future<T> &> : std::true_type
    {};

    /// Specialization explicitly setting when_any_future<T> && as a type of
    /// future
    template <typename T>
    struct is_future<when_any_future<T> &&> : std::true_type
    {};

    /// Specialization explicitly setting const when_any_future<T> as a type of
    /// future
    template <typename T>
    struct is_future<const when_any_future<T>> : std::true_type
    {};

    /// Specialization explicitly setting const when_any_future<T> & as a type
    /// of future
    template <typename T>
    struct is_future<const when_any_future<T> &> : std::true_type
    {};
    /// @}
#endif

    namespace detail {
        /// \name Useful traits for when_any
        /// @{

        /// \brief Check if type is a when_any_future as a type
        template <typename>
        struct is_when_any_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_any_future<when_any_future<Sequence>> : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<const when_any_future<Sequence>>
            : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<when_any_future<Sequence> &> : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<when_any_future<Sequence> &&> : std::true_type
        {};
        template <typename Sequence>
        struct is_when_any_future<const when_any_future<Sequence> &>
            : std::true_type
        {};

        /// \brief Check if type is a when_any_future as constant bool
        template <class T>
        constexpr bool is_when_any_future_v = is_when_any_future<T>::value;

        /// \brief Check if a type can be used in a future disjunction (when_any
        /// or operator|| for futures)
        template <class T>
        using is_valid_when_any_argument = std::
            disjunction<is_future<T>, std::is_invocable<T>>;
        template <class T>
        constexpr bool is_valid_when_any_argument_v
            = is_valid_when_any_argument<T>::value;

        /// \brief Trait to identify valid when_any inputs
        template <class...>
        struct are_valid_when_any_arguments : std::true_type
        {};
        template <class B1>
        struct are_valid_when_any_arguments<B1> : is_valid_when_any_argument<B1>
        {};
        template <class B1, class... Bn>
        struct are_valid_when_any_arguments<B1, Bn...>
            : std::conditional_t<
                  is_valid_when_any_argument_v<B1>,
                  are_valid_when_any_arguments<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_valid_when_any_arguments_v
            = are_valid_when_any_arguments<Args...>::value;

        /// \subsection Helpers for operator|| on futures, functions and
        /// when_any futures

        /// \brief Check if type is a when_any_future with tuples as a sequence
        /// type
        template <typename T, class Enable = void>
        struct is_when_any_tuple_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_any_tuple_future<
            when_any_future<Sequence>,
            std::enable_if_t<is_tuple_v<Sequence>>> : std::true_type
        {};
        template <class T>
        constexpr bool is_when_any_tuple_future_v = is_when_any_tuple_future<
            T>::value;

        /// \brief Check if all template parameters are when_any_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_any_tuple_futures : std::true_type
        {};
        template <class B1>
        struct are_when_any_tuple_futures<B1>
            : is_when_any_tuple_future<std::decay_t<B1>>
        {};
        template <class B1, class... Bn>
        struct are_when_any_tuple_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_tuple_future_v<std::decay_t<B1>>,
                  are_when_any_tuple_futures<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_when_any_tuple_futures_v
            = are_when_any_tuple_futures<Args...>::value;

        /// \brief Check if type is a when_any_future with a range as a sequence
        /// type
        template <typename T, class Enable = void>
        struct is_when_any_range_future : std::false_type
        {};
        template <typename Sequence>
        struct is_when_any_range_future<
            when_any_future<Sequence>,
            std::enable_if_t<is_range_v<Sequence>>> : std::true_type
        {};
        template <class T>
        constexpr bool is_when_any_range_future_v = is_when_any_range_future<
            T>::value;

        /// \brief Check if all template parameters are when_any_future with
        /// tuples as a sequence type
        template <class...>
        struct are_when_any_range_futures : std::true_type
        {};
        template <class B1>
        struct are_when_any_range_futures<B1> : is_when_any_range_future<B1>
        {};
        template <class B1, class... Bn>
        struct are_when_any_range_futures<B1, Bn...>
            : std::conditional_t<
                  is_when_any_range_future_v<B1>,
                  are_when_any_range_futures<Bn...>,
                  std::false_type>
        {};
        template <class... Args>
        constexpr bool are_when_any_range_futures_v
            = are_when_any_range_futures<Args...>::value;

        /// \brief Constructs a when_any_future that is a concatenation of all
        /// when_any_futures in args It's important to be able to merge
        /// when_any_future objects because of operator|| When the user asks for
        /// f1 && f2 && f3, we want that to return a single future that waits
        /// for <f1,f2,f3> rather than a future that wait for two futures
        /// <f1,<f2,f3>> \note This function only participates in overload
        /// resolution if all types in std::decay_t<WhenAllFutures>... are
        /// specializations of when_any_future with a tuple sequence type
        /// \overload "Merging" a single when_any_future of tuples. Overload
        /// provided for symmetry.
        template <
            class WhenAllFuture,
            std::enable_if_t<is_when_any_tuple_future_v<WhenAllFuture>, int> = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture &&arg0) {
            return std::forward<WhenAllFuture>(arg0);
        }

        /// \overload Merging a two when_any_future objects of tuples
        template <
            class WhenAllFuture1,
            class WhenAllFuture2,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFuture2>,
                int> = 0>
        decltype(auto)
        when_any_future_cat(WhenAllFuture1 &&arg0, WhenAllFuture2 &&arg1) {
            auto s1 = std::move(std::forward<WhenAllFuture1>(arg0).release());
            auto s2 = std::move(std::forward<WhenAllFuture2>(arg1).release());
            auto s = std::tuple_cat(std::move(s1), std::move(s2));
            return when_any_future(std::move(s));
        }

        /// \overload Merging two+ when_any_future of tuples
        template <
            class WhenAllFuture1,
            class... WhenAllFutures,
            std::enable_if_t<
                are_when_any_tuple_futures_v<WhenAllFuture1, WhenAllFutures...>,
                int> = 0>
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

    /// \brief Create a future object that becomes ready when any of the futures
    /// in the range is ready
    ///
    /// This function does not participate in overload resolution unless
    /// InputIt's value type (i.e., typename
    /// std::iterator_traits<InputIt>::value_type) @ref is_future .
    ///
    /// This overload uses a small vector to avoid further allocations for such
    /// a simple operation.
    ///
    /// \return @ref when_any_future with all future objects
    template <
        class InputIt
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::is_valid_when_any_argument_v<
                typename std::iterator_traits<InputIt>::value_type>,
            int> = 0
#endif
        >
    when_any_future<detail::small_vector<
        to_future_t<typename std::iterator_traits<InputIt>::value_type>>>
    when_any(InputIt first, InputIt last) {
        // Infer types
        using input_type = std::decay_t<
            typename std::iterator_traits<InputIt>::value_type>;
        constexpr bool input_is_future = is_future_v<input_type>;
        constexpr bool input_is_invocable = std::is_invocable_v<input_type>;
        static_assert(input_is_future || input_is_invocable);
        using output_future_type = to_future_t<input_type>;
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

    /// \brief Create a future object that becomes ready when any of the futures
    /// in the range is ready
    ///
    /// This function does not participate in overload resolution unless every
    /// argument is either a (possibly cv-qualified) std::shared_future or a
    /// cv-unqualified std::future.
    ///
    /// \return @ref when_any_future with all future objects
    template <
        class Range
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_range_v<std::decay_t<Range>>, int> = 0
#endif
        >
    when_any_future<
        detail::small_vector<to_future_t<typename std::iterator_traits<
            typename std::decay_t<Range>::iterator>::value_type>>>
    when_any(Range &&r) {
        return when_any(
            std::begin(std::forward<Range>(r)),
            std::end(std::forward<Range>(r)));
    }

    /// \brief Create a future object that becomes ready when any of the input
    /// futures is ready
    ///
    /// This function does not participate in overload resolution unless every
    /// argument is either a (possibly cv-qualified) std::shared_future or a
    /// cv-unqualified std::future.
    ///
    /// \return @ref when_any_future with all future objects
    template <
        class... Futures
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<
            detail::are_valid_when_any_arguments_v<Futures...>,
            int> = 0
#endif
        >
    when_any_future<std::tuple<to_future_t<Futures>...>>
    when_any(Futures &&...futures) {
        // Infer sequence type
        using sequence_type = std::tuple<to_future_t<Futures>...>;

        // When making the tuple for when_any_future:
        // - futures need to be moved
        // - shared futures need to be copied
        // - lambdas need to be posted
        [[maybe_unused]] constexpr auto move_share_or_post = [](auto &&f) {
            if constexpr (is_shared_future_v<decltype(f)>) {
                return std::forward<decltype(f)>(f);
            } else if constexpr (is_future_v<decltype(f)>) {
                return std::move(std::forward<decltype(f)>(f));
            } else /* if constexpr (std::is_invocable_v<decltype(f)>) */ {
                return ::futures::async(std::forward<decltype(f)>(f));
            }
        };

        // Create sequence (and infer types as we go)
        sequence_type v = std::make_tuple((move_share_or_post(futures))...);

        return when_any_future<sequence_type>(std::move(v));
    }

    /// \brief Operator to create a future object that becomes ready when any of
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
            detail::is_valid_when_any_argument_v<
                T1> && detail::is_valid_when_any_argument_v<T2>,
            int> = 0
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
                    std::is_invocable_v<
                        decltype(f)> && (!is_future_v<decltype(f)>) ) {
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
#endif // FUTURES_WHEN_ANY_H
