#ifndef TEST_H
#define TEST_H

#include <stdint.h>

// Unified test interface header
// All test functions follow the pattern: uint64_t test_name(uint64_t argc, char *argv[])

uint64_t test_mm(uint64_t argc, char *argv[]);
uint64_t test_processes(uint64_t argc, char *argv[]);
uint64_t test_prio(uint64_t argc, char *argv[]);
uint64_t test_semaphore(uint64_t argc, char *argv[]);

#endif // TEST_H
