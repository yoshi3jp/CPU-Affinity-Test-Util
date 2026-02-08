#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <errno.h>
#include "cpu_affinity_init.h"

int main() {
    printf("=== CPU Affinity Test Util ===\n\n");

    // 1. Initialize
    cpu_affinity_info_t info;
    if (cpu_affinity_init(&info) != 0) {
        fprintf(stderr, "Init failed\n");
        return EXIT_FAILURE;
    }

    printf("Diagnostics:\n");
    cpu_affinity_dump(&info);
    printf("-------------------------------------------------------------\n");
    printf("| CPU | Allowed? | Request Pin | Result      | Actual Core  |\n");
    printf("|-----|----------|-------------|-------------|--------------|\n");

    // 2. Loop Strategy: 
    // Go from 0 up to (nproc_online + 2). 
    // This ensures we cover all valid CPUs and push into the invalid range.
    int limit = info.nproc_online + 2;

    for (int cpu = 0; cpu < limit; cpu++) {
        // Reset errno for clean reporting
        errno = 0;

        // Check if our logic expects this to work
        int expected_allowed = cpu_is_allowed(&info, cpu);
        
        // Attempt the pin
        int res = pin_on_cpu_checked(cpu, &info);
        int current_actual_cpu = sched_getcpu();

        // Formatting the output row
        printf("| %3d | %-8s | ", cpu, expected_allowed ? "YES" : "NO ");

        if (res == 0) {
            // Pin function returned Success
            if (expected_allowed) {
                // Determine if the OS actually moved us
                if (current_actual_cpu == cpu) {
                     printf("Success     | OK          | %3d          |\n", current_actual_cpu);
                } else {
                     // This can happen if the OS ignores the request (rare) or hasn't switched yet
                     printf("Success     | DRIFT?      | %3d          |\n", current_actual_cpu);
                }
            } else {
                // DANGER: We pinned to a CPU that should have been disallowed!
                printf("Success     | FAILURE!    | %3d (BAD)    |\n", current_actual_cpu);
            }
        } else {
            // Pin function returned Error (-1)
            if (!expected_allowed) {
                printf("Rejected    | OK (Caught) | %3d          |\n", current_actual_cpu);
            } else {
                // Valid CPU, but failed to pin (system error?)
                printf("Failed      | SYS ERR     | %3d          |\n", current_actual_cpu);
            }
        }
    }
    
    printf("-------------------------------------------------------------\n");
    return EXIT_SUCCESS;
}
