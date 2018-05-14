#ifdef CS333_P3P4
#include "types.h"
#include "user.h"
#include "uproc.h"
#define SHORT_WAIT 3
#define LONG_WAIT 10
#define SUPER_WAIT 20
#define TEST2
//#define TEST3


#ifdef TEST2
static void
test_2()
{
  int pids[16];
  int count = 0;
  int rc = 0;
  printf(1, "Demotion test started...\n");
  memset(&pids, 0, sizeof(pids));
  while(count < 16){
    rc = fork();
    if(rc){
      pids[count] = rc;
      count++;
    }
    else
      break;
  }
  if(rc == 0){
    if((getpid() % 2) != 0){
      while(1);
    }
    else{
      sleep(1000000);
    }
  }
  else{
    printf(1, "Press control-p to show initial state, sleeping for %d seconds.\n", LONG_WAIT);
    sleep(LONG_WAIT * 1000);
    printf(1, "Press control-p to show Sleep/Running Promotion. Sleeping for %d seconds.\n", SUPER_WAIT);
    for(int i=0; i < 16; i++){
      setpriority(pids[i], 4);
    }
    sleep(SUPER_WAIT * 1000);
    printf(1, "Cleaning up children...\n");
    count = 0;
    while(pids[count] > 0){
      kill(pids[count++]);
      wait();
    }
    printf(1, "Test concluded.\n");
  }
}
#endif

#ifdef TEST3
static void
test_3()
{
  int pids[16];
  int count = 0;
  int rc = 0;
  printf(1, "Demotion test started...\n");
  memset(&pids, 0, sizeof(pids));
  while(count < 16){
    rc = fork();
    if(rc){
      pids[count] = rc;
      count++;
    }
    else
      break;
  }
  if(rc == 0)
    while(1);
  else{
    printf(1, "Press control-r to show demotions for %d seconds.\n", LONG_WAIT);
    sleep(LONG_WAIT * 1000);
    printf(1, "Cleaning up children...\n");
    count = 0;
    while(pids[count] > 0){
      kill(pids[count++]);
      wait();
    }
    printf(1, "Test concluded.\n");
  }
}
#endif


int
main(int argc, char *argv[])
{
#ifdef TEST2
  test_2();
#endif
#ifdef TEST3
  test_3();
#endif
  printf(1, "All tests concluded.\n");
  exit();
}
#endif
