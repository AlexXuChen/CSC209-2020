#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    // TODO: Implement.
	int num_of_digits = 0;
	while (num/BASE != 0){
		num = num/BASE;
		num_of_digits++;
	}
	num_of_digits++;
    return num_of_digits;
}

int get_ith_from_right(int num, int i) {
    // TODO: Implement.
	for (int j = 0; j < i; j++) {
		num = num/BASE;
	}
    return num % BASE;
}

int get_ith_from_left(int num, int i) {
    // TODO: Implement.
	int digits = count_digits(num) - 1;
	int result = get_ith_from_right(num, digits - i);
    return result;
}

void add_to_tally(int num, int i, int *tally) {
    // TODO: Implement.
	int ith_digit = get_ith_from_left(num, i);
	tally[ith_digit]++;
}
