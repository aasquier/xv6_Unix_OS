#ifdef CS333_P2
#include "types.h"
#include "user.h"
#include "uproc.h"

int
main(int argc, char ** argv)
{
	int max = 5;   // Default value mor getprocs() max will be 5

	if(argc > 1)
		max = atoi(argv[1]);
	printf(1, "%d  %d\n\n", argc, max);

	struct uproc* table = malloc(sizeof(struct uproc) * max);

	int size = getprocs(max, table);

	if(size <= 0)
		printf(2, "There was an error getting processes\n");
	else{
		printf(1, "\nPID\tName\tUID\tGID\tPPID\tElapsed\t  CPU\t\tState\tSize\n");
		for(int i = 0; i < size; i++){
			printf(1, "%d\t%s\t%d\t%d\t%d\t", table[i].pid, table[i].name, table[i].uid, table[i].gid, table[i].ppid);

			uint elapsed_secs = table[i].elapsed_ticks / 1000;
			uint elapsed_mils = table[i].elapsed_ticks % 1000;

			if(elapsed_mils < 1000 && elapsed_mils > 99)
				printf(1, "%d.%d\t  ", elapsed_secs, elapsed_mils);
			else if(elapsed_mils < 100 && elapsed_mils > 9)
				printf(1, "%d.0%d\t  ", elapsed_secs, elapsed_mils);
			else
				printf(1, "%d.00%d\t  ", elapsed_secs, elapsed_mils);

			uint CPU_secs     = table[i].CPU_total_ticks / 1000;
			uint CPU_mils     = table[i].CPU_total_ticks % 1000;

			if(CPU_mils < 1000 && CPU_mils > 99)
				printf(1, "%d.%d\t\t", CPU_secs, CPU_mils);
			else if(CPU_mils < 100 && CPU_mils > 9)
				printf(1, "%d.0%d\t\t", CPU_secs, CPU_mils);
			else
				printf(1, "%d.00%d\t\t", CPU_secs, CPU_mils);

			printf(1, "%s\t%d\t\n", table[i].state, table[i].size);
		}
	}

	free(table);

  exit();
}
#endif
