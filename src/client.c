#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/client.h"
#include "../include/utils.h"

int main() {
	int sfd, reqType;
	struct sockaddr_in saddr;
	char buf[BUFSIZE];
	char response[BUFSIZE];
	ssize_t bytesSent, bytesRead;

	printf("Connecting to server at %s:%d...\n", SERVER_IP, SERVER_PORT);

	/* TODO: separate init socket function */
	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket()");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port	 = htons(SERVER_PORT);

	if (inet_pton(AF_INET, SERVER_IP, &saddr.sin_addr) <= 0) {
		perror("inet_pton()");
		return -2;
	}

	if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		perror("connect()");
		close(sfd);
		return -3;
	}

	/* TODO: fgets() */
	printf("Enter request type: ");
	scanf("%s", buf);

	/* TODO: validate input */
	reqType = identify_request(buf);

	/* TODO: switch-case for reqType */
	if (!strncmp(buf, "UPLOAD$", 7) && !strncmp(response, "$SUCCESS$", 9)) {
		char *filepath = buf + 8;
		FILE *file	   = fopen(filepath, "rb");

		if (file == NULL) {
			perror("fopen()");
			close(sfd);
			return -6;
		}

		char buffer[BUFSIZE];
		ssize_t bytesRead;
		size_t bytesToUpload = 0;

		/* TODO: replace with stat */
		fseek(file, 0, SEEK_END);
		bytesToUpload = ftell(file);
		fseek(file, 0, SEEK_SET);

		/* send() file size */
		if (send(sfd, &bytesToUpload, sizeof(bytesToUpload), 0) == -1) {
			perror("Failed to send upload size");
			close(sfd);
			fclose(file);
			return -7;
		}

		/* TODO: recv() server's response */

		// if response = $SUCCESS$, start uploading
		// else, exit

		while ((bytesRead = fread(buffer, 1, BUFSIZE, file)) > 0) {

			buffer[bytesRead] = '\0';
			encoded			  = run_length_encode(buffer, encoded, BUFSIZE);

			if (encoded == NULL) {
				perror("run_length_encode()");
				close(sfd);
				fclose(file);
				return -8;
			}

			if (serv_upload(filepath, bytesToUpload, sfd) != 0) {
				printf("File upload failed.\n");
				close(sfd);
				fclose(file);
				free(encoded);
				return -9;
			}

			free(encoded);
		}
	}

	if (!strncmp(buf, "DOWNLOAD$", 9) && !strncmp(response, "$SUCCESS$", 9)) {

		char *filepath = buf + 10;
		size_t bytes_to_download;

		if (recv(sfd, &bytes_to_download, sizeof(bytes_to_download), 0) == -1) {
			perror("Failed to receive download size");
			close(sfd);
			return -1;
		}

		if (serv_download(filepath, bytes_to_download, sfd) != 0) {
			printf("File download failed.\n");
		} else {
			printf("File download completed.\n");
		}
	}

	close(sfd);
	return 0;
}
