#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define THREAD_POOL_SIZE 5
#define BUFFER_SIZE 4096

int server_socket;

void *worker_thread(void *arg)
{
    while (1)
    {
        int client_socket;

        struct sockaddr_in client_addr;

        socklen_t client_len = sizeof(client_addr);

        // Chờ client kết nối
        client_socket = accept(server_socket,
                               (struct sockaddr *)&client_addr,
                               &client_len);

        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }

        printf("New client connected\n");

        // Nhận request HTTP
        char buffer[BUFFER_SIZE];

        int bytes = recv(client_socket,
                         buffer,
                         BUFFER_SIZE - 1,
                         0);

        if (bytes <= 0)
        {
            close(client_socket);
            continue;
        }

        buffer[bytes] = '\0';

        printf("======= HTTP REQUEST =======\n");
        printf("%s\n", buffer);

        // Nội dung HTML
        char body[] =
            "<html>"
            "<body>"
            "<h1>Xin chao cac ban</h1>"
            "<h2>HTTP Server Prethreading</h2>"
            "</body>"
            "</html>";

        // HTTP Response
        char response[BUFFER_SIZE];

        sprintf(response,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n"
                "\r\n"
                "%s",
                strlen(body),
                body);

        // Gửi về client
        send(client_socket,
             response,
             strlen(response),
             0);

        // Đóng kết nối
        close(client_socket);

        printf("Client disconnected\n\n");
    }

    return NULL;
}

int main()
{
    struct sockaddr_in server_addr;

    // Tạo socket
    server_socket = socket(AF_INET,
                           SOCK_STREAM,
                           0);

    if (server_socket < 0)
    {
        perror("socket");
        return 1;
    }

    // Bind
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

    // Listen
    if (listen(server_socket, 10) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("HTTP Server running at port %d...\n",
           PORT);

    // ===== Tạo sẵn thread pool =====

    pthread_t threads[THREAD_POOL_SIZE];

    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_create(&threads[i],
                       NULL,
                       worker_thread,
                       NULL);

        printf("Thread %d created\n", i);
    }

    // Chờ thread
    for (int i = 0; i < THREAD_POOL_SIZE; i++)
    {
        pthread_join(threads[i], NULL);
    }

    close(server_socket);

    return 0;
}