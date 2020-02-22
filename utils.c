#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "helpers.h"

/* initialize empty topics list */
TopicList* create_topic_list() {
	return NULL;
}

/* add a new topic to a client's topics list - used for subscribe */
void add_topic(TopicList** head, char* name, int SF) {
	TopicList* temp, *prev, *next;
    
    temp = (TopicList*) calloc(1, sizeof(TopicList));

    sprintf(temp->topic_name, "%s", name);
    temp->SF = SF;
    temp->message_list = create_message_list();
    temp->next = NULL;
    
    if (*head == NULL) {
        *head = temp;
    } else {
        prev = NULL;
        next = *head;

        while (next != NULL) {
            prev = next;
            next = next->next;
        }

        prev->next = temp;  
    }
}

/* remove a topic from a client's topics list - used for unsubscribe */
void remove_topic(TopicList** head, char* name) {
	TopicList* current = *head;
	TopicList* prev = NULL;

	do {
		if (strcmp(current->topic_name, name) == 0) {
			break;
		}

		prev = current;
		current = current->next;
	} while (current != NULL);

	if (current == NULL) {
		return;
	}

	/* if the first element */
	if (current == *head) {
		/* reuse prev */
		prev = *head;
		*head = current->next;
		destroy_message_list(&prev->message_list);
		free(prev);
		return;
	}

	/* if the last element */
	if (current->next == NULL) {
		prev->next = NULL;
		destroy_message_list(&current->message_list);
		free(current);
		return;
	}

	prev->next = current->next;
	destroy_message_list(&current->message_list);
	free(current);
}

/* destroy the topics list of a client */
void destroy_topic_list(TopicList** head) {
	TopicList* node = *head;
	TopicList* tmp = NULL;

	while (node != NULL) {
		tmp = node;
		node = node->next;
		destroy_message_list(&tmp->message_list);
		free(tmp);
	}


}

/* initialize empty clients list */
ClientList* create_client_list() {
	return NULL;
}

/* add a new client to the clients list - used after a new connection accepted */
ClientList* add_client(ClientList** head, char* ID, char* IP, int port, int socket) {
	ClientList* temp, *prev, *next;
    
    temp = (ClientList*) calloc(1, sizeof(ClientList));
    strcpy(temp->client_ID, ID);
    strcpy(temp->IP, IP);
    temp->port = port;
    temp->socket = socket;
    temp->connected = 1;
    temp->topic_list = create_topic_list();
    temp->next = NULL;
    
    if (*head == NULL) {
        *head = temp;
    } else {
        prev = NULL;
        next = *head;

        while (next != NULL) {
        	/* if the client was just disconnected already is part of the list */
        	if (strcmp(next->client_ID, ID) == 0) {
        		break;
        	}

            prev = next;
            next = next->next;
        }

        /* if reconnect just update IP, port and socket */
        if (next != NULL) {
        	memset(temp->IP, 0, 15);
        	strcpy(temp->IP, IP);
        	next->port = port;
        	next->socket = socket;
        	next->connected = 1;

        	return next;

        } else {
        	prev->next = temp;
        }
    }

    return temp;
}

/* disconnect a client - used when the client disconnects */
ClientList* disconnect_client(ClientList** head, int socket) {
	ClientList* current = *head;

	do {
		if (current->socket == socket) {
			break;
		}

		current = current->next;
	} while (current != NULL);

	if (current != NULL) {
		current->socket = -1;
		current->connected = 0;
	}

	return current;
}

/* destroy the client list - used before closing the server */
void destroy_client_list(ClientList** head) {
	ClientList* node = *head;
	ClientList* tmp = NULL;

	while (node != NULL) {
		tmp = node;
		node = node->next;
		destroy_topic_list(&tmp->topic_list);
		free(tmp);
	}
}

/* initialize empty messages list */
MessageList* create_message_list() {
	return NULL;
}

/* add a new message to a client's message list for a certain topic - used when the client is disconnected */
void add_message(MessageList** head, char* IP, int port, char* topic, char type, char* content) {
	MessageList* temp, *prev, *next;
     
    temp = (MessageList*) calloc(1, sizeof(MessageList));
    
    sprintf(temp->source_IP, "%s", IP);
	temp->source_port = port;
	sprintf(temp->topic_name, "%s", topic);
	temp->type = type;
	sprintf(temp->content, "%s", content);
    temp->next = NULL;
    
    if (*head == NULL) {
        *head = temp;
    } else {
        prev = NULL;
        next = *head;

        while (next != NULL) {
            prev = next;
            next = next->next;
        }

        prev->next = temp;  
    }
}

/* destroy the messages list */
void destroy_message_list(MessageList** head) {
	MessageList* node = *head;
	MessageList* tmp = NULL;

	while (node != NULL) {
		tmp = node;
		node = node->next;
		free(tmp);
	}
}

void subscribe_usage() {
	printf("Invalid command. Usage: subscribe <Nume_Topic> <SF=0/1>\n");
}

void unsubscribe_usage() {
	printf("Invalid command. Usage: unsubscribe <Nume_Topic>\n");
}

/* extract topic name from a subscribe/unsubscribe command */
char* extract_topic(char* buffer) {
	char* topic = calloc(50, sizeof(char));
	int k = 0;

	while (buffer[k] != ' ' && buffer[k] != '\0' && buffer[k] != '\n') {
		topic[k] = buffer[k];
		k++; 
	}

	return topic;
}

/* checks is a client with the given ID is already connected to the server */
int already_exists(ClientList* head, char* ID) {
	ClientList* current = head;
	
	while (current != NULL) {
		if (strcmp(current->client_ID, ID) == 0 && current->connected) {
			return 1;
		}

		current = current->next;
	}

	return 0;
}

/* returns the client connected on the given socket */
ClientList* get_client(ClientList* head, int socket) {
	ClientList* temp = head;

	while(temp != NULL) {
		if (temp->socket == socket) {
			return temp;
		}

		temp = temp->next;
	}

	return NULL;
}

/* returns the topic with the given name from a topics list */
TopicList* get_topic(TopicList* head, char* topic_name) {
	TopicList* temp = head;

	while(temp != NULL) {
		if (strcmp(temp->topic_name, topic_name) == 0) {
			return temp;
		}

		temp = temp->next;
	}

	return NULL;
}

/* checks if a client is subscribed to a given topic */
int is_subscribed(ClientList* client, char* topic) {
	TopicList* topics = client->topic_list;

	while (topics != NULL) {
		if (strcmp(topics->topic_name, topic) == 0) {
			return 1;
		}

		topics = topics->next;
	}

	return 0;
}

/* returns the type of the message received from the UDP client */
char* get_type(char type) {
	char* ret;

	switch (type) {
		case 0:
			ret = (char*) calloc(3, sizeof(char));
			sprintf(ret, "INT");
			break;

		case 1:
			ret = (char*) calloc(10, sizeof(char));
			sprintf(ret, "SHORT_REAL");
			break;

		case 2:
			ret = (char*) calloc(5, sizeof(char));
			sprintf(ret, "FLOAT");
			break;

		default:
			ret = (char*) calloc(6, sizeof(char));
			sprintf(ret, "STRING");
	}

	return ret;
}

/* compute the content of the message sent by the UDP client */
char* compute_content(char type, char* content) {
	char* ret = (char*) calloc(1500, sizeof(char));
	uint32_t int_nr;

	uint16_t short_nr;
	float short_real;

	FloatMsg msg;
	double real;
	int int_part;
	char* buff1 = (char*) calloc(100, sizeof(char));
	char* buff2 = (char*) calloc(100, sizeof(char));
	double dec_part;


	switch (type) {
		case 0:
			/* extract the number */
			int_nr = *((uint32_t*) (content + 1));

			/* decide sign of number */
			if (content[0] == 0) {
				sprintf(ret, "%d", ntohl(int_nr));
			} else {
				sprintf(ret, "-%d", ntohl(int_nr));
			}

			break;

		case 1:
			/* extract the short real number */
			short_nr = *((uint16_t*) content);

			/* devide it by 100 to get the expected value */
			short_real = (float) ntohs(short_nr) / 100;
			sprintf(ret, "%.2f", short_real);
			break;

		case 2:
			/* extract the absolute value and the power of 10 to divide by */
			msg = *((FloatMsg*) (content + 1));

			real = ntohl(msg.modul) / pow(10, msg.putere);

			/* compute integer part and decimal part */
			int_part = (int) real;
			dec_part = real - int_part;

			/* if the number is not divided, simply print the integer part */
			if (msg.putere == 0) {
				sprintf(buff1, "%d", int_part);

			/* if the number is devided, concat the integer part and the exact decimal digits */
			} else {
				sprintf(buff1, "%d.", int_part);
				sprintf(buff2, "%lf", dec_part);
				strncat(buff1, buff2 + 2, msg.putere);
			}

			/* decide sign of number */
			if (content[0] == 0) {
				sprintf(ret, "%s", buff1);
			} else {
				sprintf(ret, "-%s", buff1);
			}

			break;

		default:
			sprintf(ret, "%s", content);
	}

	return ret;
}

/* send the messages list of a subscribed topic to a reconnected client */
void send_stored_messages(MessageList** head, int socket) {
	MessageList* messages = *head;
	MessageList* current_msg = NULL;
	char buffer[BUFLEN];
							
	while (messages != NULL) {
		current_msg = messages;
		messages = messages->next;

		memset(buffer, 0, BUFLEN);
		sprintf(buffer, "%s:%d - %s - %s - %s\n", current_msg->source_IP,
				current_msg->source_port, current_msg->topic_name, 
				get_type(current_msg->type), current_msg->content);

		int n = send(socket, buffer, strlen(buffer), 0);
		DIE(n < 0, "send");

		free(current_msg);
	}

	*head = NULL;
}