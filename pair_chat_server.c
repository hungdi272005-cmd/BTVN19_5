#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void* forward_thread(void* arg) {
    int* fds = (int*)arg;
    int src = fds[0];
    int dst = fds[1];
    free(arg);

    char buffer[BUFFER_SIZE];
    char formatted_msg[BUFFER_SIZE + 32]; // Thêm khoảng trống cho chuỗi định danh

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(src, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            break;
        }
        
        // Thêm định danh [Client X]: vào trước nội dung tin nhắn gửi đi
        snprintf(formatted_msg, sizeof(formatted_msg), "[Client %d]: %s", src, buffer);
        
        // Chuyển tiếp gói tin đã định danh sang client kia
        send(dst, formatted_msg, strlen(formatted_msg), 0);
    }

    close(src);
    close(dst);
    pthread_exit(NULL);
}

void start_chat_pair(int c1, int c2) {
    pthread_t t1, t2;

    // Hướng 1: Client 1 gửi -> Client 2 nhận
    int* args1 = malloc(2 * sizeof(int));
    args1[0] = c1; args1[1] = c2;
    pthread_create(&t1, NULL, forward_thread, (void*)args1);
    pthread_detach(t1);

    // Hướng 2: Client 2 gửi -> Client 1 nhận
    int* args2 = malloc(2 * sizeof(int));
    args2[0] = c2; args2[1] = c1;
    pthread_create(&t2, NULL, forward_thread, (void*)args2);
    pthread_detach(t2);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);

    int waiting_client = -1;

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        if (waiting_client == -1) {
            waiting_client = client_fd;
        } else {
            int client1 = waiting_client;
            int client2 = client_fd;
            waiting_client = -1;

            start_chat_pair(client1, client2);
        }
    }

    close(server_fd);
    return 0;
}