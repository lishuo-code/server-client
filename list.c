#include "list.h"
#include <stdlib.h>
#include <string.h>
//create a node
PLNode makeNode(char *data, int size)
{
    PLNode result = calloc(sizeof(LNode), 1);
    result->data = data;
    result->size = size;
    return result;
}
//make a list, and fill its CompareFunction, FreeFunction
PList makeList()
{
    PList result = calloc(sizeof(List), 1);
    pthread_rwlock_init(&result->rwlock, 0);
    return result;
}
////insert new data into list
int ListInsert(PList pl, char *data)
{
    if (ListSearch(pl, data, 1))
    {
        return -1;
    }
    pthread_rwlock_wrlock(&pl->rwlock);
    if (ListSearch(pl, data, 0))
    {
        pthread_rwlock_unlock(&pl->rwlock);
        return -1;
    }
    int size = strlen(data);
    PLNode node = makeNode(data, size);
    if (!pl->head)
    {
        pl->head = node;
        pl->tail = pl->head;
    }
    else
    {
        pl->tail->next = node;
        node->prev = pl->tail;
        pl->tail = node;
    }
    pl->elements_count++;
    pl->elements_total_size += size;
    pthread_rwlock_unlock(&pl->rwlock);
    return 0;
}
//remove the node from pl
//if node is not null, it must exist in pl
static void ListRemove1(PList pl, PLNode node)
{
    if (node)
    {
        pl->elements_count--;
        pl->elements_total_size -= strlen(node->data);
        if (node->prev)
        {
            node->prev->next = node->next;
        }
        else
        {
            pl->head = node->next;
        }
        if (node->next)
        {
            node->next->prev = node->prev;
        }
        else
        {
            pl->tail = node->prev;
        }
        free(node->data);
        free(node);
    }
}
//remove value from pl, first find it and then remove the node form pl
int ListRemove(PList pl, char *data)
{
    if (ListSearch(pl, data, 1))
    {
        pthread_rwlock_wrlock(&pl->rwlock);
        PLNode n = ListSearch(pl, data, 0);
        if (n)
        {
            ListRemove1(pl, n);
            pthread_rwlock_unlock(&pl->rwlock);
            return 0;
        }
        pthread_rwlock_unlock(&pl->rwlock);
        return -1;
    }
    return -1;
}
//search data in teh list
PLNode ListSearch(PList pl, char *data, int lock)
{
    if (lock)
    {
        pthread_rwlock_rdlock(&pl->rwlock);
    }
    PLNode cur = pl->head;
    while (cur)
    {
        if (strcmp(data, cur->data) == 0)
        {
            break;
        }
        cur = cur->next;
    }
    if (lock)
    {
        pthread_rwlock_unlock(&pl->rwlock);
    }
    return cur;
}
//release all the memory by pl.
void freeList(PList pl)
{
    if (pl)
    {
        PLNode cur = pl->head;
        while (cur)
        {
            PLNode n = cur->next;
            free(cur->data);
            free(cur);
            cur = n;
        }
        pthread_rwlock_destroy(&pl->rwlock);
        free(pl);
    }
}