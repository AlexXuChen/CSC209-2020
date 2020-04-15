#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }

    // TODO: Implement.
	int position = strtol(argv[1], NULL, 10);
	int tally_list[BASE];
	for (int i = 0; i < BASE; i++) {
		tally_list[i] = 0;
	}
	int num;
	if (argc == 2) {
		while (fscanf(stdin, "%d", &num) != EOF) {
			add_to_tally(num, position, tally_list);
		}
	} else {
	        FILE *count_file;
		count_file = fopen(argv[2], "r");
		if (count_file == NULL) {
			return 1;
		}
 		while (fscanf(count_file, "%d", &num) == 1) {
			add_to_tally(num, position, tally_list);
		}
		if (fclose(count_file) != 0) {
			return 1;
		}
	}
	for (int i = 0; i < BASE; i++) {
		printf("%ds: %d\n", i, tally_list[i]);
	}
	return 0;
}

