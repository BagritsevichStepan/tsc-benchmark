#ifndef TSC_BENCHMARK_AFFINITY_H
#define TSC_BENCHMARK_AFFINITY_H

#include <iostream>
#include <sched.h>

namespace benchmarking::details {

    void PinThread(int cpu) {
        if (cpu < 0) {
            std::cerr << "The CPU core number must be greater than or equal to 0" << std::endl;
            return;
        }

        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(cpu, &cpu_set);
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set)) {
            std::cerr << "Failed during pthread_setaffinity_np" << std::endl;
        }
    }

}

#endif //TSC_BENCHMARK_AFFINITY_H
