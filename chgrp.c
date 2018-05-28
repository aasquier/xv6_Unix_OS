#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  int rc;
  int group;

  if(argc < 4 && argc > 1){
    group = atoi(argv[1]);
    if(group < 0){
      printf(2, "Invalid Group ID.\n\n");
      exit();
    }
    rc = chgrp(argv[2], group);
    if(rc == -1)
      printf(2, "Invalid filename or Group ID.");
  }
  else
    printf(2, "Incorrect Number of Arguments.\n\n");

  exit();
}

#endif
