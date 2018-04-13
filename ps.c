#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"
#define MAX 5

int
main(void)
{
	struct uproc* utable = malloc(sizeof(struct uproc) * MAX);
	printf(1, "%xn", utable);

	int size = getprocs(MAX, utable);
	printf(1, "%d\n", size);
	if(size <= 0)
		printf(2, "Get Proc has failed\n");
	else{
		printf(1, "\nPID\tName\tUID\tGID\tPPID\tElapsed\t  CPU\t\tState\tSize\n");
		for(int i = 0; i < size; i++){
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
