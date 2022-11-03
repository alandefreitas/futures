//
// Copyright (c) 2021 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_IMPL_FUTURE_HPP
#define FUTURES_IMPL_FUTURE_HPP

namespace futures {
    template <class R, class Options>
    basic_future<R, Options>::~basic_future() {
        if constexpr (Options::is_stoppable && !Options::is_shared) {
            if (valid() && !is_ready()) {
                get_stop_source().request_stop();
            }
        }
        wait_if_last();
    }

    template <class R, class Options>
    auto
    basic_future<R, Options>::operator=(basic_future &&other) noexcept
        -> basic_future & {
        state_ = std::move(other.state_);
        join_ = std::exchange(other.join_, false);
        return *this;
    }

    template <class R, class Options>
    basic_future<R, detail::append_future_option_t<shared_opt, Options>>
    basic_future<R, Options>::share() {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }

        // Determine type of corresponding shared future
        using shared_options = detail::
            append_future_option_t<shared_opt, Options>;
        using shared_future_t = basic_future<R, shared_options>;

        // Create future state for the shared future
        if constexpr (Options::is_shared) {
            shared_future_t other{ state_ };
            other.join_ = join_;
            return other;
        } else {
            shared_future_t other{ std::move(state_) };
            other.join_ = std::exchange(join_, false);
            return other;
        }
    }

    template <class R, class Options>
    FUTURES_DETAIL(decltype(auto))
    basic_future<R, Options>::get() {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        state_.wait();
        if constexpr (Options::is_shared) {
            // state_.get() should handle the return type for us
            return state_.get();
        } else {
            future_state_type tmp(std::move(state_));
            if constexpr (std::is_reference_v<R> || std::is_void_v<R>) {
                return tmp.get();
            } else {
                return R(std::move(tmp.get()));
            }
        }
    }

    template <class R, class Options>
    std::exception_ptr
    basic_future<R, Options>::get_exception_ptr() {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        state_.wait();
        return state_.get_exception_ptr();
    }

    template <class R, class Options>
    void
    basic_future<R, Options>::wait() const {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        if constexpr (Options::is_always_deferred) {
            detail::throw_exception(future_deferred{});
        }
        state_.wait();
    }

    template <class R, class Options>
    void
    basic_future<R, Options>::wait() {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        state_.wait();
    }

    template <class R, class Options>
    template <class Rep, class Period>
    std::future_status
    basic_future<R, Options>::wait_for(
        std::chrono::duration<Rep, Period> const &timeout_duration) const {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        if constexpr (Options::is_always_deferred) {
            return std::future_status::deferred;
        }
        return state_.wait_for(timeout_duration);
    }

    template <class R, class Options>
    template <class Rep, class Period>
    std::future_status
    basic_future<R, Options>::wait_for(
        std::chrono::duration<Rep, Period> const &timeout_duration) {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        return state_.wait_for(timeout_duration);
    }

    template <class R, class Options>
    template <class Clock, class Duration>
    std::future_status
    basic_future<R, Options>::wait_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time) const {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        return state_.wait_until(timeout_time);
    }

    template <class R, class Options>
    template <class Clock, class Duration>
    std::future_status
    basic_future<R, Options>::wait_until(
        std::chrono::time_point<Clock, Duration> const &timeout_time) {
        if (!valid()) {
            detail::throw_exception(future_uninitialized{});
        }
        if constexpr (Options::is_always_deferred) {
            detail::throw_exception(future_deferred{});
        }
        return state_.wait_until(timeout_time);
    }

    template <class R, class Options>
    [[nodiscard]] bool
    basic_future<R, Options>::is_ready() const {
        if (!valid()) {
            detail::throw_exception(
                std::future_error{ std::future_errc::no_state });
        }
        return state_.is_ready();
    }

    template <class R, class Options>
    template <
        class Executor,
        class Fn FUTURES_ALSO_SELF_REQUIRE_IMPL(
            (Options::is_continuable || Options::is_always_deferred))>
    FUTURES_DETAIL(decltype(auto))
    basic_future<R, Options>::then(Executor const &ex, Fn &&fn) {
        // Throw if invalid
        if (!valid()) {
            detail::throw_exception(
                std::future_error{ std::future_errc::no_state });
        }

        if constexpr (Options::is_continuable) {
            // Determine traits for the next future
            using traits = detail::
                next_future_traits<Executor, Fn, basic_future>;
            using next_value_type = typename traits::next_value_type;
            using next_future_options = typename traits::next_future_options;
            using next_future_type
                = basic_future<next_value_type, next_future_options>;

            // Both futures are eager and continuable
            static_assert(!is_always_deferred_v<basic_future>);
            static_assert(!is_always_deferred_v<next_future_type>);
            static_assert(is_continuable_v<basic_future>);
            static_assert(is_continuable_v<next_future_type>);

            // Store a backup of the continuations source
            auto cont_source = get_continuations_source();

            // Create continuation function
            // note: this future is moved into this task
            // note: this future being shared allows this to be copy
            // constructible
            detail::future_continue_task<
                std::decay_t<basic_future>,
                std::decay_t<Fn>>
                task{ detail::move_if_not_shared(*this), std::forward<Fn>(fn) };

            // Create a shared operation state for next future
            // note: we use a shared state because the continuation is also
            // eager
            using operation_state_t = detail::
                operation_state<next_value_type, next_future_options>;
            auto state = std::make_shared<operation_state_t>(ex);
            next_future_type fut(state);

            // Create task to set next future state
            // note: this function might become non-copy-constructible
            // because it stores the continuation function.
            auto set_state_fn =
                [state = std::move(state), task = std::move(task)]() mutable {
                state->apply(std::move(task));
            };
            using set_state_fn_type = decltype(set_state_fn);

            // Attach set_state_fn to this continuation list
            if constexpr (std::is_copy_constructible_v<set_state_fn_type>) {
                cont_source.push(ex, std::move(set_state_fn));
            } else {
                // Make the continuation task copyable if we have to
                // note: the continuation source uses `std::function` to
                // represent continuations.
                // note: This could be improved with an implementation of
                // `std::move_only_function` to be used by the continuation
                // source.
                auto fn_shared_ptr = std::make_shared<set_state_fn_type>(
                    std::move(set_state_fn));
                auto copyable_handle = [fn_shared_ptr]() {
                    (*fn_shared_ptr)();
                };
                cont_source.push(ex, copyable_handle);
            }
            return fut;
        } else if constexpr (Options::is_always_deferred) {
            // Determine traits for the next future
            using traits = detail::
                next_future_traits<Executor, Fn, basic_future>;
            using next_value_type = typename traits::next_value_type;
            using next_future_options = typename traits::next_future_options;
            using next_future_type
                = basic_future<next_value_type, next_future_options>;

            // Both future types are deferred
            static_assert(is_always_deferred_v<basic_future>);
            static_assert(is_always_deferred_v<next_future_type>);

            // Create continuation function
            // note: this future is moved into this task
            // note: this future is not always shared, in which case
            // the operation state is still inline in another address,
            // which is OK because the value hasn't been requested
            detail::future_continue_task<
                std::decay_t<basic_future>,
                std::decay_t<Fn>>
                task{ detail::move_if_not_shared(*this), std::forward<Fn>(fn) };

            // Create the operation state for the next future
            // note: this state is inline because the continuation
            // is also deferred
            // note: This operation contains the task
            using deferred_operation_state_t = detail::
                deferred_operation_state<next_value_type, next_future_options>;
            deferred_operation_state_t state(ex, std::move(task));

            // Move operation state into the new future
            // note: this is the new future representing the deferred
            // task graph now. It has inline access to the parent
            // operation.
            next_future_type fut(std::move(state));
            return fut;
        }
    }

    template <class R, class Options>
    template <class Fn FUTURES_ALSO_SELF_REQUIRE_IMPL(
        (Options::is_continuable || Options::is_always_deferred))>
    FUTURES_DETAIL(decltype(auto))
    basic_future<R, Options>::then(Fn &&fn) {
        if constexpr (Options::has_executor) {
            return this->then(get_executor(), std::forward<Fn>(fn));
        } else {
            return this->then(make_default_executor(), std::forward<Fn>(fn));
        }
    }

    template <class R, class Options>
    void
    basic_future<R, Options>::wait_if_last() {
        if constexpr (Options::is_shared) {
            if (join_ && valid() && !is_ready() && state_.use_count() == 1) {
                wait();
            }
        } else {
            if (join_ && valid() && !is_ready()) {
                wait();
            }
        }
    }
} // namespace futures

#endif // FUTURES_IMPL_FUTURE_HPP
