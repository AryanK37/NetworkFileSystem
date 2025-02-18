#include "lru_cache.h"
#define MAX_CACHE_SIZE 10

void insert(LRU_Cache *cache, int ind, char *str)
{
    struct node *temp = (struct node *)malloc(sizeof(struct node));
    temp->hashind = ind;
    strcpy(temp->name, str);
    temp->next = NULL;
    temp->prev = NULL;
    cache->size++;

    if (cache->head == NULL)
    {
        cache->head = temp;
    }
    else
    {
        temp->next = cache->head;
        cache->head->prev = temp;
        cache->head = temp;
    }
}

void delete_from_cache(LRU_Cache *cache, char *str)
{
    struct node *temp = cache->head;
    // while (temp != NULL)
    // {
    //     node *copy = temp;
    //     temp = temp->next;
    //     free(copy);
    // }

    int ind = 0;
    while (temp != NULL && ind < MAX_CACHE_SIZE + 1)
    {
        node *next_node = temp->next;
        // if (strcmp(str, temp->name) == 0)
        if (strstr(temp->name, str) != NULL)
        {
            if (temp->prev == NULL && temp->next == NULL)
            {
                cache->head = NULL;
            }
            else if (temp->prev == NULL)
            {
                cache->head = temp->next;
                if (cache->head != NULL)
                    cache->head->prev = NULL;
                // temp->next->prev = NULL;
            }
            else if (temp->next == NULL)
            {
                temp->prev->next = NULL;
            }
            else
            {
                temp->prev->next = temp->next;
                temp->next->prev = temp->prev;
            }
            // free(temp);
        }
        temp = next_node;
        ind++;
    }
    // while (temp != NULL) // free the remaining nodes :|
    // {
    //     node *copy = temp;
    //     temp = temp->next;
    //     free(copy);
    // }
}

int find_in_cache(LRU_Cache *lru, char *temp)
{
    if (!lru || !lru->head) return -1;

    node *current = lru->head;
    int ind = 0;

    while (current != NULL && ind < MAX_CACHE_SIZE)
    {
        if (strcmp(current->name, temp) == 0)
        {
            if (current->prev)
            {
                current->prev->next = current->next;
            }
            if (current->next)
            {
                current->next->prev = current->prev;
            }
            current->prev = NULL;
            current->next = lru->head;
            lru->head->prev = current;
            lru->head = current;
            return current->hashind;
        }
        current = current->next;
        ind++;
    }

    return -1; // Not found :(
}
// int find_in_cache(LRU_Cache *lru, char *temp)
// {
//     node *current = lru->head;
//     int ind = 0;
//     // while (current != NULL)
//     while (current != NULL && ind < MAX_CACHE_SIZE)
//     {
//         if (strcmp(current->name, temp) == 0)
//         {
//             return current->hashind;
//         }
//         current = current->next;
//         ind++;
//     }
//     return -1;
// }

// int find_and_update(LRU_Cache *cache, char *str)
// {
//     int x = find_in_cache(cache, str);
//     if (x != -1)
//     {
//         delete_from_cache(cache, str);
//         insert(cache, x, str);
//         return x;
//     }
//     return -1;
// }
