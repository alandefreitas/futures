//
// Created by alandefreitas on 11/18/21.
//

#ifndef FUTURES_ASIO_COMPILE_H
#define FUTURES_ASIO_COMPILE_H

/// \file The user can compile and link this file to make the library not header-only

#ifdef _WIN32
#include <SDKDDKVer.h>
#endif

#include <asio/impl/src.hpp>

#endif // FUTURES_ASIO_COMPILE_H
