//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include "filesystem.hpp"
#include <algorithm>

std::pair<fs::path, bool>
find_file(std::vector<fs::path> const &include_paths, fs::path const &filename) {
    for (auto const &path: include_paths) {
        auto p = path / fs::path(filename).make_preferred();
        if (fs::exists(p)) {
            return std::make_pair(p, true);
        }
    }
    return std::make_pair(filename, false);
}

std::pair<fs::path, bool>
find_file(
    std::vector<fs::path> const &include_paths,
    std::ssub_match const &filename) {
    std::string filename_str = filename;
    fs::path p(filename_str.begin(), filename_str.end());
    return find_file(include_paths, p);
}

bool
is_parent(fs::path const &dir, fs::path const &p) {
    for (auto b = dir.begin(), s = p.begin(); b != dir.end(); ++b, ++s) {
        if (s == p.end() || *s != *b) {
            return false;
        }
    }
    return true;
}

std::vector<fs::path>::const_iterator
find_parent_path(
    std::vector<fs::path> const &include_paths,
    fs::path const &filename) {
    if (filename.is_absolute()) {
        return std::find_if(
            include_paths.begin(),
            include_paths.end(),
            [&filename](fs::path const &dir) {
            return is_parent(dir, filename);
            });
    } else {
        return std::find_if(
            include_paths.begin(),
            include_paths.end(),
            [&filename](fs::path const &dir) {
            return fs::exists(dir / filename);
            });
    }
}

bool
is_cpp_file(fs::path const &p) {
    constexpr std::string_view extensions[] = { ".h", ".hpp", ".cpp", ".ipp" };
    for (auto const &extension: extensions) {
        if (p.extension() == extension) {
            return true;
        }
    }
    return false;
}