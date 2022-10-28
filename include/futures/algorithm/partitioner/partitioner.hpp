#ifndef FUTURES_ALGORITHM_PARTITIONER_PARTITIONER_HPP
#define FUTURES_ALGORITHM_PARTITIONER_PARTITIONER_HPP

#include <futures/algorithm/traits/is_input_iterator.hpp>
#include <futures/algorithm/traits/is_input_range.hpp>
#include <futures/algorithm/traits/is_range.hpp>
#include <futures/algorithm/traits/is_sentinel_for.hpp>
#include <futures/executor/default_executor.hpp>
#include <futures/executor/hardware_concurrency.hpp>
#include <futures/detail/traits/future_value.hpp>
#include <algorithm>
#include <thread>

/**
 *  @file algorithm/partitioner/partitioner.hpp
 *  @brief Default partitioners
 *
 *  A partitioner is a light callable object that takes a pair of iterators and
 *  returns the middle of the sequence. In particular, it returns an iterator
 *  `middle` that forms a subrange `first`/`middle` which the algorithm should
 *  solve inline before scheduling the subrange `middle`/`last` in the executor.
 */

namespace futures {
    /** @addtogroup algorithms Algorithms
     *  @{
     */

    /** @addtogroup partitioners Partitioners
     *  @{
     */

    /// A partitioner that always splits the problem in half
    /**
     *  The halve partitioner always splits the sequence into two parts
     *  of roughly equal size
     *
     *  The sequence is split up to a minimum grain size.
     *  As a concept, the result from the partitioner is considered a suggestion
     *  for parallelization. For algorithms such as for_each, a partitioner with
     *  a very small grain size might be appropriate if the operation is very
     *  expensive. Some algorithms, such as a binary search, might naturally
     *  adjust this suggestion so that the result makes sense.
     */
    class halve_partitioner {
        std::size_t min_grain_size_;

    public:
        /// Constructor
        /**
         * The constructor has a minimum grain size after which the range
         * should not be split.
         *
         * @param min_grain_size_ Minimum grain size used to split ranges
         */
        constexpr explicit halve_partitioner(std::size_t min_grain_size_)
            : min_grain_size_(min_grain_size_) {}

        /// Split a range of elements
        /**
         *  @tparam I Iterator type
         *  @tparam S Sentinel type
         *  @param first First element in range
         *  @param last Last element in range
         *  @return Iterator to point where sequence should be split
         */
        template <
            class I,
            class S
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>,
                int>
            = 0
#endif
            >
        I
        operator()(I first, S last) {
            std::size_t size = std::distance(first, last);
            return (size <= min_grain_size_) ?
                       last :
                       std::next(first, (size + 1) / 2);
        }
    };

    /// A partitioner that always splits the problem when moving to new threads
    /**
     *  A partitioner that splits the ranges until it identifies we are
     *  not moving to new threads.
     *
     *  This partitioner splits the ranges until it identifies we are not moving
     *  to new threads. Apart from that, it behaves as a halve_partitioner,
     *  splitting the range up to a minimum grain size.
     */
    class thread_partitioner {
        std::size_t min_grain_size_;
        std::size_t num_threads_{ hardware_concurrency() };
        std::thread::id last_thread_id_{};

    public:
        explicit thread_partitioner(std::size_t min_grain_size)
            : min_grain_size_(min_grain_size) {}

        template <
            class I,
            class S
#ifndef FUTURES_DOXYGEN
            ,
            std::enable_if_t<
                is_input_iterator_v<I> && is_sentinel_for_v<S, I>,
                int>
            = 0
#endif
            >
        I
        operator()(I first, S last) {
            if (num_threads_ <= 1) {
                return last;
            }
            std::thread::id current_thread_id = std::this_thread::get_id();
            bool const threads_changed = current_thread_id != last_thread_id_;
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

    /// Default partitioner used by parallel algorithms
    /**
     *  Its type and parameters might change
     */
    using default_partitioner =
#ifdef FUTURES_DOXYGEN
        __see_below__;
#else
        thread_partitioner;
#endif

    /// Determine a reasonable minimum grain size depending on the number
    /// of elements in a sequence
    /**
     * The grain size considers the number of threads available.
     * It's never more than 2048 elements.
     *
     * @param n Sequence size
     * @return The recommended grain size for a range of the specified size
     */
    FUTURES_CONSTANT_EVALUATED_CONSTEXPR std::size_t
    make_grain_size(std::size_t n) {
        std::size_t const nthreads = futures::hardware_concurrency();
        std::size_t const safe_nthreads = std::
            max(nthreads, static_cast<std::size_t>(1));
        std::size_t const expected_nthreads = 8 * safe_nthreads;
        std::size_t const grain_per_thread = n / expected_nthreads;
        return std::clamp(grain_per_thread, size_t(1), size_t(2048));
    }

    /// Create an instance of the default partitioner with a reasonable
    /// grain size for `n` elements
    /**
     *  The default partitioner type and parameters might change
     */
    inline default_partitioner
    make_default_partitioner(size_t n) {
        return default_partitioner(make_grain_size(n));
    }

    /// Create an instance of the default partitioner with a reasonable
    /// grain for the range `first`, `last`
    /**
     *  The default partitioner type and parameters might change
     */
    template <
        class I,
        class S
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_input_iterator_v<I> && is_sentinel_for_v<S, I>, int>
        = 0
#endif
        >
    default_partitioner
    make_default_partitioner(I first, S last) {
        return make_default_partitioner(std::distance(first, last));
    }

    /// Create an instance of the default partitioner with a reasonable
    /// grain for the range `r`
    /**
     *  The default partitioner type and parameters might change
     */
    template <
        class R
#ifndef FUTURES_DOXYGEN
        ,
        std::enable_if_t<is_input_range_v<R>, int> = 0
#endif
        >
    default_partitioner
    make_default_partitioner(R &&r) {
        return make_default_partitioner(std::begin(r), std::end(r));
    }

    /// Determine if P is a valid partitioner for the iterator range [I,S]
    template <class T, class I, class S = I>
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
    template <class P, class I, class S = I>
    constexpr bool is_partitioner_v = is_partitioner<P, I, S>::value;

    /// Determine if P is a valid partitioner for the range `R`
    template <class P, class R, class = void>
    struct is_range_partitioner : std::false_type {};

    /// @copydoc is_range_partitioner
    template <class P, class R>
    struct is_range_partitioner<P, R, std::enable_if_t<is_range_v<R>>>
        : is_partitioner<P, iterator_t<R>, iterator_t<R>> {};

    /// @copydoc is_range_partitioner
    template <class P, class R>
    constexpr bool is_range_partitioner_v = is_range_partitioner<P, R>::value;

    /** @} */ // @addtogroup partitioners Partitioners
    /** @} */ // @addtogroup algorithms Algorithms
} // namespace futures

#endif // FUTURES_ALGORITHM_PARTITIONER_PARTITIONER_HPP