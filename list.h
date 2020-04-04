#ifndef __LIST_H_
#define __LIST_H_
#define _GNU_SOURCE
#include <pthread.h>
typedef struct LNodeInner
{
    char* data;
    struct LNodeInner* next;
    struct LNodeInner* prev;
    int size;
}LNode, *PLNode;

//create a node
PLNode makeNode(char* data, int size);

//list structure
typedef struct ListInner
{
    pthread_rwlock_t  rwlock;
    PLNode head;
    PLNode tail;
    int elements_count;
    int elements_total_size;
}List, *PList;

//make a list
PList makeList();
//insert new data into list
int ListInsert(PList pl, char* data);
//remove data from pl
int ListRemove(PList pl, char* data);
//search data in teh list
PLNode ListSearch(PList pl, char* data, int lock);
//free list
void freeList(PList pl);


#endif//__LIST_H_