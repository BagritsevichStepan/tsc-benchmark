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
The benchmark uses [`rdtsc`](https://www.felixcloutier.com/x86/rdtsc) instruction and simple arithmatic operations to implement a clock with 1 ns precision, and is much faster and stable in terms of latency in less than 10 ns.

Also, the [`rdtscp`](https://www.felixcloutier.com/x86/rdtscp) instruction can be used to check that the programm did not switch to another cpu between tsc calls, which can significantly distort the measurements. To check cpu migration during the benchmarks please pass to the `TSCBenchmarking` template parameter `bool CheckCpuMigration` the true value.

## Hardware Support
The benchmark checks that your `/proc/cpuinfo` contains `nonstop_tsc`, `constant_tsc`. But in general, the TSC, on the all modern x86 systems, runs at constant rate and never stops across all P states.

Also, the benchmark checks whether your system supports [Invariant TSC](https://docs.xilinx.com/r/en-US/ug1586-onload-user/Timer-TSC-Stability), which can significantly affect the accuracy of measurements.

# <a name="reordering"></a>TSC Reordering
The compiler may reorder the reading of the TSC during benchmark. To avoid this, `benchmarking::TSCClock<Barrier BarrierType>` class is used, which implements different approaches of barriers:

OneCpuId barrier (default barrier type): 
```asm
cpuid
rdtsc
code
cpuid
rdtsc
```

LFence barrier: 
```asm
cpuid
rdtsc
code
lfence
rdtsc
cpuid
```

MFence barrier: 
```asm
cpuid
rdtsc
code
cpuid
rdtsc
mfence
```

Rdtscp barrier (intel approach):
```asm
cpuid
rdtsc
code
rdtscp
cpuid
```

Four cpuid barrier:
```asm
cpuid
rdtsc
cpuid
code
cpuid
rdtsc
cpuid
```

# <a name="overhead"></a>TSC Overhead
In the `benchmarking::TSCBenchmarking::Initialize` method, the benchmark prepare and configure the system, calibrates the TSC for accurate results.

In addition, it makes several tests to calculate the overhead from tsc calls, which then needs to be subtracted from the final measured time.

# <a name="benchmark"></a>Run Benchmark
After initialization, you can run the benchmark using the `benchmarking::TSCBenchmarking::Run` method.
 
This method sets the cpu on which the benchmark will be performed, warm up the benchmark and your code, makes several runs of your code and returns the average time.

In addition, you can use a minimalistic method `benchmarking::TSCBenchmarking::MeasureTime` of the benchmark. Which does nothing except reading the tsc. This method can be used in the code hot path to take simple measurements first, and then to translate them in another process into a more readable format.
