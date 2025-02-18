#include "wrapper_opt.h"

trienode *__global_trie = NULL;
LRU_Cache *__global_lru = NULL;

int IS_FILE(char *str)
{
    if (__global_trie == NULL)
    {
        return -1; // path is invalid
    }

    char *newstr = handle_slash(str);
    if (newstr == NULL)
    {
        return -1;
    }

    int x = is_file(__global_trie, newstr);
    free(newstr);
    return x;
}
char *handle_slash_v2(char *str) // dont ignore trailing '/'
{
    if (str == NULL)
    {
        return NULL;
    }

    int len = (int)strlen(str);
    char *newstr = malloc((len + 3) * sizeof(char));
    int i = 0; // for str
    int j = 0; // for newstr
    if (str[i] != '/')
    {
        newstr[j++] = '/';
    }
    for (; i < len; i++)
    {
        newstr[j++] = str[i];
    }
    newstr[j] = '\0';
    return newstr;
}
int ls_v2(char *str, FILE *fp) // lists all files and subfiles
{
    if (str == NULL || fp == NULL)
    {
        return 0;
    }

    if (!__global_trie)
    {
        // fprintf(fp, "", str); // to create file if it doesnt exist or make it empty
        return strcmp(str, "/") == 0;
    }
    char *newstr = handle_slash_v2(str);
    int arr = 0;
    int x = search_v2(newstr, &arr);
    if (x == -1 || arr)
    {
        return 0;
    }
    if (newstr == NULL)
    {
        return 0;
    }

    int result = print_all_childs_v2(__global_trie, newstr, fp);
    return result;
}

int search_v2(char *str, int *is_partial)
{

    if (!__global_trie)
    {
        return -1; // is partial doesnt matter invalid str path
    }

    char *newstr = handle_slash(str);
    char *newstr2 = handle_slash_v2(str);
    if (newstr == NULL)
    {
        return -1;
    }

    int x = find_in_cache(__global_lru, newstr2);
    if (x != -1)
    {
        is_partial = 0;
        return x;
    }

    x = find_new(__global_trie, newstr, is_partial);
    insert(__global_lru, x, newstr2);
    return x;
}

char *handle_slash(char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    int len = (int)strlen(str);
    char *newstr = malloc((len + 3) * sizeof(char));
    int i = 0; // for str
    int j = 0; // for newstr
    if (str[i] != '/')
    {
        newstr[j++] = '/';
    }
    for (; i < len - 1; i++)
    {
        newstr[j++] = str[i];
    }
    if (str[len - 1] == '/') // ignore trailing '/'
    {
        newstr[j++] = '\0';
    }
    else
    {
        newstr[j++] = str[len - 1];
    }
    newstr[j] = '\0';
    return newstr;
}

int create(int main_server, char *str) // takes main server and string path inserts in trie updates lru
{
    if (str == NULL)
    {
        return -1;
    }

    char *newstr = handle_slash_v2(str);
    if (newstr == NULL)
    {
        return -1;
    }

    if (__global_trie == NULL)
    {
        initialize_trie(&__global_trie);
    }
    int x = trieinsert(__global_trie, newstr, main_server);
    if (x == -1)
    {
        insert(__global_lru, main_server, newstr);
        return x;
    }

    if (__global_lru == NULL)
    {
        __global_lru = (LRU_Cache *)malloc(sizeof(LRU_Cache));
        __global_lru = malloc(sizeof(LRU_Cache));
        __global_lru->head = NULL;
        __global_lru->size = 0;
    }
    // insert(__global_lru, main_server, newstr);
    // return x;
    // }

    insert(__global_lru, main_server, newstr);
    if (x == -1)
    {
        return -1;
    }
    return x;
}

int delete_file_folder(char *str) // first deletes from LRU, then deletes from trie
{
    if (str == NULL)
    {
        return -1;
    }

    if (!__global_trie)
    {
        return -1;
    }

    char *newstr = handle_slash(str);
    char *newstr2 = handle_slash_v2(str);
    int arr = 0;
    int x = search_v2(newstr, &arr);
    if (x == -1 || arr)
    {
        return -1;
    }

    delete_from_cache(__global_lru, newstr2);
    delete_from_trie(__global_trie, newstr, x);
    return x;
    // return 1;
}

int ls(char *str, FILE *fp) // lists all files and subfiles
{
    if (str == NULL || fp == NULL)
    {
        return 0;
    }

    if (!__global_trie)
    {
        return strcmp(str, "/") == 0;
    }

    char *newstr = handle_slash(str);
    int result = print_all_childs(__global_trie, newstr, fp);
    return result;
}