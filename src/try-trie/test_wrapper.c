#include "wrapper_opt.h"
#include <assert.h>
// extern trienode* __global_trie = NULL;
// extern LRU_Cache* __global_lru = NULL;
// void test_create()
// {
//     char *path = "/example/path";
//     int main_server = 1;
//     int result = create(main_server, path);
//     assert(result != -1);
//     result = create(main_server, path);
//     assert(result == -1);
// }
// void test_search()
// {
//     char *path = "/example/path";
//     int main_server = 1;
//     create(main_server, path);
//     int result = search(path);
//     assert(result == main_server);
//     result = search("/non/existent/path");
//     assert(result == -1);
// }
// void test_delete_file_folder()
// {
//     char *path = "/example/path";
//     int main_server = 1;

//     create(main_server, path);
//     // printf("HERE\n");
//     int *deleted_paths = delete_file_folder(path);
//     for (int i = 0; i < 1024; i++)
//     {
//         if (deleted_paths[i])
//         {
//             printf("path is in server %d \n", i);
//         }
//     }
//     assert(deleted_paths != NULL);
//     int result = search(path);
//     printf("%d\n", result);
//     // assert(result == -1); NOT WORKING WITH OTHERS

//     free(deleted_paths);
// }
// void test_what_the_lock()
// {
//     char *path = "/example/path";
//     int main_server = 1;
//     create(main_server, path);
//     pthread_mutex_t *lock = what_the_lock(path);
//     assert(lock != NULL);
// }
// void test_ls()
// {
//     char *path1 = "/example";
//     char *path2 = "/example/path";
//     int main_server = 1;
//     create(main_server, path1);
//     create(main_server, path2);
//     FILE *fp = fopen("output.txt", "w+");
//     if (fp == NULL)
//     {
//         printf("Error opening file!\n");
//         exit(1);
//     }

//     int result = ls(path1, fp);
//     if (result == 0)
//     {
//         fprintf(fp, "Error: Directory not found (Error code: %d)\n", 0);
//     }

//     ls(path1, fp);
// }

// void tough_test()
// {
//     assert(create(1, "home") != -1);
//     assert(create(2, "home/abc") != -1);
//     assert(create(4, "home/abc/def/ghi") != -1);
//     assert(create(6, "home/abc/def/ghi/jkl/mno") != -1);
//     assert(create(3, "home/abc/def") != -1);
//     assert(create(5, "home/abc/def/ghi/jkl") != -1);
//     assert(create(7, "abhi") != -1);
//     assert(create(8, "abhiram") != -1);
//     assert(create(9, "abhiram/abc") != -1);
//     assert(create(10, "abhijeet") != -1);

//     int x = search("home");
//     assert(x == 1);

//     assert(search("home/abc") == 2);
//     assert(search("home/abc/def") == 3);

//     int *arr = delete_file_folder("home/abc/def");
//     if (arr == NULL)
//     {
//         printf("Invalid Path");
//     }
//     else
//     {
//         for (int i = 0; i < 1024; i++)
//         {
//             // printf("%d ", i);
//             if (arr[i])
//             {
//                 printf("%d", i);
//             }
//         }
//         printf("\n");
//         free(arr);
//     }

//     assert(search("home/abc/def") == -1);
//     assert(search("home/abc/invalid") == -1);
//     assert(search("abhiram/abc") == 9);
// }

// void test_search_new()
// {
//     // assert(create(1, "home") != -1);
//     assert(create(2, "home/abc/") != -1);
//     assert(create(4, "home/abc/def/ghi/") != -1);
//     assert(create(6, "home/abc/def/ghi/jkl/mno/") != -1);
//     assert(create(3, "home/abc/def/") != -1);
//     assert(create(5, "home/abc/def/ghi/jkl/") != -1);

//     int is_partial = -1;
//     int x;
//     x = search_v2("home", &is_partial);
//     assert(x == 2);
//     assert(is_partial == 0);

//     x = search_v2("home/abc", &is_partial);
//     assert(x == 2);
//     assert(is_partial == 0);

//     x = search_v2("home/abc/def", &is_partial);
//     assert(x == 3);
//     assert(is_partial == 0);

//     x = search_v2("home/abc/def/ghi", &is_partial);
//     assert(x == 4);
//     assert(is_partial == 0);

//     x = search_v2("home/abc/def/ghi/jkl", &is_partial);
//     assert(x == 5);
//     assert(is_partial == 0);

//     x = search_v2("home/abc/def/ghi/jkl/mno/", &is_partial);
//     assert(x == 6);
//     assert(is_partial == 0);

//     x = search_v2("home/abc/def/ghi/jkl/mno/pqr", &is_partial);
//     assert(x == 6);  // failed
//     assert(is_partial == 1);

//     x = search_v2("home/abc/def/ghi/jkl/mno/pqr/", &is_partial);
//     assert(x == 6);
//     assert(is_partial == 1);

//     x = search_v2("home/abc/def/ghi/jkl/mno/pqr/stu", &is_partial);
//     assert(x == 6);
//     assert(is_partial == 1);

//     x = search_v2("home/abc/def/ghi/jkl/mno/pqr/stu/", &is_partial);
//     assert(x == 6);
//     assert(is_partial == 1);
//     x = search_v2("/abhiram/", &is_partial);
//     assert(x == -1);
//     // return;
//     // /home/
//     x = search_v2("/home/abhiram", &is_partial);
//     // printf("x: %d\n", x);
//     assert(x == 2);
//     assert(is_partial == 1);
//     // return;

//     x = search_v2("/", &is_partial);
//     assert(x == 2);
//     assert(is_partial == 0);
// }

// void test_ls_v2()
// {
//     assert(create(2, "home/abc/") != -1);
//     assert(create(4, "home/abc/def/ghi/") != -1);
//     assert(create(4, "home/abc/def/a.txt") != -1);
//     assert(create(6, "home/abc/def/ghi/jkl/mno/") != -1);
//     assert(create(3, "home/abc/def/") != -1);
//     assert(create(5, "home/abc/def/ghi/jkl/") != -1);
//     ls_v2("home/abc/", stdout);
//     printf("--------------------------------------\n");
//     ls_v2("home/abc/def/a.txt", stdout);
// }

// void my_doubt()
// {
//     int is_partial = -1;
//     assert(create(1, "abhiram/") != -1);
//     int x = search_v2("abhiram", &is_partial);
//     assert(x == 1);

//     x = search_v2("abhi", &is_partial);
//     // to handle this if abhi is a file trie should end there
//     // if abhiram is a file then it should end with /
//     assert(x == 1); // need -1 here
//     assert(is_partial == 0);

//     x = search_v2("abhi/def", &is_partial);
//     assert(x == 1);
//     assert(is_partial == 1);
// }

// // if abhiram is a directory then abhi is not a file
// void my_fix()
// {
//     int is_partial = -1;
//     assert(create(1, "abhiram/") != -1);
//     int x;
//     x = search_v2("abhiram", &is_partial);
//     assert(x == 1);
//     assert(is_partial == 0);

//     x = search_v2("abhiram/", &is_partial);
//     assert(x == 1);
//     assert(is_partial == 0);

//     x = search_v2("abhi", &is_partial);
//     // to handle this if abhi is a file trie should end there
//     // if abhiram is a file then it should end with /
//     assert(x == -1); // need -1 here

//     x = search_v2("abhi/def", &is_partial);
//     assert(x == -1);
// }

// void Is_file(){
//     assert(create(2, "home/abc/") != -1);
//     assert(create(4, "home/abc/def/ghi/") != -1);
//     assert(create(4, "home/abc/def/a.txt") != -1);
//     assert(create(6, "home/abc/def/ghi/jkl/mno/") != -1);
//     assert(create(3, "home/abc/def/") != -1);
//     assert(create(3, "home/abc/def/abs.txt") != -1);
//     assert(create(5, "home/abc/def/ghi/jkl/") != -1);
//     assert(IS_FILE("asb")==-1);
//     assert(IS_FILE("home/abc/def/ghi/jkl/")==0);
//     assert(IS_FILE("home/abc/def/abs.txt")==1);
// }
void delete_test()
{
    assert(create(2, "home/abc/") != -1);
    // assert(create(2, "home/okbro/") != -1);
    // assert(create(2, "home/b.txt") != -1);
    assert(create(5, "home/abc/def/ghi/") != -1);
    assert(create(4, "home/ab.txt") != -1);
    assert(create(4, "home/abc/def/a.txt") != -1);
    assert(create(6, "home/abc/def/ghi/jkl/mno/") != -1);
    assert(create(3, "home/abc/def/") != -1);
    assert(create(3, "home/abc/def/abs.txt") != -1);
    assert(create(5, "home/abc/def/ghi/jkl/") != -1);

    // delete_file_folder("home/abc/def/ghi/jkl/");
    // ls_v2("/", stdout);
    // printf("--------------------------------------\n");
    // delete_file_folder("home/abc/def/abs.txt");
    // printf("--------------------------------------\n");
    // ls_v2("/", stdout);
    // ls_v2("home/abc/def", stdout);
    // ls("home", stdout);
    ls_v2("/", stdout);
    printf("--------------------------------------\n");
    printf("calling delete on home/abc/\n");
    delete_file_folder("home/abc/");
    printf("AFTER THIS--------------------------------------\n");
    ls("home", stdout);
    ls_v2("/", stdout);
}
void test_serach_delete()
{
    assert(create(2, "home/abc/") != -1);
    // assert(create(2, "home/okbro/") != -1);
    // assert(create(2, "home/b.txt") != -1);
    assert(create(5, "home/abc/def/ghi/") != -1);
    assert(create(0, "home/ab.txt") != -1);
    assert(create(4, "home/abc/def/a.txt") != -1);
    assert(create(6, "home/abc/def/ghi/jkl/mno/") != -1);
    assert(create(3, "home/abc/def/") != -1);
    assert(create(3, "home/abc/def/abs.txt") != -1);
    assert(create(5, "home/abc/def/ghi/jkl/") != -1);

    delete_file_folder("home/abc/");
    int ispar;
    int x = search_v2("home/abc/def", &ispar);
    printf("%d", x);
}
void test_new_del()
{
    assert(create(2,"/jo/mo/")!=-1);
    assert(create(2,"/jo/m.txt")!=-1);
    ls_v2("/",stdout);
    delete_file_folder("/jo/mo/");
    printf("-----------------------\n");
    ls_v2("/",stdout);
}
void test_new_del_2()
{
    assert(create(2,"/jo/mon/")!=-1);
    assert(create(2,"/jo/mok.txt")!=-1);
    ls_v2("/",stdout);
    delete_file_folder("/jo/mon/");
    printf("-----------------------\n");
    ls_v2("/",stdout);
}
int main()
{
    test_new_del();
    // test_new_del_2();
    // test_serach_delete();
    // delete_test();
    // Is_file();
    // my_fix();
    // test_search_new();
    // test_search();
    // my_doubt();
    // test_ls_v2();
    // test_create();
    // test_delete_file_folder();
    // test_what_the_lock();
    // test_ls();
    // tough_test();
    // test_ls();
    // printf("All tests passed successfully.\n");
    // char *path = "/example/path";
    // char *path1 = "example";
    // char *path2 = "example/path/";
    // char *path3 = "/example/path/";
    // char *result = handle_slash(path);
    // printf("for %s function output: %s\n", path, result);

    // char *result1 = handle_slash(path1);
    // printf("for %s function output: %s\n", path1, result1);

    // char *result2 = handle_slash(path2);
    // printf("for %s function output: %s\n", path2, result2);

    // char *result3 = handle_slash(path3);
    // printf("for %s function output: %s\n", path3, result3);

    // create(1, "home");
    // create(2, "home/abc");
    // create(4, "home/abc/def/ghi");
    // create(6, "home/abc/def/ghi/jkl/mno");
    // create(3, "home/abc/def");
    // create(5, "home/abc/def/ghi/jkl");
    // create(7, "abhi");
    // create(8, "abhiram");
    // create(9, "abhiram/abc");
    // create(10, "abhijeet");
    // int x = search("home");
    // printf("find home ouput %d\n", x);
    // printf("find home/abc ouput %d\n", search("home/abc"));
    // printf("find home/abc/def ouput %d\n", search("home/abc/def"));

    // delete_file_folder("home/abc/def");

    // printf("find home/abc/def ouput %d\n", search("home/abc/def"));
    // printf("find home/abc/invalid ouput %d\n", search("home/abc/invalid"));
    // printf("find abhiram/abc ouput %d\n", search("abhiram/abc"));

    return 0;
}
