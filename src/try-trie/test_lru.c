#include "lru_cache.h"

int main()
{
    LRU_Cache mycash;
    mycash.head = NULL;
    mycash.size = 0;

    // initialize_cache(&mycash);
    mycash.head=(node*)malloc(sizeof(node));
    mycash.head->hashind=1;
    strcpy(mycash.head->name,"Abhi");
    mycash.head->next=NULL;
    mycash.head->prev=NULL;
    mycash.size=1;
    // insert(&mycash, 1, "Abhi");
    insert(&mycash, 2, "Abhiram");
    insert(&mycash, 3, "Abhijeet");
    insert(&mycash, 4, "Abhilaksh");
    insert(&mycash, 5, "Abhinav");
    debug_print_cache(&mycash);
    printf("\n");
    printf("Abhiram: %d\n", find_in_cache(&mycash, "Abhiram"));
    printf("Akshara: %d\n", find_in_cache(&mycash, "Akshara"));

    delete_from_cache(&mycash, "Abhiram");
    printf("Abhiram: %d\n", find_in_cache(&mycash, "Abhiram"));

    return 0;
}