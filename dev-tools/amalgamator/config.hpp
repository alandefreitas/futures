//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_CONFIG_HPP
#define FUTURES_CONFIG_HPP

#include "filesystem.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

struct config {
    // Files used as a starting point for the recursive process
    std::vector<fs::path> entry_points;

    // Path where we can look for files
    std::vector<fs::path> include_paths;

    // Output file
    fs::path output;

    // Files we are allowed to include twice
    std::vector<fs::path> double_include;

    // Remove leading comments from files
    bool remove_leading_comments{ true };

    // Directory with bundled dependencies
    // We CANNOT remove leading comments from these files
    fs::path bundled_deps_path;

    bool show_progress{ false };
    bool verbose{ false };
};

constexpr bool
is_key(std::string_view arg) {
    return !arg.empty() && arg.front() == '-';
};

constexpr bool
is_false(std::string_view value) {
    return is_key(value) || value.empty() || value == "false"
           || value == "FALSE" || value == "0";
}

bool
parse_config(config &c, const std::vector<std::string_view> &args) {
    auto find_key = [&](std::string_view key) {
        return std::find_if(args.begin(), args.end(), [&](std::string_view arg) {
            return is_key(arg) && arg.substr(arg.find_first_not_of('-')) == key;
        });
    };

    auto key_it = find_key("show_progress");
    if (key_it != args.end()) {
        c.show_progress = is_key(*key_it) || !is_false(*key_it);
    }

    key_it = find_key("verbose");
    if (key_it != args.end()) {
        c.verbose = is_key(*key_it) || !is_false(*key_it);
    }

    key_it = find_key("remove_leading_comments");
    if (key_it != args.end()) {
        c.remove_leading_comments = is_key(*key_it) || !is_false(*key_it);
    }

    auto get_values = [&](std::string_view key) {
        auto arg_begin = find_key(key);
        if (arg_begin == args.end()) {
            return std::make_pair(arg_begin, arg_begin);
        }
        ++arg_begin;
        return std::
            make_pair(arg_begin, std::find_if(arg_begin, args.end(), is_key));
    };

    auto [entry_points_begin, entry_points_end] = get_values("entry_points");
    c.entry_points = { entry_points_begin, entry_points_end };
    if (c.entry_points.empty()) {
        std::cerr << "No entry points provided\n";
        return false;
    }

    auto [double_include_begin, double_include_end] = get_values(
        "double_include");
    c.double_include = { double_include_begin, double_include_end };

    auto [include_paths_begin, include_paths_end] = get_values("include_paths");
    c.include_paths = { include_paths_begin, include_paths_end };
    if (c.include_paths.empty()) {
        std::cerr << "No include paths provided\n";
        return false;
    }

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
    if (!std::all_of(
            c.include_paths.begin(),
            c.include_paths.end(),
            exist_as_directory))
    {
        return false;
    }

    auto make_absolute = [&](fs::path &p) {
        if (p.is_relative()) {
            auto contains_path = [&](const fs::path &include_path) {
                return fs::exists(include_path / p);
            };
            auto it = std::find_if(
                c.include_paths.begin(),
                c.include_paths.end(),
                contains_path);
            if (it == c.include_paths.end()) {
                std::cerr << "No include path contains " << p << "\n";
                return false;
            } else {
                p = (*it) / p;
            }
        }
        return true;
    };
    if (!std::
            all_of(c.entry_points.begin(), c.entry_points.end(), make_absolute))
    {
        return false;
    }
    if (!std::all_of(
            c.double_include.begin(),
            c.double_include.end(),
            make_absolute))
    {
        return false;
    }

    auto exists_as_regular = [&](const fs::path &p) {
        if (!fs::exists(p)) {
            std::cerr << "Path " << p << " does not exist\n";
            return false;
        }
        if (fs::is_directory(p)) {
            std::cerr << "Path " << p << " is a directory\n";
            return false;
        }
        return true;
    };
    if (!std::all_of(
            c.entry_points.begin(),
            c.entry_points.end(),
            exists_as_regular))
    {
        return false;
    }
    if (!std::all_of(
            c.double_include.begin(),
            c.double_include.end(),
            exists_as_regular))
    {
        return false;
    }

    auto [output_begin, output_end] = get_values("output");
    if (output_begin == output_end) {
        std::cerr << "No output file provided\n";
        return false;
    }
    if (std::next(output_begin) != output_end) {
        std::cerr << "More than one output file provided\n";
        return false;
    }
    c.output = *output_begin;

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

    return true;
}

bool
parse_config(config &c, int argc, char **argv) {
    std::vector<std::string_view> args(argv, argv + argc);
    return parse_config(c, args);
}

#endif // FUTURES_CONFIG_HPP
