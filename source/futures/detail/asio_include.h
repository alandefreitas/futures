//
// Created by alandefreitas on 11/18/21.
//

#ifndef FUTURES_ASIO_INCLUDE_H
#define FUTURES_ASIO_INCLUDE_H

/// \file Whenever including <asio.hpp>, we include this file instead
/// This ensures the logic of including asio or boost::asio is consistent

#ifdef _WIN32
#include <SDKDDKVer.h>
#endif

#include <asio.hpp>

#endif // FUTURES_ASIO_INCLUDE_H
