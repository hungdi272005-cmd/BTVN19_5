#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 2048

int check_login(char *user, char *pass)
{
    FILE *fp = fopen("users.txt", "r");

    if (fp == NULL)
    {
        perror("users.txt");
        return 0;
    }

    char file_user[100];
    char file_pass[100];

    while (fscanf(fp, "%s %s", file_user, file_pass) != EOF)
    {
        if (strcmp(user, file_user) == 0 &&
            strcmp(pass, file_pass) == 0)
        {
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

void send_file_to_client(int client_socket, char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        char *msg = "Khong mo duoc file ket qua\n";
        send(client_socket, msg, strlen(msg), 0);
        return;
    }

    char buffer[BUFFER_SIZE];

    while (fgets(buffer, BUFFER_SIZE, fp) != NULL)
    {
        send(client_socket, buffer, strlen(buffer), 0);
    }

    fclose(fp);
}

void *client_handler(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char user[100];
    char pass[100];

    // ===== LOGIN =====

    send(client_socket, "Username: ", 10, 0);

    memset(buffer, 0, BUFFER_SIZE);

    recv(client_socket, buffer, BUFFER_SIZE, 0);

    buffer[strcspn(buffer, "\r\n")] = '\0';

    strcpy(user, buffer);

    send(client_socket, "Password: ", 10, 0);

    memset(buffer, 0, BUFFER_SIZE);

    recv(client_socket, buffer, BUFFER_SIZE, 0);

    buffer[strcspn(buffer, "\r\n")] = '\0';

    strcpy(pass, buffer);

    if (!check_login(user, pass))
    {
        send(client_socket,
             "Dang nhap that bai!\n",
             21,
             0);

        close(client_socket);

        pthread_exit(NULL);
    }

    send(client_socket,
         "Dang nhap thanh cong!\n",
         24,
         0);

    printf("%s da dang nhap\n", user);

    // ===== COMMAND =====

    while (1)
    {
        send(client_socket,
             "\ncmd> ",
             7,
             0);

        memset(buffer, 0, BUFFER_SIZE);

        int bytes = recv(client_socket,
                         buffer,
                         BUFFER_SIZE,
                         0);

        if (bytes <= 0)
        {
            printf("%s da ngat ket noi\n", user);
            close(client_socket);
            pthread_exit(NULL);
        }

        buffer[bytes] = '\0';

        buffer[strcspn(buffer, "\r\n")] = '\0';

        // exit
        if (strcmp(buffer, "exit") == 0)
        {
            send(client_socket,
                 "Tam biet!\n",
                 10,
                 0);

            close(client_socket);

            pthread_exit(NULL);
        }

        // Tao command
        char command[BUFFER_SIZE];

        sprintf(command,
                "%s > out.txt 2>&1",
                buffer);

        // Chay lenh
        system(command);

        // Gui ket qua
        send_file_to_client(client_socket, "out.txt");
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

    printf("Telnet server dang chay tai port %d...\n",
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

        printf("Client moi ket noi\n");

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