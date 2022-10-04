//
// Created by alandefreitas on 17/01/2022.
//

/** \file
 * \brief A very simple amalgamator to generate the single header version of
 * futures
 */

#include "config.hpp"
#include "filesystem.hpp"
#include <fstream>
#include <regex>
#include <string>

// #include <algorithm>
// #include <fstream>
// #include <vector>
// #include <iostream>
// #include <optional>

// #include <string_view>

class application {
public:
    application(int argc, char **argv) {
        if (!parse_config(c, argc, argv)) {
            ok = false;
        }
    }

    int
    run() {
        if (!ok) {
            return 1;
        }
        populate_entry_points();
        if (!patch_includes()) {
            return 1;
        }
        return !ok;
    }

    void
    populate_entry_points() {
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
    }

    bool
    patch_includes() {
        std::regex include_expression(
            "(^|\n) *# *include *< *([a-zA-Z0-9_/\\. ]+) *>");
        std::string::const_iterator search_begin(content.cbegin());
        std::smatch include_match;
        while (std::regex_search(
            search_begin,
            content.cend(),
            include_match,
            include_expression))
        {
            bool newline_prefix = (include_match[1].second
                                   - include_match[1].first)
                                  > 0;
            std::string header = include_match[2];
            log_progress(header, search_begin);

            // Check if already included
            auto [file_path, exists_in_source]
                = find_file(c.include_paths, header);
            auto [lb, ub] = std::equal_range(
                patched_files.begin(),
                patched_files.end(),
                file_path);
            const bool already_patched = lb != patched_files.end()
                                         && *lb == file_path;

            // Patch comment
            std::string patch;
            if (bool include_helper_comment = exists_in_source
                                              && already_patched;
                include_helper_comment)
            {
                if (newline_prefix) {
                    patch += '\n';
                }
                patch += "// #include <";
                patch += header;
                patch += ">\n";
            }

            // Patch contents
            if (!exists_in_source) {
                patch += include_match[0];
            } else if (!already_patched) {
                std::ifstream t(file_path);
                if (file_path.filename() == "algorithm.hpp")
                    std::cout << "here we go...";
                if (c.remove_leading_comments
                    && !is_parent(c.bundled_deps_path, file_path))
                {
                    std::string last_line = consume_leading_comments(t);
                    last_line.push_back('\n');
                    patch.append(last_line);
                }
                patch.append("\n");
                patch.append(
                    std::istreambuf_iterator<char>(t),
                    std::istreambuf_iterator<char>());

                patch.append("\n");
            }

            // Apply patch
            auto begin_offset = include_match[0].first - content.cbegin();
            content.replace(
                std::next(include_match[0].first, *include_match[0].first == '\n'),
                include_match[0].second,
                patch);

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
        fs::create_directories(c.output.parent_path());
        if (std::ofstream fout(c.output); fout.good()) {
            fout << content;
        } else {
            std::cerr << "- Error opening " << c.output << "\n";
            return false;
        }
        return true;
    }

    void
    log_progress(
        std::string const &header,
        std::string::const_iterator search_begin) {
        // Identify file

        double perc = static_cast<double>(search_begin - content.cbegin())
                      / static_cast<double>(content.size());
        if (c.show_progress) {
            if (c.verbose) {
                std::cout << "- " << 100 * perc << "% - Patching <" << header
                          << ">\n";
            } else if (perc > next_perc) {
                std::cout << "- " << 100 * perc << "% - "
                          << patched_files.size() << " files patched\n";
                if (!c.verbose) {
                    next_perc += 0.1;
                }
            }
        }
    }

    static std::string
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
private:
    // config
    config c;
    bool ok{ true };

    // content
    std::string content;
    std::vector<fs::path> patched_files;

    // stats
    double next_perc = 0.0;
};

int
main(int argc, char **argv) {
    return application(argc, argv).run();
}
