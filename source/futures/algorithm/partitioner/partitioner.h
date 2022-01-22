#ifndef FUTURES_PARTITIONER_H
#define FUTURES_PARTITIONER_H

#include <futures/algorithm/traits/is_input_iterator.h>
#include <futures/algorithm/traits/is_range.h>
#include <futures/adaptor/detail/traits/has_get.h>
#include <futures/algorithm/detail/traits/range/range/concepts.h>
#include <thread>
#include <algorithm>

/// \file Default partitioners
/// A partitioner is a light callable object that takes a pair of iterators and
/// returns the middle of the sequence. In particular, it returns an iterator
/// `middle` that forms a subrange `first`/`middle` which the algorithm should
/// solve inline before scheduling the subrange `middle`/`last` in the executor.

namespace futures {
    /** \addtogroup algorithms Algorithms
     *  @{
     */

    /** \addtogroup partitioners Partitioners
     *  @{
     */

    /// \brief The halve partitioner always splits the sequence into two parts
    /// of roughly equal size
    ///
    /// The sequence is split up to a minimum grain size.
    /// As a concept, the result from the partitioner is considered a suggestion
    /// for parallelization. For algorithms such as for_each, a partitioner with
    /// a very small grain size might be appropriate if the operation is very
    /// expensive. Some algorithms, such as a binary search, might naturally
    /// adjust this suggestion so that the result makes sense.
    class halve_partitioner
    {
        std::size_t min_grain_size_;

    public:
        /// \brief Halve partition constructor
        /// \param min_grain_size_ Minimum grain size used to split ranges
        inline explicit halve_partitioner(std::size_t min_grain_size_)
            : min_grain_size_(min_grain_size_) {}

        /// \brief Split a range of elements
        /// \tparam I Iterator type
        /// \tparam S Sentinel type
        /// \param first First element in range
        /// \param last Last element in range
        /// \return Iterator to point where sequence should be split
        template <typename I, typename S>
        auto
        operator()(I first, S last) {
            std::size_t size = std::distance(first, last);
            return (size <= min_grain_size_) ?
                       last :
                       std::next(first, (size + 1) / 2);
        }
    };

    /// \brief A partitioner that splits the ranges until it identifies we are
    /// not moving to new threads.
    ///
    /// This partitioner splits the ranges until it identifies we are not moving
    /// to new threads. Apart from that, it behaves as a halve_partitioner,
    /// splitting the range up to a minimum grain size.
    class thread_partitioner
    {
        std::size_t min_grain_size_;
        std::size_t num_threads_;
        std::thread::id last_thread_id_{};

    public:
        explicit thread_partitioner(std::size_t min_grain_size)
            : min_grain_size_(min_grain_size),
              num_threads_(std::max(
                  std::thread::hardware_concurrency(),
                  static_cast<unsigned int>(1))) {}

        template <typename I, typename S>
        auto
        operator()(I first, S last) {
            if (num_threads_ <= 1) {
                return last;
            }
            std::thread::id current_thread_id = std::this_thread::get_id();
            const bool threads_changed = current_thread_id != last_thread_id_;
            if (threads_changed) {
                last_thread_id_ = current_thread_id;
                num_threads_ += 1;
                num_threads_ /= 2;
                std::size_t size = std::distance(first, last);
                return (size <= min_grain_size_) ?
                           last :
                           std::next(first, (size + 1) / 2);
            }
            return last;
        }
    };

    /// \brief Default partitioner used by parallel algorithms
    ///
    /// Its type and parameters might change
    using default_partitioner = thread_partitioner;

    /// \brief Determine a reasonable minimum grain size depending on the number
    /// of elements in a sequence
    inline std::size_t
    make_grain_size(std::size_t n) {
        return std::clamp(
            n
                / (8
                   * std::
                       max(std::thread::hardware_concurrency(),
                           static_cast<unsigned int>(1))),
            size_t(1),
            size_t(2048));
    }

    /// \brief Create an instance of the default partitioner with a reasonable
    /// grain size for @ref n elements
    ///
    /// The default partitioner type and parameters might change
    inline default_partitioner
    make_default_partitioner(size_t n) {
        return default_partitioner(make_grain_size(n));
    }

    /// \brief Create an instance of the default partitioner with a reasonable
    /// grain for the range @ref first , @ref last
    ///
    /// The default partitioner type and parameters might change
    template <
        class I,
        class S,
        std::enable_if_t<
            is_input_iterator_v<I> && futures::detail::sentinel_for<S, I>,
            int> = 0>
    default_partitioner
    make_default_partitioner(I first, S last) {
        return make_default_partitioner(std::distance(first, last));
    }

    /// \brief Create an instance of the default partitioner with a reasonable
    /// grain for the range @ref r
    ///
    /// The default partitioner type and parameters might change
    template <
        class R,
        std::enable_if_t<futures::detail::input_range<R>, int> = 0>
    default_partitioner
    make_default_partitioner(R &&r) {
        return make_default_partitioner(std::begin(r), std::end(r));
    }

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S>
    using is_partitioner = std::conjunction<
        std::conditional_t<
            is_input_iterator_v<I>,
            std::true_type,
            std::false_type>,
        std::conditional_t<
            is_input_iterator_v<S>,
            std::true_type,
            std::false_type>,
        std::is_invocable<T, I, S>>;

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S>
    constexpr bool is_partitioner_v = is_partitioner<T, I, S>::value;

    /// Determine if P is a valid partitioner for the range R
    template <class T, class R, typename = void>
    struct is_range_partitioner : std::false_type
    {};
    template <class T, class R>
    struct is_range_partitioner<T, R, std::enable_if_t<is_range_v<R>>>
        : is_partitioner<
              T,
              futures::detail::range_common_iterator_t<R>,
              futures::detail::range_common_iterator_t<R>>
    {};

    template <class T, class R>
    constexpr bool is_range_partitioner_v = is_range_partitioner<T, R>::value;

    /** @}*/ // \addtogroup partitioners Partitioners
    /** @}*/ // \addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_PARTITIONER_H