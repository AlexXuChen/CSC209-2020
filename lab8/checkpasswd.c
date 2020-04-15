#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];

  /* The user will type in a user name on one line followed by a password 
     on the next.
     DO NOT add any prompts.  The only output of this program will be one 
	 of the messages defined above.
   */

  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  
  // TODO
  // File descriptor
  int fd[2];
  if (pipe(fd) == -1) {
	perror("pipe");
	exit(1);
  }
  // Make child process for executing validate
  int n = fork();
  if (n < 0) {
	perror("fork");
	  exit(1);
  }
  
  if (n == 0) {
	// Close unnecessary fd channel
	if (close(fd[1]) == -1) {
		perror("close write in child");
		exit(1);
	}
	if (dup2(fd[0], STDIN_FILENO) == -1) {
		perror("dup2 read in child");
		exit(1);
	}
	if (execl("./validate", "validate", NULL) == -1) {
		perror("execl in child");
		exit(1);
	}
	// Close fd channel when finished
	if (close(fd[0]) == -1) {
		perror("close read in child");
		exit(1);
	}
  } else if (n > 0) {
	// Close unnecessary fd channel
	if (close(fd[0]) == -1) {
		perror("close read in parent");
		exit(1);
	}
	if (write(fd[1], user_id, 10) == -1) {
		perror("write user_id in parent");
		exit(1);
	}
	if (write(fd[1], password, 10) == -1) {
		perror("write password in parent");
		exit(1);
	}
  }
  int status;
  if (wait(&status) == -1) {
	perror("wait");
	exit(1);
  }
  
  if (WIFEXITED(status)) {
	int exit_status = WEXITSTATUS(status);
	if (exit_status == 0) {
		fprintf(stdout, SUCCESS);
	} else if (exit_status == 2) {
		fprintf(stdout, INVALID);
	} else if (exit_status == 3) {
		fprintf(stdout, NO_USER);
	} 
  }
	// Close fd channel when finished
  	if (close(fd[1]) == -1) {
		perror("close write in parent");
		exit(1);
	}
  
  return 0;
}