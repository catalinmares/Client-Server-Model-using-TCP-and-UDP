#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "utils.h"

void usage(char* file) {

	fprintf(stderr, "Usage: %s <Desired_Port>\n", file);
	exit(0);
}

int main(int argc, char** argv) {

	int tcp_sockfd, udp_sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret, stop = 0;
	int yes = 1;
	socklen_t clilen;
	ClientList* client;
	TopicList* subscribed_topics;
	Message msg;


	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	if (argc < 2) {
		usage(argv[0]);
	}

	/* validate the port */
	portno = atoi(argv[1]);
	DIE(portno < 1025, "atoi");

	/* reset the descriptors set */
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	/* open socket for TCP clients and set socket options */
	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "socket TCP");
	
	ret = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &yes, sizeof(int));
	DIE(ret < 0, "socket opt");

	/* open socket for UDP clients */
	udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "socket UDP");

	/* setup server data */
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	/* bind the sockets */
	ret = bind(tcp_sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind TCP");

	ret = bind(udp_sockfd, (struct sockaddr*) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind UDP");

	/* start listening for connections from the TCP clients */
	ret = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	/* add the stdin and the main sockets to the descriptor set */
	FD_SET(0, &read_fds);
	FD_SET(tcp_sockfd, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	fdmax = (tcp_sockfd > udp_sockfd) ? tcp_sockfd : udp_sockfd;

	/* initialize the TCP clients list */
	ClientList* subscribers = create_client_list();

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				/* read a new command from stdin */
				if (i == 0) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					/* command was exit */
					if (strncmp(buffer, "exit", 4) == 0) {
						stop = 1;
						break;
					} else {
						printf("Invalid command.\n");
						continue;
					}
				}

				/* a new connection request from a TCP client is received */
				if (i == tcp_sockfd) {
					/* accept the new connection and set socket options */
					clilen = sizeof(cli_addr);
					newsockfd = accept(tcp_sockfd, (struct sockaddr*) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					ret = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &yes, sizeof(int));
					DIE(ret < 0, "socket opt");

					/* add the new socket to the descriptor set */
					FD_SET(newsockfd, &read_fds);

					/* receive the client ID from the new subscriber */
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					/* check if another client connected has the same ID */
					if (already_exists(subscribers, buffer)) {
						printf("Client ID %s already in use.\n", buffer);
						memset(buffer, 0, BUFLEN);
						sprintf(buffer, "Client ID already in use.\n");

						/* send feedback to client to change ID */
						n = send(newsockfd, buffer, strlen(buffer), 0);
						DIE(n < 0, "send");
						FD_CLR(newsockfd, &read_fds);

						continue;
					}

					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					/* add the new client to the subscribers list */
					client = add_client(&subscribers, buffer, 
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

					printf("New client %s connected from %s:%d.\n", client->client_ID, client->IP, client->port);
					
					/* send stored messages to the reconnected client */
					/* if the client is new, there are no messages to send */

					subscribed_topics = client->topic_list;

					while (subscribed_topics != NULL) {
						/* send only the messages for topics with SF set */
						if (subscribed_topics->SF == 1) {
							send_stored_messages(&subscribed_topics->message_list, newsockfd);
						}

						subscribed_topics = subscribed_topics->next;
					}

					continue;
				}

				/* a new message is received from one of the UDP clients */
				if (i == udp_sockfd) {
					/* receive the message */
					memset(&msg, 0, MSGSIZE);
					n = recvfrom(udp_sockfd, (char*) &msg, MSGSIZE, 0, (struct sockaddr*) &cli_addr, &clilen);
					DIE(n < 0, "recvfrom");

					/* the UDP client disconnected */
					if (n == 0) {
						continue;
					}

					/* send the received message to all the TCP clients who are subscribed to the message's topic */
					client = subscribers;

					while (client != NULL) {
						/* check if the client is subscribed to the topic */
						if (is_subscribed(client, msg.topic)) {
							/* if the client is connected */
							if (client->connected) {
								/* send the message */
								memset(buffer, 0, BUFLEN);
								sprintf(buffer, "%s:%d - %s - %s - %s\n", inet_ntoa(cli_addr.sin_addr),
										ntohs(cli_addr.sin_port), msg.topic, 
										get_type(msg.type), compute_content(msg.type, msg.content));

								n = send(client->socket, buffer, strlen(buffer), 0);
								DIE(n < 0, "send");

							/* if the client is disconnected */
							} else {
								/* store the message in order to send it when the client reconnects */
								TopicList* topic = get_topic(client->topic_list, msg.topic);

								if (topic->SF == 1) {
									add_message(&topic->message_list, inet_ntoa(cli_addr.sin_addr), 
											ntohs(cli_addr.sin_port), msg.topic, 
											msg.type, compute_content(msg.type, msg.content));
								}
							}
						}

						client = client->next;
					}

					continue;
				}

				/* a new request is received from one of the TCP clients */
				memset(buffer, 0, BUFLEN);
				n = recv(i, buffer, sizeof(buffer), 0);
				DIE(n < 0, "recv");
				
				/* empty message received - client disconnected */
				if (n == 0) {
					/* set disconnected for the client */
					client = disconnect_client(&subscribers, i);
					close(i);
					
					printf("Client %s disconnected.\n", client->client_ID);
						
					// se scoate din multimea de citire socketul inchis 
					FD_CLR(i, &read_fds);

				/* command was subscribe */
				} else if (strncmp(buffer, "subscribe", 9) == 0) {
					/* extract topic and SF from request string */
					char* topic_name = extract_topic(buffer + 10);
					int SF = atoi(buffer + 10 + strlen(topic_name) + 1);

					/* add the requested topic to the client's topics list */
					client = get_client(subscribers, i);
					add_topic(&client->topic_list, topic_name, SF);

				/* command was unsubscribe */
				} else {
					/* extract topic from request string */
					char* topic_name = extract_topic(buffer + 12);

					/* remove the requested topic from the client's topics list */
					client = get_client(subscribers, i);
					remove_topic(&client->topic_list, topic_name);
				}
			}
		}

		if (stop) {
			break;
		}
	}

	close(tcp_sockfd);
	close(udp_sockfd);

	destroy_client_list(&subscribers);

	return 0;
}