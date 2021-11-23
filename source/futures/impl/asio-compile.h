//
// Created by alandefreitas on 11/18/21.
//

#ifndef FUTURES_ASIO_COMPILE_H
#define FUTURES_ASIO_COMPILE_H

/// \file Compilation proxy to make the library not header-only
///
/// To make the library *not* header-only, the user can:
/// - create a source file,
/// - include this header in it,
/// - compile it,
/// - link it with the binary, and
/// - include the ASIO_SEPARATE_COMPILATION compile definition
///

#ifdef _WIN32
#include <SDKDDKVer.h>
#endif

#include <asio/impl/src.hpp>

#endif // FUTURES_ASIO_COMPILE_H
