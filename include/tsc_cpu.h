#ifndef TSC_BENCHMARK_CPU_H
#define TSC_BENCHMARK_CPU_H

#include "utils/compiler.h"
#include "utils/types.h"

namespace benchmarking::details {

    FORCE_INLINE Register CombineRegisters(Register low, Register high) {
        return (high << 32) | low;
    }

    FORCE_INLINE TimePoint Rdtsc() {
        TimePoint low, high;
        asm volatile("rdtsc" : "=a"(low), "=d"(high));
        return CombineRegisters(low, high);
    }

    FORCE_INLINE TimePoint Rdtscp() {
        TimePoint low, high;
        asm volatile("rdtscp" : "=a"(low), "=d"(high) :: "%rcx");
        return CombineRegisters(low, high);
    }

    FORCE_INLINE TimePoint Rdtscp(uint32_t& chip_number) {
        TimePoint low, high;
        Register rcx;
        asm volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(rcx));
        chip_number = rcx & 0xFFFFFF;
        return CombineRegisters(low, high);
    }

    FORCE_INLINE TimePoint Rdtscp(uint32_t& chip, uint32_t& core) {
        TimePoint low, high;
        Register rcx;
        asm volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(rcx));
        chip = (rcx & 0xFFF000) >> 12;
        core = rcx & 0xFFF;
        return CombineRegisters(low, high);
    }


    FORCE_INLINE void CpuId() {
        asm volatile("cpuid" ::: "%rax", "%rbx", "%rcx", "%rdx");
    }

    FORCE_INLINE void LFence() {
        asm volatile("lfence" ::: "memory");
    }

    FORCE_INLINE void MFence() {
        asm volatile("mfence" ::: "memory");
    }


    class CpuInfo {
    private:
        using Register = std::uint32_t;

        static const Register kTSCPos = 1u << 4;
        static const Register kRdtscpPos = 1u << 27;
        static const Register kInvariantTSCPos = 1u << 8;

    public:
        CpuInfo();

        CpuInfo(const CpuInfo&) = default;
        CpuInfo(CpuInfo&&) = default;
        CpuInfo& operator=(const CpuInfo&) = default;
        CpuInfo& operator=(CpuInfo&&) = default;

        [[nodiscard]] bool IsTSCEnabled() const;
        [[nodiscard]] bool IsInvariantTSCEnabled() const;
        [[nodiscard]] bool IsRdtscpEnabled() const;

        ~CpuInfo() = default;

    private:
        [[nodiscard]] bool IsEnabled(Register reg, Register mask) const;

        [[nodiscard]] const Register& GetEax() const;
        [[nodiscard]] const Register& GetEbx() const;
        [[nodiscard]] const Register& GetEcx() const;
        [[nodiscard]] const Register& GetEdx() const;

        Register registers_[4]{};
    };

    // Implementation

    CpuInfo::CpuInfo() {
#ifdef _WIN32
        __cpuidex((int *)regs, (int)funcId, (int)subFuncId);

#else
        asm volatile("cpuid" : "=a" (registers_[0]), "=b" (registers_[1]),
                "=c" (registers_[2]), "=d" (registers_[3]));
#endif
    }

    bool CpuInfo::IsTSCEnabled() const {
        return IsEnabled(GetEdx(), kTSCPos);
    }

    bool CpuInfo::IsInvariantTSCEnabled() const {
        return IsEnabled(GetEdx(), kInvariantTSCPos);
    }

    bool CpuInfo::IsRdtscpEnabled() const {
        return IsEnabled(GetEdx(), kRdtscpPos);
    }

    bool CpuInfo::IsEnabled(CpuInfo::Register reg, CpuInfo::Register mask) const {
        return reg & mask;
    }

    const CpuInfo::Register& CpuInfo::GetEax() const {
        return registers_[0];
    }

    const CpuInfo::Register& CpuInfo::GetEbx() const {
        return registers_[1];
    }

    const CpuInfo::Register& CpuInfo::GetEcx() const {
        return registers_[2];
    }

    const CpuInfo::Register& CpuInfo::GetEdx() const {
        return registers_[3];
    }


} // End of namespace benchmarking::details

#endif //TSC_BENCHMARK_CPU_H
