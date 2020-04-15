#include <stdio.h>
#include <stdlib.h>


void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void update_state(int *board, int num_rows, int num_cols) {
    // TODO: Implement.
	int check_board[num_rows * num_cols];
	for (int i = 0; i < num_rows * num_cols; i++) {
		check_board[i] = board[i];
	}
	for (int i = 0; i < num_rows * num_cols; i++) {
		if (
			(i >= 0 && i < num_cols) || 
			(i >= (num_rows * (num_cols - 1)) && i < num_rows * num_cols) ||
			(i % num_cols == 0) ||
			(i % num_cols == num_cols - 1)
		) {
			// do nothing
		} else {
			int neighbour_count = 0;
			
			if (check_board[i - num_cols - 1] == 1) {
				neighbour_count++;
			}
			if (check_board[i - num_cols] == 1) {
				neighbour_count++;
			}
			if (check_board[i - num_cols + 1] == 1) {
				neighbour_count++;
			}
			if (check_board[i - 1] == 1) {
				neighbour_count++;
			}
			if (check_board[i + 1] == 1) {
				neighbour_count++;
			}
			if (check_board[i + num_cols - 1] == 1) {
				neighbour_count++;
			}
			if (check_board[i + num_cols] == 1) {
				neighbour_count++;
			}
			if (check_board[i + num_cols + 1] == 1) {
				neighbour_count++;
			}
			
			if ((neighbour_count < 2 || neighbour_count > 3) && check_board[i] == 1) {
				board[i] = 0;
			} else if ((neighbour_count == 2 || neighbour_count == 3) && check_board[i] == 0) {
				board[i] = 1;
			}
		}
	}
}

