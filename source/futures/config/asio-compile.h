//
// Copyright (c) Alan de Freitas 11/18/21.
//

#ifndef FUTURES_ASIO_COMPILE_H
#define FUTURES_ASIO_COMPILE_H

/// \file
/// Compilation proxy to make the library not header-only
///
/// To make the library *not* header-only, the user can:
/// - create a source file,
/// - include this header in it,
/// - compile it,
/// - link it with the binary, and
/// - include the ASIO_SEPARATE_COMPILATION compile definition
///
#ifdef FUTURES_SEPARATE_COMPILATION

#    if __has_include(<asio.hpp>)
#        define FUTURES_HAS_ASIO
#    endif

#    if __has_include(<boost/asio.hpp>)
#        define FUTURES_HAS_BOOST_ASIO
#    endif

#    if !defined(FUTURES_HAS_BOOST_ASIO) && !defined(FUTURES_HAS_ASIO)
#        error Asio headers not found
#    endif

#    ifdef _WIN32
#        include <SDKDDKVer.h>
#    endif

// If the standalone is available, this is what we assume the user usually
// prefers, since it's more specific
#    if defined(FUTURES_HAS_ASIO)           \
        && !(                               \
            defined(FUTURES_HAS_BOOST_ASIO) \
            && defined(FUTURES_PREFER_BOOST_DEPENDENCIES))
#        define FUTURES_USE_ASIO
#        define ASIO_SEPARATE_COMPILATION
#        include <asio/impl/src.hpp>
#    else
#        define FUTURES_USE_BOOST_ASIO
#        define BOOST_ASIO_SEPARATE_COMPILATION
#        include <boost/asio/impl/src.hpp>
#    endif

#endif // FUTURES_SEPARATE_COMPILATION

#endif // FUTURES_ASIO_COMPILE_H
