#include "common.h"
int recv_message(PShmMessages psm, char *buffer, int enable_time_out)
{
    if (enable_time_out)
    {
        /*
            if the wait is failed after default_max_wait_time seconds.
            I treat the error reason is: the opponent is dead,
            although there are many reasons to cause the case.
        */
        static struct timespec default_max_wait_time;
        clock_gettime(CLOCK_REALTIME, &default_max_wait_time);
        default_max_wait_time.tv_sec += MAX_WAIT_TIME;
        if (sem_timedwait(&psm->can_recv_count, &default_max_wait_time) != 0)
        {
            return -1;
        }
        if (sem_timedwait(&psm->data_lock, &default_max_wait_time) != 0)
        {
            return -1;
        }
    }
    else
    {
        sem_wait(&psm->can_recv_count);
        sem_wait(&psm->data_lock);
    }
    strcpy(buffer, psm->message_data[psm->current_read_index]);
    if (++(psm->current_read_index) >= MSG_BUFFER_COUNT)
    {
        psm->current_read_index = 0;
    }
    sem_post(&psm->data_lock);
    sem_post(&psm->can_send_count);
    return 0;
}
int send_message(PShmMessages psm, char *buffer, int enable_time_out)
{
    if (enable_time_out)
    {
        /*
            if the wait is failed after default_max_wait_time seconds.
            I treat the error reason is: the opponent is dead,
            although there are many reasons to cause the case.
        */
        static struct timespec default_max_wait_time;
        clock_gettime(CLOCK_REALTIME, &default_max_wait_time);
        default_max_wait_time.tv_sec += MAX_WAIT_TIME;
        if (sem_timedwait(&psm->can_send_count, &default_max_wait_time) != 0)
        {
            return -1;
        }
        if (sem_timedwait(&psm->data_lock, &default_max_wait_time) != 0)
        {
            return -1;
        }
    }
    else
    {
        sem_wait(&psm->can_send_count);
        sem_wait(&psm->data_lock);
    }

    int cur_put_idx = psm->current_write_index;
    if ((++(psm->current_write_index)) >= MSG_BUFFER_COUNT)
    {
        psm->current_write_index = 0;
    }
    sem_post(&psm->data_lock);
    strcpy(psm->message_data[cur_put_idx], buffer);
    sem_post(&psm->can_recv_count);
    return 0;
}

PShmMessages access_messages(const char *name, int create_or_read)
{
    int fd = -1;
    if (create_or_read)
    {
        shm_unlink(name);
        fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, FILE_MODE);
    }
    else
    {
        fd = shm_open(name, O_RDWR, FILE_MODE);
    }
    if (fd < 0)
    {
        printf("%s can not be accessed by %s\n", name, create_or_read ? "created" : "read");
        return 0;
    }
    if (create_or_read)
    {
        ftruncate(fd, sizeof(struct ShmMessages));
    }
    struct ShmMessages *psm = (struct ShmMessages *)mmap(0, sizeof(struct ShmMessages), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT_LOG(psm != MAP_FAILED);
    close(fd);
    
    if (create_or_read)
    {
        sem_init(&psm->data_lock, 1, 1);
        sem_init(&psm->can_send_count, 1, MSG_BUFFER_COUNT);
        sem_init(&psm->can_recv_count, 1, 0);
        psm->current_read_index = 0;
        psm->current_write_index = 0;
    }
    return psm;
}
char *clone_string(const char *str)
{
    if (!str)
        return 0;
    int length = strlen(str);
    char *result = (char *)malloc(length + 1);
    strcpy(result, str);
    result[length] = 0;
    return result;
}
unsigned long hash_function(char *key)
{
    unsigned long result = 5381;
    int c;
    while ((c = *key++) != '\0')
        result = result * 33 + c;
    return result;
}

PHashTable createHashTable(int hash_size)
{
    if (hash_size <= 0)
    {
        hash_size = 1;
    }
    PHashTable result = calloc(sizeof(struct HashTableInner), 1);
    result->hash_size = hash_size;
    result->list_array = (PList *)calloc(sizeof(PList) * hash_size, 1);
    for (int i = 0; i < hash_size; i++)
    {
        result->list_array[i] = makeList();
    }
    return result;
}
void freeHashTable(PHashTable pht)
{
    if (pht)
    {
        for (int i = 0; i < pht->hash_size; i++)
        {
            freeList(pht->list_array[i]);
        }
        free(pht->list_array);
        free(pht);
    }
}