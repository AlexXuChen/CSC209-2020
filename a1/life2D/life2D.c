#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }

    // TODO: Implement.
	int num_rows = strtol(argv[1], NULL, 10);
	int num_cols = strtol(argv[2], NULL, 10);
	int num_states = strtol(argv[3], NULL, 10);
	int board[num_rows * num_cols];
    
	int num;
	int index = 0;
	while (fscanf(stdin, "%d", &num) != EOF) {
		board[index] = num;
		index++;
	}
	for (int i = 0; i < num_states; i++) {
		print_state(board, num_rows, num_cols);
		update_state(board, num_rows, num_cols);
	}
}
