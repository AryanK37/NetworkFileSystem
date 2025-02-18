#include "trie.h"

int main()
{
    trienode *root = NULL;
    initialize_trie(&root);
    trieinsert(root, "home", 1);
    // trieinsert(root, "home/abc", 2);
    trieinsert(root, "home/abc/aryan.txt", 11);
    trieinsert(root, "home/abc/def/ghi", 4);
    trieinsert(root, "home/abc/def/ghi/jkl/mno", 6);
    trieinsert(root, "home/abc/def", 3);
    trieinsert(root, "home/abc/def/ghi/jkl", 5);

    print_all_childs(root, "home/abc");

    // trieinsert(root, "abhi", 7);
    // trieinsert(root, "abhiram", 8);
    // trieinsert(root, "abhiram/abc", 9);
    // trieinsert(root, "abhijeet", 10);

    // debug_print_trie(root);

    // int x = find_in_trie(root, "home");
    // if (x != -1)
    // {
    //     printf("find home ouput %d\n", x);
    //     printf("HOME found\n");
    // }
    // else
    // {
    //     printf("Not found\n");
    // }

    // x = find_in_trie(root, "akshara");
    // if (x != -1)
    // {
    //     printf("find akshara ouput %d\n", x);
    //     printf("Akshara found\n");
    // }
    // else
    // {
    //     printf("Akshara Not found\n");
    // }

    // x = find_in_trie(root, "abhiram");
    // if (x != -1)
    // {
    //     printf("find abhiram ouput %d\n", x);
    //     printf("Abhiram found\n");
    // }
    // else
    // {
    //     printf("Abhiram Not found\n");
    // }

    // delete_from_trie(root, "abhiram");

    // x = find_in_trie(root, "abhiram");
    // if (x != -1)
    // {
    //     printf("find abhiram ouput %d\n", x);
    //     printf("Abhiram found\n");
    // }
    // else
    // {
    //     printf("Abhiram Not found\n");
    // }

    return 0;
}