//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include "config.hpp"
#include <algorithm>
#include <iostream>

// Check if a given argv represents a key
constexpr bool
is_key(std::string_view arg) {
    return !arg.empty() && arg.front() == '-';
};

// Check if value is falsy
constexpr bool
is_falsy(std::string_view value) {
    return is_key(value) || value.empty() || value == "false"
           || value == "FALSE" || value == "0";
}

bool
parse(config &c, const std::vector<std::string_view> &args) {
    // Find the specified key in args
    auto find_key = [&](std::string_view key) {
        return std::find_if(args.begin(), args.end(), [&](std::string_view arg) {
            return is_key(arg) && arg.substr(arg.find_first_not_of('-')) == key;
        });
    };

    // Get values with specified key
    auto get_values = [&](std::string_view key) {
        auto arg_begin = find_key(key);
        if (arg_begin == args.end()) {
            return std::make_pair(arg_begin, arg_begin);
        }
        ++arg_begin;
        return std::
            make_pair(arg_begin, std::find_if(arg_begin, args.end(), is_key));
    };

    // Check if path is a directory that exists
    auto exist_as_directory = [](const fs::path &p) {
        if (!fs::exists(p)) {
            std::cerr << "Path " << p << " does not exist\n";
            return false;
        }
        if (!fs::is_directory(p)) {
            std::cerr << "Path " << p << " is not a directory\n";
            return false;
        }
        return true;
    };

    auto [include_paths_begin, include_paths_end] = get_values("include_paths");
    c.include_paths = { include_paths_begin, include_paths_end };
    if (c.include_paths.empty()) {
        std::cerr << "No include paths provided\n";
        return false;
    }
    if (!std::all_of(
            c.include_paths.begin(),
            c.include_paths.end(),
            exist_as_directory))
    {
        return false;
    }

    auto [dep_include_paths_begin, dep_include_paths_end] = get_values(
        "dep_include_paths");
    c.dep_include_paths = { dep_include_paths_begin, dep_include_paths_end };
    if (c.dep_include_paths.empty()) {
        std::cerr << "No dependency include paths provided\n";
    }
    if (!std::all_of(
            c.dep_include_paths.begin(),
            c.dep_include_paths.end(),
            exist_as_directory))
    {
        return false;
    }

    auto [bundle_ignore_prefix_begin, bundle_ignore_prefix_end] = get_values(
        "bundle_ignore_prefix");
    c.bundle_ignore_prefix = { bundle_ignore_prefix_begin, bundle_ignore_prefix_end };

    auto [bundled_deps_path_begin, bundled_deps_path_end] = get_values(
        "bundled_deps_path");
    if (bundled_deps_path_begin != bundled_deps_path_end) {
        c.bundled_deps_path = *bundled_deps_path_begin;
        if (c.bundled_deps_path.empty()) {
            std::cerr << "Empty destination path for bundled dependencies\n";
        }
    } else {
        std::cerr << "No destination path for bundled dependencies\n";
        return false;
    }

    auto [unit_test_ignore_paths_begin, unit_test_ignore_paths_end] = get_values(
        "unit_test_ignore_paths");
    c.unit_test_ignore_paths = std::vector<std::string>{ unit_test_ignore_paths_begin, unit_test_ignore_paths_end };
    if (c.unit_test_ignore_paths.empty()) {
        std::cerr << "No unit test ignore path segments provided\n";
    }

    auto [unit_test_path_begin, unit_test_path_end] = get_values(
        "unit_test_path");
    if (unit_test_path_begin != unit_test_path_end)
        c.unit_test_path = *unit_test_path_begin;

    auto [unit_test_template_begin, unit_test_template_end] = get_values(
        "unit_test_template");
    if (unit_test_template_begin != unit_test_template_end)
        c.unit_test_template = *unit_test_template_begin;

    auto [deps_headers_path_begin, deps_headers_path_end] = get_values(
        "deps_headers_path");
    if (deps_headers_path_begin != deps_headers_path_end) {
        c.deps_headers_path = *deps_headers_path_begin;
        if (c.deps_headers_path.empty()) {
            std::cerr << "Empty destination path for bundled dependencies\n";
        }
    } else {
        std::cerr << "No destination path for bundled dependencies\n";
        return false;
    }

    auto key_it = find_key("show_progress");
    if (key_it != args.end()) {
        c.show_progress = is_key(*key_it) || !is_falsy(*key_it);
    }

    key_it = find_key("verbose");
    if (key_it != args.end()) {
        c.verbose = is_key(*key_it) || !is_falsy(*key_it);
    }

    key_it = find_key("dry_run");
    if (key_it != args.end()) {
        c.dry_run = is_key(*key_it) || !is_falsy(*key_it);
    }

    key_it = find_key("fix_include_guards");
    if (key_it != args.end()) {
        c.fix_include_guards = is_key(*key_it) || !is_falsy(*key_it);
    }

    key_it = find_key("redirect_dep_includes");
    if (key_it != args.end()) {
        c.redirect_dep_includes = is_key(*key_it) || !is_falsy(*key_it);
    }

    key_it = find_key("bundle_dependencies");
    if (key_it != args.end()) {
        c.bundle_dependencies = is_key(*key_it) || !is_falsy(*key_it);
    }

    key_it = find_key("remove_unused_dependency_headers");
    if (key_it != args.end()) {
        c.remove_unused_dependency_headers = is_key(*key_it)
                                             || !is_falsy(*key_it);
    }

    key_it = find_key("update_main_headers");
    if (key_it != args.end()) {
        c.update_main_headers = is_key(*key_it) || !is_falsy(*key_it);
    }

    return true;
}
