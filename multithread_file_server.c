#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define STORAGE_DIR "./files" // Thư mục chứa file của bạn trong ảnh

void trim_newline(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

void sigchld_handler(int s) {
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    DIR *dir = opendir(STORAGE_DIR);
    char file_names[100][256];
    int file_count = 0;

    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                strcpy(file_names[file_count++], entry->d_name);
            }
        }
        closedir(dir);
    }

    // Yêu cầu 2: Nếu không có file nào
    if (file_count == 0) {
        char *err = "ERROR No files to download\r\n";
        send(client_fd, err, strlen(err), 0);
        close(client_fd);
        exit(0);
    }

    // Yêu cầu 1: Gửi danh sách file dạng "OK N\r\nfile1\r\nfile2\r\n\r\n"
    char response[4096];
    sprintf(response, "OK %d\r\n", file_count);
    for (int i = 0; i < file_count; i++) {
        strcat(response, file_names[i]);
        strcat(response, "\r\n");
    }
    strcat(response, "\r\n"); 
    send(client_fd, response, strlen(response), 0);

    // Yêu cầu 3: Nhận tên file và xử lý
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;

        trim_newline(buffer);
        char path[512];
        sprintf(path, "%s/%s", STORAGE_DIR, buffer);

        struct stat st;
        if (stat(path, &st) == 0) {
            // Nếu file tồn tại: Gửi "OK N\r\n" + nội dung file -> Đóng kết nối
            char header[128];
            sprintf(header, "OK %ld\r\n", st.st_size);
            send(client_fd, header, strlen(header), 0);

            FILE *f = fopen(path, "rb");
            if (f != NULL) {
                int read_bytes;
                while ((read_bytes = fread(buffer, 1, BUFFER_SIZE, f)) > 0) {
                    send(client_fd, buffer, read_bytes, 0);
                }
                fclose(f);
            }
            break; // Gửi xong nội dung file -> Ngắt kết nối luôn
        } else {
            // Nếu không tồn tại: Chỉ gửi thông báo lỗi và yêu cầu gửi lại (giữ vòng lặp)
            char *err_msg = "ERROR File not found\r\n";
            send(client_fd, err_msg, strlen(err_msg), 0);
        }
    }
    close(client_fd);
    exit(0);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    struct sigaction sa;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;
        if (fork() == 0) {
            close(server_fd);
            handle_client(client_fd);
        }
        close(client_fd);
    }
    return 0;
}