//
// Created by alandefreitas on 11/18/21.
//

#ifndef FUTURES_SMALL_VECTOR_INCLUDE_H
#define FUTURES_SMALL_VECTOR_INCLUDE_H

/// \file Whenever including <small/vector.h>, we include this file instead
/// This ensures the logic of including small::vector or boost::container::small_vector is consistent

/*
 * Check what versions of small vectors are available
 *
 * We use __has_include<...> because we are targeting C++17
 *
 */
#if __has_include(<small/vector.h>)
#define FUTURES_HAS_SMALL_VECTOR
#endif

#if __has_include(<boost/container/small_vector.hpp>)
#define FUTURES_HAS_BOOST_SMALL_VECTOR
#endif

#if !defined(FUTURES_HAS_BOOST_SMALL_VECTOR) && !defined(FUTURES_HAS_SMALL_VECTOR)
#define FUTURES_HAS_NO_SMALL_VECTOR
#endif

/*
 * Decide what version of small vector to use
 */

#ifdef FUTURES_HAS_NO_SMALL_VECTOR
// If no small vectors are available, we recur to regular vectors
#include <vector>
#elif defined(FUTURES_HAS_SMALL_VECTOR) && !(defined(FUTURES_HAS_BOOST_SMALL_VECTOR) && defined(FUTURES_PREFER_BOOST_DEPENDENCIES))
// If the standalone is available, this is what we assume the user usually prefers, since it's more specific
#define FUTURES_USE_SMALL_VECTOR
#include <small/vector.h>
#else
// This is what boost users will often end up using
#define FUTURES_USE_BOOST_SMALL_VECTOR
#include <boost/container/small_vector.hpp>
#endif

/*
 * Create the aliases
 */

namespace futures {
#if defined(FUTURES_USE_SMALL_VECTOR) || defined(FUTURES_DOXYGEN)
    /// \brief Alias to the small vector class we use
    ///
    /// This futures::small_vector alias might refer to:
    /// - ::small::vector if small is available
    /// - ::boost::container::small_vector if boost is available
    /// - ::std::vector if no small vector is available
    ///
    /// If you are referring to this small vector class and need it
    /// to match whatever class is being used by futures, prefer `futures::small_vector`
    /// instead of using `::small::vector` or `::boost::container::small_vector` directly.
    template <typename T, size_t N = ::small::default_inline_storage_v<T>, typename A = std::allocator<T>>
    using small_vector = ::small::vector<T, N, A>;
#elif defined(FUTURES_USE_BOOST_SMALL_VECTOR)
    template <typename T, size_t N = 5, typename A = std::allocator<T>>
    using small_vector = ::boost::container::small_vector<T, N, A>;
#else
    template <typename T, size_t N = ::small::default_inline_storage_v<T>, typename A = std::allocator<T>>
    using small_vector = ::std::vector<T, A>;
#endif
}

#endif // FUTURES_SMALL_VECTOR_INCLUDE_H
