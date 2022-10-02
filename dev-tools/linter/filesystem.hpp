//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_FILESYSTEM_HPP
#define FUTURES_FILESYSTEM_HPP

#include <filesystem>
#include <regex>
#include <vector>

namespace fs = std::filesystem;

/// Find file in one of the include paths
std::pair<fs::path, bool>
find_file(const std::vector<fs::path> &include_paths, const fs::path &filename);

std::pair<fs::path, bool>
find_file(
    const std::vector<fs::path> &include_paths,
    const std::ssub_match &filename);

/// Check is `dir` is parent of `p`
bool
is_parent(const fs::path &dir, const fs::path &p);

/// Find which include path is the parent of `filename`
std::vector<fs::path>::const_iterator
find_parent_path(
    const std::vector<fs::path> &include_paths,
    const fs::path &filename);

/// Check if a file is a C++ file
bool
is_cpp_file(const fs::path &p);

#endif // FUTURES_FILESYSTEM_HPP
