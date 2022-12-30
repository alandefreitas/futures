//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#include <futures/launch.hpp>
#include <futures/executor/inline_executor.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>

int
main() {
    // task
    auto t = [](std::chrono::nanoseconds d) {
        std::this_thread::sleep_for(d);
    };
    // instances
    using clock = std::chrono::system_clock;
    constexpr std::size_t n_instances = 1000;
    constexpr std::size_t n_replicates = 300;
    struct replicate_t {
        std::size_t instance_idx;
        clock::duration eager_dur;
        clock::duration lazy_dur;
        clock::duration inline_dur;
        clock::duration no_future_dur;
        clock::duration invoke_dur;

        std::chrono::nanoseconds
        task_dur() {
            return std::chrono::nanoseconds(instance_idx * 100 + 100);
        }
    };
    std::vector<replicate_t> replicates;
    for (std::size_t i = 0; i < n_instances; ++i) {
        replicate_t r;
        r.instance_idx = i;
        for (std::size_t j = 0; j < n_replicates; ++j) {
            replicates.push_back(r);
        }
    }
    std::default_random_engine g(std::random_device{}());
    std::shuffle(replicates.begin(), replicates.end(), g);

    // measure
    for (auto& r: replicates) {
        {
            auto tic = clock::now();
            auto f = futures::async(t, r.task_dur());
            f.wait();
            auto toc = clock::now();
            r.eager_dur = (toc - tic);
        }
        {
            auto tic = clock::now();
            auto f = futures::schedule(t, r.task_dur());
            f.wait();
            auto toc = clock::now();
            r.lazy_dur = (toc - tic);
        }
        {
            auto tic = clock::now();
            auto f = futures::
                schedule(futures::inline_executor(), t, r.task_dur());
            f.wait();
            auto toc = clock::now();
            r.inline_dur = (toc - tic);
        }
        {
            auto tic = clock::now();
            futures::inline_executor().execute([&t, &r] { t(r.task_dur()); });
            auto toc = clock::now();
            r.no_future_dur = (toc - tic);
        }
        {
            auto tic = clock::now();
            futures::detail::invoke(t, r.task_dur());
            auto toc = clock::now();
            r.invoke_dur = (toc - tic);
        }
    }

    // vegalite
    std::cout << "{\n"
                 "  \"$schema\": "
                 "\"https://vega.github.io/schema/vega-lite/v4.json\",\n"
                 "  \"data\": {\n"
                 "    \"values\": [\n"
                 "       ";
    std::vector<double> eager_per_lazy;
    for (std::size_t i = 0; i < n_instances; ++i) {
        if (i != 0) {
            std::cout << ", \n       ";
        }
        std::chrono::nanoseconds task_dur(0);
        clock::duration eager_total(0);
        clock::duration lazy_total(0);
        clock::duration inline_total(0);
        clock::duration no_future_total(0);
        clock::duration invoke_total(0);
        for (auto& r: replicates) {
            if (r.instance_idx == i) {
                task_dur = r.task_dur();
                eager_total += r.eager_dur;
                lazy_total += r.lazy_dur;
                inline_total += r.inline_dur;
                no_future_total += r.no_future_dur;
                invoke_total += r.invoke_dur;
            }
        }
        std::cout << "{\"Task Duration (ns)\": " << task_dur.count() << ", ";
        std::cout << "\"Eager (Thread Pool)\": "
                  << eager_total.count() / n_replicates
                  << ", \"l1\": \"Eager (Thread Pool)\", ";
        std::cout << "\"Deferred (Thread Pool)\": "
                  << lazy_total.count() / n_replicates
                  << ", \"l2\": \"Deferred (Thread Pool)\", ";
        std::cout << "\"Deferred (Inline Executor)\": "
                  << inline_total.count() / n_replicates
                  << ", \"l3\": \"Deferred (Inline Executor)\", ";
        std::cout << "\"Execute (Inline Executor)\": "
                  << no_future_total.count() / n_replicates
                  << ", \"l4\": \"Execute (Inline Executor)\", ";
        std::cout << "\"Direct Invoke\": "
                  << invoke_total.count() / n_replicates
                  << ", \"l5\": \"Direct Invoke\"}";
        eager_per_lazy.push_back(
            static_cast<double>(eager_total.count())
            / static_cast<double>(lazy_total.count()));
    }
    std::cout << "]\n";
    std::cout << "  },\n"
                 "  \"layer\": [\n    ";

    std::vector<std::string> labels = {
        "Eager (Thread Pool)",
        "Deferred (Thread Pool)",
        "Deferred (Inline Executor)",
        "Execute (Inline Executor)",
        "Direct Invoke"
    };

    for (std::size_t i = 0; i < labels.size(); ++i) {
        if (i != 0) {
            std::cout << ",\n    ";
        }
        std::cout << "{\n"
                     "      \"mark\": \"line\",\n"
                     "      \"encoding\": {\n";
        std::cout << "        \"x\": {\"field\": \"Task Duration (ns)\", "
                     "\"type\": "
                     "\"quantitative\"},\n";
        std::cout << "        \"y\": {\"field\": \"" << labels[i]
                  << "\", "
                     "\"type\": "
                     "\"quantitative\", \"title\": \"Execution Time (ns)\"},\n";
        std::cout << "        \"color\": {\"field\": \"l"
                  << std::to_string(i + 1)
                  << "\", \"type\": \"nominal\", \"legend\": {\"title\": "
                     "\"Execution\"}}\n"
                     "      }\n"
                     "    }";
    }

    std::cout << "  ]\n"
                 "}\n";

    std::cout
        << "Eager/lazy total:"
        << std::accumulate(eager_per_lazy.begin(), eager_per_lazy.end(), 0.)
               / eager_per_lazy.size()
        << '\n';
}