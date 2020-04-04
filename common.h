#ifndef __COMMON_H_
#define __COMMON_H_
#define _XOPEN_SOURCE 600
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "list.h"

#define MAX_MSG_BYTES 2048
#define MSG_BUFFER_COUNT 30

//my wrapper structure about the shared-memory content
typedef struct ShmMessages
{
    sem_t can_send_count;
    sem_t can_recv_count;

    sem_t data_lock;
    int current_read_index;
    int current_write_index;
    char message_data[MSG_BUFFER_COUNT][MAX_MSG_BYTES];
}*PShmMessages;
//some useful macro
#define MAX_WAIT_TIME 60
#define CLIENT_IS_DEAD "CLIENT_IS_DEAD"
#define LINE_COUNT_REMINDER "LINE_COUNT_REMINDER12344321"
#define USING_SHM_NAME "/myshm12344321"
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define ASSERT_LOG(x)                                     \
    {                                                     \
        if (!(x))                                         \
        {                                                 \
            printf("%s %s %d\n", #x, __FILE__, __LINE__); \
            assert(0);                                    \
        }                                                 \
    }

//useful wrapper function about message

//buffer: contains MAX_MSG_BYTES bytes.
int recv_message(PShmMessages psm, char* buffer, int enable_time_out);
//buffer: contains MAX_MSG_BYTES bytes.
int send_message(PShmMessages psm, char* buffer, int enable_time_out);
PShmMessages access_messages(const char* name, int create_or_read);
char *clone_string(const char *str);

//hash function
unsigned long hash_function(char *key);
//hash table structure
typedef struct HashTableInner
{
    PList* list_array;
    int hash_size;
}*PHashTable;
//create hash table
PHashTable createHashTable(int hash_size);
//free hash table
void freeHashTable(PHashTable pht);

#endif //__COMMON_H_