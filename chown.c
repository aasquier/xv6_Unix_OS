#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  int rc;
  int owner;

  if(argc < 4 && argc > 1){
    owner = atoi(argv[1]);
    if(owner < 0 || owner > 32767){
      printf(2, "Invalid User ID.\n\n");
      exit();
    }
    rc = chown(argv[2], owner);
    if(rc == -1)
      printf(2, "Invalid filename or User ID.\n\n");
  }
  else
    printf(2, "Incorrect Number of Arguments.\n\n");

  exit();
}

#endif
