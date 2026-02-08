// cpu_affinity_init.h
#pragma once
#include <sched.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef CPU_SETSIZE
#define CPU_SETSIZE 1024
#endif

typedef struct {
    cpu_set_t allowed;
    int allowed_ids[CPU_SETSIZE];
    int allowed_count;
    int nproc_online;
} cpu_affinity_info_t;

// Returns 0 on success, -1 on error (errno set)
static inline int cpu_affinity_init(cpu_affinity_info_t* out) {
    if (!out) { errno = EINVAL; return -1; }
    memset(out, 0, sizeof(*out));

    out->nproc_online = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (out->nproc_online < 1) out->nproc_online = 1;

    CPU_ZERO(&out->allowed);
    if (sched_getaffinity(0, sizeof(cpu_set_t), &out->allowed) != 0) {
        // If this fails, you’re in a very odd environment; caller can decide fallback.
        return -1;
    }

    out->allowed_count = 0;
    for (int cpu = 0; cpu < CPU_SETSIZE; cpu++) {
        if (CPU_ISSET(cpu, &out->allowed)) {
            out->allowed_ids[out->allowed_count++] = cpu;
        }
    }

    // Sanity: if allowed_count is 0, treat as error
    if (out->allowed_count == 0) { errno = EINVAL; return -1; }

    return 0;
}

// Utility: check whether a specific CPU id is allowed for this process
static inline int cpu_is_allowed(const cpu_affinity_info_t* info, int cpu) {
    if (!info || cpu < 0 || cpu >= CPU_SETSIZE) return 0;
    return CPU_ISSET(cpu, &info->allowed) ? 1 : 0;
}

// Utility: dump diagnostics
static inline void cpu_affinity_dump(const cpu_affinity_info_t* info) {
    if (!info) return;
    fprintf(stderr, "[cpu] online=%d, allowed_count=%d, allowed={",
            info->nproc_online, info->allowed_count);
    for (int i = 0; i < info->allowed_count; i++) {
        fprintf(stderr, "%s%d", (i ? "," : ""), info->allowed_ids[i]);
    }
    fprintf(stderr, "}\n");
}

// Guarded pin: refuses invalid/unavailable CPUs with a clear message
static inline int pin_on_cpu_checked(int cpu, const cpu_affinity_info_t* info) {
    if (!info) { errno = EINVAL; return -1; }
    if (!cpu_is_allowed(info, cpu)) {
        errno = EINVAL;
        fprintf(stderr, "[cpu] pin request cpu=%d rejected (not in allowed set)\n", cpu);
        return -1;
    }

    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(cpu, &set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) != 0) {
        fprintf(stderr, "[cpu] sched_setaffinity(cpu=%d) failed: %s\n",
                cpu, strerror(errno));
        return -1;
    }
    return 0;
}
