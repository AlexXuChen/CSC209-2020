#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 54347
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: "
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5

#define MSG_LIMIT_ERROR "You have already sent the maximum number of messages\r\n"
#define USER_NOT_FOUND "Invalid username: user not found\r\n"
#define FOLLOWER_LIMIT_EXCEEDED "Follower limit has been exceeded\r\n"
#define FOLLOWING_LIMIT_EXCEEDED "Following limit has been exceeded\r\n"
#define NOT_FOLLOWING_ERROR "You are not following this user\r\n"
#define INVALID_USERNAME "Invalid username\r\n"
#define ALREADY_FOLLOWING "You are already following this user\r\n"

struct client {
    int fd;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions. 
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);

// These are some of the function prototypes that we used in our solution 
// You are not required to write functions that match these prototypes, but
// you may find them helpful when thinking about operations in your program.

// Send the message in s to all clients in active_clients. 
void announce(struct client *active_clients, char *s);

// Move client c from new_clients list to active_clients list. 
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr);

// Find network newline helper
int find_network_newline(const char *buf, int n);

// Read buf helper
int read_from(struct client *c, char *buf_output);

// Check if username is valid
int check_valid_username(struct client *active_clients, char *username);

// User command functions
int follow_user(struct client *c, struct client *active_clients, char *username);
int unfollow_user(struct client *c, struct client *active_clients, char *username);
int show_messages(struct client *c);
int send_message(struct client *c, char *message); 

// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails. 
fd_set allset;

/* 
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));
    p->fd = fd;
    p->ipaddr = addr;
    p->username[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients;
    
    // Initialize followers/following to NULL
    for(int i = 0; i < FOLLOW_LIMIT; i++) {
	p->followers[i] = NULL;
	p->following[i] = NULL;
    }

    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    *clients = p;
}

/* 
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    struct client **p;

    for (p = clients; *p && (*p)->fd != fd; p = &(*p)->next)
        ;

    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        // TODO: Remove the client from other clients' following/followers
        // lists
	    for(int i = 0; i < FOLLOW_LIMIT; i++) {
		    // Remove from following followers
		if ((*p)->following[i] != NULL) {
			unfollow_user(*p, *clients, (*p)->following[i]->username);
		}
		// Remove from follower following
		if ((*p)->followers[i] != NULL) {
			unfollow_user((*p)->following[i], *clients, (*p)->username);
		}
	    }
	    printf("%s has disconnected\n", (*p)->username);

        // Remove the client
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, 
            "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}


int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal. 
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // A list of active clients (who have already entered their names). 
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or 
    // or receive announcements. 
    struct client *new_clients = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);

    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select 
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, 
                    "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_client(&new_clients, clientfd);
            }
        }

        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be 
        // valid.
        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        // TODO: handle input from a new client who has not yet
                        // entered an acceptable name
			    
			char buf_output[BUF_SIZE];
			int r = read_from(p, buf_output);
			// Check if the username entered is valid
			int valid = check_valid_username(active_clients, buf_output);
			if (valid == 0) {
				if (r == 0) {
					// If username and write are valid, activate the new user
					strncpy(p->username, buf_output, strlen(buf_output) + 1);
					activate_client(p, &active_clients, &new_clients);
					char join_msg[strlen(p->username) + 21];
					strncpy(join_msg, p->username, strlen(p->username) + 1);
					strncat(join_msg, " has just joined.\r\n", 20);
					announce(active_clients, join_msg);
				}
			} else { // Remove the unused client
				write(cur_fd, INVALID_USERNAME, strlen(INVALID_USERNAME));
				remove_client(&new_clients, p->fd);
			}
                        handled = 1;
                        break;
                    }
                }

                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {
                            // TODO: handle input from an active client
				
				char buf_output[BUF_SIZE];
				int r = read_from(p, buf_output);
				if (r == 0) {
					if (strncmp(FOLLOW_MSG, buf_output, strlen(FOLLOW_MSG)) == 0) { // follow <username>
						follow_user(p, active_clients, &buf_output[strlen(FOLLOW_MSG) + 1]);
					} else if (strncmp(UNFOLLOW_MSG, buf_output, strlen(UNFOLLOW_MSG)) == 0) { // unfollow <username>
						unfollow_user(p, active_clients, &buf_output[strlen(UNFOLLOW_MSG) + 1]);
					} else if (strncmp(SHOW_MSG, buf_output, strlen(SHOW_MSG)) == 0) { // show
						show_messages(p);
					} else if (strncmp(SEND_MSG, buf_output, strlen(SEND_MSG)) == 0) { // send <message>
						send_message(p, &buf_output[strlen(SEND_MSG) + 1]);
					} else if (strlen(buf_output) == 4 && strncmp("quit", buf_output, 4) == 0) { // quit
						char quit_msg[strlen(p->username) + 11];
						// Create goodbye announcment
						strncpy(quit_msg, "Goodbye ", 9);
						strncat(quit_msg, p->username, strlen(p->username) + 1);
						strncat(quit_msg, "\r\n", 3);
						remove_client(&active_clients, p->fd);
						announce(active_clients, quit_msg);
					} else { // Invalid command
						int w = write(p->fd, "Invalid command\r\n", 17);
						if (w != 17) {
							perror("write invalid command");
							remove_client(&active_clients, p->fd);
						}
					}
				}
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

// Helper functions

// Announce message to all clients and server
void announce(struct client *active_clients, char *s) {
	struct client *curr = active_clients;
	while(curr != NULL) {
		int w = write(curr->fd, s, strlen(s));
		if (w != strlen(s)) {
			perror("write announce");
			// exit(1);
			// return fd;
		}
		curr = curr->next;
	}
	printf("%s", s);
}

// Move a new client to the head of the active clients linked list
void activate_client(struct client *c, struct client **active_clients_ptr, struct client **new_clients_ptr) {	
	// Remove from new clients
	struct client **curr;
	for(curr = new_clients_ptr; *curr && *curr != c; curr = &(*curr)->next);
	*curr = (*curr)->next;
	// Add to active clients
	c->next = *active_clients_ptr;
	*active_clients_ptr = c;
}

// Find the network newline in a buf
int find_network_newline(const char *buf, int n) {
	for (int i = 0; i < n; i++) {
		if (buf[i] == '\r') {
			if (buf[i + 1] == '\n') {
				return i + 2;
			}
		}
	}
    return -1;
}

// Read client input and check for partial read
int read_from(struct client *c, char *buf_output) {
	int nbytes = read(c->fd, c->in_ptr, BUF_SIZE - (c->in_ptr - c->inbuf));
	c->in_ptr += nbytes;
	if (nbytes <= 0) {
		return c->fd;
	} else{
		printf("[%d] Read %d bytes\n", c->fd, nbytes);
	}
	int where = find_network_newline(c->inbuf, c->in_ptr - c->inbuf);
	if (where > 0) {
		c->inbuf[where - 2] = '\0';
		strncpy(buf_output, c->inbuf, BUF_SIZE);
		printf("[%d] Found newline: %s\n", c->fd, buf_output);
		memmove(c->inbuf, c->inbuf + where, BUF_SIZE - where);
		c->in_ptr -= where;
		return 0;
	} else {
		return 1;
	}
}

// Check if a username is amoung active users
int check_valid_username(struct client *active_clients, char *username) {
	if (strlen(username) > 0) {
		struct client *curr;
		for (curr = active_clients; curr != NULL; curr = curr->next) {
			if (strncmp(username, curr->username, strlen(username)) == 0) {
				return 1;
			}
		}
		return 0;
	} else { // empty string
		return 1;
	}
}

// User Commands

// When user issues follow <username>, adds <username> to following and user to followers
int follow_user(struct client *c, struct client *active_clients, char *username) {
	// Check if user is already following username
	int is_following = 0;
	for (int i = 0; i < FOLLOW_LIMIT; i++){
		if (c->following[i] != NULL && strcmp(username, c->following[i]->username) == 0){
			is_following = 1;
			break;
		}
	}
	if (is_following) {
		int w = write(c->fd, ALREADY_FOLLOWING, strlen(ALREADY_FOLLOWING));
		if (w != strlen(ALREADY_FOLLOWING)) {
			perror("write follow_user");
			return 1;
		}
		return 0;
	}
	// Check if the username exists in active_clients
	int found = 0;
	struct client *curr;
	struct client *followee;
	// Seach for the followee in the active_clients
	for(curr = active_clients; curr != NULL; curr = curr->next) {
		if (strcmp(curr->username, username) == 0){
				found = 1;
				followee = curr;
				break;
		}
	}
	if (!found) { // Send user not found prompt to user
		int w = write(c->fd, USER_NOT_FOUND, strlen(USER_NOT_FOUND));
		if (w != strlen(USER_NOT_FOUND)) {
			perror("write follow_user");
			return 1;
		}
		return 0;
	}else { 
		// Check if the follow limit has been exceeded for both user following and followee follower lists 
		if (c->followers[FOLLOW_LIMIT - 1] != NULL) { // Check limit of user following list
			int w = write(c->fd, FOLLOWER_LIMIT_EXCEEDED, strlen(FOLLOWER_LIMIT_EXCEEDED));
			if (w != strlen(FOLLOWER_LIMIT_EXCEEDED)) {
				perror("write follow_user");
				return 1;
			}
			return 0;
		}else if (followee->following[FOLLOW_LIMIT - 1] != NULL) { // Check limit of followee follower list
			int w = write(c->fd, FOLLOWING_LIMIT_EXCEEDED, strlen(FOLLOWING_LIMIT_EXCEEDED));
			if (w != strlen(FOLLOWING_LIMIT_EXCEEDED)) {
				perror("write follow_user");
				return 1;
			}
			return 0;
		} else {
			// Add the user to the next empty space in the followers list of the other user
			// Add to follower list
			for(int i = 0; i < FOLLOW_LIMIT; i++) {
				if (c->following[i] == NULL) {
					c->following[i] = followee;
					printf("%s is following %s\n", c->username, username);
					break;
				}
			}
			// Add to following list
			for(int i = 0; i < FOLLOW_LIMIT; i++) {
				if (followee->followers[i] == NULL) {
					followee->followers[i] = c;
					printf("%s has %s as a follower\n", username, c->username);
					break;
				}
			}
			return 0;
		}
	}
}

// When user issues unfollow <username>, remove <username> from following and user from followers
int unfollow_user(struct client *c, struct client *active_clients, char *username) {
	// Check if user is not following username
	int is_not_following = 1;
	for (int i = 0; i < FOLLOW_LIMIT; i++){
		if (c->following[i] != NULL && strcmp(username, c->following[i]->username) == 0){
			is_not_following = 0;
			break;
		}
	}
	if (is_not_following) { // Send prompt to user if followee is not found
		int w = write(c->fd, NOT_FOLLOWING_ERROR, strlen(NOT_FOLLOWING_ERROR));
		if (w != strlen(NOT_FOLLOWING_ERROR)) {
			perror("write unfollow_user");
			return 1;
		}
		return 0;
	}
	// Check if the username exists in active_clients
	int found = 0;
	struct client *curr;
	struct client *followee;
	// Seach for the followee in the active_clients
	for(curr = active_clients; curr != NULL; curr = curr->next) {
		if (strcmp(curr->username, username) == 0){
				found = 1;
				followee = curr;
				break;
		}
	}
	if (!found) { // Send user not found prompt to user
		int w = write(c->fd, USER_NOT_FOUND, strlen(USER_NOT_FOUND));
		if (w != strlen(USER_NOT_FOUND)) {
			perror("write follow_user");
			return 1;
		}
		return 0;
	}else { 
		// Remove from user following list
		for(int i = 0; i < FOLLOW_LIMIT; i++) {
			if (followee->followers[i] != NULL && strncmp(c->username, (followee->followers[i])->username, BUF_SIZE) == 0) {
				followee->followers[i] = NULL;
				printf("%s no longer has %s as a follower\n", followee->username, c->username);
				// Remove from followee follower list
				for(int j = 0; j < FOLLOW_LIMIT; j++) {
					if (c->following[j] != NULL && strncmp((c->following[j])->username, followee->username, BUF_SIZE) == 0) {
						c->following[j] = NULL;
						printf("%s unfollows %s\n", c->username, username);
						return 0;
					}
				}
			}
		}
		int w = write(c->fd, NOT_FOLLOWING_ERROR, strlen(NOT_FOLLOWING_ERROR));
		if (w != strlen(NOT_FOLLOWING_ERROR)) {
			perror("write unfollow_user");
			return 1;
		}
		return 0;
	}
}

// When user issues show, display all the messages from followed users
int show_messages(struct client *c) {
	for(int i = 0; i < FOLLOW_LIMIT; i++) {
		if (c->following[i] != NULL) {
			// Create tag "<username> wrote: "
			char username_tag[strlen(c->following[i]->username) + 9];
			strncpy(username_tag, c->following[i]->username, strlen(c->following[i]->username)+1);
			strncat(username_tag, " wrote: ", 9);
			// Send all messages to followers
			for(int j = 0; j < MSG_LIMIT; j++) {
				if (strlen(c->following[i]->message[j]) > 0) {
					char prev_message[strlen(username_tag) + strlen(c->following[i]->message[j]) + 4];
					strncpy(prev_message, username_tag, strlen(username_tag) + 1);
					strncat(prev_message, c->following[i]->message[j], strlen(c->following[i]->message[j])+1);
					strncat(prev_message, "\r\n", 3);
					int w = write(c->fd, prev_message, strlen(prev_message));
					if (w != strlen(prev_message)) {
						perror("write show_messages");
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

// When user issues send <message>, display the messages to all following users
int send_message(struct client *c, char *message) {
	// Add message to message list
	int i;
	for(i = 0; strlen(c->message[i]) > 0 && i < MSG_LIMIT; i++);
	if(i != MSG_LIMIT) {
		strncpy(c->message[i], message, strlen(message)+1);
	} else {
		int w = write(c->fd, MSG_LIMIT_ERROR, strlen(MSG_LIMIT_ERROR));
		if (w != strlen(MSG_LIMIT_ERROR)) {
			perror("write send_message");
			return 1;
		}
		return 0;
	}
	char sent_message[strlen(c->username) + strlen(message) + 5];
	// Add username to message
	strncpy(sent_message, c->username, strlen(c->username) + 1);
	strncat(sent_message, ": ", 3);
	strncat(sent_message, message, strlen(message) + 1);
	strncat(sent_message, "\r\n", 3);
	// Send message to followers
	for(int i = 0; i < FOLLOW_LIMIT; i++) {
		if (c->followers[i] != NULL) {
			int w = write(c->followers[i]->fd, sent_message, strlen(sent_message));
			if (w != strlen(sent_message)) {
				perror("write send_message");
				return 1;
			}
		}
	}
	return 0;
}
