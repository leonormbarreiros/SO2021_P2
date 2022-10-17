#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();

	/*Garbage values*/
	int inodes_locked[1] = {0};
	int n_inodes_locked = 0;
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY, inodes_locked, &n_inodes_locked);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}

	inode_unlock(inodes_locked[--n_inodes_locked]);
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}



/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	int inodes_locked[INODE_TABLE_SIZE] = {-1}, n_inodes_locked = 0;

	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* find the parent i-number */
	parent_inumber = lookup_aux(parent_name, inodes_locked, &n_inodes_locked, WRITE);

	/* validation */
	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n", name, parent_name);
		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	/* validation */
	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);

		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	/* find the child i-number, subnode of the parent('s pdata) - with validation */
	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	/* it's supposed to fail because it's not meant to be already created */
	if ( child_inumber != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType, inodes_locked, &n_inodes_locked);

	/* validation */
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	/* add entry to folder that contains created node - with validation */
	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	/* unlock the i-nodes locked in the travessy */
	while (n_inodes_locked > 0) {
        if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL) {
			fprintf(stderr, "Error: could not unlock\n");
			exit(EXIT_FAILURE);
		}
    }

	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];

	int inodes_locked[INODE_TABLE_SIZE] = {-1}, n_inodes_locked = 0;

	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* find the parent i-number */
	parent_inumber = lookup_aux(parent_name, inodes_locked, &n_inodes_locked, WRITE);

	/* validation */
	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);

		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	/* validation */
	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);

		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	/* find the child i-number, subnode of the parent('s pdata) */
	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);
	
	/* validation */
	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		
		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}

		return FAIL;
	}

	if (inode_lock(child_inumber, WRITE) == FAIL) {
		fprintf(stderr, "Error: unable to lock\n");
		exit(EXIT_FAILURE);
    }

	inodes_locked[n_inodes_locked++] = child_inumber;
	
	inode_get(child_inumber, &cType, &cdata);

	/* validation */
	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	/* remove entry from folder that contained deleted node - with validation */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}

		return FAIL;
	}

	/* delete the i-node - with validation */
	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);

		while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
				fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
			}
    	}
		return FAIL;
	}

	inodes_locked[--n_inodes_locked] = -1;

	/* unlock the i-nodes locked in the travessy */
	while (n_inodes_locked > 0) {
        if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
			fprintf(stderr, "Error: could not unlock\n");
			exit(EXIT_FAILURE);
		}
    }
	
	return SUCCESS;
}




/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name) {

	int inodes_locked[INODE_TABLE_SIZE] = {-1}, n_inodes_locked = 0;
	int current_inumber;

	/* find the i-number using the auxiliary function */
	current_inumber = lookup_aux(name, inodes_locked, &n_inodes_locked, READ);

	/* unlock what we've locked */
	while (n_inodes_locked > 0) {
        if (inode_unlock(inodes_locked[--n_inodes_locked]) != SUCCESS) {
            fprintf(stderr, "Error: unable to unlock\n");
			exit(EXIT_FAILURE);
        }
    }
	
    return current_inumber;
}



/*
 * Removes the entry from the old_location and inserts it in the new_location
 * Input:
 * - old_location: the previous pathname
 * - new_location: the new pathname
 * Returns: SUCCESS or FAIL
 */
int move(char *old_location, char *new_location) {
	int inodes_locked[INODE_TABLE_SIZE] = {-1}, n_inodes_locked = 0;

	int old_parent = -1, new_parent = -1;
	char *old_parent_name, *new_parent_name;
	
	int old_child = -1, new_child = -1;
	char *old_child_name, *new_child_name;

	char old_location_copy[MAX_FILE_NAME], new_location_copy[MAX_FILE_NAME];

	/* get the parent and child names in old location */
	strcpy(old_location_copy, old_location);
	split_parent_child_from_path(old_location_copy, &old_parent_name, &old_child_name);

	/* get the parent and child names in new location */
	strcpy(new_location_copy, new_location);
	split_parent_child_from_path(new_location_copy, &new_parent_name, &new_child_name);

	/* VALIDATION */
	/* the strategy we chose to avoid deadlocks is to lock firstly the origin or the destiny depending on the alfabetical order */

	int first = strcmp(old_location_copy, new_location_copy);
	if (first > 0) { /* old location first */
		int val_old = validation_old_location(old_location_copy, &old_child, &old_parent, old_parent_name, old_child_name, inodes_locked, &n_inodes_locked);
		if (val_old == FAIL) {
			printf("unable to move %s, problems with old location\n", old_location);
			
			/* unlock what we've locked */
			while (n_inodes_locked > 0) {
        		if (inode_unlock(inodes_locked[--n_inodes_locked]) != SUCCESS) {
            		fprintf(stderr, "Error: unable to unlock\n");
					exit(EXIT_FAILURE);
        		}
    		}
			return FAIL;
		}
		int  val_new = validation_new_location(new_location_copy, &new_child, &new_parent, new_parent_name, new_child_name, inodes_locked, &n_inodes_locked);
		if (val_new == FAIL) {
			printf("unable to move %s, problems with new location %s\n", old_location, new_location);
			
			/* unlock what we've locked */
			while (n_inodes_locked > 0) {
        		if (inode_unlock(inodes_locked[--n_inodes_locked]) != SUCCESS) {
            		fprintf(stderr, "Error: unable to unlock\n");
					exit(EXIT_FAILURE);
        		}
    		}
			return FAIL;
		}
	} else { /* new location first */
		int val_new = validation_new_location(new_location_copy, &new_child, &new_parent, new_parent_name, new_child_name, inodes_locked, &n_inodes_locked);
		if (val_new == FAIL) {
			printf("unable to move %s, problems with new location %s\n", old_location, new_location);
			
			/* unlock what we've locked */
			while (n_inodes_locked > 0) {
        		if (inode_unlock(inodes_locked[--n_inodes_locked]) != SUCCESS) {
            		fprintf(stderr, "Error: unable to unlock\n");
					exit(EXIT_FAILURE);
        		}
    		}
			return FAIL;
		}
		int val_old = validation_old_location(old_location_copy, &old_child, &old_parent, old_parent_name, old_child_name, inodes_locked, &n_inodes_locked);
		if (val_old == FAIL) {
			printf("unable to move %s, problems with old location\n", old_location);
			
			/* unlock what we've locked */
			while (n_inodes_locked > 0) {
        		if (inode_unlock(inodes_locked[--n_inodes_locked]) != SUCCESS) {
            		fprintf(stderr, "Error: unable to unlock\n");
					exit(EXIT_FAILURE);
        		}
    		}
			return FAIL;
		}
	}
	
	/* EXECUTION */
		
	if (dir_reset_entry(old_parent, old_child) == FAIL) {
    	printf("unable to move: failed to delete %s from dir %s\n", old_child_name, old_parent_name);    
    	while (n_inodes_locked > 0) {    
			if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL) {
            	fprintf(stderr, "Error: could not unlock\n");
				exit(EXIT_FAILURE);
        	}
	 	}
    	return FAIL;
	}
		
	if (dir_add_entry(new_parent, old_child, new_child_name) == FAIL) {
        printf("unable to move: could not add entry %s in dir %s\n", new_child_name, new_parent_name);
    	while (n_inodes_locked > 0) {
        	if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL) {
                	fprintf(stderr, "Error: could not unlock\n");
					exit(EXIT_FAILURE);
            }
        }
        return FAIL;
    }
	
	/* unlock the i-nodes locked in the travessy */
	while (n_inodes_locked > 0) {
        if (inode_unlock(inodes_locked[--n_inodes_locked]) == FAIL)  {
			fprintf(stderr, "Error: could not unlock\n");
			exit(EXIT_FAILURE);
		}
    }
	return SUCCESS;
}



/*
 * Lookup for a given path (auxiliary funtion)
 * Input:
 *  - name: path of node
 *  - inodes_locked: the i-nodes locked in the travessy
 *  - n_inodes_locked: how many i-nodes were locked in the travessy
 *  - permission: for locking the i-node corresponding to the returned i-number
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup_aux(char *name, int inodes_locked[], int *n_inodes_locked, permission p) {

	char full_path[MAX_FILE_NAME], last[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	char *newpath = strtok_r(full_path, delim, &saveptr);

	if(!newpath) {	/* if the newpath is NULL, we're dealing with the root */
		if(inode_lock(current_inumber, p) == FAIL){
			fprintf(stderr, "Error: Could not lock the root!");
			exit(EXIT_FAILURE);
		}

		inodes_locked[*n_inodes_locked] = current_inumber;
        (*n_inodes_locked)++;

		/* get root inode data */
		inode_get(current_inumber, &nType, &data);

		return current_inumber;

	} else { /* if it isn't, we have to do a travessy and look for the i-node */
		if(inode_lock(current_inumber, READ)==FAIL){
			fprintf(stderr, "Error: Could not lock the root!");
			exit(EXIT_FAILURE);
		}
		inodes_locked[*n_inodes_locked] = current_inumber;
        (*n_inodes_locked)++;
	}

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);
	strcpy(last, newpath);

	/* start looking */

	newpath = strtok_r(NULL, delim, &saveptr); /* each time we do this we're advancing on our travessy */
	/* search for all sub nodes 
	 * when newpath is NULL it means we've reached the end of our travessy 
	 * when lookup_sub_node fails it means it has no subnode
	 */ 
	while (newpath != NULL && (current_inumber = lookup_sub_node(last, data.dirEntries)) != FAIL) {
		if (inode_lock(current_inumber, READ) == FAIL)
			return FAIL;
        
		inodes_locked[*n_inodes_locked] = current_inumber;
        (*n_inodes_locked)++;

		strcpy(last, newpath);
		inode_get(current_inumber, &nType, &data);
		newpath = strtok_r(NULL, delim, &saveptr);
		if (!newpath) break;
    }
	/* last inode in the path (parent) */
	if((current_inumber = lookup_sub_node(last, data.dirEntries))!=FAIL) {
		
		if (inode_lock(current_inumber, p) == FAIL) {
			fprintf(stderr, "Error: unable to lock\n");
			exit(EXIT_FAILURE);
        }
		inodes_locked[*n_inodes_locked] = current_inumber;
        (*n_inodes_locked)++;

		inode_get(current_inumber, &nType, &data);
		return current_inumber;
	}
	return FAIL;	
}



/*
 * Performs validation in the old location (auxiliary to the move command).
 * there has to be a file/directory corresponding to it:
 *  - parent must exist and be a directory
 *  - child must exist
 * Input:
 *  - old_location: where we're performing validation
 *  - child_inumber: the child_inumber we're looking up
 *  - parent_inumber: the parent_inumber we're looking up
 *  - child_name: the child's name
 *  - parent_name: the parent's name
 *  - inodes_locked[]: inodes locked during validation
 *  - n_inodes_locked: number of inodes locked during validation
 * Returns: SUCCESS or FAIL
 */ 
int validation_old_location(char *old_location, int *child_inumber, int *parent_inumber, char *parent_name, char *child_name, int inodes_locked[], int *n_inodes_locked) {
	
	/* use for copy */
    type pType;
    union Data pdata;

    (*parent_inumber) = lookup_aux(parent_name, inodes_locked, n_inodes_locked, WRITE);
    /* parent has to exist */
    if ((*parent_inumber) == FAIL) {
        printf("could not move %s, invalid parent dir %s\n", child_name, parent_name);
        return FAIL;
    }
    inode_get((*parent_inumber), &pType, &pdata);
    
    /* parent has to be a directory */
    if (pType != T_DIRECTORY) {
        printf("failed to move %s, parent %s is not a dir\n", child_name, parent_name);
        return FAIL;
    }

   (*child_inumber) = lookup_sub_node(child_name, pdata.dirEntries);

    /* child has to exist */
    if ((*child_inumber) == FAIL) {
        printf("could not move %s, does not exist in dir %s\n", old_location, parent_name);
        return FAIL;
    }

	if (inode_lock((*child_inumber), WRITE) == FAIL) {
		fprintf(stderr, "Error: unable to lock\n");
		exit(EXIT_FAILURE);
    }

	inodes_locked[*n_inodes_locked] = (*child_inumber);
    (*n_inodes_locked)++;

	return SUCCESS;
}

/*
 * Performs validation in the new location (auxiliary to the move command).
 * there cannot be a file/directory corresponding to it:
 *  - parent must exist and be a directory
 *  - child must not exist
 * Input:
 *  - new_location: where we're performing validation
 *  - child_inumber: the child_inumber we're looking up
 *  - parent_inumber: the parent_inumber we're looking up
 *  - child_name: the child's name
 *  - parent_name: the parent's name
 *  - inodes_locked[]: inodes locked during validation
 *  - n_inodes_locked: number of inodes locked during validation
 * Returns: SUCCESS or FAIL
 */ 
int validation_new_location(char *new_location, int *child_inumber, int *parent_inumber, char *parent_name, char *child_name, int inodes_locked[], int *n_inodes_locked) {
	/* use for copy */
    type pType;
    union Data pdata;

	(*parent_inumber) = lookup_aux(parent_name, inodes_locked, n_inodes_locked, WRITE);

    /* parent has to exist */
    if ((*parent_inumber) == FAIL) {
        printf("could not move %s, invalid parent dir %s\n", child_name, parent_name);
        return FAIL;
    }

    inode_get((*parent_inumber), &pType, &pdata);

    /* parent has to be a directory */
    if (pType != T_DIRECTORY) {
        printf("failed to move %s, parent %s is not a dir\n", child_name, parent_name);
        return FAIL;
    }

    (*child_inumber) = lookup_sub_node(child_name, pdata.dirEntries);

    /* child must not exist */
    if ((*child_inumber) != FAIL) {
        printf("failed to move %s, already exists in dir %s\n", child_name, parent_name);
        return FAIL;
    }

	return SUCCESS;
}



/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}



/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}




/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}



