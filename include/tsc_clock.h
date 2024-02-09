#ifndef TSC_BENCHMARK_BARRIER_H
#define TSC_BENCHMARK_BARRIER_H

#include "tsc_cpu.h"
#include "utils/compiler.h"
#include "utils/types.h"

namespace benchmarking {

    enum class Barrier {
        kOneCpuId, // Default realisation
        kLFence, kMFence,
        kRdtscp,
        kTwoCpuId
    };

    template<Barrier BarrierType = Barrier::kOneCpuId>
    class TSCClock {
    public:
        FORCE_INLINE TimePoint StartTime() {
            details::CpuId();
            return details::Rdtsc();
        }

        FORCE_INLINE TimePoint StartTime(uint32_t& cpu_number) {
            details::CpuId();
            return details::Rdtscp(cpu_number);
        }

        FORCE_INLINE TimePoint EndTime() {
            details::CpuId();
            return details::Rdtsc();
        }

        FORCE_INLINE TimePoint EndTime(uint32_t& cpu_number) {
            details::CpuId();
            return details::Rdtscp(cpu_number);
        }
    };

    template<>
    class TSCClock<Barrier::kLFence> {
    public:
        FORCE_INLINE TimePoint EndTime() {
            details::LFence();
            TimePoint time = details::Rdtsc();
            details::CpuId();
            return time;
        }

        FORCE_INLINE TimePoint EndTime(uint32_t& cpu_number) {
            details::LFence();
            TimePoint time = details::Rdtscp(cpu_number);
            details::CpuId();
            return time;
        }
    };

    template<>
    class TSCClock<Barrier::kMFence> {
    public:
        FORCE_INLINE TimePoint EndTime() {
            details::CpuId();
            TimePoint time = details::Rdtsc();
            details::MFence();
            return time;
        }

        FORCE_INLINE TimePoint EndTime(uint32_t& cpu_number) {
            details::CpuId();
            TimePoint time = details::Rdtscp(cpu_number);
            details::MFence();
            return time;
        }
    };

    template<>
    class TSCClock<Barrier::kRdtscp> {
    public:
        FORCE_INLINE TimePoint EndTime() {
            TimePoint time = details::Rdtscp();
            details::CpuId();
            return time;
        }

        FORCE_INLINE TimePoint EndTime(uint32_t& cpu_number) {
            TimePoint time = details::Rdtscp(cpu_number);
            details::CpuId();
            return time;
        }
    };

    template<>
    class TSCClock<Barrier::kTwoCpuId> {
    public:
        FORCE_INLINE TimePoint StartTime() {
            return Now();
        }

        FORCE_INLINE TimePoint StartTime(uint32_t& cpu_number) {
            return Now(cpu_number);
        }

        FORCE_INLINE TimePoint EndTime() {
            return Now();
        }

        FORCE_INLINE TimePoint EndTime(uint32_t& cpu_number) {
            return Now(cpu_number);
        }

    private:
        FORCE_INLINE TimePoint Now();
        FORCE_INLINE TimePoint Now(uint32_t& cpu_number);
    };

    TimePoint TSCClock<Barrier::kTwoCpuId>::Now() {
        details::CpuId();
        TimePoint time = details::Rdtsc();
        details::CpuId();
        return time;
    }

    TimePoint TSCClock<Barrier::kTwoCpuId>::Now(uint32_t& cpu_number) {
        details::CpuId();
        TimePoint time = details::Rdtscp(cpu_number);
        details::CpuId();
        return time;
    }


} // End of namespace benchmarking

#endif //TSC_BENCHMARK_BARRIER_H
