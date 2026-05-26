#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void get_time_string(char *format, char *result)
{
    time_t now = time(NULL);

    struct tm *t = localtime(&now);

    if (strcmp(format, "dd/mm/yyyy") == 0)
    {
        strftime(result,
                 100,
                 "%d/%m/%Y",
                 t);
    }
    else if (strcmp(format, "dd/mm/yy") == 0)
    {
        strftime(result,
                 100,
                 "%d/%m/%y",
                 t);
    }
    else if (strcmp(format, "mm/dd/yyyy") == 0)
    {
        strftime(result,
                 100,
                 "%m/%d/%Y",
                 t);
    }
    else if (strcmp(format, "mm/dd/yy") == 0)
    {
        strftime(result,
                 100,
                 "%m/%d/%y",
                 t);
    }
    else
    {
        strcpy(result,
               "ERROR: Unsupported format\n");
    }
}

void *client_handler(void *arg)
{
    int client_socket = *(int *)arg;

    free(arg);

    char buffer[BUFFER_SIZE];

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(client_socket,
                         buffer,
                         BUFFER_SIZE - 1,
                         0);

        if (bytes <= 0)
        {
            printf("Client disconnected\n");

            close(client_socket);

            pthread_exit(NULL);
        }

        buffer[bytes] = '\0';

        buffer[strcspn(buffer, "\r\n")] = '\0';

        printf("Client send: %s\n", buffer);

        char command[100];
        char format[100];

        int n = sscanf(buffer,
                       "%s %s",
                       command,
                       format);

        if (n != 2)
        {
            char *msg =
                "ERROR: Syntax must be GET_TIME [format]\n";

            send(client_socket,
                 msg,
                 strlen(msg),
                 0);

            continue;
        }

        if (strcmp(command, "GET_TIME") != 0)
        {
            char *msg =
                "ERROR: Unknown command\n";

            send(client_socket,
                 msg,
                 strlen(msg),
                 0);

            continue;
        }

        char result[100];

        get_time_string(format, result);

        strcat(result, "\n");

        send(client_socket,
             result,
             strlen(result),
             0);
    }

    return NULL;
}

int main()
{
    int server_socket;
    int client_socket;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET,
                           SOCK_STREAM,
                           0);

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

    printf("Time server running on port %d...\n",
           PORT);

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

        printf("New client connected\n");

        pthread_t tid;

        int *pclient = malloc(sizeof(int));

        *pclient = client_socket;

        pthread_create(&tid,
                       NULL,
                       client_handler,
                       pclient);

        pthread_detach(tid);
    }

    close(server_socket);

    return 0;
}