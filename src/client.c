#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../include/utils.h"

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    const char *server_ip = "127.0.0.1"; 
    int port = SERVER_PORT; 
    char request[BUFSIZE];
    char response[BUFSIZE];
    ssize_t bytes_sent, bytes_received;

    printf("Connecting to server at %s:%d...\n",server_ip, port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    printf("Attempting to connect to the server...\n");
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection to the server failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connection to the server successful.\n");

    printf("Enter request type: ");
    scanf("%s", request);

    bytes_sent = send(sockfd, request, strlen(request), 0);
    if (bytes_sent == -1) {
        perror("Failed to send request");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    bytes_received = recv(sockfd, response, BUFSIZE, 0);
    if (bytes_received == -1) {
        perror("Failed to receive response from server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    response[bytes_received] = '\0'; 
    printf("Server response: %s\n", response);

    if (strncmp(request, "UPLOAD$", 7) == 0 && strncmp(response, "$SUCCESS$", 9) == 0) {
        char *filepath = request + 8;  
        FILE *file = fopen(filepath, "rb"); 
        if (file == NULL) {
            perror("Failed to open file");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        char file_buffer[BUFSIZE];
        size_t bytes_read;
        while ((bytes_read = fread(file_buffer, 1, BUFSIZE, file)) > 0) {
            bytes_sent = send(sockfd, file_buffer, bytes_read, 0);
            if (bytes_sent == -1) {
                perror("Failed to send file data");
                fclose(file);
                close(sockfd);
                exit(EXIT_FAILURE);
            }
        }

        fclose(file);
        printf("File upload completed.\n");
    }

    if (strncmp(request, "DOWNLOAD$", 9) == 0 && strncmp(response, "$SUCCESS$", 9) == 0) {
        char *filepath = request + 10;  
        FILE *file = fopen(filepath, "wb");  
        if (file == NULL) {
            perror("Failed to open file");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        char file_buffer[BUFSIZE];
        while ((bytes_received = recv(sockfd, file_buffer, BUFSIZE, 0)) > 0) {
            fwrite(file_buffer, 1, bytes_received, file);
        }

        fclose(file);
        printf("File download completed.\n");
    }

    close(sockfd);

    return 0;
}