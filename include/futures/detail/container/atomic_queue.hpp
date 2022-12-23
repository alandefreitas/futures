//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_DETAIL_CONTAINER_ATOMIC_QUEUE_HPP
#define FUTURES_DETAIL_CONTAINER_ATOMIC_QUEUE_HPP

#include <futures/throw.hpp>
#include <futures/detail/deps/boost/core/allocator_access.hpp>
#include <futures/detail/deps/boost/core/empty_value.hpp>
#include <atomic>
#include <memory>
#include <stdexcept>

namespace futures {
    namespace detail {
        template <class T>
        struct lock_free_queue_node {
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
        /// @tparam T Data type
        /// @tparam Allocator Node allocator
        template <class T, class Allocator = std::allocator<T>>
        class atomic_queue
            : private boost::empty_value<
                  boost::allocator_rebind_t<Allocator, lock_free_queue_node<T>>,
                  0>
            , private boost::
                  empty_value<boost::allocator_rebind_t<Allocator, T>, 1> {
            using node = lock_free_queue_node<T>;
            using node_allocator_type = boost::
                allocator_rebind_t<Allocator, node>;
            using empty_node_allocator_type = boost::
                empty_value<node_allocator_type, 0>;

            using value_type = T;
            using value_type_allocator_type = boost::
                allocator_rebind_t<Allocator, value_type>;
            using empty_value_type_allocator_type = boost::
                empty_value<value_type_allocator_type, 1>;

            node_allocator_type&
            get_node_allocator() {
                return empty_node_allocator_type::get();
            }

            node_allocator_type const&
            get_node_allocator() const {
                return empty_node_allocator_type::get();
            }

            value_type_allocator_type&
            get_value_allocator() {
                return value_type_allocator_type::get();
            }

            value_type_allocator_type const&
            get_value_allocator() const {
                return value_type_allocator_type::get();
            }

        public:
            using allocator_type = boost::allocator_rebind_t<Allocator, T>;

            ~atomic_queue() {
                node* old_head = head_.load(std::memory_order_relaxed);
                while (old_head) {
                    head_.store(old_head->next, std::memory_order_relaxed);
                    boost::
                        allocator_destroy(this->get_node_allocator(), old_head);
                    this->get_node_allocator().deallocate(old_head, 1);
                    old_head = head_.load(std::memory_order_relaxed);
                }
            }

            explicit atomic_queue(Allocator const& alloc = std::allocator<T>{})
                : empty_node_allocator_type(boost::empty_init, alloc)
                , empty_value_type_allocator_type(boost::empty_init, alloc) {
                node* dummy_node_ptr = this->get_node_allocator().allocate(1);
                boost::allocator_construct(
                    this->get_node_allocator(),
                    dummy_node_ptr);
                head_.store(dummy_node_ptr);
                tail_.store(dummy_node_ptr);
            }

            atomic_queue(atomic_queue const&) = delete;

            atomic_queue&
            operator=(atomic_queue const&)
                = delete;

            FUTURES_NODISCARD bool
            empty() const {
                return head_.load(std::memory_order_acquire)
                       == tail_.load(std::memory_order_acquire);
            }

            void
            push(T const& data) {
                // Construct node we should push
                node* new_node_ptr = this->get_node_allocator().allocate(1);
                boost::allocator_construct(
                    this->get_node_allocator(),
                    new_node_ptr,
                    data);
                push(new_node_ptr);
            }

            void
            push(T&& data) {
                // Construct node we should push
                node* new_node_ptr = this->get_node_allocator().allocate(1);
                boost::allocator_construct(
                    this->get_node_allocator(),
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
                    node* old_head_reload = head_.load(
                        std::memory_order_acquire);
                    if (old_head == old_head_reload) {
                        if (old_head == old_tail) {
                            if (!old_head_next) {
                                // both are dummy -> empty queue
                                throw_exception(std::runtime_error{
                                    "Attempting to pop from an empty queue" });
                            }
                            // head == tail and not dummy -> race -> update tail
                            tail_.compare_exchange_strong(
                                old_tail,
                                old_head_next);
                        } else /* head != tail */ {
                            if (!old_head_next) {
                                // head next can't be empty unless dummy
                                // -> race -> try again
                                continue;
                            }
                            // Success: Store return value
                            auto ret = std::move(old_head_next->data);
                            // Update head_
                            if (head_.compare_exchange_weak(
                                    old_head,
                                    old_head_next))
                            {
                                boost::allocator_destroy(
                                    this->get_node_allocator(),
                                    old_head);
                                this->get_node_allocator()
                                    .deallocate(old_head, 1);
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
                                    new_node_ptr))
                            {
                                // update tail
                                tail_.compare_exchange_strong(
                                    old_tail,
                                    new_node_ptr);
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
    } // namespace detail
} // namespace futures

#endif // FUTURES_DETAIL_CONTAINER_ATOMIC_QUEUE_HPP
