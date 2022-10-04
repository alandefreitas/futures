//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FILESYSTEM_HPP
#define FUTURES_FILESYSTEM_HPP

#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

std::pair<fs::path, bool>
find_file(
    const std::vector<fs::path> &include_paths,
    const std::string_view &filename) {
    for (const auto &path: include_paths) {
        auto p = path / filename;
        if (fs::exists(p)) {
            return std::make_pair(p, true);
        }
    }
    return std::make_pair(fs::path(filename), false);
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


#endif // FUTURES_FILESYSTEM_HPP
