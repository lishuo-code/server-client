#include "common.h"

int main()
{
    //access server shm
    struct ShmMessages *psm_server = access_messages(USING_SHM_NAME, 0);
    srand(time(0));
    char client_send_message_name[MAX_MSG_BYTES] = {0};
    char client_recv_message_name[MAX_MSG_BYTES] = {'/', 0};
    snprintf(client_send_message_name, MAX_MSG_BYTES, "/%d_%d_%d", (int)time(NULL), (int)getpid(), (int)rand());
    int len = strlen(client_send_message_name);
    for (int i = 1; i < len; i++)
    {
        client_recv_message_name[i] = client_send_message_name[len - i];
    }
    char client_message_name[MAX_MSG_BYTES] = {0};
    //access 2 client shm: send or receive
    snprintf(client_message_name, MAX_MSG_BYTES, "%s %s", client_send_message_name, client_recv_message_name);
    struct ShmMessages *psm_client_send = access_messages(client_send_message_name, 1);
    struct ShmMessages *psm_client_recv = access_messages(client_recv_message_name, 1);
    //send the client shm name to the server
    send_message(psm_server, client_message_name, 1);
    {
        while (1)
        {
            char line[MAX_MSG_BYTES] = {0};
            if (!fgets(line, MAX_MSG_BYTES, stdin))
            {
                break;
            }
            int len = strlen(line);
            if (line > 0 && line[len - 1] == '\n')
            {
                line[len - 1] = 0;
            }
            char message[MAX_MSG_BYTES] = {0};
            snprintf(message, MAX_MSG_BYTES, "%s", line);
            if (strcmp(message, "quit") == 0)
            {
                break;
            }
            //send firstly
            if (send_message(psm_client_send, message, 1) < 0)
            {
                break;
            }
            if (recv_message(psm_client_recv, message, 1) < 0)
            {
                break;
            }
            //process multiple-line data
            if (strstr(message, LINE_COUNT_REMINDER) != 0)
            {
                int recv_error = 0;
                int lc = atoi(message + sizeof(LINE_COUNT_REMINDER) - 1);
                for (int i = 0; i < lc; i++)
                {
                    if (recv_message(psm_client_recv, message, 1) < 0)
                    {
                        recv_error = 1;
                        break;
                    }
                    else
                    {
                        printf("%s", message);
                    }
                }
                printf("\n");
                if(recv_error)
                {
                    break;
                }
            }
            //process single-line data
            else
            {
                printf("%s\n", message);
            }
        }
    }
    send_message(psm_client_send, CLIENT_IS_DEAD, 1);
    shm_unlink(client_send_message_name);
    shm_unlink(client_recv_message_name);
}