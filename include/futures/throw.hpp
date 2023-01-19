//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_THROW_HPP
#define FUTURES_THROW_HPP

/**
 *  @file throw.hpp
 *  @brief Functions to handle exceptions
 *
 *  This file defines free functions to handle and throw library exceptions.
 *  The default behavior is throwing all exceptions. If exception handling is
 *  disabled, the library might call terminate or call a user defined function.
 */

#include <futures/config.hpp>
#include <futures/detail/deps/boost/assert/source_location.hpp>
#include <exception>

#ifdef __cpp_lib_source_location
#    include <source_location>
#endif


namespace futures {
    /** @addtogroup futures Futures
     *  @{
     */
    /** @addtogroup error Error
     *  @{
     */

    /// A library type equivalent to `std::source_location`
    /**
     * The source_location class represents certain information about the source
     * code, such as file names, line numbers, and function names.
     *
     * @see
     * [`std::source_location`](https://en.cppreference.com/w/cpp/utility/source_location)
     */
#if defined(FUTURES_DOXYGEN) || defined(__cpp_lib_source_location)
    using source_location = std::source_location;
#else
    using source_location = boost::source_location;
#endif

    /// Customization point to handle exceptions
    /**
     * When exception support is disabled with @ref FUTURES_NO_EXCEPTIONS, this
     * function will be called to handle exceptions.
     *
     * To customize how exceptions will be handled, define the macro
     * @ref FUTURES_CUSTOM_EXCEPTION_HANDLE, and define an alternative
     * implementation for this function.
     */
    FUTURES_DECLARE
    void
    handle_exception(std::exception const&, boost::source_location const&);

    /// Library function used to throw exceptions
    /**
     * This is the main library function used to throw exceptions according
     * to the functions available for handling exceptions.
     *
     * @tparam E Exception type
     * @param e Exception object
     * @param loc Location where the exception occurred
     */
    template <class E>
    BOOST_NORETURN void
    throw_exception(
        E&& e,
        source_location const& loc = FUTURES_CURRENT_LOCATION);

    /** @} */
    /** @} */
} // namespace futures

#include <futures/impl/throw.hpp>
#ifdef FUTURES_HEADER_ONLY
#    include <futures/impl/throw.ipp>
#endif

#endif // FUTURES_THROW_HPP
