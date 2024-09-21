#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/utils.h"

int main() {
	int sockfd;
	struct sockaddr_in saddr;
	const char *serverIp = "127.0.0.1";
	int port			 = SERVER_PORT;
	char request[BUFSIZE];
	char response[BUFSIZE];
	ssize_t bytesSent, bytesRead;

	printf("Connecting to server at %s:%d...\n", serverIp, port);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port	 = htons(port);

	if (inet_pton(AF_INET, serverIp, &saddr.sin_addr) <= 0) {
		perror("inet_pton()");
		return -2;
	}

	if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		perror("connect()");
		close(sockfd);
		return -3;
	}

	printf("Enter request type: ");
	scanf("%s", request);

	if ((bytesSent = send(sockfd, request, strlen(request), 0)) == -1) {
		perror("send()");
		close(sockfd);
		return -4;
	}

	if ((bytesRead = recv(sockfd, response, BUFSIZE, 0)) == -1) {
		perror("recv()");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	response[bytesRead] = '\0';
	printf("Server response: %s\n", response);

	if (!strncmp(request, "UPLOAD$", 7) && !strncmp(response, "$SUCCESS$", 9)) {
		char *filepath = request + 8;
		FILE *file	   = fopen(filepath, "rb");

		if (file == NULL) {
			perror("fopen()");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		char buf[BUFSIZE];
		size_t bytesRead;

		while ((bytesRead = fread(buf, 1, BUFSIZE, file)) > 0) {
			if (send(sockfd, buf, bytesRead, 0) == -1) {
				perror("Failed to send file data");
				fclose(file);
				close(sockfd);
				exit(EXIT_FAILURE);
			}
		}

		fclose(file);
		printf("File upload completed.\n");
	}

	if (!strncmp(request, "DOWNLOAD$", 9) &&
		!strncmp(response, "$SUCCESS$", 9)) {

		char *filepath = request + 10;
		FILE *fp	   = fopen(filepath, "wb");

		if (fp == NULL) {
			perror("Failed to open file");
			close(sockfd);
			exit(EXIT_FAILURE);
		}

		char buf[BUFSIZE];
		while ((bytesRead = recv(sockfd, buf, BUFSIZE, 0)) > 0) {
			fwrite(buf, 1, bytesRead, fp);
		}

		fclose(fp);
		printf("File download completed.\n");
	}

	close(sockfd);
	return 0;
}
