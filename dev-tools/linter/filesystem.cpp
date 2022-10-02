//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include "filesystem.hpp"
#include <algorithm>

std::pair<fs::path, bool>
find_file(
    const std::vector<fs::path> &include_paths,
    const fs::path &filename) {
    for (const auto &path: include_paths) {
        auto p = path / filename;
        if (fs::exists(p)) {
            return std::make_pair(p, true);
        }
    }
    return std::make_pair(filename, false);
}

std::pair<fs::path, bool>
find_file(
    const std::vector<fs::path> &include_paths,
    const std::ssub_match &filename) {
    std::string filename_str = filename;
    fs::path p(filename_str.begin(), filename_str.end());
    return find_file(include_paths, p);
}

bool
is_parent(const fs::path &dir, const fs::path &p) {
    for (auto b = dir.begin(), s = p.begin(); b != dir.end(); ++b, ++s) {
        if (s == p.end() || *s != *b) {
            return false;
        }
    }
    return true;
}

std::vector<fs::path>::const_iterator
find_parent_path(
    const std::vector<fs::path> &include_paths,
    const fs::path &filename) {
    return std::find_if(
        include_paths.begin(),
        include_paths.end(),
        [&filename](const fs::path &dir) { return is_parent(dir, filename); });
}

bool
is_cpp_file(const fs::path &p) {
    constexpr std::string_view extensions[] = { ".h", ".hpp", ".cpp", ".ipp" };
    for (const auto &extension: extensions) {
        if (p.extension() == extension) {
            return true;
        }
    }
    return false;
}