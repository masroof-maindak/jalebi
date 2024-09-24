#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../include/client.h"
#include "../include/utils.h"

uint8_t validate_user_input(const char* input)
{
	size_t length = strlen(input);
    
    if (length < 7) {
        return 0; 
    }

    if (strncmp(input, "$VIEW$", 6) == 0 && input[6] == '\n') {
        return 1;  
    }

    if (strncmp(input, "$DOWNLOAD$", 10) == 0) {

        if (length > 11 && input[length - 2] != '$') {
            return 0;  
        }
        return 1;  
    }

    if (strncmp(input, "$UPLOAD$", 8) == 0) {
       
        if (length > 9 && input[length - 2] != '$') {
            return 0;  
        }
        return 1;  
    }

    return 0;
}

int main() {
	int sfd, reqType;
	struct sockaddr_in saddr;
	char buf[BUFSIZE];
	char response[BUFSIZE];
	char statusBuf [BUFSIZE >> 1];
	ssize_t bytesSent, bytesRead;
	struct stat fstat;

	printf("Connecting to server at %s:%d...\n", SERVER_IP, SERVER_PORT);

	/* TODO: separate init socket function - Mujtaba */
	if ((sfd = init_client_socket) < 0)
		return 1;
	
	printf("Enter request type: ");
	fgets(buf, BUFSIZE, stdin);

	if (validate_user_input(buf) == 0) {
		printf("Invalid input.\n");
		return -1;
	}

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

		if(stat(filepath, &fstat) != 0){
			perror("stat");
			return 
		}
		bytesToUpload = fstat.st_size;

		/* send() file size */
		if (send(sfd, &bytesToUpload, sizeof(bytesToUpload), 0) == -1) {
			perror("Failed to send upload size");
			close(sfd);
			fclose(file);
			return -7;
		}
        
		
		/* TODO: recv() server's response */  /*Murtaza*/

		// if response = $SUCCESS$, start uploading
		// else, exit

		if(recv(sfd,&statusBuf,sizeof(statusBuf),0) == -1){
			perror("recv()");
			close(sfd);
			fclose(file);
			return -10;
		}
		if(strcnmp(statusBuf, ULOAD_FAILURE_MSG ,sizeof(ULOAD_FAILURE_MSG)) != 0){
			fprintf(stderr, "Not enough space available\n");
			close(sfd);
			fclose(file);
			return -11;
		}

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

int init_client_socket(struct sockaddr_in *saddr) {
	int sfd;

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		return -1;
	}

	saddr->sin_family	   = AF_INET;
	saddr->sin_port	 = htons(SERVER_PORT);
	memset(saddr->sin_zero, '\0', sizeof(saddr->sin_zero));

	if (inet_pton(AF_INET, SERVER_IP, saddr.sin_addr) <= 0) {
		perror("inet_pton()");
		return -2;
	}

	if (connect(sfd, (struct sockaddr *)saddr, sizeof(saddr)) < 0) {
		perror("connect()");
		close(sfd);
		return -3;
	}

	return sfd;
}