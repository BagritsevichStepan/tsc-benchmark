#ifndef TSC_BENCHMARK_TSC_BENCHMARK_H
#define TSC_BENCHMARK_TSC_BENCHMARK_H

#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <sys/types.h>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <limits>
#include <utility>

#include "tsc_clock.h"
#include "utils/compiler.h"
#include "utils/types.h"
#include "utils/affinity.h"

namespace benchmarking {

    namespace details {

        inline constexpr auto kEmptyCode = []() {};

    } // End of namespace benchmarking::details

    template<bool CheckCpuMigration = true, Barrier BarrierType = Barrier::kOneCpuId>
    class TSCBenchmarking {
    public:
        class Result;
        class Settings;

        TSCBenchmarking();

        void Initialize(); //todo split

        template<typename Code>
        FORCE_INLINE TimePoint MeasureTime(Code&& code); // Use in time-critical task

        template<typename Code>
        Result Run(Code&& code, Settings settings);

        ~TSCBenchmarking() = default;

        class Settings {
        public:
            std::size_t cycles_number_{kDefaultCyclesNumber};
            int cpu_{0};
            std::size_t cache_warmup_cycles_number_{0};
        };

        class Result {
        public:
            using Nanos = std::chrono::nanoseconds;
            TimePoint time_;
            TimePoint overhead_;
        };

    private:
        static constexpr std::size_t kDefaultCyclesNumber = 100;
        static constexpr std::size_t kDefaultStabilizedThreshold = kDefaultCyclesNumber * 10 / 100;
        static constexpr std::size_t kDefaultRunsNumber = 100;

        static constexpr auto kGetTime = []() {
            timespec ts{};
            clock_gettime(CLOCK_REALTIME, &ts);
        };
        
        
        std::pair<TimePoint, TimePoint> MeasureOverhead(std::size_t cycles_number = kDefaultCyclesNumber);
        
        std::pair<TimePoint, TimePoint> MeasureStabilizedOverhead(std::size_t cycles_number = kDefaultCyclesNumber,
                                                                  std::size_t stabilized_threshold = kDefaultStabilizedThreshold);

        template<typename Code>
        FORCE_INLINE TimePoint MeasureMinLatency(std::size_t cycles_number, Code&& code);

        template<typename Code>
        FORCE_INLINE TimePoint MeasureStabilizedMinLatency(std::size_t cycles_number,
                                                           std::size_t stabilized_threshold,
                                                           Code&& code);

        template<typename Code>
        FORCE_INLINE bool Measure(TimePoint& start, TimePoint& end, Code&& code);

    private:
        TSCClock<BarrierType> clock_{};
        TimePoint tsc_overhead_{0};
        TimePoint clock_overhead_{0};
    };


    // Implementation
    template<bool CheckCpuMigration, Barrier BarrierType>
    TSCBenchmarking<CheckCpuMigration, BarrierType>::TSCBenchmarking() {
        details::CpuInfo cpu_info{};
        assert(cpu_info.IsTSCEnabled());
        assert(BarrierType != Barrier::kRdtscp || cpu_info.IsRdtscpEnabled());
        if (cpu_info.IsInvariantTSCEnabled()) {
            std::cerr << "[Warning] Invariant TSC is not supported on your system" << std::endl;
        }
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    void TSCBenchmarking<CheckCpuMigration, BarrierType>::Initialize() {
        if (geteuid() == 0) {
            sched_param sp{};
            std::memset(&sp, 0, sizeof(sp));
            sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
            if (sched_setscheduler(0, SCHED_FIFO, &sp) == -1) {
                std::cerr << "[Warning] Error changing scheduling policy to RT class: " << errno << std::endl;
            } else {
                std::cout << "Scheduling policy is changed to RT class with max priority" << std::endl;
            }

            if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
                std::cerr << "[Warning] Error locking pages: " << errno << std::endl;
            } else {
                std::cout << "All pages of process are locked (paging is disabled)" << std::endl;
            }
        } else {
            std::cerr << "[Warning] Benchmark is launched without ROOT permissions:"
                      << " default scheduler, default priority, pages are not locked" << std::endl;
        }


        auto overheads = MeasureOverhead();
        tsc_overhead_ = overheads.first;
        clock_overhead_ = overheads.second;
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TimePoint TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureTime(Code&& code) {
        TimePoint start = clock_.StartTime();
        code();
        TimePoint end = clock_.EndTime();
        return end - start;
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TSCBenchmarking<CheckCpuMigration, BarrierType>::Result TSCBenchmarking<CheckCpuMigration, BarrierType>::Run(Code&& code,
                                                                           TSCBenchmarking::Settings settings) {
        details::PinThread(settings.cpu_);

        TimePoint start, end;

        for (int r = 0; r < settings.cache_warmup_cycles_number_; r++) {
            if constexpr (CheckCpuMigration) {
                uint32_t cpu_number0{0}, cpu_number1{1};
                start = clock_.StartTime(cpu_number0);
                code.operator()();
                end = clock_.EndTime(cpu_number1);
            } else {
                start = clock_.StartTime();
                code.operator()();
                end = clock_.EndTime();
            }
        }

        uint64_t summary_time = 0;
        for (int r = 0; r < settings.cycles_number_;) {
            if constexpr (CheckCpuMigration) {
                uint32_t cpu_number0{0}, cpu_number1{1};
                start = clock_.StartTime(cpu_number0);
                code.operator()();
                end = clock_.EndTime(cpu_number1);
                if (cpu_number0 != cpu_number1) {
                    continue;
                }
            } else {
                start = clock_.StartTime();
                code.operator()();
                end = clock_.EndTime();
            }

            if (end > start) {
                TimePoint time = end - start;
                if (time > tsc_overhead_) {
                    summary_time += time;
                    r++;
                }
            }
        }
        return TSCBenchmarking::Result{summary_time / settings.cycles_number_, tsc_overhead_};
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    std::pair<TimePoint, TimePoint> TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureOverhead(std::size_t cycles_number) {
        TimePoint min_tsc_overhead = MeasureMinLatency(cycles_number, details::kEmptyCode);
        TimePoint min_clock_overhead = MeasureMinLatency(cycles_number, kGetTime);
        return {min_tsc_overhead, min_clock_overhead - min_tsc_overhead};
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    std::pair<TimePoint, TimePoint> TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureStabilizedOverhead(
            std::size_t cycles_number, std::size_t stabilized_threshold) {
        TimePoint min_tsc_overhead = MeasureStabilizedMinLatency(cycles_number,
                                                                 stabilized_threshold,
                                                                 details::kEmptyCode);
        TimePoint min_clock_overhead = MeasureStabilizedMinLatency(cycles_number,
                                                                   stabilized_threshold,
                                                                   kGetTime);
        return {min_tsc_overhead, min_clock_overhead - min_tsc_overhead};
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TimePoint TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureMinLatency(std::size_t cycles_number,
                                                                                 Code&& code) {
        TimePoint start, end;
        TimePoint min_latency = std::numeric_limits<TimePoint>::max();
        for (int i = 0; i < cycles_number;) {
            if (Measure<Code>(start, end, std::forward<Code>(code)) && end > start) {
                min_latency = std::min(min_latency, end - start);
                i++;
            }
        }
        return min_latency;
    }

    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    TimePoint TSCBenchmarking<CheckCpuMigration, BarrierType>::MeasureStabilizedMinLatency(
            std::size_t cycles_number,
            std::size_t stabilized_threshold,
            Code&& code) {
        TimePoint start, end;
        TimePoint min_latency = std::numeric_limits<TimePoint>::max();
        int min_latency_cycles_number = 0;
        for (int i = 0; i < cycles_number && min_latency_cycles_number < stabilized_threshold;) {
            if (Measure<Code>(start, end, std::forward<Code>(code)) && end > start) {
                TimePoint latency = end - start;

                if (latency < min_latency) {
                    min_latency = latency;
                    min_latency_cycles_number = 0;
                }

                min_latency_cycles_number++;
                i++;
            }
        }
        return min_latency;
    }


    template<bool CheckCpuMigration, Barrier BarrierType>
    template<typename Code>
    bool TSCBenchmarking<CheckCpuMigration, BarrierType>::Measure(TimePoint& start, TimePoint& end, Code&& code) {
        if constexpr (CheckCpuMigration) {
            uint32_t start_cpu_number{0}, end_spu_number{1};
            start = clock_.StartTime(start_cpu_number);
            code();
            end = clock_.EndTime(end_spu_number);
            return start_cpu_number == end_spu_number;
        } else {
            start = clock_.StartTime();
            code();
            end = clock_.EndTime();
            return true;
        }
    }

} // End of namespace benchmarking

#endif //TSC_BENCHMARK_TSC_BENCHMARK_H
