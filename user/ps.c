// ps.c - Display process information (like ps command in Linux)
// Shows all active processes with their PID, PPID, state, priority, and name

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

// Convert process state number to string
// States: 0=UNUSED, 1=USED, 2=SLEEPING, 3=RUNNABLE, 4=RUNNING, 5=ZOMBIE
const char*
state_to_string(int state)
{
  switch(state) {
    case 0: return "unused";
    case 1: return "used  ";
    case 2: return "sleep ";
    case 3: return "runble";
    case 4: return "run   ";
    case 5: return "zombie";
    default: return "???   ";
  }
}

int
main(int argc, char *argv[])
{
  struct pstat ps;
  int i;

  // Call getpinfo system call to get process information
  if(getpinfo(&ps) < 0) {
    fprintf(2, "ps: getpinfo failed\n");
    exit(1);
  }

  // Print header
  printf("PID\tPPID\tSTATE\tPRI\tTICKS\tTOTAL\tWAIT\tSIZE\tNAME\n");
  printf("---\t----\t-----\t---\t-----\t-----\t----\t----\t----\n");

  // Print information for each active process
  for(i = 0; i < NPROC; i++) {
    if(ps.inuse[i]) {
      printf("%d\t%d\t%s\t%d\t%d\t%d\t%d\t%d\t%s\n",
             ps.pid[i],
             ps.ppid[i],
             state_to_string(ps.state[i]),
             ps.priority[i],
             ps.ticks_used[i],
             ps.ticks_total[i],
             ps.wait_ticks[i],
             (int)ps.sz[i],
             ps.name[i]);
    }
  }

  exit(0);
}
