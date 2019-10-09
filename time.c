#ifdef CS333_P2
#include "types.h"
#include "user.h"

int
main(int argc, char ** argv)
{
  if(argc < 2)
    printf(1, "ran in 0.000 seconds\n\n");
  else{

    uint in = uptime();
    int ret = fork();
    if (ret == 0){

      exec(argv[1], argv+1);

      printf(2, "FAILED: exec failed to execute %s\n", argv[1]);
      exit();
    }
    else if(ret == -1){
      printf(2, "FAILED: fork failed\n");
    }
    else
      wait();

      uint out = uptime();
      uint elapsed = out - in;
      uint elapsed_secs = elapsed / 1000;
      uint elapsed_mils = elapsed % 1000;

      if((elapsed_mils < 1000) && (elapsed_mils > 99))
        printf(1, "%s ran in %d.%d seconds\n", argv[1], elapsed_secs, elapsed_mils);
      else if((elapsed_mils < 100) && (elapsed_mils > 9))
        printf(1, "%s ran in %d.0%d seconds\n", argv[1], elapsed_secs, elapsed_mils);
      else
        printf(1, "%s ran in %d.00%d seconds\n", argv[1], elapsed_secs, elapsed_mils);
  }

  exit();
}

#endif
