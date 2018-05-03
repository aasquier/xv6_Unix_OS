#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"
#include "param.h"

int
main(int argc, char ** argv)
{
  int max;

  if(argc > 1){
    max = atoi(argv[1]);
    if(max < 1){
      printf(2, "Cannot have a negative maximum value for the ps command.");
      return -1;
    }
  }
  else
    max = NPROC;   // Default max size is 64 unless otherwise specified

  struct uproc* utable = malloc(sizeof(struct uproc) * max);
  if(!utable)
    printf(2, "Malloc failed to allocate user table. (ps.c)");
  int size = getprocs(max, utable);
  if(size < 1)
    printf(2, "There was an error getting processes\n");
  else{
    printf(1, "\nPID\tName\t\tUID\tGID\tPPID\tElapsed\t  CPU\t\tState\tSize\n");
    for(int i = 0; i < size; i++){
      if(strlen(utable[i].name) < 8)
        printf(1, "%d\t%s\t\t%d\t%d\t%d\t", utable[i].pid, utable[i].name, utable[i].uid, utable[i].gid, utable[i].ppid);
      else
        printf(1, "%d\t%s\t%d\t%d\t%d\t", utable[i].pid, utable[i].name, utable[i].uid, utable[i].gid, utable[i].ppid);

      uint elapsed_secs = utable[i].elapsed_ticks / 1000;
      uint elapsed_mils = utable[i].elapsed_ticks % 1000;

      if(elapsed_mils < 1000 && elapsed_mils > 99)
        printf(1, "%d.%d\t  ", elapsed_secs, elapsed_mils);
      else if(elapsed_mils < 100 && elapsed_mils > 9)
        printf(1, "%d.0%d\t  ", elapsed_secs, elapsed_mils);
      else
        printf(1, "%d.00%d\t  ", elapsed_secs, elapsed_mils);

      uint CPU_secs     = utable[i].CPU_total_ticks / 1000;
      uint CPU_mils     = utable[i].CPU_total_ticks % 1000;

      if(CPU_mils < 1000 && CPU_mils > 99)
        printf(1, "%d.%d\t\t", CPU_secs, CPU_mils);
      else if(CPU_mils < 100 && CPU_mils > 9)
        printf(1, "%d.0%d\t\t", CPU_secs, CPU_mils);
      else
        printf(1, "%d.00%d\t\t", CPU_secs, CPU_mils);

      printf(1, "%s\t%d\t\n", utable[i].state, utable[i].size);
    }
  }

  free(utable);

  exit();
}
#endif
