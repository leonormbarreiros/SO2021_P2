#ifndef FS_H
#define FS_H
#include "state.h"

void init_fs();
void destroy_fs();
void print_tecnicofs_tree(FILE *fp);

int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
int move(char *old_location, char *new_location);

int lookup_aux(char *name, int inodes_locked[], int *n_inodes_locked, permission p);
int validation_old_location(char *old_location, int *child_inumber, int *parent_inumber, char *parent_name,char *child_name, int inodes_locked[], int *n_inodes_locked);
int validation_new_location(char *new_location, int *child_inumber, int *parent_inumber, char *parent_name,char *child_name, int inodes_locked[], int *n_inodes_locked);
int lookup_sub_node(char *name, DirEntry *entries);
int is_dir_empty(DirEntry *dirEntries);
void split_parent_child_from_path(char * path, char ** parent, char ** child);

#endif /* FS_H */
