#pragma once

// ---------------------------------------------------------------------------
// Central emulator configuration
// ---------------------------------------------------------------------------

// 1 = every process writes its print commands to logs/<process>.txt
//     (needed ONLY for the recorded homework test case).
// 0 = no text files are created  -> set this to 0 for the machine project
//     submission, because file I/O hurts scheduler performance.
#define ENABLE_PRINT_LOGS 1

constexpr int NUM_CORES           = 4;    // required by the assignment
constexpr int NUM_TEST_PROCESSES  = 10;   // test case: 10 processes
constexpr int PRINTS_PER_PROCESS  = 100;  // test case: 100 print commands each

// Simulated time one instruction takes on a core. 100 ms -> each process runs
// ~10 s, so the whole test lasts ~30 s and "screen -ls" can be sampled many times.
constexpr int INSTRUCTION_DELAY_MS = 100;
