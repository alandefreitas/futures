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
parse(config &c, std::vector<std::string_view> const &args) {
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
    auto exist_as_directory = [](fs::path const &p) {
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

    // get values as vector<path>
    auto get_paths = [&](std::string_view key) {
        auto [first, last] = get_values(key);
        return std::vector<fs::path>(first, last);
    };

    // get values as vector<string>
    auto get_strings = [&](std::string_view key) {
        auto [first, last] = get_values(key);
        return std::vector<std::string>(first, last);
    };

    // get value as path
    auto get_path = [&](std::string_view key) -> fs::path {
        auto [first, last] = get_values(key);
        if (first != last) {
            return { *first };
        } else {
            return {};
        }
    };

    // get value as bool
    auto get_bool = [&](bool &v, std::string_view key) {
        auto key_it = find_key(key);
        if (key_it == args.end()) {
            return;
        }
        auto [first, last] = get_values(key);
        if (first == last) {
            v = true;
            return;
        }
        v = !is_falsy(*first);
    };

#define CHECK(test, msg)          \
    if (!(test)) {                \
        std::cerr << msg << "\n"; \
        return false;             \
    }

    c.include_paths = get_paths("include_paths");
    CHECK(!c.include_paths.empty(), "No include paths provided")
    CHECK(
        std::all_of(
            c.include_paths.begin(),
            c.include_paths.end(),
            exist_as_directory),
        "Include directories don't exist")

    c.main_headers = get_paths("main_headers");
    CHECK(!c.main_headers.empty(), "No main headers provided")
    CHECK(
        std::all_of(
            c.main_headers.begin(),
            c.main_headers.end(),
            [&c](fs::path const &p) {
        return find_file(c.include_paths, p).second;
            }),
        "Main headers don't exist");

    c.dep_include_paths = get_paths("dep_include_paths");
    CHECK(
        std::all_of(
            c.dep_include_paths.begin(),
            c.dep_include_paths.end(),
            exist_as_directory),
        "Dep include paths must exist");

    c.bundle_ignore_prefix = get_paths("bundle_ignore_prefix");

    c.bundled_deps_path = get_path("bundled_deps_path");
    CHECK(
        !c.bundled_deps_path.empty(),
        "No destination path for bundled dependencies");

    c.unit_test_ignore_paths = get_strings("unit_test_ignore_paths");
    CHECK(
        !c.unit_test_ignore_paths.empty(),
        "No unit test ignore path segments provided");

    c.unit_test_path = get_path("unit_test_path");
    CHECK(!c.unit_test_path.empty(), "No unit test path not provided");

    c.unit_test_template = get_path("unit_test_template");
    CHECK(
        !c.unit_test_template.empty(),
        "No unit test template path not provided");
    CHECK(
        fs::exists(c.unit_test_template),
        "No unit test template file does not exist");

    c.deps_headers_path = get_path("deps_headers_path");
    CHECK(!c.deps_headers_path.empty(), "No dep headers path not provided");

    get_bool(c.show_progress, "show_progress");
    get_bool(c.verbose, "verbose");
    get_bool(c.dry_run, "dry_run");
    get_bool(c.fix_include_guards, "fix_include_guards");
    get_bool(c.redirect_dep_includes, "redirect_dep_includes");
    get_bool(c.bundle_dependencies, "bundle_dependencies");
    get_bool(c.remove_unused_dependency_headers,
        "remove_unused_dependency_headers");
    get_bool(c.update_main_headers, "update_main_headers");
#undef CHECK

    return true;
}
