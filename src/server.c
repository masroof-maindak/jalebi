#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/server.h"
#include "../include/utils.h"

int identifyRequest(char *type) {
	if (!strncmp(type, "VIEW", 4))
		return 1;
	else if (!strcmp(type, "DOWNLOAD"))
		return 2;
	else if (!strcmp(type, "UPLOAD"))
		return 3;
	return -1;
}

int main() {
	ensure_srv_dir_exists();

	int sfd, cfd, reuse, reqType;
	ssize_t bytesRead;
	char *buf;
	struct sockaddr_in saddr;
	struct sockaddr_storage caddr;
	socklen_t addrSize = sizeof(caddr);

	printf("Listening on port %d...\n", SERVER_PORT);

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket()");
		exit(1);
	}

	reuse = 1;
	if ((setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) ==
		-1) {
		perror("setsockopt(SO_REUSEADDR)");
		exit(2);
	}

	saddr.sin_family	  = AF_INET;
	saddr.sin_port		  = htons(SERVER_PORT);
	saddr.sin_addr.s_addr = INADDR_ANY;
	memset(saddr.sin_zero, '\0', sizeof(saddr.sin_zero));

	if ((bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) == -1) {
		perror("bind()");
		exit(3);
	}

	if (listen(sfd, MAXCLIENTS) == -1) {
		perror("listen()");
		exit(4);
	}

	if ((buf = malloc(BUFSIZE)) == NULL) {
		perror("malloc()");
		exit(5);
	}

	for (;;) {
		if ((cfd = accept(sfd, (struct sockaddr *)&caddr, &addrSize)) == -1) {
			free(buf);
			perror("accept()");
			exit(6);
		}

		bytesRead = recv(cfd, buf, BUFSIZE, 0);

		if (bytesRead == -1) {
			free(buf);
			perror("recv()");
			exit(7);
		}

		printf("They sent: %s", buf);

		/* TODO - proper request type identification later */
		/* NOTE - for now, it is assumed the client will only pass 'VIEW' */
		char *ret = malloc(BUFSIZE);

		reqType = identifyRequest(buf);

		switch (reqType) {
		case 1:
			view(cfd);
			memcpy(ret, "success", 7);
			break;
		case 2:
			memcpy(ret, "idiot", 5);
			break;
		case 3:
			memcpy(ret, "idiot", 5);
			break;
		default:
			memcpy(ret, INVALID_REQUEST_MESSAGE,
				   sizeof(INVALID_REQUEST_MESSAGE));
		}

		if ((send(cfd, ret, BUFSIZE, 0)) == -1) {
			perror("send()");
			exit(9);
		}

		free(ret);
	}

	close(sfd);
	free(buf);
	return 0;
}
