#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define ALL_CHARS 256

typedef struct trienode
{
    bool lastnode;
    int hashind;
    pthread_mutex_t lock;
    struct trienode *child[ALL_CHARS];
} trienode;

trienode *newnode();                                    // done
int print_all_childs_v2(trienode *root, char *str, FILE *fp);
int trieinsert(trienode *temp, char *str, int hashind); // returns -1 if duplicate, if success returns hashind
int is_file(trienode *root, char *str);

int delete_from_trie(trienode *root, char *str, int assn);
void initialize_trie(trienode **root);

int find_subtree_new(trienode *node);
int find_new(trienode *root, char *str, int *is_partial);


int print_all_childs(trienode *root, char *str, FILE *fp);

#endif