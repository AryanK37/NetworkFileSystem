#ifndef WRAPPER_OPT_H
#define WRAPPER_OPT_H

#include "trie.h"
#include "lru_cache.h"
#include "../common.h"

char *handle_slash(char *str);             // removes trailing slash and adds / at start
int create(int main_server, char *str);    // takes main server and string path inserts in trie updates lru
int delete_file_folder(char *str);         // first deletes from lru then deletes from from trie
int ls(char* str,FILE* fp); // lists all files and subfiles
int ls_v2(char* str,FILE* fp); // lists all files and subfiles
int search_v2(char *str,int* is_partial);
int IS_FILE(char *str); // returns 1 if file, 0 if directory, -1 if not found

#endif