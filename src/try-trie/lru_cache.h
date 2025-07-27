#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct node
{
    int hashind;
    char name[100];
    struct node *next;
    struct node *prev;
} node;

typedef struct LRU_Cache
{
    node *head;
    int size;
} LRU_Cache;


// void initialize_cache(LRU_Cache *cache);
void insert(LRU_Cache *cache, int ind, char *str);
void delete_from_cache(LRU_Cache *cache, char *str);
int find_in_cache(LRU_Cache *lru, char *temp);
// int find_and_update(LRU_Cache *cache, char *str);

#endif