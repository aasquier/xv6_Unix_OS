#ifdef CS333_P3P4
#include "types.h"
#include "user.h"
#include "uproc.h"
#include "param.h"
#define SHORT_WAIT 3
#define LONG_WAIT 10
#define SUPER_WAIT 20
#define TEST1
//#define TEST2S
//#define TEST2R
//#define TEST3
//#define TEST4

#ifdef TEST1
static void
spin(int ms)
{
  int time = uptime();
  while(1)
    if((time + ms) < uptime())
      break;
}

static void
test_1()
{
  int pids[20];
  int count = 0;
  int rc = 0;
  printf(1, "Round-Robin test started...\n");
  memset(&pids, 0, sizeof(pids));
  while(count < 20){
    rc = fork();
    if(rc){
      pids[count] = rc;
      count++;
    }
    else
      break;
  }
  if(rc == 0){
    while(1);
  }
  else{
    printf(1, "Press control-r to show Round-Robin. Sleeping for %d seconds.\n", SUPER_WAIT);
    spin(SUPER_WAIT * 1000);
    printf(1, "Cleaning up children...\n");
    count = 0;
    while(count < 20){
      kill(pids[count++]);
      wait();
    }
    printf(1, "Test concluded.\n");
  }
}
#endif

#ifdef TEST2S
static void
test_2S()
{
  int pids[4];
  int count = 0;
  int rc = 0;
  printf(1, "Sleeping promotion test started...\n");
  memset(&pids, 0, sizeof(pids));
  while(count < 4){
    rc = fork();
    if(rc){
      pids[count] = rc;
      count++;
    }
    else
      break;
  }
  if(rc == 0){
    sleep(SUPER_WAIT * 10000);
  }
  else{
    printf(1, "Press control-p to show initial state, sleeping for %d seconds.\n", LONG_WAIT);
    sleep(LONG_WAIT * 1000);
    printf(1, "Press control-p to show Sleeping Promotion. Processes priority changed to 3. Sleeping for %d seconds.\n", LONG_WAIT);
    for(int i=0; i < 4; i++){
      setpriority(pids[i], 3);
    }
    setpriority(getpid(), 3);
    sleep(LONG_WAIT * 1000);
    printf(1, "Press control-p to show Sleeping Promotion. Processes priority changed to 5. Sleeping for %d seconds.\n", LONG_WAIT);
    for(int i=0; i < 4; i++){
      setpriority(pids[i], 5);
    }
    setpriority(getpid(), 5);
    sleep(LONG_WAIT * 1000);
    printf(1, "Cleaning up children...\n");
    count = 0;
    while(count < 6){
      kill(pids[count++]);
      wait();
    }
    printf(1, "Test concluded.\n");
  }
}
#endif
#ifdef TEST2R
static void
test_2R()
{
  int pids[2];
  int count = 0;
  int rc = 0;
  printf(1, "Running promotion test started...\n");
  memset(&pids, 0, sizeof(pids));
  while(count < 2){
    rc = fork();
    if(rc){
      pids[count] = rc;
      count++;
    }
    else
      break;
  }
  if(rc == 0){
    while(1);
  }
  else{
    printf(1, "Press control-p to show initial state, sleeping for 2 seconds.\n");
    sleep(2000);
    printf(1, "Press control-p to show Running Promotion. Processes priority changed to 5. Sleeping for %d seconds.\n", LONG_WAIT);
    for(int i=0; i < 2; i++){
      setpriority(pids[i], 5);
    }
    setpriority(getpid(), 5);
    sleep(LONG_WAIT * 1000);
    printf(1, "Press control-p to show Running Promotion. Processes priority changed to 3. Sleeping for %d seconds.\n", LONG_WAIT);
    for(int i=0; i < 2; i++){
      setpriority(pids[i], 5);
    }
    setpriority(getpid(), 5);
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

#ifdef TEST3
static void
spin(int ms)
{
  int time = uptime();
  while(1)
    if((time + ms) < uptime())
      break;
}

static void
test_3()
{
  int pids[6];
  int count = 0;
  int rc = 0;
  printf(1, "Demotion test started...\n");
  memset(&pids, 0, sizeof(pids));
  while(count < 6){
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
    printf(1, "Press control-r to show demotions for %d seconds.\n", 20);
    spin(20000);
    printf(1, "Cleaning up children...\n");
    count = 0;
    while(count < 6){
      kill(pids[count++]);
      wait();
    }
    printf(1, "Test concluded.\n");
  }
}
#endif

#ifdef TEST4
static void
test_4()
{
  int pids[5];
  int count = 0;
  int rc = 0;
  printf(1, "Set-Priority test started...\n");
  memset(&pids, 0, sizeof(pids));
  while(count < 5){
    rc = fork();
    if(rc){
      pids[count] = rc;
      count++;
    }
    else
      break;
  }
  if(rc == 0){
    while(1);
  }
  else{
    int retc;
    printf(1, "Setting priorities to 3 Press control-r. Sleeping for %d seconds.\n", LONG_WAIT);
    for(int i=0; i < 5; i++){
      setpriority(pids[i], 3);
    }
    setpriority(getpid(), 3);
    sleep(LONG_WAIT * 1000);
    printf(1, "Attempting to set priority of out of range pids, 99 and 199.\n\n");

    retc = setpriority(99, 3);
    if(retc == -1)
      printf(1, "Attempted to set a pid (99) of a process that does not exist.\n\n");
    retc = setpriority(199, 3);
    if(retc == -1)
      printf(1, "Attempted to set a pid (199) of a process that does not exist.\n\n");
    printf(1, "MAXPRIO is set to %d.\n", MAXPRIO);
    printf(1, "Attempting to set priority of process 3 to out of bounds values -33, and 33.\n\n");
    retc = setpriority(3, -33);
    if(retc == -1)
      printf(1, "Attempted to set pid 3 to priority -33 failed.\n\n");
    retc = setpriority(3, 33);
    if(retc == -1)
      printf(1, "Attempted to set pid 3 to priority 33 failed.\n\n");
    sleep(3000);
    printf(1, "Cleaning up children...\n");
    count = 0;
    while(count < 5){
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
#ifdef TEST1
  test_1();
#endif
#ifdef TEST2S
  test_2S();
#endif
#ifdef TEST2R
  test_2R();
#endif
#ifdef TEST3
  test_3();
#endif
#ifdef TEST4
  test_4();
#endif
  printf(1, "All tests concluded.\n");
  exit();
}
#endif
