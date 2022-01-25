//
// Created by alandefreitas on 17/01/2022.
//

/** \file
 * \brief A very simple amalgamator to generate the single header version of
 * futures
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <vector>
#include <string_view>

namespace fs = std::filesystem;

struct config
{
    std::vector<fs::path> entry_points;
    std::vector<fs::path> include_paths;
    fs::path output;
    std::vector<fs::path> double_include;
    bool remove_leading_comments{ true };
    bool show_progress{ false };
};

std::string
consume_leading_comments(std::ifstream &t) {
    std::string line;
    while (std::getline(t, line)) {
        if (auto pos = line.find_first_not_of(' ');
            pos != std::string::npos
            && std::string_view(line).substr(pos, 2) == "//")
        {
            continue;
        }
        if (std::all_of(line.begin(), line.end(), [](char line_c) {
                return std::isspace(line_c);
            }))
        {
            continue;
        }
        return line;
    }
    return "";
}

constexpr
bool
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
    return true;
}

bool
parse_config(config &c, int argc, char **argv) {
    std::vector<std::string_view> args(argv, argv + argc);
    return parse_config(c, args);
}

std::pair<fs::path, bool>
find_file(
    const std::vector<fs::path> &include_paths,
    const std::ssub_match &filename) {
    for (const auto &path: include_paths) {
        auto p = path / filename.str();
        if (fs::exists(p)) {
            return std::make_pair(p, true);
        }
    }
    return std::make_pair(fs::path(filename.first, filename.second), false);
}

int
main(int argc, char **argv) {
    config c;
    if (!parse_config(c, argc, argv)) {
        return 1;
    }

    std::string content;
    std::vector<fs::path> patched_files;

    for (std::size_t i = 0; i < c.entry_points.size(); ++i) {
        auto const &entry_point = c.entry_points[i];
        std::ifstream t(entry_point);
        if (i != 0 && c.remove_leading_comments) {
            std::string last_line = consume_leading_comments(t);
            last_line.push_back('\n');
            content.append(last_line);
        }
        content.append(
            std::istreambuf_iterator<char>(t),
            std::istreambuf_iterator<char>());
        patched_files.emplace_back(entry_point);
    }
    std::sort(patched_files.begin(), patched_files.end());

    std::regex include_expression(
        "(^|\n) *# *include *< *([a-zA-Z_/\\. ]+) *>");
    auto search_begin(content.cbegin());
    std::smatch include_match;
    while (std::regex_search(
        search_begin,
        content.cend(),
        include_match,
        include_expression))
    {
        // Identify file
        auto [file_path, exists_in_source]
            = find_file(c.include_paths, include_match[2]);
        double perc = static_cast<double>(search_begin - content.cbegin())
                      / static_cast<double>(content.size());
        if (c.show_progress) {
            std::cout << "- " << 100 * perc << "% - " << file_path << "\n";
        }

        // Check if already included
        auto [lb, ub] = std::
            equal_range(patched_files.begin(), patched_files.end(), file_path);
        const bool already_patched = lb != patched_files.end()
                                     && *lb == file_path;

        // Patch comment
        std::string patch;
        if (bool include_helper_comment = exists_in_source || already_patched;
            include_helper_comment)
        {
            patch += include_match[1];
            patch += "// #include <";
            patch += include_match[2];
            patch += ">\n";
        }

        // Patch contents
        if (!already_patched) {
            if (!exists_in_source) {
                patch += include_match[0];
            } else {
                std::ifstream t(file_path);
                if (c.remove_leading_comments) {
                    std::string last_line = consume_leading_comments(t);
                    last_line.push_back('\n');
                    patch.append(last_line);
                }
                patch.append(
                    std::istreambuf_iterator<char>(t),
                    std::istreambuf_iterator<char>());
            }
        }

        // Apply patch
        auto begin_offset = include_match[0].first - content.cbegin();
        content.replace(include_match[0].first, include_match[0].second, patch);

        // Marked file as included
        if (!already_patched
            && std::find(
                   c.double_include.begin(),
                   c.double_include.end(),
                   file_path)
                   == c.double_include.end())
        {
            patched_files.emplace(ub, file_path);
        }

        // Update search range
        search_begin = content.cbegin() + begin_offset;
        if (!exists_in_source) {
            search_begin += patch.size();
        }
    }

    if (c.show_progress) {
        std::cout << "- 100% - Saving " << c.output << "\n";
    }
    std::ofstream fout(c.output);
    fout << content;

    return 0;
}
