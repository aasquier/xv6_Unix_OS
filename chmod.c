#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  int rc;
  int mode;

  if(argc < 4 && argc > 1){
    mode = atoi(argv[1]);
    if(mode < 0 || mode > 1777){
      printf(2, "Invalid Mode.\n\n");
      exit();
    }
    rc = chmod(argv[2], atoo(argv[1]));
    if(rc == -1)
      printf(2, "Invalid Filename or Mode.\n\n");
  }
  else
    printf(2, "Incorrect Number of Arguments.\n\n");

  exit();
}

#endif
