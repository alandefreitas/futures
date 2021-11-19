//
// Created by Alan Freitas on 8/21/21.
//

#ifndef FUTURES_STOP_TOKEN_H
#define FUTURES_STOP_TOKEN_H

/// \file This header contains is a slightly adapted version of std::stop_token for futures rather than threads
/// \author This is slightly adapted from Baker Josuttis' reference implementation for C++20
/// Although the jfuture class is obviously different from std::jthread, this stop_token is not different
/// from std::stop_token. The main goal here is just to provide a stop sources in C++17 with the
/// reference implementation. In the future, we might replace this with C++20 std::stop_token.
/// \see https://github.com/josuttis/jthread

#include <atomic>
#include <thread>
#include <type_traits>
#include <utility>
#ifdef SAFE
#include <iostream>
#endif

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace futures {
    /** \addtogroup future Futures
     *  @{
     */
    /** \addtogroup cancellation Cancellation
     *  @{
     */


    namespace detail {
        inline void spin_yield() noexcept {
            // TODO: Platform-specific code here
#if defined(__x86_64__) || defined(_M_X64)
            _mm_pause();
#endif
        }
    } // namespace detail

    //-----------------------------------------------
    // internal types for shared stop state
    //-----------------------------------------------

    namespace detail {
        struct stop_callback_base {
            void (*callback_)(stop_callback_base *) = nullptr;

            // Next and prev callbacks
            stop_callback_base *next_ = nullptr;
            stop_callback_base **prev_ = nullptr;
            bool *is_removed_ = nullptr;
            std::atomic<bool> callback_finished_executing{false};

            void execute() noexcept { callback_(this); }

          protected:
            // it shall only by us who deletes this
            // (workaround for virtual execute() and destructor)
            ~stop_callback_base() = default;
        };

        struct stop_state {
          public:
            void add_token_reference() noexcept { state_.fetch_add(token_ref_increment, std::memory_order_relaxed); }

            void remove_token_reference() noexcept {
                auto old_state = state_.fetch_sub(token_ref_increment, std::memory_order_acq_rel);
                if (old_state < (token_ref_increment + source_ref_increment)) {
                    delete this;
                }
            }

            void add_source_reference() noexcept { state_.fetch_add(source_ref_increment, std::memory_order_relaxed); }

            void remove_source_reference() noexcept {
                auto old_state = state_.fetch_sub(source_ref_increment, std::memory_order_acq_rel);
                if (old_state < (token_ref_increment + source_ref_increment)) {
                    delete this;
                }
            }

            bool request_stop() noexcept {

                if (!try_lock_and_signal_until_signalled()) {
                    // Stop has already been requested.
                    return false;
                }

                // Set the 'stop_requested' signal and acquired the lock.

                signalling_thread_ = std::this_thread::get_id();

                while (head_ != nullptr) {
                    // Dequeue the head of the queue
                    auto *cb = head_;
                    head_ = cb->next_;
                    const bool any_more = head_ != nullptr;
                    if (any_more) {
                        head_->prev_ = &head_;
                    }
                    // Mark this item as removed from the list.
                    cb->prev_ = nullptr;

                    // Don't hold lock while executing callback
                    // so we don't block other threads from deregistering callbacks.
                    unlock();

                    // TRICKY: Need to store a flag on the stack here that the callback
                    // can use to signal that the destructor was executed inline
                    // during the call. If the destructor was executed inline then
                    // it's not safe to dereference cb after execute() returns.
                    // If the destructor runs on some other thread then the other
                    // thread will block waiting for this thread to signal that the
                    // callback has finished executing.
                    bool is_removed = false;
                    cb->is_removed_ = &is_removed;

                    cb->execute();

                    if (!is_removed) {
                        cb->is_removed_ = nullptr;
                        cb->callback_finished_executing.store(true, std::memory_order_release);
                    }

                    if (!any_more) {
                        // This was the last item in the queue when we dequeued it.
                        // No more items should be added to the queue after we have
                        // marked the state as interrupted, only removed from the queue.
                        // Avoid acquring/releasing the lock in this case.
                        return true;
                    }

                    lock();
                }

                unlock();

                return true;
            }

            bool is_stop_requested() noexcept { return is_stop_requested(state_.load(std::memory_order_acquire)); }

            bool is_stop_requestable() noexcept { return is_stop_requestable(state_.load(std::memory_order_acquire)); }

            bool try_add_callback(detail::stop_callback_base *cb, bool increment_ref_count_if_successful) noexcept {
                std::uint64_t old_state;
                goto load_state;
                do {
                    goto check_state;
                    do {
                        detail::spin_yield();
                    load_state:
                        old_state = state_.load(std::memory_order_acquire);
                    check_state:
                        if (is_stop_requested(old_state)) {
                            cb->execute();
                            return false;
                        } else if (!is_stop_requestable(old_state)) {
                            return false;
                        }
                    } while (is_locked(old_state));
                } while (!state_.compare_exchange_weak(old_state, old_state | locked_flag, std::memory_order_acquire));

                // Push callback onto callback list.
                cb->next_ = head_;
                if (cb->next_ != nullptr) {
                    cb->next_->prev_ = &cb->next_;
                }
                cb->prev_ = &head_;
                head_ = cb;

                if (increment_ref_count_if_successful) {
                    unlock_and_increment_token_ref_count();
                } else {
                    unlock();
                }

                // Successfully added the callback.
                return true;
            }

            void remove_callback(detail::stop_callback_base *cb) noexcept {
                lock();

                if (cb->prev_ != nullptr) {
                    // Still registered, not yet executed
                    // Just remove from the list.
                    *cb->prev_ = cb->next_;
                    if (cb->next_ != nullptr) {
                        cb->next_->prev_ = cb->prev_;
                    }

                    unlock_and_decrement_token_ref_count();

                    return;
                }

                unlock();

                // Callback has either already executed or is executing
                // concurrently on another thread.

                if (signalling_thread_ == std::this_thread::get_id()) {
                    // Callback executed on this thread or is still currently executing
                    // and is deregistering itself from within the callback.
                    if (cb->is_removed_ != nullptr) {
                        // Currently inside the callback, let the request_stop() method
                        // know the object is about to be destructed and that it should
                        // not try to access the object when the callback returns.
                        *cb->is_removed_ = true;
                    }
                } else {
                    // Callback is currently executing on another thread,
                    // block until it finishes executing.
                    while (!cb->callback_finished_executing.load(std::memory_order_acquire)) {
                        detail::spin_yield();
                    }
                }

                remove_token_reference();
            }

          private:
            static bool is_locked(std::uint64_t state) noexcept { return (state & locked_flag) != 0; }

            static bool is_stop_requested(std::uint64_t state) noexcept { return (state & stop_requested_flag) != 0; }

            static bool is_stop_requestable(std::uint64_t state) noexcept {
                // Interruptable if it has already been interrupted or if there are
                // still interrupt_source instances in existence.
                return is_stop_requested(state) || (state >= source_ref_increment);
            }

            bool try_lock_and_signal_until_signalled() noexcept {
                std::uint64_t old_state = state_.load(std::memory_order_acquire);
                do {
                    if (is_stop_requested(old_state))
                        return false;
                    while (is_locked(old_state)) {
                        detail::spin_yield();
                        old_state = state_.load(std::memory_order_acquire);
                        if (is_stop_requested(old_state))
                            return false;
                    }
                } while (!state_.compare_exchange_weak(old_state, old_state | stop_requested_flag | locked_flag,
                                                       std::memory_order_acq_rel, std::memory_order_acquire));
                return true;
            }

            void lock() noexcept {
                auto old_state = state_.load(std::memory_order_relaxed);
                do {
                    while (is_locked(old_state)) {
                        detail::spin_yield();
                        old_state = state_.load(std::memory_order_relaxed);
                    }
                } while (!state_.compare_exchange_weak(old_state, old_state | locked_flag, std::memory_order_acquire,
                                                       std::memory_order_relaxed));
            }

            void unlock() noexcept { state_.fetch_sub(locked_flag, std::memory_order_release); }

            void unlock_and_increment_token_ref_count() noexcept {
                state_.fetch_sub(locked_flag - token_ref_increment, std::memory_order_release);
            }

            void unlock_and_decrement_token_ref_count() noexcept {
                auto old_state = state_.fetch_sub(locked_flag + token_ref_increment, std::memory_order_acq_rel);
                // Check if new state is less than token_ref_increment which would
                // indicate that this was the last reference.
                if (old_state < (locked_flag + token_ref_increment + token_ref_increment)) {
                    delete this;
                }
            }

            static constexpr std::uint64_t stop_requested_flag = 1u;
            static constexpr std::uint64_t locked_flag = 2u;
            static constexpr std::uint64_t token_ref_increment = 4u;
            static constexpr std::uint64_t source_ref_increment = static_cast<std::uint64_t>(1u) << 33u;

            // bit 0 - stop-requested
            // bit 1 - locked
            // bits 2-32 - token ref count (31 bits)
            // bits 33-63 - source ref count (31 bits)
            std::atomic<std::uint64_t> state_{source_ref_increment};
            detail::stop_callback_base *head_ = nullptr;
            std::thread::id signalling_thread_{};
        };
    } // namespace detail

    //-----------------------------------------------
    // forward declarations
    //-----------------------------------------------

    class stop_source;
    template <typename Callback> class stop_callback;

    /// \brief Empty struct to initialize a @ref stop_source without shared stop state
    struct nostopstate_t {
        explicit nostopstate_t() = default;
    };

    /// \brief Empty struct to initialize a @ref stop_source without shared stop state
    inline constexpr nostopstate_t nostopstate{};

    /// \brief Token to check if a stop request has been made
    ///
    /// The stop_token class provides the means to check if a stop request has been made or can be made, for its
    /// associated std::stop_source object. It is essentially a thread-safe "view" of the associated stop-state.
    class stop_token {
      public:
        // construct:
        stop_token() noexcept : state_(nullptr) {}

        // copy/move/assign/destroy:
        stop_token(const stop_token &it) noexcept : state_(it.state_) {
            if (state_ != nullptr) {
                state_->add_token_reference();
            }
        }

        stop_token(stop_token &&it) noexcept : state_(std::exchange(it.state_, nullptr)) {}

        ~stop_token() {
            if (state_ != nullptr) {
                state_->remove_token_reference();
            }
        }

        stop_token &operator=(const stop_token &it) noexcept {
            if (state_ != it.state_) {
                stop_token tmp{it};
                swap(tmp);
            }
            return *this;
        }

        stop_token &operator=(stop_token &&it) noexcept {
            stop_token tmp{std::move(it)};
            swap(tmp);
            return *this;
        }

        void swap(stop_token &it) noexcept { std::swap(state_, it.state_); }

        // stop handling:
        [[nodiscard]] bool stop_requested() const noexcept { return state_ != nullptr && state_->is_stop_requested(); }

        [[nodiscard]] bool stop_possible() const noexcept { return state_ != nullptr && state_->is_stop_requestable(); }

        [[nodiscard]] friend bool operator==(const stop_token &a, const stop_token &b) noexcept {
            return a.state_ == b.state_;
        }
        [[nodiscard]] friend bool operator!=(const stop_token &a, const stop_token &b) noexcept {
            return a.state_ != b.state_;
        }

      private:
        friend class stop_source;
        template <typename Callback> friend class stop_callback;

        explicit stop_token(detail::stop_state *state) noexcept : state_(state) {
            if (state_ != nullptr) {
                state_->add_token_reference();
            }
        }

        detail::stop_state *state_;
    };

    /// \brief Object used to issue a stop request
    ///
    /// The stop_source class provides the means to issue a stop request, such as for std::jthread cancellation.
    /// A stop request made for one stop_source object is visible to all stop_sources and std::stop_tokens of
    /// the same associated stop-state; any std::stop_callback(s) registered for associated std::stop_token(s)
    /// will be invoked, and any std::condition_variable_any objects waiting on associated std::stop_token(s)
    /// will be awoken.
    class stop_source {
      public:
        stop_source() : state_(new detail::stop_state()) {}

        explicit stop_source(nostopstate_t) noexcept : state_(nullptr) {}

        ~stop_source() {
            if (state_ != nullptr) {
                state_->remove_source_reference();
            }
        }

        stop_source(const stop_source &other) noexcept : state_(other.state_) {
            if (state_ != nullptr) {
                state_->add_source_reference();
            }
        }

        stop_source(stop_source &&other) noexcept : state_(std::exchange(other.state_, nullptr)) {}

        stop_source &operator=(stop_source &&other) noexcept {
            stop_source tmp{std::move(other)};
            swap(tmp);
            return *this;
        }

        stop_source &operator=(const stop_source &other) noexcept {
            if (state_ != other.state_) {
                stop_source tmp{other};
                swap(tmp);
            }
            return *this;
        }

        [[nodiscard]] bool stop_requested() const noexcept { return state_ != nullptr && state_->is_stop_requested(); }

        [[nodiscard]] bool stop_possible() const noexcept { return state_ != nullptr; }

        bool request_stop() noexcept {
            if (state_ != nullptr) {
                return state_->request_stop();
            }
            return false;
        }

        [[nodiscard]] stop_token get_token() const noexcept { return stop_token{state_}; }

        void swap(stop_source &other) noexcept { std::swap(state_, other.state_); }

        [[nodiscard]] friend bool operator==(const stop_source &a, const stop_source &b) noexcept {
            return a.state_ == b.state_;
        }
        [[nodiscard]] friend bool operator!=(const stop_source &a, const stop_source &b) noexcept {
            return a.state_ != b.state_;
        }

      private:
        detail::stop_state *state_;
    };

    /// \brief A stop callback for a stop token
    ///
    /// The stop_callback class template provides an RAII object type that registers a callback function for
    /// an associated std::stop_token object, such that the callback function will be invoked when the
    /// std::stop_token's associated std::stop_source is requested to stop.
    template <typename Callback>
    // requires Destructible<Callback> && Invocable<Callback>
    class [[nodiscard]] stop_callback : private detail::stop_callback_base {
      public:
        using callback_type = Callback;

        template <typename CB, std::enable_if_t<std::is_constructible_v<Callback, CB>, int> = 0>
        // requires Constructible<Callback, C>
        explicit stop_callback(const stop_token &token, CB &&cb) noexcept(std::is_nothrow_constructible_v<Callback, CB>)
            : detail::stop_callback_base{[](detail::stop_callback_base *that) noexcept {
                  static_cast<stop_callback *>(that)->execute();
              }},
              state_(nullptr), cb_(static_cast<CB &&>(cb)) {
            if (token.state_ != nullptr && token.state_->try_add_callback(this, true)) {
                state_ = token.state_;
            }
        }

        template <typename CB, std::enable_if_t<std::is_constructible_v<Callback, CB>, int> = 0>
        // requires Constructible<Callback, C>
        explicit stop_callback(stop_token &&token, CB &&cb) noexcept(std::is_nothrow_constructible_v<Callback, CB>)
            : detail::stop_callback_base{[](detail::stop_callback_base *that) noexcept {
                  static_cast<stop_callback *>(that)->execute();
              }},
              state_(nullptr), cb_(static_cast<CB &&>(cb)) {
            if (token.state_ != nullptr && token.state_->try_add_callback(this, false)) {
                state_ = std::exchange(token.state_, nullptr);
            }
        }

        ~stop_callback() {
#ifdef SAFE
            if (in_execute_.load()) {
                std::cerr << "*** OOPS: ~stop_callback() while callback executed\n";
            }
#endif
            if (state_ != nullptr) {
                state_->remove_callback(this);
            }
        }

        stop_callback &operator=(const stop_callback &) = delete;
        stop_callback &operator=(stop_callback &&) = delete;
        stop_callback(const stop_callback &) = delete;
        stop_callback(stop_callback &&) = delete;

      private:
        void execute() noexcept {
            // Executed in a noexcept context
            // If it throws then we call std::terminate().
#ifdef SAFE
            in_execute_.store(true);
            cb_();
            in_execute_.store(false);
#else
            cb_();
#endif
        }

        detail::stop_state *state_;
        Callback cb_;
#ifdef SAFE
        std::atomic<bool> in_execute_{false};
#endif
    };

    /// Registers a callback function for an associated std::stop_token object
    template <typename Callback> stop_callback(stop_token, Callback) -> stop_callback<Callback>;

    /** @} */  // \addtogroup cancellation Cancellation
    /** @} */  // \addtogroup future Futures
} // namespace futures

#endif // FUTURES_STOP_TOKEN_H
