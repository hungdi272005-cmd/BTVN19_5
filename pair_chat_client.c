#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void* receive_thread(void* arg) {
    int client_fd = *(int*)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            break;
        }
        printf("%s", buffer);
        fflush(stdout);
    }

    close(client_fd);
    exit(0);
}

int main() {
    int client_fd;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
    pthread_t recv_tid;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return 1;
    }

    pthread_create(&recv_tid, NULL, receive_thread, &client_fd);
    pthread_detach(recv_tid);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) break;
        if (send(client_fd, buffer, strlen(buffer), 0) <= 0) {
            break;
        }
    }

    close(client_fd);
    return 0;
}