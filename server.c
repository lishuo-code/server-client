#include "common.h"
#include <pthread.h>
PHashTable global_hash_table = 0;
void *client_handler(void *p);
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Please sepecify hash table size!\n");
        exit(1);
    }
    //creathe the hash table
    global_hash_table = createHashTable(atoi(argv[1]));

    //access server shm
    struct ShmMessages *psm = access_messages(USING_SHM_NAME, 1);

    while (1)
    {
        char message[MAX_MSG_BYTES] = {0};
        //access child request
        recv_message(psm, message, 0);
        int len = strlen(message);
        if (len == 0)
            continue;
        pthread_t thread_id = -1;
        //sub-thread process for every client request
        pthread_create(&thread_id, 0, client_handler, clone_string(message));
        pthread_detach(thread_id);
    }

    freeHashTable(global_hash_table);
}

void *client_handler(void *p)
{
    char *client_message_name = (char *)p;
    printf("server for %s begin\n", client_message_name);
    char client_send_message_name[MAX_MSG_BYTES] = {0};
    char client_recv_message_name[MAX_MSG_BYTES] = {0};
    sscanf(client_message_name, "%s %s", client_send_message_name, client_recv_message_name);
    //access 2 client shm
    struct ShmMessages *psm_client_send = access_messages(client_send_message_name, 0);
    struct ShmMessages *psm_client_recv = access_messages(client_recv_message_name, 0);
    while (psm_client_send && psm_client_recv)
    {
        char message[MAX_MSG_BYTES] = {0};
        if (recv_message(psm_client_send, message, 1) < 0)
        {
            break;
        }
        if (strcmp(message, CLIENT_IS_DEAD) == 0)
        {
            break;
        }

        char operation[MAX_MSG_BYTES] = {0};
        char value[MAX_MSG_BYTES] = {0};
        char send_msg[MAX_MSG_BYTES] = {0};
        int has_send_message = 0;
        strcpy(send_msg, "invalid input");

        //process for every commnd
        if (sscanf(message, "%s %s", operation, value) == 2)
        {
            if (strcmp(operation, "insert") == 0)
            {
                int bucket_idx = hash_function(value) % global_hash_table->hash_size;
                char *copy_str = clone_string(value);
                if (ListInsert(global_hash_table->list_array[bucket_idx], copy_str) == 0)
                {
                    snprintf(send_msg, MAX_MSG_BYTES, "insert it into bucket %d: successful.", bucket_idx);
                }
                else
                {
                    snprintf(send_msg, MAX_MSG_BYTES, "insert it into bucket %d: failed.", bucket_idx);
                    free(copy_str);
                }
            }
            else if (strcmp(operation, "delete") == 0)
            {
                int bucket_idx = hash_function(value) % global_hash_table->hash_size;
                if (ListRemove(global_hash_table->list_array[bucket_idx], value) == 0)
                {
                    snprintf(send_msg, MAX_MSG_BYTES, "remove it from bucket %d: successful.", bucket_idx);
                }
                else
                {
                    snprintf(send_msg, MAX_MSG_BYTES, "remove it from bucket %d: failed.", bucket_idx);
                }
            }
            //multiple line data may generate here.
            else if (strcmp(operation, "read") == 0)
            {
                int x = atoi(value);
                if (x < 0 || x >= global_hash_table->hash_size)
                {
                    snprintf(send_msg, MAX_MSG_BYTES, "a bucket number from [0, %d) should be inputed.", global_hash_table->hash_size);
                }
                else
                {
                    has_send_message = 1;
                    PList cur_list = global_hash_table->list_array[x];
                    pthread_rwlock_rdlock(&cur_list->rwlock);
                    int total_size = cur_list->elements_total_size + cur_list->elements_count - 1;
                    if (total_size <= 0)
                    {
                        has_send_message = 1;
                        send_message(psm_client_recv, "", 1);
                        pthread_rwlock_unlock(&cur_list->rwlock);
                    }
                    else
                    {
                        char *total_buffer = malloc(total_size);

                        PLNode cur = cur_list->head;
                        int offset = 0;
                        for (int i = 0; i < cur_list->elements_count; i++)
                        {
                            if (i + 1 == cur_list->elements_count)
                            {
                                sprintf(total_buffer + offset, "%s", cur->data);
                            }
                            else
                            {
                                sprintf(total_buffer + offset, "%s ", cur->data);
                                offset += 1;
                            }
                            offset += cur->size;
                            cur = cur->next;
                        }
                        pthread_rwlock_unlock(&cur_list->rwlock);
                        offset = 0;

                        //calculate line count firstly
                        int line_count = 0;

                        int copy_total_size = total_size;
                        while (copy_total_size >= MAX_MSG_BYTES)
                        {
                            copy_total_size -= (MAX_MSG_BYTES - 1);
                            offset += MAX_MSG_BYTES - 1;
                            line_count++;
                        }
                        if (copy_total_size > 0)
                        {
                            line_count++;
                        }

                        char message_tmp[MAX_MSG_BYTES] = {0};
                        //if line_count>=2, the line count information should be sended to the client.
                        if (line_count >= 2)
                        {
                            snprintf(message_tmp, MAX_MSG_BYTES, "%s%d\n", LINE_COUNT_REMINDER, line_count);
                            if (send_message(psm_client_recv, message_tmp, 1) < 0)
                            {
                                break;
                            }
                        }

                        //send every single line.
                        int send_error = 0;
                        offset = 0;
                        while (total_size >= MAX_MSG_BYTES)
                        {
                            memcpy(message_tmp, total_buffer + offset, MAX_MSG_BYTES - 1);
                            if (send_message(psm_client_recv, message_tmp, 1) < 0)
                            {
                                send_error = 1;
                                break;
                            }
                            total_size -= (MAX_MSG_BYTES - 1);
                            offset += MAX_MSG_BYTES - 1;
                        }
                        if (total_size > 0)
                        {
                            char message_tmp[MAX_MSG_BYTES] = {0};
                            memcpy(message_tmp, total_buffer + offset, total_size);
                            if (send_message(psm_client_recv, message_tmp, 1) < 0)
                            {
                                send_error = 1;
                                break;
                            }
                        }

                        free(total_buffer);
                        if (send_error)
                        {
                            break;
                        }
                    }
                }
            }
        }
        if (has_send_message == 0)
        {
            if (send_message(psm_client_recv, send_msg, 1) < 0)
            {
                break;
            }
        }
    }
    printf("server for %s end\n", client_message_name);
    shm_unlink(client_message_name);
    free(client_message_name);
}