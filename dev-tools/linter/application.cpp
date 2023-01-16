//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include "application.hpp"
#include <functional>
#include <string_view>

std::regex const application::include_regex{
    "(^|\n) *# *include *[<\"] *([a-zA-Z0-9_/\\. ]+) *[>\"]"
};

std::regex const application::define_boost_config_regex{
    "(^|\n) *# *define (BOOST_[A-Z]+_CONFIG) *([\"<]) *([a-zA-Z0-9_/\\. "
    "]+.hpp) *([\">])"
};

application::application(int argc, char **argv)
    : ok_(parse(config_, argc, argv)) {}

int
application::run() {
    if (!ok_) {
        return 1;
    }

    if (!setup()) {
        return 1;
    }

    // Find files in directory
    find_project_files();

    // Find files in directory
    if (!sanitize_all() || !ok_) {
        return 1;
    }
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
    std::sort(file_paths.begin(), file_paths.end());
}

bool
application::sanitize_all() {
    log_header("LINTING SOURCE FILES");
    for (auto &p: file_paths) {
        // Validate the file
        trace("Sanitize", relative_path(p));
        ++log_level;
        if (!fs::exists(p)) {
            trace("File is not in include paths");
            --log_level;
            continue;
        }

        // Calculate expected guard
        // Find which include path it belongs to
        auto parent_it = find_parent_path(config_.include_paths, p);
        if (parent_it == config_.include_paths.end()) {
            log("Cannot find include paths for", p);
            --log_level;
            return false;
        }
        fs::path parent = *parent_it;

        // Open file
        std::ifstream t(p);
        if (!t) {
            log("Failed to open file");
            --log_level;
            continue;
        }

        // Read file
        std::string content{ std::istreambuf_iterator<char>(t),
                             std::istreambuf_iterator<char>() };
        t.close();

        // Lint file
        ++log_level;
        trace("Apply include glob");
        apply_include_globs(p, parent, content);
        --log_level;

        trace("Sanitize include guards");
        ++log_level;
        sanitize_include_guards(p, parent, content);
        --log_level;

        trace("Bundle includes");
        ++log_level;
        bundle_includes(p, parent, content);
        --log_level;

        trace("Generate unit tests");
        ++log_level;
        generate_unit_test(p, parent);
        --log_level;

        // Save results
        std::ofstream fout(p);
        fout << content;
        --log_level;
    }
    remove_unused_bundled_headers();
    remove_unreachable_headers();
    print_stats();
    return true;
}

fs::path
application::relative_path(fs::path const &p) {
    if (p.is_relative()) {
        return p;
    }
    auto it = find_parent_path(config_.include_paths, p);
    if (it != config_.include_paths.end()) {
        return fs::relative(p, *it);
    }
    it = find_parent_path(config_.dep_include_paths, p);
    if (it != config_.dep_include_paths.end()) {
        return fs::relative(p, *it);
    }
    return p;
}

void
application::apply_include_globs(
    fs::path const &p,
    fs::path const &parent,
    std::string &content) {
    fs::path relative_p = fs::relative(p, parent);
    if (is_parent(config_.bundled_deps_path, p)) {
        trace("We do not apply globs in bundled deps", relative_p);
    }
    static std::regex const glob_regex{
        "// #glob < *([a-zA-Z0-9_/\\. \\*]+) *>"
    };
    static std::regex const glob_except_regex{
        " * - * < *([a-zA-Z0-9_/\\. \\*]+) *>"
    };
    std::size_t i = 0;
    std::smatch glob_match;
    std::smatch glob_except_match;

    while (std::regex_search(
        content.cbegin() + i,
        content.cend(),
        glob_match,
        glob_regex))
    {
        trace("Found glob regex", glob_match[0]);
        bool has_except = false;
        if (std::regex_search(
                glob_match[0].second,
                content.cend(),
                glob_except_match,
                glob_except_regex))
        {
            has_except = true;
        }
        std::size_t replace_begin = (has_except ? glob_except_match[0].second :
                                                  glob_match[0].second)
                                    - content.begin();
        while (content[replace_begin] != '\n') {
            ++replace_begin;
        }
        ++replace_begin;
        std::size_t replace_end = replace_begin;
        while (replace_end != content.size()) {
            while (content[replace_end] == '\n' || content[replace_end] == ' ')
            {
                ++replace_end;
            }
            std::smatch include_match;
            bool match = std::regex_search(
                content.cbegin() + replace_end,
                content.cend(),
                include_match,
                include_regex);
            if (!match || include_match[0].first != content.cbegin() + replace_end) {
                break;
            }
            replace_end = include_match[0].second - content.cbegin();
        }
        std::string patch;
        std::regex file_path_regex = glob_to_regex(glob_match[1]);
        std::regex file_except_regex = has_except ?
                                           glob_to_regex(glob_except_match[1]) :
                                           std::regex("a^");
        auto self_r = relative_path(p);
        for (auto &abs_h: file_paths) {
            auto r = relative_path(abs_h);
            if (r != self_r && std::regex_match(r.generic_string(), file_path_regex)
                && !std::regex_match(r.generic_string(), file_except_regex))
            {
                patch += "#include <";
                patch += r.generic_string();
                patch += ">\n";
            }
        }
        std::size_t replace_n = replace_end - replace_begin;
        patch += "\n\n";
        if (patch != std::string_view(content).substr(replace_begin, replace_n))
        {
            ++stats_.n_glob_includes_applied;
        }
        if (!config_.dry_run) {
            content.replace(replace_begin, replace_n, patch);
        }
        i = replace_begin + patch.size();
    }
}

std::regex
application::glob_to_regex(std::string const &exp) {
    std::string file_glob_exp = exp;
    std::string file_regex_exp = std::
        regex_replace(file_glob_exp, std::regex("\\."), "\\.");
    file_regex_exp = std::
        regex_replace(file_regex_exp, std::regex("\\*"), "[^/]*");
    file_regex_exp = std::regex_replace(
        file_regex_exp,
        std::regex("\\[\\^\\/\\]\\*\\[\\^\\/\\]\\*"),
        ".*");
    return std::regex(file_regex_exp);
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
        ++stats_.n_header_guards_not_found;
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
            ++stats_.n_header_guards_mismatch;
        } else if (n == 2) {
            log(p, "include guard", expected_guard, "only found twice");
            ++stats_.n_header_guards_mismatch;
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
        ++stats_.n_header_guards_invalid_macro;
        return false;
    }

    // Replace all guards in the file
    std::size_t guard_search_begin = 0;
    std::size_t guard_match_pos;
    while ((guard_match_pos = content.find(prev_guard, guard_search_begin))
           != std::string::npos)
    {
        content.replace(guard_match_pos, prev_guard.size(), expected_guard);
        guard_search_begin = guard_match_pos + expected_guard.size();
    }
    ++stats_.n_header_guards_fixed;

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
        {include_regex,
         2,
         [is_bundled, this](std::smatch &include_match, std::string& as_str) {
             std::string patch;
             patch = include_match[1];
             patch += "#include <";
             if (!is_bundled) {
                 patch += rel_deps_dir.string();
             } else {
                 patch += rel_bundle_dir.string();
             }
             if (patch.back() != '/')
                patch += '/';
             patch += as_str;
             patch += ">";
             return patch;
        }},
        {define_boost_config_regex,
         4,
         [is_bundled, this](std::smatch &include_match, std::string& as_str) {
             std::string patch;
             patch = include_match[1];
             patch += "#  define ";
             patch += include_match[2];
             patch += " ";
             patch += include_match[3];
             if (!is_bundled) {
                patch += rel_deps_dir.string();
             } else {
                patch += rel_bundle_dir.string();
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
                std::string rel_deps_dir_str = rel_deps_dir.string();
                std::string rel_bundle_dir_str = rel_bundle_dir.string();
                if (as_str.substr(0, rel_deps_dir_str.size())
                    == rel_deps_dir_str)
                {
                    trace(
                        as_str,
                        "points to a dependency ( pos ",
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
                    ++log_level;
                    trace("Looking for", as_str, "in bundled headers");
                    --log_level;
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
                ++log_level;
                trace(
                    as_path,
                    "is a C++ header ( pos ",
                    include_match[0].first - content.cbegin(),
                    ")");
                --log_level;
                search_begin = include_match[0].second;
                continue;
            }

            // Look for this header in our dependency paths
            auto [abs_file_path, exists_in_deps]
                = find_file(config_.dep_include_paths, as_path);
            if (!exists_in_deps) {
                fs::path dest = config_.bundled_deps_path / as_path;
                if (fs::exists(dest)) {
                    ++log_level;
                    trace(
                        as_path,
                        "is not available but it's already bundled ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                    --log_level;
                    if (std::find(
                            bundled_headers.begin(),
                            bundled_headers.end(),
                            as_path)
                        == bundled_headers.end())
                    {
                        bundled_headers.emplace_back(as_path);
                        std::ifstream t(dest);
                        if (!t) {
                            ++log_level;
                            log("Failed to open file", dest);
                            --log_level;
                            search_begin = include_match[0].second;
                            continue;
                        }
                        std::string indirect_content{
                            std::istreambuf_iterator<char>(t),
                            std::istreambuf_iterator<char>()
                        };
                        t.close();
                        trace("Bundle includes", relative_path(dest));
                        ++log_level;
                        bundle_includes(
                            dest,
                            bundle_parent,
                            indirect_content,
                            true);
                        --log_level;
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
                    ++log_level;
                    trace(
                        as_path,
                        "doesn't belong to the same dependency as",
                        rel_this,
                        " ( pos ",
                        include_match[0].first - content.cbegin(),
                        ")");
                    --log_level;
                    search_begin = include_match[0].second;
                    continue;
                }
            }

            // Check if we should ignore includes with this prefix
            bool ignore = false;
            std::string ig_str;
            for (auto const &ig: config_.bundle_ignore_prefix) {
                ig_str = ig.string();
                if (as_str.substr(0, ig_str.size()) == ig_str) {
                    ignore = true;
                    break;
                }
            }
            if (ignore) {
                ++log_level;
                trace(
                    as_path,
                    "is an external header whose prefix",
                    ig_str,
                    "is ignored ( pos ",
                    include_match[0].first - content.cbegin(),
                    ")");
                --log_level;
                search_begin = include_match[0].second;
                continue;
            }

            // This included file is an external header
            ++log_level;
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
                    log("Bundle", as_path);
                    ++stats_.n_bundled_files_created;
                }

                // Recursively bundle indirect include headers
                std::ifstream t(dest);
                if (!t) {
                    log("Failed to open file", dest);
                    --log_level;
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
            --log_level;

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
        {
            return static_cast<char>(std::toupper(x));
        } else {
            return '_';
        }
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
    std::string bundle_include_name = rel_bundle_dir.string();
    if (bundle_include_name.back() != '/') {
        bundle_include_name += '/';
    }
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
          "// Include " + as_path.string() + " from external or bundled " + as_path.begin()->string() + " \n"
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
        log("Create dep header: ", redirect_header_p);
        ++stats_.n_deps_files_created;
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
            log("Remove bundled", p);
            if (!config_.dry_run) {
                fs::remove(p);
            }
            ++stats_.n_bundled_files_removed;
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
    {
        return;
    }

    if (std::any_of(p.begin(), p.end(), [this](auto &p) {
            return std::any_of(
                config_.unit_test_ignore_paths.begin(),
                config_.unit_test_ignore_paths.end(),
                [&p](auto i) { return i == p; });
        }))
    {
        return;
    }

    std::ifstream f(config_.unit_test_template);
    std::string content{ std::istreambuf_iterator<char>(f),
                         std::istreambuf_iterator<char>() };

    fs::path rel_p = fs::relative(p, parent);
    std::string rel_p_str = rel_p.string();
    fs::path testcase_name_path;
    for (auto it = std::next(rel_p.begin()); it != rel_p.end(); ++it) {
        testcase_name_path /= *it;
    }
    testcase_name_path.replace_extension("cpp");
    std::string testcase_name
        = fs::path(testcase_name_path).replace_extension().string();
    std::transform(
        testcase_name.begin(),
        testcase_name.end(),
        testcase_name.begin(),
        [](char x) {
        if ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z')
            || (x >= '0' && x <= '9'))
        {
            return x;
        } else {
            return ' ';
        }
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
        ++stats_.n_unit_tests_created;
    }
}

void
application::remove_unreachable_headers() {
    std::vector<fs::path> collected;
    for (auto &p: config_.main_headers) {
        collected.emplace_back(find_file(config_.include_paths, p).first);
    }
    for (std::size_t i = 0; i < collected.size(); ++i) {
        fs::path &p = collected[i];
        std::ifstream fin(p);
        std::string content{ std::istreambuf_iterator<char>(fin),
                             std::istreambuf_iterator<char>() };
        std::smatch include_guard_match;
        std::vector<fs::path> includes;
        std::size_t j = 0;
        while (std::regex_search(
            content.cbegin() + j,
            content.cend(),
            include_guard_match,
            include_regex))
        {
            std::string as_str(include_guard_match[2]);
            fs::path as_path(as_str);
            includes.push_back(as_path);
            j = include_guard_match[0].second - content.cbegin();
        }
        for (auto &ip: includes) {
            auto [abs, ok] = find_file(config_.include_paths, ip);
            if (ok
                && std::find(collected.begin(), collected.end(), abs)
                       == collected.end())
            {
                collected.push_back(abs);
            }
        }
        j = 0;
        while (std::regex_search(
            content.cbegin() + j,
            content.cend(),
            include_guard_match,
            define_boost_config_regex))
        {
            std::string as_str(include_guard_match[4]);
            fs::path as_path(as_str);
            includes.push_back(std::move(as_path));
            j = include_guard_match[0].second - content.cbegin();
        }
        for (auto &ip: includes) {
            auto [abs, ok] = find_file(config_.include_paths, ip);
            if (ok
                && std::find(collected.begin(), collected.end(), abs)
                       == collected.end())
            {
                collected.push_back(abs);
            }
        }
    }
    std::sort(collected.begin(), collected.end());
    bool first = true;
    for (auto &p: file_paths) {
        if (!std::binary_search(collected.begin(), collected.end(), p)) {
            if (first) {
                log_header("UNREACHABLE HEADERS");
                first = false;
            }
            log(p);
            ++stats_.n_unreachable_headers;
        }
    }
}


void
application::print_stats() {
    log_header("HEADERS");
    log("Unreachable headers:", stats_.n_unreachable_headers);
    log("Glob includes applied:", stats_.n_glob_includes_applied);
    log_header("HEADER GUARDS");
    log("Header guards fixed:", stats_.n_header_guards_fixed);
    log("Header guards not found:", stats_.n_header_guards_not_found);
    log("Header guards not completely identified:",
        stats_.n_header_guards_mismatch);
    log("Invalid header guards generated:",
        stats_.n_header_guards_invalid_macro);
    log_header("DEPENDENCIES");
    log("Bundled files created:", stats_.n_bundled_files_created);
    log("Bundled files removed:", stats_.n_bundled_files_removed);
    log("Deps files created:", stats_.n_deps_files_created);
    log_header("UNIT TESTS");
    log("Unit tests created:", stats_.n_unit_tests_created);
}
