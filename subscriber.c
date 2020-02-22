#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "helpers.h"
#include "utils.h"

void usage(char* file) {

	fprintf(stderr, "Usage: %s <Client_ID> <Server_IP> <Server_Port>\n", file);
	exit(0);
}

int main(int argc, char** argv) {

	int sockfd, n, ret, i;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	int fdmax;
	int stop = 0;
	int yes = 1;
	fd_set read_fds;
	fd_set tmp_fds;

	if  (argc < 4) {
		usage(argv[0]);
	}

	if  (strlen(argv[1]) > 10) {
		printf("<ID_Client> must be at most 10 characters long.\n");
		exit(0);
	}

	/* open socket for connection to server */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	/* setup server data */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	/* connect to server and set socket options */
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &yes, sizeof(int));
	DIE(ret < 0, "socket opt");

	/* send client ID to server */
	sprintf(buffer, "%s", argv[1]);

	n = send(sockfd, buffer, strlen(buffer), 0);
	DIE(n < 0, "send");

	/* add stdin and connection socket to the set of sockets */
	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds;
  		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				/* receive a new message from server */
				if (i == sockfd) {
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					/* the connection was closed */
					if (n == 0) {
						printf("The server closed the connection.\n");
						stop = 1;
						break;

					/* the chosen ID was already in use by another client connected */
					} else if (strcmp(buffer, "Client ID already in use.\n") == 0) {
						printf("%s", buffer);
						stop = 1;
						break;

					/* a message from an UDP client was received */
					} else {
						printf("%s", buffer);
					}

				/* read a new command from stdin */
				} else {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					/* command was exit */
					if (strncmp(buffer, "exit", 4) == 0) {
						stop = 1;
						break;
					}

					/* command was subscribe */
					if (strncmp(buffer, "subscribe", 9) == 0) {

						/* check for space after "subscribe" */
						if (buffer[9] != ' ') {
							subscribe_usage();
							continue;
						}

						int buffIdx = 10;

						/* find index of SF */
						while (buffer[buffIdx] != '\0') {
							if (buffer[buffIdx] == ' ') {
								break;
							}

							buffIdx++;
						}

						/* SF is missing */
						if (buffer[buffIdx] == '\0') {
							subscribe_usage();
							continue;
						}

						/* SF is invalid */
						if (buffer[buffIdx + 1] != 48 &&
							buffer[buffIdx + 1] != 49)
						{
							subscribe_usage();
							continue;
						}

						printf("Subscribed %s.\n", extract_topic(buffer + 10));

						/* send to server the subscribe request */
						n = send(sockfd, buffer, strlen(buffer), 0);
						DIE(n < 0, "send");

					/* command was unsubscribe */
					} else if (strncmp(buffer, "unsubscribe", 11) == 0) {

						/* check for space after "unsubscribe" */
						if (buffer[11] != ' ') {
							unsubscribe_usage();
							continue;
						}

						/* topic is missing */
						if (buffer[12] == '\n') {
							unsubscribe_usage();
							continue;
						}

						int buffIdx = 12;

						/* check for content after topic */
						while (buffer[buffIdx] != '\0') {
							if (buffer[buffIdx] == ' ') {
								break;
							}

							buffIdx++;
						}

						/* content after topic found */
						if (buffer[buffIdx] == ' ') {
							unsubscribe_usage();
							continue;
						}

						printf("Unubscribed %s.\n", extract_topic(buffer + 12));

						/* send to server the unsubscribe request */
						n = send(sockfd, buffer, strlen(buffer), 0);
						DIE(n < 0, "send");

					} else {
						printf("Invalid command. Commands available: subscribe, unsubscribe, exit.\n");
					}
				}
			}
		}

		if (stop) {
			break;
		}
	}

	close(sockfd);

	return 0;
}
