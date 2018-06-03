#ifdef CS333_P5
#include "types.h"
#include "user.h"
int
main(int argc, char **argv)
{
  int rc;
  int mode;
  char *md;

  if(argc == 3){
    if(strlen(argv[1]) > 4){
      printf(2, "Too many numbers for mode.\n\n");
      exit();
    }
    md = argv[1];
    while(*md != '\0'){
      if(*md == '+' || *md == '-'){
        md++;
        continue;
      }
      if(*md < '0' || *md > '7'){
        printf(2, "Invalid Octal Representation.\n\n");
        exit();
      }
      md++;
    }
    mode = atoo(argv[1]);
    if(mode < 0 || mode > 1023){
      printf(2, "Invalid mode.\n\n");
      exit();
    }
    rc = chmod(argv[2], mode);
    if(rc == -1)
      printf(2, "Invalid Filename or Mode.\n\n");
  }
  else
    printf(2, "Incorrect Number of Arguments.\n\n");

  exit();
}

#endif
