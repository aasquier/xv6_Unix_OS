#ifdef CS333_P2
#include "types.h"
#include "user.h"
#define MAX 5
struct uproc;

int
main(void)
{
	struct uproc* utable = malloc(sizeof(struct uproc*) * MAX);

	int size = getprocs(MAX, utable);
	if(size < 0)
		printf(0, "Get Proc has failed");
	else{
		printf(0, "\nPID\tName\tUID\tGID\tPPID\tElapsed\t  CPU\t\tState\tSize\n");
	}
  exit();
}
#endif
