// Process statistics structure for getpinfo system call
// Contains information about all active processes for MLFQ scheduler

#ifndef _PSTAT_H_
#define _PSTAT_H_

#include "param.h"

// Number of priority levels in MLFQ
#define MLFQ_LEVELS 4

// Wait threshold multiplier for priority boost (10 times the time slice)
#define BOOST_MULTIPLIER 10

// Structure to hold process information
struct pstat {
  int inuse[NPROC];            // Whether this slot is in use
  int pid[NPROC];              // Process ID
  int ppid[NPROC];             // Parent process ID
  int state[NPROC];            // Process state
  int priority[NPROC];         // Current priority level (0-3)
  int ticks_used[NPROC];       // Ticks used in current time slice
  int ticks_total[NPROC];      // Total ticks used by process
  int wait_ticks[NPROC];       // Ticks spent waiting (for boost mechanism)
  uint64 sz[NPROC];            // Size of process memory (bytes)
  char name[NPROC][16];        // Process name
};

#endif // _PSTAT_H_
