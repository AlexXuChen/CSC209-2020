#include <stdio.h>
// Add your system includes here.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ftree.h"

struct TreeNode *create_node(const char *, char *);
/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname) {

    // Your implementation here.

    // Hint: consider implementing a recursive helper function that
    // takes fname and a path.  For the initial call on the 
    // helper function, the path would be "", since fname is the root
    // of the FTree.  For files at other depths, the path would be the
    // file path from the root to that file.

	//printf("%s", fname);
	struct stat stat_buf;
	if (lstat(fname, &stat_buf) == -1) {
		fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
		return NULL;
	} else {
		struct TreeNode *t = create_node(fname, "");
		t->next = NULL;
		return t;
	}
}

struct TreeNode *create_node(const char *fname, char *path) {	
	// New path creation
	char new_path[strlen(fname) + strlen(path) + 2];
	strcpy(new_path, path);
	strcat(new_path, fname);
	
	// Decide file type
	struct stat stat_buf;
	if (lstat(new_path, &stat_buf) == -1) {
		fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
		return NULL;
	}
	
	// Create the TreeNode
	struct TreeNode *t = malloc(sizeof(struct TreeNode));
	if (t == NULL) {
		perror("malloc");
		exit(1);
	}
	
	t->contents = NULL;
	t->next = NULL;
	
	if (S_ISREG(stat_buf.st_mode)) {
		t->type = '-';
	} else if (S_ISLNK(stat_buf.st_mode)) {
		t->type = 'l';
	}
	else if (S_ISDIR(stat_buf.st_mode)){
		t->type = 'd';
		
		DIR *d_ptr = opendir(new_path);
		// Open directory error check
		if (d_ptr == NULL) {
			perror("opendir");
			exit(1);
		}
		
		// Get entry point
		struct dirent *entry_ptr;
		entry_ptr = readdir(d_ptr);
		
		if (entry_ptr == NULL) {
			t->contents = NULL;
		} else {
			// Add / to new path
			strcat(new_path, "/");
			// Determine if the root node is being created
			bool made_contents = false;
			struct TreeNode  *curr;
			while (entry_ptr != NULL) {
				// Ignore . files and . and .. folders 
				if (!(entry_ptr->d_name[0] == '.')) {
					if (!made_contents){
						// Enter the contents to link the next items
						curr = create_node(entry_ptr->d_name, new_path);
						t->contents = curr;
						made_contents = true;
					}else{
						curr->next = create_node(entry_ptr->d_name, new_path);
						curr = curr->next;
					}
				}
				entry_ptr = readdir(d_ptr);
			}
			closedir(d_ptr);
		}
	}
		
	// Allocate space for and store the fname
	t->fname = malloc(sizeof(char*) * strlen(fname));
	if (t->fname == NULL) {
		perror("malloc");
		exit(1);
	}
	strcpy(t->fname, fname);
	
	// Store the permissions
	t->permissions = stat_buf.st_mode & 0777;
	
	return t;
}

/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions)
 *
 */
void print_ftree(struct TreeNode *root) {
	
    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    printf("%*s", depth * 2, "");

    // Your implementation here.
	if ((root->type == '-')||(root->type == 'l')) {
		printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
	} else {
		printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
		struct TreeNode *curr = root->contents;
		depth++;
		// Traverse the next contents
		while (curr != NULL) {
			print_ftree(curr);
			curr = curr->next;
		}
		depth--;
	}
}


/* 
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 * 
 */
void deallocate_ftree (struct TreeNode *node) {
   
   // Your implementation here.
	if (node->next != NULL) {
		deallocate_ftree(node->next);
	}
	if(node->contents != NULL) {
		deallocate_ftree(node->contents);
	}
	free(node->fname);
	free(node);
}