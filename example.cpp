#include <iostream>
#include <vector>

#include "include/tsc_benchmark.h"


int main() {
    std::vector<int> v{100};
    for (int i = 0; i < 100; i++) {
        v.push_back(i);
    }

    using Bechmark = benchmarking::TSCBenchmarking<false, benchmarking::Barrier::kOneCpuId>;

    Bechmark::Settings settings{};
    auto code_for_benchmarking = [&v]() {
        for (int i = 100; i < 200; i++) {
            v.push_back(i);
        }
    };

    Bechmark benchmark{};
    benchmark.Initialize();
    auto result = benchmark.Run(code_for_benchmarking, settings);
    std::cout << "Benchmark result: " << result.time_ << " ns" << std::endl;
    return 0;
}
