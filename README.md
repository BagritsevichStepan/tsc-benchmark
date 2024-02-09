# TSC Benchmark
A low overhead nanosecond TSC benchmark which allows you to measure small sections of code to nanosecond accuracy.

# Links
+ [RDTSC And RDTSCP Instructions](#rdtsc)
+ [TSC Reordering](#reordering)
+ [TSC Overhead](#overhead)
+ [Run Benchmark](#benchmark)

# Example
```cpp
benchmarking::TSCBenchmarking benchmark{};
benchmark.Initialize();
auto result = benchmark.Run(code_for_benchmarking, settings);
std::cout << "Benchmark result: " << result.time_ << " ns" << std::endl;
```

# <a name="rdtsc"></a>RDTSC And RDTSCP Instructions


# <a name="reordering"></a>TSC Reordering

# <a name="overhead"></a>TSC Overhead

# <a name="benchmark"></a>Run Benchmark
