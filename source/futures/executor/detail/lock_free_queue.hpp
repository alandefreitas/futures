//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_LOCK_FREE_QUEUE_HPP
#define FUTURES_LOCK_FREE_QUEUE_HPP

#include <futures/executor/detail/allocator_construct.hpp>
#include <futures/executor/detail/allocator_destroy.hpp>
#include <futures/executor/detail/allocator_rebind.hpp>
#include <futures/futures/detail/empty_base.h>
#include <futures/futures/detail/throw_exception.h>
#include <atomic>
#include <memory>

namespace futures::detail {
    template <class T>
    struct lock_free_queue_node
    {
        std::unique_ptr<T> data{ nullptr };
        std::atomic<lock_free_queue_node*> next{ nullptr };
        explicit lock_free_queue_node(T const& data_)
            : data(std::make_unique<T>(data_)) {}
        explicit lock_free_queue_node(T&& data_)
            : data(std::make_unique<T>(std::move(data_))) {}
        lock_free_queue_node() = default;
    };

    /// A very simple lock-free queue
    ///
    /// The queue is implemented in as a linked list.
    ///
    /// The linked list stores node with pointers to elements. Both
    /// the nodes and the elements need to be stored in a vector
    /// to make this cache-friendly.
    ///
    /// \tparam T Data type
    /// \tparam Allocator Node allocator
    template <class T, class Allocator = std::allocator<T>>
    class lock_free_queue
        : private maybe_empty<
              allocator_rebind_t<Allocator, lock_free_queue_node<T>>>
        , private maybe_empty<allocator_rebind_t<Allocator, T>>
    {
        using node = lock_free_queue_node<T>;
        using node_allocator_type = allocator_rebind_t<Allocator, node>;

        allocator_rebind_t<Allocator, T>&
        get_element_allocator() {
            return maybe_empty<allocator_rebind_t<Allocator, T>>::get();
        }

        allocator_rebind_t<Allocator, node>&
        get_node_allocator() {
            return maybe_empty<allocator_rebind_t<Allocator, node>>::get();
        }

    public:
        using allocator_type = allocator_rebind_t<Allocator, T>;

        ~lock_free_queue() {
            node* old_head = head_.load(std::memory_order_relaxed);
            while (old_head) {
                head_.store(old_head->next, std::memory_order_relaxed);
                allocator_destroy(get_node_allocator(), old_head);
                get_node_allocator().deallocate(old_head, 1);
                old_head = head_.load(std::memory_order_relaxed);
            }
        }

        explicit lock_free_queue(const Allocator& alloc = std::allocator<T>{})
            : maybe_empty<allocator_rebind_t<Allocator, node>>(alloc),
              maybe_empty<allocator_rebind_t<Allocator, T>>(alloc) {
            node* dummy_node_ptr = get_node_allocator().allocate(1);
            allocator_construct(get_node_allocator(), dummy_node_ptr);
            head_.store(dummy_node_ptr);
            tail_.store(dummy_node_ptr);
        }

        lock_free_queue(const lock_free_queue&) = delete;

        lock_free_queue&
        operator=(const lock_free_queue&)
            = delete;

        [[nodiscard]] bool
        empty() const {
            return head_.load(std::memory_order_acquire)
                   == tail_.load(std::memory_order_acquire);
        }

        void
        push(const T& data) {
            // Construct node we should push
            node* new_node_ptr = get_node_allocator().allocate(1);
            allocator_construct(get_node_allocator(), new_node_ptr, data);
            push(new_node_ptr);
        }

        void
        push(T&& data) {
            // Construct node we should push
            node* new_node_ptr = get_node_allocator().allocate(1);
            allocator_construct(
                get_node_allocator(),
                new_node_ptr,
                std::move(data));
            push(new_node_ptr);
        }

        T
        pop() {
            while (true) {
                node* old_head = head_.load(std::memory_order_acquire);
                node* old_tail = tail_.load(std::memory_order_acquire);
                node* old_head_next = old_head->next.load(
                    std::memory_order_acquire);
                node* old_head_reload = head_.load(std::memory_order_acquire);
                if (old_head == old_head_reload) {
                    if (old_head == old_tail) {
                        if (!old_head_next) {
                            // both are dummy -> empty queue
                            throw_exception<std::runtime_error>(
                                "Attempting to pop from an empty queue");
                        }
                        // head == tail and not dummy -> race -> update tail
                        tail_.compare_exchange_strong(old_tail, old_head_next);
                    } else /* head != tail */ {
                        if (!old_head_next) {
                            // head next can't be empty unless dummy
                            // -> race -> try again
                            continue;
                        }
                        // Success: Store return value
                        auto ret = std::move(old_head_next->data);
                        // Update head_
                        if (head_.compare_exchange_weak(old_head, old_head_next))
                        {
                            allocator_destroy(get_node_allocator(), old_head);
                            get_node_allocator().deallocate(old_head, 1);
                            return std::move(*ret);
                        }
                    }
                }
            }
        }

    private:
        void
        push(node* new_node_ptr) {
            // Construct node we should push
            std::atomic<node*> new_node{ new_node_ptr };

            while (true) {
                // Check tail
                node* old_tail = tail_.load(std::memory_order_acquire);
                node* old_tail_next = old_tail->next.load(
                    std::memory_order_acquire);

                // Try to change the tail
                node* tail_reload = tail_.load(std::memory_order_acquire);
                if (old_tail == tail_reload) {
                    if (!old_tail_next) {
                        // tail is last element -> update tail next
                        if (old_tail->next.compare_exchange_weak(
                                old_tail_next,
                                new_node_ptr)) {
                            // update tail
                            tail_
                                .compare_exchange_strong(old_tail, new_node_ptr);
                            return;
                        }
                    } else {
                        // update tail
                        node* new_tail = old_tail_next;
                        tail_.compare_exchange_strong(old_tail, new_tail);
                    }
                }
            }
        }


        std::atomic<node*> head_;
        std::atomic<node*> tail_;
    };
} // namespace futures::detail

#endif // FUTURES_LOCK_FREE_QUEUE_HPP
