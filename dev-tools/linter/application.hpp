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
            for (std::size_t i = 0; i < log_level; ++i)
                std::cout << "==";
            if (log_level)
                std::cout << ' ';
            std::cout << std::forward<Arg>(arg);
            ((std::cout << ' ' << std::forward<Args>(args)), ...);
            std::cout << "\n";
        }
    }

    void
    log_header(std::string_view title) {
        static constexpr std::size_t cols = 50;
        std::size_t pad = cols - title.size();
        std::size_t padl = pad / 2;
        std::size_t padr = pad - padl;
        log(std::string(padl, '='), title, std::string(padr, '='));
    }

    template <typename Arg, typename... Args>
    void
    trace(Arg&& arg, Args&&... args) {
        if (config_.verbose) {
            for (std::size_t i = 0; i < log_level; ++i)
                std::cout << "--";
            if (log_level)
                std::cout << ' ';
            std::cout << std::forward<Arg>(arg);
            ((std::cout << ' ' << std::forward<Args>(args)), ...);
            std::cout << "\n";
        }
    }

    static std::string
    generate_include_guard(fs::path const& p, fs::path const& parent);

    void
    generate_unit_test(fs::path const& p, fs::path const& parent);

    void
    apply_include_globs(
        fs::path const& p,
        fs::path const& parent,
        std::string& content);

    void
    remove_unused_bundled_headers();

    void
    remove_unreachable_headers();

    // get path relative to one of the include paths
    fs::path
    relative_path(fs::path const& p);

    static std::regex glob_to_regex(std::string const& exp);

    struct stats {
        std::size_t n_header_guards_fixed = 0;
        std::size_t n_glob_includes_applied = 0;
        std::size_t n_header_guards_mismatch = 0;
        std::size_t n_header_guards_not_found = 0;
        std::size_t n_header_guards_invalid_macro = 0;
        std::size_t n_bundled_files_created = 0;
        std::size_t n_deps_files_created = 0;
        std::size_t n_unreachable_headers = 0;
        std::size_t n_bundled_files_removed = 0;
        std::size_t n_unit_tests_created = 0;
    };

    stats stats_;

    void
    print_stats();

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

    std::size_t log_level = 0;

    static std::regex const include_regex;
    static std::regex const define_boost_config_regex;
};


#endif // FUTURES_APPLICATION_HPP
