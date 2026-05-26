#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct
{
    int socket;
    char id[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message, int sender_socket)
{
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].socket != sender_socket)
        {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }

    pthread_mutex_unlock(&mutex);
}

void remove_client(int socket)
{
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].socket == socket)
        {
            for (int j = i; j < client_count - 1; j++)
            {
                clients[j] = clients[j + 1];
            }

            client_count--;
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
}

void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char client_id[50];

    // Yêu cầu nhập ID
    send(client_socket,
         "Nhap theo cu phap client_id:client_name\n",
         41,
         0);

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0)
        {
            close(client_socket);
            pthread_exit(NULL);
        }

        buffer[bytes] = '\0';

        // Xóa \n
        buffer[strcspn(buffer, "\r\n")] = '\0';

        char *token = strtok(buffer, ":");

        if (token != NULL)
        {
            strcpy(client_id, token);
            break;
        }
        else
        {
            send(client_socket,
                 "Sai cu phap. Nhap lai!\n",
                 25,
                 0);
        }
    }

    // Lưu client
    pthread_mutex_lock(&mutex);

    clients[client_count].socket = client_socket;
    strcpy(clients[client_count].id, client_id);

    client_count++;

    pthread_mutex_unlock(&mutex);

    printf("%s da ket noi\n", client_id);

    // Nhận tin nhắn
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0)
        {
            printf("%s da ngat ket noi\n", client_id);

            remove_client(client_socket);

            close(client_socket);

            pthread_exit(NULL);
        }

        buffer[bytes] = '\0';

        buffer[strcspn(buffer, "\r\n")] = '\0';

        // Lấy thời gian
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        char time_str[100];

        strftime(time_str,
                 sizeof(time_str),
                 "%Y/%m/%d %H:%M:%S",
                 t);

        char message[BUFFER_SIZE];

        sprintf(message,
                "%s %s: %s\n",
                time_str,
                client_id,
                buffer);

        printf("%s", message);

        broadcast_message(message, client_socket);
    }

    return NULL;
}

int main()
{
    int server_socket, client_socket;

    struct sockaddr_in server_addr, client_addr;

    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0)
    {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    if (listen(server_socket, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("Chat server dang chay tai port %d...\n", PORT);

    while (1)
    {
        client_socket = accept(server_socket,
                               (struct sockaddr *)&client_addr,
                               &client_len);

        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }

        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;

        pthread_t tid;

        pthread_create(&tid,
                       NULL,
                       handle_client,
                       pclient);

        pthread_detach(tid);
    }

    close(server_socket);

    return 0;
}