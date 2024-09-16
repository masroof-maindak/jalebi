#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../include/server.h"
#include "../include/utils.h"

#define BUFSIZE 1024

int main() {
	int sfd, cfd, reuse, bytesRead;
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

		bytesRead = recv(sfd, buf, BUFSIZE, 0);

		if (bytesRead == -1) {
			free(buf);
			perror("recv()");
			exit(7);
		}

		printf("Client sent: %s", buf);
	}

	free(buf);
	return 0;
}
