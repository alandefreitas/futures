//  Boost compiler configuration selection header file

//  (C) Copyright John Maddock 2001 - 2002. 
//  (C) Copyright Jens Maurer 2001. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

// locate which platform we are on and define BOOST_PLATFORM_CONFIG as needed.
// Note that we define the headers to include using "header_name" not
// <header_name> in order to prevent macro expansion within the header
// name (for example "linux" is a macro on linux systems).

#if (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)
// linux, also other platforms (Hurd etc) that use GLIBC, should these really have their own config headers though?
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/linux.hpp" // redirect boost include

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
// BSD:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/bsd.hpp" // redirect boost include

#elif defined(sun) || defined(__sun)
// solaris:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/solaris.hpp" // redirect boost include

#elif defined(__sgi)
// SGI Irix:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/irix.hpp" // redirect boost include

#elif defined(__hpux)
// hp unix:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/hpux.hpp" // redirect boost include

#elif defined(__CYGWIN__)
// cygwin is not win32:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/cygwin.hpp" // redirect boost include

#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
// win32:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/win32.hpp" // redirect boost include

#elif defined(__HAIKU__)
// Haiku
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/haiku.hpp" // redirect boost include

#elif defined(__BEOS__)
// BeOS
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/beos.hpp" // redirect boost include

#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
// MacOS
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/macos.hpp" // redirect boost include

#elif defined(__TOS_MVS__)
// IBM z/OS
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/zos.hpp" // redirect boost include

#elif defined(__IBMCPP__) || defined(_AIX)
// IBM AIX
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/aix.hpp" // redirect boost include

#elif defined(__amigaos__)
// AmigaOS
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/amigaos.hpp" // redirect boost include

#elif defined(__QNXNTO__)
// QNX:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/qnxnto.hpp" // redirect boost include

#elif defined(__VXWORKS__)
// vxWorks:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/vxworks.hpp" // redirect boost include

#elif defined(__SYMBIAN32__) 
// Symbian: 
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/symbian.hpp" // redirect boost include 

#elif defined(_CRAYC)
// Cray:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/cray.hpp" // redirect boost include 

#elif defined(__VMS) 
// VMS:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/vms.hpp" // redirect boost include 

#elif defined(__CloudABI__)
// Nuxi CloudABI:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/cloudabi.hpp" // redirect boost include

#elif defined (__wasm__)
// Web assembly:
#  define BOOST_PLATFORM_CONFIG "futures/detail/bundled/boost/config/platform/wasm.hpp" // redirect boost include

#else

#  if defined(unix) \
      || defined(__unix) \
      || defined(_XOPEN_SOURCE) \
      || defined(_POSIX_SOURCE)

   // generic unix platform:

#  ifndef BOOST_HAS_UNISTD_H
#     define BOOST_HAS_UNISTD_H
#  endif

#include <futures/detail/bundled/boost/config/detail/posix_features.hpp>

#  endif

#  if defined (BOOST_ASSERT_CONFIG)
      // this must come last - generate an error if we don't
      // recognise the platform:
#     error "Unknown platform - please configure and report the results to boost.org"
#  endif

#endif

#if 0
//
// This section allows dependency scanners to find all the files we *might* include:
//
#include <futures/detail/bundled/boost/config/platform/linux.hpp>
#include <futures/detail/bundled/boost/config/platform/bsd.hpp>
#include <futures/detail/bundled/boost/config/platform/solaris.hpp>
#include <futures/detail/bundled/boost/config/platform/irix.hpp>
#include <futures/detail/bundled/boost/config/platform/hpux.hpp>
#include <futures/detail/bundled/boost/config/platform/cygwin.hpp>
#include <futures/detail/bundled/boost/config/platform/win32.hpp>
#include <futures/detail/bundled/boost/config/platform/beos.hpp>
#include <futures/detail/bundled/boost/config/platform/macos.hpp>
#include <futures/detail/bundled/boost/config/platform/zos.hpp>
#include <futures/detail/bundled/boost/config/platform/aix.hpp>
#include <futures/detail/bundled/boost/config/platform/amigaos.hpp>
#include <futures/detail/bundled/boost/config/platform/qnxnto.hpp>
#include <futures/detail/bundled/boost/config/platform/vxworks.hpp>
#include <futures/detail/bundled/boost/config/platform/symbian.hpp> 
#include <futures/detail/bundled/boost/config/platform/cray.hpp> 
#include <futures/detail/bundled/boost/config/platform/vms.hpp> 
#include <futures/detail/bundled/boost/config/detail/posix_features.hpp>



#endif

