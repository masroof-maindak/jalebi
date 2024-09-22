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
	char request[BUFSIZE];
	char response[BUFSIZE];
	ssize_t bytesSent, bytesRead;

	printf("Connecting to server at %s:%d...\n", serverIp, SERVER_PORT);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port	 = htons(SERVER_PORT);

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
		return -5;
	}

	response[bytesRead] = '\0';
	printf("Server response: %s\n", response);

	if (!strncmp(request, "UPLOAD$", 7) && !strncmp(response, "$SUCCESS$", 9)) {
		char *filepath = request + 8;
		if (upload(filepath, sockfd) != 0) {
			printf("File upload failed.\n");
		} else {
			printf("File upload completed.\n");
		}
	}

	if (!strncmp(request, "DOWNLOAD$", 9) &&
		!strncmp(response, "$SUCCESS$", 9)) {

		char *filepath = request + 10;
		size_t bytes_to_download;

		if (recv(sockfd, &bytes_to_download, sizeof(bytes_to_download), 0) ==
			-1) {
			perror("Failed to receive download size");
			close(sockfd);
			return -1;
		}

		if (download(filepath, bytes_to_download, sockfd) != 0) {
			printf("File download failed.\n");
		} else {
			printf("File download completed.\n");
		}
	}

	close(sockfd);
	return 0;
}
