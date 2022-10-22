//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_APPLICATION_HPP
#define FUTURES_APPLICATION_HPP

#include "config.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <string_view>

class application {
public:
    /// Constructor
    application(int argc, char** argv);

    /// Run the application
    int
    run();

private:
    bool
    setup();

    void
    find_project_files();

    bool
    sanitize_all();

    // Fix any include guards that might not match the relative filename
    bool
    sanitize_include_guards(
        fs::path const& p,
        fs::path const& parent,
        std::string& content);

    // Bundle any headers that are included from the source
    bool
    bundle_includes(
        fs::path const& p,
        fs::path const& parent,
        std::string& content,
        bool is_bundled = false);

    // Bundle any headers that are included from the source
    void
    create_redirect_header(fs::path const& as_path);

    template <typename Arg, typename... Args>
    void
    log(Arg&& arg, Args&&... args) {
        if (config_.show_progress || config_.verbose) {
            std::cout << std::forward<Arg>(arg);
            ((std::cout << ' ' << std::forward<Args>(args)), ...);
            std::cout << "\n";
        }
    }

    template <typename Arg, typename... Args>
    void
    trace(Arg&& arg, Args&&... args) {
        if (config_.verbose) {
            std::cout << std::forward<Arg>(arg);
            ((std::cout << ' ' << std::forward<Args>(args)), ...);
            std::cout << "\n";
        }
    }

    static std::string
    generate_include_guard(fs::path const& p, fs::path const& parent);

    void
    generate_unit_test(fs::path const& p, fs::path const& parent);

    // Configuration
    config config_;
    bool ok_;

    // Include path that contains the deps dir (e.g.:
    // /home/.../futures/detail/deps)
    fs::path deps_parent;

    // Relative include path that contains the deps dir (e.g.:
    // futures/detail/deps)
    fs::path rel_deps_dir;

    // Include path that contains the bundle dir (e.g.:
    // /home/.../futures/detail/bundle)
    fs::path bundle_parent;

    // Relative path that contains the bundle dir (e.g.: futures/detail/bundle)
    fs::path rel_bundle_dir;

    // Project files
    std::vector<fs::path> file_paths;

    // This is the subset of file_includes that refer to external dependencies
    std::vector<fs::path> redirect_headers;

    // This is the set of indirect external headers we need
    std::vector<fs::path> bundled_headers;
    void
    remove_unused_bundled_headers();
};


#endif // FUTURES_APPLICATION_HPP
