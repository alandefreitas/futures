//
// Copyright (c) 2022 alandefreitas (alandefreitas@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//

#ifndef FUTURES_STRESS_HPP
#define FUTURES_STRESS_HPP

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>

template <class D>
std::string
pretty_duration(D d, D ref) {
    namespace cron = std::chrono;
    std::string s;
    if (ref > cron::hours(1)) {
        auto h = cron::duration_cast<cron::hours>(d);
        s += std::to_string(h.count());
        s += "h:";
        d -= h;
    }
    if (ref > cron::minutes(1)) {
        auto h = cron::duration_cast<cron::minutes>(d);
        s += std::to_string(h.count());
        s += "m:";
        d -= h;
    }
    if (ref > cron::seconds(1)) {
        auto h = cron::duration_cast<cron::seconds>(d);
        s += std::to_string(h.count());
        s += "s:";
        d -= h;
    }
    if (ref > cron::milliseconds(1)) {
        auto h = cron::duration_cast<cron::milliseconds>(d);
        s += std::to_string(h.count());
        s += "ms:";
        d -= h;
    }
    if (ref > cron::microseconds(1)) {
        auto h = cron::duration_cast<cron::microseconds>(d);
        s += std::to_string(h.count());
        s += "mcs:";
        d -= h;
    }
    if (ref > cron::nanoseconds(1)) {
        auto h = cron::duration_cast<cron::nanoseconds>(d);
        s += std::to_string(h.count());
        s += "ns";
    }
    return s;
}

template <class D>
std::string
pretty_duration(D d) {
    return pretty_duration(d, d);
}

template <class Dur>
void
print_stats(std::vector<Dur>& dur) {
    // Stats
    using duration = Dur;
    using rep = typename Dur::rep;
    std::size_t n = dur.size();
    duration total = std::accumulate(dur.begin(), dur.end(), duration{ 0 });
    duration avg = total / n;
    std::nth_element(dur.begin(), dur.begin() + n / 2, dur.end());
    duration median = *(dur.begin() + n / 2);
    duration ssq = std::accumulate(
        dur.begin(),
        dur.end(),
        duration(0),
        [avg](duration t, duration x) {
        duration diff = x - avg;
        return t + duration(diff.count() * diff.count());
        });
    duration std_dev(static_cast<rep>(
        std::sqrt(ssq.count()) / ((std::max)(n - 1, std::size_t(1)))));
    std::cout << "Total time:   " << pretty_duration(total) << "\n";
    std::cout << "Avg. time:    " << pretty_duration(avg, total) << "\n";
    std::cout << "Median time:  " << pretty_duration(median, total) << "\n";
    std::cout << "Stddev. time: " << pretty_duration(std_dev, total) << "\n";
}

int
decimals(int n) {
    int d = 1;
    n /= 10;
    while (n > 0) {
        ++d;
        n /= 10;
    }
    return d;
}

template <class Fn>
int
STRESS(int n, Fn fn) {
    using clock = std::chrono::steady_clock;
    std::vector<clock::duration> dur;
    clock::duration no_cout_dur(0);
    int n_decs = decimals(n);
    for (int i = 0; i < n; ++i) {
        if (no_cout_dur > std::chrono::seconds(1)) {
            std::cout << std::setfill('0') << std::setw(n_decs) << i + 1 << '/'
                      << n << " (" << std::fixed << std::setprecision(2)
                      << static_cast<double>(i) * 100. / n << "%)\n";
            no_cout_dur = clock::duration(0);
        }
        auto start = clock::now();
        fn();
        auto end = clock::now();
        auto d = end - start;
        dur.emplace_back(d);
        no_cout_dur += d;
    }
    std::cout << "100%\n";
    print_stats(dur);
    return 0;
}

template <class Fn>
int
STRESS(int argc, char** argv, Fn fn) {
    if (argc == 1) {
        std::cerr << "Provide the number of iterations in the cmd-line\n";
        return 1;
    }
    int n = std::stoi(argv[1]);
    return STRESS(n, fn);
}

#endif // FUTURES_STRESS_HPP
