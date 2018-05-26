#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  int rc;

  if(argc < 4 && argc > 1){
    rc = chmod(argv[2], atoo(argv[1]));
    if(rc == -1)
      printf(2, "Invalid filename or User ID.");
  }

  exit();
}

#endif
