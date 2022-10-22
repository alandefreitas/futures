//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_CONFIG_HPP
#define FUTURES_CONFIG_HPP

#include "filesystem.hpp"
#include <vector>
#include <string_view>

struct config {
    // Input paths
    std::vector<fs::path> include_paths;

    // Unit tests
    fs::path unit_test_path;
    fs::path unit_test_template;
    std::vector<std::string> unit_test_ignore_paths;

    // Dependency include paths
    std::vector<fs::path> dep_include_paths;

    // Directory where we should store headers redirecting to real or bundled
    fs::path deps_headers_path;

    // Directory where we should store our bundled dependencies
    fs::path bundled_deps_path;

    // List of prefixes we should ignore when bundling files
    // For things like "arpa", "linux", ...
    // This is useful when the script happens to be running on /usr/include/
    std::vector<fs::path> bundle_ignore_prefix;

    // Log: show progress
    bool show_progress{ false };
    bool verbose{ false };

    // Linting options

    // Run but don't change anything
    //
    // If the linter finds any failures in dryrun, we return 1, so that we
    // can use this option to use the linter as a checker
    bool dry_run{ false };

    // Adjust include guards to match the file path
    // e.g.: <futures/detail/deps/boost/asio/thread_pool.hpp> is updated with
    // the include-guards FUTURES_DETAIL_DEPS_BOOST_ASIO_THREAD_POOL_HPP
    bool fix_include_guards{ true };

    // Redirect includes to bundled includes
    //
    // eg.:
    // 1) include-guards such as <boost/asio/thread_pool.hpp> become
    // <futures/detail/deps/boost/asio/thread_pool.hpp>
    // 2) Create new header futures/detail/deps/boost/asio/thread_pool.hpp,
    // if it doesn't exist, to detect whether boost is available and
    // include
    bool redirect_dep_includes{ true };

    // Bundle dependency files in detail/bundled
    //
    // eg.:
    // 1) copy <boost/asio/thread_pool.hpp> to
    // <futures/detail/deps/boost/asio/thread_pool.hpp> and recursively copy all
    // dependencies included by thread_pool.hpp
    // 2) The include-guards such as <boost/config.hpp> in these new files are
    // also adjusted to <futures/detail/deps/boost/config.hpp>
    // 3) Any license files are also copied
    bool bundle_dependencies{ true };

    // Remove unused dependency files
    //
    // Detect any headers that exist in the project but are not included by any
    // other header
    bool remove_unused_dependency_headers{ true };

    // Ensure the main headers do include all other headers
    bool update_main_headers{ true };


};

bool
parse(config &c, const std::vector<std::string_view> &args);

inline
bool
parse(config &c, int argc, char **argv) {
    std::vector<std::string_view> args(argv, argv + argc);
    return parse(c, args);
}

#endif // FUTURES_CONFIG_HPP
