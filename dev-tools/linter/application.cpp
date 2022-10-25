//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include "application.hpp"
#include <string_view>

application::application(int argc, char **argv)
    : ok_(parse(config_, argc, argv)) {}

int
application::run() {
    if (!ok_) {
        return 1;
    }

    setup();

    // Find files in directory
    find_project_files();

    // Find files in directory
    if (!sanitize_all() || !ok_)
        return 1;
    return 0;
}

bool
application::setup() {
    // Find deps dir relative to source include path
    auto deps_parent_it
        = find_parent_path(config_.include_paths, config_.deps_headers_path);
    if (deps_parent_it == config_.include_paths.end()) {
        log("Cannot find", config_.deps_headers_path, "in any include path");
        return false;
    }
    deps_parent = *deps_parent_it;
    rel_deps_dir = fs::relative(config_.deps_headers_path, deps_parent);

    auto bundle_parent_it
        = find_parent_path(config_.include_paths, config_.bundled_deps_path);
    if (bundle_parent_it == config_.include_paths.end()) {
        log("Cannot find", config_.bundled_deps_path, "in any include path");
        return false;
    }
    bundle_parent = *bundle_parent_it;
    rel_bundle_dir = fs::relative(config_.bundled_deps_path, bundle_parent);
    return true;
}

void
application::find_project_files() {
    for (auto const &include_path: config_.include_paths) {
        fs::recursive_directory_iterator dir_paths{ include_path };
        for (auto &p: dir_paths) {
            if (is_cpp_file(p.path())) {
                file_paths.emplace_back(p);
            }
        }
    }
}

bool
application::sanitize_all() {
    log("Linting files");
    for (auto &p: file_paths) {
        // Validate the file
        trace("Sanitize", p);
        if (!fs::exists(p)) {
            trace("File is not in include paths");
            continue;
        }

        // Calculate expected guard
        // Find which include path it belongs to
        auto parent_it = find_parent_path(config_.include_paths, p);
        if (parent_it == config_.include_paths.end()) {
            log("Cannot find include paths for", p);
            return false;
        }
        fs::path parent = *parent_it;

        // Open file
        std::ifstream t(p);
        if (!t) {
            log("Failed to open file");
            continue;
        }

        // Read file
        std::string content{ std::istreambuf_iterator<char>(t),
                             std::istreambuf_iterator<char>() };
        t.close();

        // Lint file
        sanitize_include_guards(p, parent, content);
        bundle_includes(p, parent, content);
        generate_unit_test(p, parent);

        // Save results
        std::ofstream fout(p);
        fout << content;
    }
    remove_unused_bundled_headers();
    return true;
}

bool
application::sanitize_include_guards(
    fs::path const &p,
    fs::path const &parent,
    std::string &content) {
    if (!config_.fix_include_guards) {
        return true;
    }
    fs::path relative_p = fs::relative(p, parent);
    if (is_parent(config_.bundled_deps_path, p)) {
        trace("We do not change guards of bundled deps", relative_p);
        return true;
    }

    // Look for current include guard
    std::regex include_guard_expression(
        "(^|\n) *# *ifndef *([a-zA-Z0-9_/\\. ]+)");
    std::smatch include_guard_match;
    if (std::regex_search(content, include_guard_match, include_guard_expression))
    {
        trace("Found guard", include_guard_match[2]);
    } else {
        log("Cannot find include guard for", p);
        return true;
    }

    // Create new guard
    std::string prev_guard = include_guard_match[2].str();
    std::string expected_guard = generate_include_guard(p, parent);
    if (prev_guard == expected_guard) {
        trace("Guard", prev_guard, "is correct");
        std::string_view content_sv(content);
        std::size_t n = 0;
        auto pos = content_sv.find(expected_guard);
        while (pos != std::string_view::npos) {
            ++n;
            pos = content_sv.find(expected_guard, pos + expected_guard.size());
        }
        if (n == 1) {
            log(p, "include guard", expected_guard, "only found once");
        } else if (n == 2) {
            log(p, "include guard", expected_guard, "only found twice");
        }
        return true;
    } else {
        log("Convert guard from", prev_guard, "to", expected_guard);
    }

    // Check if the expected guard is a valid macro name
    bool new_guard_ok = std::
        all_of(expected_guard.begin(), expected_guard.end(), [](char x) {
            return std::isalnum(x) || x == '_';
        });
    if (!new_guard_ok) {
        log("Inferred guard", expected_guard, "is not a valid macro name");
        return false;
    }

    // Replace all guards in the file
    std::size_t guard_search_begin = 0;
    std::size_t guard_match_pos;
    while ((guard_match_pos = content.find(prev_guard, guard_search_begin))
           != std::string::npos)
    {
        content.replace(guard_match_pos, prev_guard.size(), expected_guard);
        guard_search_begin = guard_match_pos + prev_guard.size();
    }

    return true;
}

bool
application::bundle_includes(
    fs::path const &p,
    fs::path const &parent,
    std::string &content,
    bool is_bundled) {
    // if any of these configurations are on, we need to collect includes
    if (!config_.redirect_dep_includes && !config_.bundle_dependencies
        && !config_.remove_unused_dependency_headers)
    {
        return true;
    }

    // this is file redirecting to real dep or bundled dep, we can't fix the
    // includes here
    if (!is_bundled && is_parent(config_.bundled_deps_path, p)) {
        trace("Don't collect includes in bundled path");
        return true;
    }

    // if this is already a bundled file, we don't need to collect its includes
    // for now
    if (is_parent(config_.deps_headers_path, p)) {
        trace("Don't collect includes in deps path");
        return true;
    }

    // Iterate file looking for includes
    // We consider these patterns to be includes:
    // - #include <xxxx>
    // - #define BOOST_XXXXXXX_CONFIG "xxxx"
    // where the second case is a special exception for boost config
    struct replace_opts {
        std::regex expression;
        std::size_t path_group_id;
        std::function<std::string(std::smatch &, std::string &)> transform;
    };
    // clang-format off
    std::vector<replace_opts> replace_options = {
        {std::regex("(^|\n) *# *include *[<\"] *([a-zA-Z0-9_/\\. ]+) *[>\"]"),
         2,
         [is_bundled, this](std::smatch &include_match, std::string& as_str) {
             std::string patch;
             patch = include_match[1];
             patch += "#include <";
             if (!is_bundled) {
                 patch += rel_deps_dir.u8string();
             } else {
                 patch += rel_bundle_dir.u8string();
             }
             if (patch.back() != '/')
                patch += '/';
             patch += as_str;
             patch += ">";
             return patch;
        }},
        {std::regex("(^|\n) *# *define (BOOST_[A-Z]+_CONFIG) *([\"<]) *([a-zA-Z0-9_/\\. ]+.hpp) *([\">])"),
         4,
         [is_bundled, this](std::smatch &include_match, std::string& as_str) {
             std::string patch;
             patch = include_match[1];
             patch += "#  define ";
             patch += include_match[2];
             patch += " ";
             patch += include_match[3];
             if (!is_bundled) {
                patch += rel_deps_dir.u8string();
             } else {
                patch += rel_bundle_dir.u8string();
             }
             if (patch.back() != '/')
                patch += '/';
             patch += as_str;
             patch += include_match[5];
             return patch;
         }}
    };
    // clang-format on
    for (auto &include_opt: replace_options) {
        auto search_begin(content.cbegin());
        std::smatch include_match;

        while (std::regex_search(
            search_begin,
            content.cend(),
            include_match,
            include_opt.expression))
        {
            // Identify file included
            std::string as_str = include_match[include_opt.path_group_id];

            // We don't touch files in details/deps and files in
            auto as_path = static_cast<const fs::path>(as_str);
            auto [file_path, exists_in_source]
                = find_file(config_.include_paths, as_path);
            bool should_exist_in_bundled = is_parent(rel_deps_dir, as_path)
                                           || is_parent(rel_bundle_dir, as_path);
            if (exists_in_source || should_exist_in_bundled) {
                std::string rel_deps_dir_str = rel_deps_dir.u8string();
                std::string rel_bundle_dir_str = rel_bundle_dir.u8string();
                if (as_str.substr(0, rel_deps_dir_str.size())
                    == rel_deps_dir_str)
                {
                    trace(
                        "Source file",
                        as_str,
                        "points to dependency ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                    as_str.erase(
                        as_str.begin(),
                        as_str.begin() + rel_deps_dir_str.size());
                    if (as_str.front() == '/') {
                        as_str.erase(as_str.begin(), as_str.begin() + 1);
                    }
                    as_path = static_cast<const fs::path>(as_str);
                    trace("Looking for ", as_str, "in deps redirects");
                } else if (
                    as_str.substr(0, rel_bundle_dir_str.size())
                    == rel_bundle_dir_str)
                {
                    trace(
                        "Source file",
                        as_str,
                        "is already a bundled dependency ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                    as_str.erase(
                        as_str.begin(),
                        as_str.begin() + rel_bundle_dir_str.size());
                    if (as_str.front() == '/') {
                        as_str.erase(as_str.begin(), as_str.begin() + 1);
                    }
                    as_path = static_cast<const fs::path>(as_str);
                    trace("Looking for ", as_str, "in bundled headers");
                } else {
                    trace(
                        as_str,
                        "is in source ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                    search_begin = include_match[0].second;
                    continue;
                }
            }

            // Check if it's a standard C++ file
            if (std::next(as_path.begin()) == as_path.end()
                || as_str.find('.') == std::string_view::npos)
            {
                trace(
                    as_path,
                    "is a C++ header ( pos ",
                    include_match[0].first - content.cbegin(),
                    ")");
                search_begin = include_match[0].second;
                continue;
            }

            // Look for this header in our dependency paths
            auto [abs_file_path, exists_in_deps]
                = find_file(config_.dep_include_paths, as_path);
            if (!exists_in_deps) {
                fs::path dest = config_.bundled_deps_path / as_path;
                if (fs::exists(dest)) {
                    trace(
                        as_path,
                        "is not available but it's already bundled ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                    if (std::find(
                            bundled_headers.begin(),
                            bundled_headers.end(),
                            as_path)
                        == bundled_headers.end())
                    {
                        bundled_headers.emplace_back(as_path);
                        std::ifstream t(dest);
                        if (!t) {
                            log("Failed to open file", dest);
                            search_begin = include_match[0].second;
                            continue;
                        }
                        std::string indirect_content{
                            std::istreambuf_iterator<char>(t),
                            std::istreambuf_iterator<char>()
                        };
                        t.close();
                        bundle_includes(
                            dest,
                            bundle_parent,
                            indirect_content,
                            true);
                        std::ofstream fout(dest);
                        fout << indirect_content;
                        fout.close();
                    }
                } else {
                    trace(
                        as_path,
                        "is an external header outside our bundled "
                        "dependencies ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                }
                search_begin = include_match[0].second;
                continue;
            }

            // Check if this is an interdependency include
            if (is_parent(config_.bundled_deps_path, p)) {
                fs::path rel_this = fs::relative(p, config_.bundled_deps_path);
                auto [this_file_path, exists_in_deps2]
                    = find_file(config_.dep_include_paths, rel_this);
                auto it0 = find_parent_path(
                    config_.dep_include_paths,
                    this_file_path);
                auto it1 = find_parent_path(
                    config_.dep_include_paths,
                    abs_file_path);
                if (it0 != it1) {
                    trace(
                        as_path,
                        "is doesn't belong to the same dependency as",
                        rel_this,
                        " ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                    search_begin = include_match[0].second;
                    continue;
                }
            }

            // Check if we should ignore includes with this prefix
            bool ignore = false;
            std::string ig_str;
            for (auto const &ig: config_.bundle_ignore_prefix) {
                ig_str = ig.u8string();
                if (as_str.substr(0, ig_str.size()) == ig_str) {
                    ignore = true;
                    break;
                }
            }
            if (ignore) {
                trace(
                    as_path,
                    "is an external header whose prefix",
                    ig_str,
                    "is ignored ( pos ",
                    include_match[0].first - content.cbegin(),
                    ")");
                search_begin = include_match[0].second;
                continue;
            }

            // This included file is an external header
            trace("External header include:", as_path);

            // Patch #include
            std::string patch;
            if (config_.redirect_dep_includes) {
                patch = include_opt.transform(include_match, as_str);
            } else {
                patch = include_match[0];
            }

            // Patch contents
            trace("Replacing", as_str, "with", std::string_view(patch).substr(1));
            auto begin_offset = include_match[0].first - content.cbegin();
            if (config_.dry_run) {
                ok_ = false;
            } else {
                content.replace(
                    include_match[0].first,
                    include_match[0].second,
                    patch);
            }

            if (std::find(bundled_headers.begin(), bundled_headers.end(), as_path)
                == bundled_headers.end())
            {
                bundled_headers.emplace_back(as_path);
                fs::path dest = config_.bundled_deps_path / as_path;
                trace("Copy", abs_file_path, "to", dest);
                if (!config_.dry_run && config_.bundle_dependencies
                    && !fs::exists(dest))
                {
                    fs::create_directories(dest.parent_path());
                    fs::copy_file(
                        abs_file_path,
                        dest,
                        fs::copy_options::overwrite_existing);
                }

                // Recursively bundle indirect include headers
                std::ifstream t(dest);
                if (!t) {
                    log("Failed to open file", dest);
                    continue;
                }
                std::string indirect_content{
                    std::istreambuf_iterator<char>(t),
                    std::istreambuf_iterator<char>()
                };
                t.close();
                bundle_includes(dest, bundle_parent, indirect_content, true);
                std::ofstream fout(dest);
                fout << indirect_content;
                fout.close();
            } else {
                trace(as_path, "has already been bundled");
            }

            if (!is_bundled && config_.redirect_dep_includes) {
                create_redirect_header(as_path);
            }

            // Update search range
            search_begin = content.cbegin() + begin_offset
                           + (config_.dry_run ? include_match[0].length() :
                                                patch.size());
        }
    }

    return true;
}

std::string
application::generate_include_guard(fs::path const &p, fs::path const &parent) {
    fs::path relative_p = fs::relative(p, parent);
    std::string expected_guard = relative_p.string();
    std::transform(
        expected_guard.begin(),
        expected_guard.end(),
        expected_guard.begin(),
        [](char x) {
        if ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z')
            || (x >= '0' && x <= '9'))
            return static_cast<char>(std::toupper(x));
        else
            return '_';
        });
    return expected_guard;
}

void
application::create_redirect_header(fs::path const &as_path) {
    if (!config_.redirect_dep_includes) {
        return;
    }

    if (std::find(redirect_headers.begin(), redirect_headers.end(), as_path)
        != redirect_headers.end())
    {
        return;
    } else {
        redirect_headers.emplace_back(as_path);
    }

    // Create a file redirect file in deps
    fs::path redirect_header_p = deps_parent / rel_deps_dir / as_path;
    if (fs::exists(redirect_header_p)) {
        return;
    }

    auto guard = generate_include_guard(redirect_header_p, deps_parent);
    std::string bundle_include_name = rel_bundle_dir.u8string();
    if (bundle_include_name.back() != '/')
        bundle_include_name += '/';
    bundle_include_name += as_path.string();
    // clang-format off
    std::string redirect_content
        = "//\n"
          "// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)\n"
          "//\n"
          "// Distributed under the Boost Software License, Version 1.0.\n"
          "// https://www.boost.org/LICENSE_1_0.txt\n"
          "//\n"
          "\n"
          "#ifndef " + guard + "\n"
          "#define " + guard + "\n"
          "\n"
          "#include <futures/config.hpp>\n"
          "\n"
          "// Include " + as_path.string() + " from external or bundled " + as_path.begin()->u8string() + " \n"
          "#if defined(FUTURES_HAS_BOOST)\n"
          "#include <" + as_path.string() + ">\n"
          "#else\n"
          "#include <" + bundle_include_name + ">\n"
          "#endif\n"
          "\n"
          "#endif // " + guard;
    // clang-format on
    trace(redirect_content);
    redirect_headers.emplace_back(redirect_header_p);
    if (!config_.dry_run) {
        fs::create_directories(redirect_header_p.parent_path());
        std::ofstream deps_out(redirect_header_p);
        deps_out << redirect_content;
        deps_out.close();
    }
}
void
application::remove_unused_bundled_headers() {
    // Remove unused files
    std::vector<fs::path> ps;
    fs::recursive_directory_iterator dir_paths{ config_.bundled_deps_path };
    for (auto &p: dir_paths) {
        if (is_cpp_file(p.path())) {
            ps.emplace_back(p);
        }
    }
    for (auto &p: ps) {
        auto rel = fs::relative(p, config_.bundled_deps_path);
        if (std::find(bundled_headers.begin(), bundled_headers.end(), rel)
            == bundled_headers.end())
        {
            log(p, "is not included in this project");
            if (!config_.dry_run) {
                fs::remove(p);
            }
        }
    }
    // Remove empty dirs
    dir_paths = fs::recursive_directory_iterator{ config_.bundled_deps_path };
    std::vector<fs::path> empty_dirs;
    for (auto &p: dir_paths) {
        if (fs::is_directory(p.path()) && fs::is_empty(p.path())) {
            empty_dirs.emplace_back(p.path());
        }
    }
    for (auto &p: empty_dirs) {
        fs::remove(p);
    }
}

void
application::generate_unit_test(fs::path const &p, fs::path const &parent) {
    if (config_.unit_test_template.empty()
        || !fs::exists(config_.unit_test_template))
        return;

    if (std::any_of(p.begin(), p.end(), [this](auto &p) {
            return std::any_of(
                config_.unit_test_ignore_paths.begin(),
                config_.unit_test_ignore_paths.end(),
                [&p](auto i) { return i == p; });
        }))
        return;

    std::ifstream f(config_.unit_test_template);
    std::string content{ std::istreambuf_iterator<char>(f),
                         std::istreambuf_iterator<char>() };

    fs::path rel_p = fs::relative(p, parent);
    std::string rel_p_str = rel_p.u8string();
    fs::path testcase_name_path;
    for (auto it = std::next(rel_p.begin()); it != rel_p.end(); ++it) {
        testcase_name_path /= *it;
    }
    testcase_name_path.replace_extension("cpp");
    std::string testcase_name
        = fs::path(testcase_name_path).replace_extension().u8string();
    std::transform(
        testcase_name.begin(),
        testcase_name.end(),
        testcase_name.begin(),
        [](char x) {
        if ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z')
            || (x >= '0' && x <= '9'))
            return x;
        else
            return ' ';
        });

    auto search_begin(content.cbegin());
    std::smatch var_match;
    std::regex template_var("@([a-zA-Z0-9_/\\. ]+)@");
    while (
        std::regex_search(search_begin, content.cend(), var_match, template_var))
    {
        auto begin_offset = var_match[0].first - content.cbegin();
        if (var_match[1] == "FILENAME") {
            content.replace(var_match[0].first, var_match[0].second, rel_p_str);
            search_begin = content.cbegin() + begin_offset
                           + (config_.dry_run ? var_match[0].length() :
                                                rel_p_str.size());
        } else if (var_match[1] == "TESTNAME") {
            content
                .replace(var_match[0].first, var_match[0].second, testcase_name);
            search_begin = content.cbegin() + begin_offset
                           + (config_.dry_run ? var_match[0].length() :
                                                testcase_name.size());
        } else {
            search_begin = var_match[0].second;
        }
    }
    fs::path dest = config_.unit_test_path / testcase_name_path;
    if (config_.dry_run) {
        log(content);
    } else if (!fs::exists(dest)) {
        log("Create ", dest);
        fs::create_directories(dest.parent_path());
        std::ofstream fout(dest);
        fout << content;
    }
}
