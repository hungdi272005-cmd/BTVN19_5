#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void trim_newline(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        len--;
    }
}

int main() {
    int client_fd;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];

    client_fd = socket(AF_INET, SOCK_STREAM, 0); // Đã sửa thành SOCK_STREAM chuẩn
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        return 1;
    }

    // 1. Nhận và in trực tiếp chuỗi danh sách file (hoặc lỗi trống thư mục) từ server
    memset(buffer, 0, BUFFER_SIZE);
    int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes <= 0) {
        close(client_fd);
        return 0;
    }
    
    // In ra chính xác những gì server gửi (OK N\r\n...)
    printf("%s", buffer); 

    if (strncmp(buffer, "ERROR No files to download", 26) == 0) {
        close(client_fd);
        return 0;
    }

    // 2. Vòng lặp gửi yêu cầu tên file
    char filename[256];
    while (1) {
        if (fgets(filename, sizeof(filename), stdin) == NULL) break;
        
        // Gửi nguyên bản chuỗi tên file (giữ \n hoặc \r\n tùy cách nhập để server xử lý)
        send(client_fd, filename, strlen(filename), 0);

        // Nhận phản hồi từ Server
        memset(buffer, 0, BUFFER_SIZE);
        bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) break;

        // In phản hồi của server ra màn hình (OK N\r\n hoặc ERROR...)
        printf("%s", buffer);

        if (strncmp(buffer, "OK ", 3) == 0) {
            long file_size = atol(buffer + 3);
            
            // Tìm vị trí kết thúc của chuỗi "OK N\r\n" để lấy dữ liệu file đi kèm nếu có
            char *header_end = strstr(buffer, "\r\n");
            long header_len = (header_end - buffer) + 2;
            long initial_data_len = bytes - header_len;

            // Mở file lưu cục bộ
            trim_newline(filename);
            char out_name[300];
            sprintf(out_name, "downloaded_%s", filename);
            FILE *f = fopen(out_name, "wb");
            
            if (f != NULL) {
                // Nếu trong lần nhận đầu có chứa một phần nội dung file, ghi vào trước
                if (initial_data_len > 0) {
                    fwrite(buffer + header_len, 1, initial_data_len, f);
                }
                
                long total_received = initial_data_len;
                while (total_received < file_size) {
                    int r = recv(client_fd, buffer, BUFFER_SIZE, 0);
                    if (r <= 0) break;
                    fwrite(buffer, 1, r, f);
                    total_received += r;
                }
                fclose(f);
            }
            break; // Xong nhiệm vụ, đóng kết nối
        }
    }

    close(client_fd);
    return 0;
}