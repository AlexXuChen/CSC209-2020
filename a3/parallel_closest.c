#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"


/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
	if (n < 4 || pdmax == 0) {
		// Base case
		return closest_serial(p, n);
	} else {
		// Spilt p into 2 lists
		
		// Left list
		int left_size = n/2;
		struct Point left[left_size];
		for (int i = 0; i < left_size; i++) {
			left[i] = p[i];
		}
		
		// Right list
		int right_size = n - (left_size);
		struct Point right[right_size];
		for (int j = left_size; j < n; j++) {
			right[j - left_size] = p[j];
		}
		
		// Operating on left list
		int fd_left[2];
		if (pipe(fd_left) == -1) {
			perror("pipe left");
			exit(1);
		}
		int child_left;
		// Close read channel in child
		if ((child_left = fork()) == -1) {
			perror("fork left");
			exit(1);
		}
		if (child_left == 0) {
			if (close(fd_left[0]) == -1) {
				perror("close left read in child");
				exit(1);
			}
			// Recursive call following divide and conquer
			double distance_left = closest_parallel(left, left_size, pdmax - 1, pcount);
			if (write(fd_left[1], &distance_left, sizeof(double)) == -1) {
				perror("write left to parent");
				exit(1);
			}
			if (close(fd_left[1]) == -1) {
				perror("close left write in child");
				exit(1);
			}
			// This process should be added to pcount
			exit(*pcount + 1);
		}
		
		// Operating on right list
		int fd_right[2];
		if (pipe(fd_right) == -1) {
			perror("pipe left");
			exit(1);
		}
		int child_right;
		if ((child_right = fork()) == -1) {
			perror("fork right");
			exit(1);
		}
		if (child_right == 0) {
			// Close read channel in child
			if (close(fd_right[0]) == -1) {
				perror("close right read in child");
				exit(1);
			}
			// Recursive call following divide and conquer
			double distance_right = closest_parallel(right, right_size, pdmax - 1, pcount);
			if (write(fd_right[1], &distance_right, sizeof(double)) == -1) {
				perror("write right to parent");
				exit(1);
			}
			if (close(fd_right[1]) == -1) {
				perror("close right write in child");
				exit(1);
			}
			// This process should be added to pcount
			exit(*pcount + 1);
		}
		
		// Wait for both processes to terminate 
		// This does not follow any order, hence cannot be named in terms of left and right
		int status_one;
		if (wait(&status_one) == -1) {
			perror("wait one");
			exit(1);
		}
		int first_processes;
		if (WIFEXITED(status_one)) {
			first_processes = WEXITSTATUS(status_one);
		}
		
		int status_two;
		if (wait(&status_two) == -1) {
			perror("wait two");
			exit(1);
		}
		int second_processes;
		if (WIFEXITED(status_two)) {
			second_processes = WEXITSTATUS(status_two);
		}
		
		// Total processes below current node
		*pcount = first_processes + second_processes;
		
		// Read from two pipes
		double result_left;
		if (read(fd_left[0], &result_left, sizeof(double)) != sizeof(double)) {
			perror("read left child");
			exit(1);
		}
		
		double result_right;
		if (read(fd_right[0], &result_right, sizeof(double)) != sizeof(double)) {
			perror("read right child");
			exit(1);
		}
		
		// Compare left closest and right closest
		double d = min(result_left, result_right);
		
		// Code from serial_closest.c
		int mid = n / 2;
		struct Point mid_point = p[mid];
		struct Point *strip = malloc(sizeof(struct Point) * n);
		if (strip == NULL) {
			perror("malloc");
			exit(1);
		}

		int j = 0;
		for (int i = 0; i < n; i++) {
			if (abs(p[i].x - mid_point.x) < d) {
			    strip[j] = p[i], j++;
			}
		}

		// Find the closest points in strip.  Return the minimum of d and closest distance in strip[].
		double new_min = min(d, strip_closest(strip, j, d));
		free(strip);
		
		return new_min;
	}
}

