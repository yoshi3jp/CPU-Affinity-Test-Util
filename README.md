# CPU-Affinity-Test-Util

A lightweight, C utility designed to take the guesswork out of CPU affinity and core pinning.

In modern multi-core environments—especially those using Docker, Kubernetes, or `taskset`—a process might physically "see" 64 cores but only be allowed to run on 2 of them. Standard pinning functions often fail silently or crash when you try to pin to a restricted or non-existent core. This utility provides a "checked" approach to ensure your threads stay exactly where they belong.

## 🏗️ Building the Test Utility

To build the test:

```bash
gcc -O2 test_affinity_loop.c -o cpu_test

```

### Running the Test

**Normal run (detects all system cores):**

```bash
./cpu_test

```

**Restricted run (simulates a constrained environment like a container):**

```bash
# Force the program to only see cores 0 and 2
taskset -c 0,2 ./cpu_test

```
