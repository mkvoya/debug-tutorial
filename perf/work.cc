// perf_flame_test.cpp
//
// Build:
//   g++ -O2 -std=c++17 -pthread -g -o perf_flame_test perf_flame_test.cpp
//   -O2 for realistic workload; -g helps perf produce better stack traces for FlameGraph.
//
// Example runs (Linux):
// 1) Run buggy workload for a while (generate profile):
//   ./perf_flame_test --mode buggy --threads 4 --iterations 200000
//
// 2) Run optimized workload:
//   ./perf_flame_test --mode optimized --threads 4 --iterations 200000
//
// How to capture perf samples and make a FlameGraph (assumes you have FlameGraph scripts):
// 1) Record (sampling at ~99Hz, capture call graphs):
//   sudo perf record -F 99 -a -g -- ./perf_flame_test --mode buggy --threads 4 --iterations 200000
//
// 2) Convert to folded stacks and produce flamegraph:
//   sudo perf script > out.perf
//   ./stackcollapse-perf.pl out.perf > out.folded
//   ./flamegraph.pl out.folded > flamegraph.svg
//
// Inspect flamegraph.svg in a browser. In "buggy" mode you should see large hot area in regex construction/search.
// In "optimized" mode that area should be much smaller (or gone).
//
// Notes:
// - Make sure FlameGraph scripts (stackcollapse-perf.pl, flamegraph.pl) from Brendan Gregg are available.
// - Use -g so that perf can show function names in flamegraph (but O2 keeps realistic speed).
// - You can run for larger iterations to get a clearer flamegraph.

#include <iostream>
#include <thread>
#include <atomic>
#include <regex>
#include <chrono>
#include <cstring>
#include <random>

using namespace std;
using namespace std::chrono;

struct Config {
    int threads = 4;
    long long iterations = 200000; // total iterations per thread
    int text_size = 2048; // size of text to search
    string pattern = R"(\b([a-z]{3,6})\d{2,}\b)"; // a regex that matches words followed by digits (synthetic)
};

static atomic<long long> processed_count{0};

// Generate a pseudo-random text of approx `size` bytes containing some tokens that match the pattern.
string generate_text(int size, mt19937 &rng) {
    static const char alpha[] = "abcdefghijklmnopqrstuvwxyz";
    uniform_int_distribution<int> len_dist(1, 12);
    uniform_int_distribution<int> char_dist(0, (int)strlen(alpha) - 1);
    uniform_int_distribution<int> digit_dist(0, 9);
    string s;
    s.reserve(size + 50);
    while ((int)s.size() < size) {
        int word_len = len_dist(rng);
        for (int i = 0; i < word_len; ++i) {
            s.push_back(alpha[char_dist(rng)]);
        }
        // occasionally add digits to create matches
        if ((rng() & 7) == 0) {
            int nd = 2 + (rng() % 5);
            for (int d = 0; d < nd; ++d) s.push_back('0' + digit_dist(rng));
        }
        s.push_back(' ' + (rng() & 7)); // add some punctuation / separators
    }
    return s;
}

// Work unit - version: constructs std::regex for every iteration (expensive).
void worker(const Config &cfg, int tid) {
    mt19937 rng((unsigned)steady_clock::now().time_since_epoch().count() + tid);
    string text = generate_text(cfg.text_size, rng);
    for (long long i = 0; i < cfg.iterations; ++i) {
        // This will show up in perf / flamegraph as time spent in regex construction/parsing.
        std::regex re(cfg.pattern);
        std::smatch m;
        // perform a search - regex_search will also be hot
        if (std::regex_search(text, m, re)) {
            // trivial work to consume the match
            volatile size_t L = m.size();
            (void)L;
        }
        processed_count.fetch_add(1, memory_order_relaxed);
    }
}

void print_usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [--threads N] [--iterations N]\n"
        "Defaults: --threads 4 --iterations 200000\n", prog);
}

// Simple command line parser (not robust, but fine for demo).
Config parse_args(int argc, char **argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "--threads" && i + 1 < argc) {
            cfg.threads = stoi(argv[++i]);
        } else if (s == "--iterations" && i + 1 < argc) {
            cfg.iterations = atoll(argv[++i]);
        } else if (s == "--text-size" && i + 1 < argc) {
            cfg.text_size = stoi(argv[++i]);
        } else if (s == "--pattern" && i + 1 < argc) {
            cfg.pattern = argv[++i];
        } else {
            print_usage(argv[0]);
            exit(1);
        }
    }
    return cfg;
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    Config cfg = parse_args(argc, argv);
    printf("Threads: %d | Iterations per thread: %lld | Text size: %d\n",
           cfg.threads, cfg.iterations, cfg.text_size);

    vector<thread> threads;
    processed_count.store(0);

    auto start = steady_clock::now();

    for (int t = 0; t < cfg.threads; ++t) {
	threads.emplace_back(worker, cfg, t);
    }

    for (auto &th : threads) th.join();

    auto end = steady_clock::now();
    double secs = duration_cast<duration<double>>(end - start).count();
    long long total = processed_count.load();
    printf("Total processed units: %lld\n", total);
    printf("Elapsed: %.3f s, throughput: %.2f ops/sec\n", secs, total / secs);
    return 0;
}
