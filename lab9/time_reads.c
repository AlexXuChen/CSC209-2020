/* The purpose of this program is to practice writing signal handling
 * functions and observing the behaviour of signals.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

/* Message to print in the signal handling function. */
#define MESSAGE "%ld reads were done in %ld seconds.\n"

/* Global variables to store number of read operations and seconds elapsed. 
 */
long num_reads, seconds;


/* The first command-line argument is the number of seconds to set a timer to run.
 * The second argument is the name of a binary file containing 100 ints.
 * Assume both of these arguments are correct.
 */

void handler(int sig){
  printf("%ld reads were done in %ld seconds.\n", num_reads, seconds);
  exit(1);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: time_reads s filename\n");
        exit(1);
    }
    seconds = strtol(argv[1], NULL, 10);

    FILE *fp;
    if ((fp = fopen(argv[2], "r")) == NULL) {
      perror("fopen");
      exit(1);
    }

    /* In an infinite loop, read an int from a random location in the file,
     * and print it to stderr.
     */

    // Declare a struct to be used by the sigaction function:
    struct sigaction newact;

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = seconds;
	// and every 250 msec after that
   timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = seconds;

    // Specify that we want the handler function to handle the signal:
    newact.sa_handler = handler;

    // Use default flags:
    newact.sa_flags = 0;

    // Specify that we don't want any signals to be blocked during execution of handler:
    sigemptyset(&newact.sa_mask);

    // Modify the signal table so that handler is called when signal SIGINT is received:
    sigaction(SIGPROF, &newact, NULL);
    setitimer (ITIMER_PROF, &timer, NULL);

    num_reads = 0;
    while(1){
      num_reads++;
      int r = rand()%100 * sizeof(int);
      fseek(fp, r, SEEK_SET);
      int i;
      fread(&i, sizeof(int), 1, fp);
      printf("%d\n", i);
    }

    return 1; // something is wrong if we ever get here!
}