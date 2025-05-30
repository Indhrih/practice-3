#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 4096

void send_404(int client_socket) {
    char response[] = "HTTP/1.1 404 Not Found\r\n"
                      "Content-Type: text/html; charset=utf-8\r\n\r\n"
                      "<html><body><h1>404 Не найдено</h1></body></html>";
    send(client_socket, response, strlen(response), 0);
}

void send_image(int client_socket, const char *image_path) {
    int fd = open(image_path, O_RDONLY);
    if (fd == -1) {
        perror("Failed to open image");
        send_404(client_socket);
        return;
    }

    struct stat file_stat;
    fstat(fd, &file_stat);
    off_t file_size = file_stat.st_size;

    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: image/jpeg\r\n"
             "Content-Length: %ld\r\n\r\n",
             file_size);

    send(client_socket, header, strlen(header), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }
    close(fd);
}

// Функция для преобразования %-последовательностей в символы
void urldecode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2]))) {
            if (a >= 'a') a -= 'a'-'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a'-'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void handle_request(int client_socket, const char *request) {
    char *student_info = strstr(request, "GET /?message=");
    if (!student_info) {
        send_404(client_socket);
        return;
    }

    student_info += strlen("GET /?message=");
    char *end = strchr(student_info, ' ');
    if (!end) {
        send_404(client_socket);
        return;
    }

    char encoded_info[256];
    strncpy(encoded_info, student_info, end - student_info);
    encoded_info[end - student_info] = '\0';

    // Декодируем URL-кодированную строку
    char decoded_info[256];
    urldecode(decoded_info, encoded_info);

    char response[2048];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html; charset=utf-8\r\n\r\n"
             "<!DOCTYPE html>"
             "<html>"
             "<head><title>Студент МИРЭА</title></head>"
             "<body style='text-align: center; font-family: Arial;'>"
             "<h1>%s</h1>"
             "<img src='/gerb' alt='Герб МИРЭА' style='width: 300px; margin-top: 20px;'>"
             "</body></html>",
             decoded_info);

    send(client_socket, response, strlen(response), 0);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Сервер запущен на http://localhost:%d\n", PORT);

    while (1) {
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        read(client_socket, buffer, BUFFER_SIZE);

        if (strstr(buffer, "GET /?message=")) {
            handle_request(client_socket, buffer);
        } else if (strstr(buffer, "GET /gerb")) {
            send_image(client_socket, "MIREA_Gerb_Colour.jpg");
        } else {
            send_404(client_socket);
        }

        close(client_socket);
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(server_fd);
    return 0;
}
