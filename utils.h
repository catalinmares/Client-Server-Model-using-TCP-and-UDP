#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define MSGSIZE 1551
#define IPLEN 15
#define TOPICLEN 50
#define IDLEN 10
#define CONTENTLEN 1500

typedef struct message_node {
	char source_IP[IPLEN];
	int source_port;
	char topic_name[TOPICLEN];
	char type;
	char content[CONTENTLEN];
	struct message_node *next;
} MessageList;

typedef struct topic_node {
	char topic_name[TOPICLEN];
	int SF;
	MessageList* message_list;
	struct topic_node *next;
} TopicList;

typedef struct client_node {
	char client_ID[IDLEN];
	char IP[IPLEN];
	int port;
	int socket;
	int connected;
	TopicList* topic_list;
	struct client_node *next;
} ClientList;

typedef struct {
	char topic[TOPICLEN];
	char type;
	char content[CONTENTLEN];
} __attribute__((packed)) Message;

typedef struct {
	uint32_t modul;
	uint8_t putere;
} FloatMsg;

TopicList* create_topic_list();

void add_topic(TopicList** head, char* name, int SF);

void remove_topic(TopicList** head, char* name);

void destroy_topic_list(TopicList** head);

ClientList* create_client_list();

ClientList* add_client(ClientList** head, char* ID, char* IP, int port, int socket);

ClientList* disconnect_client(ClientList** head, int socket);

void destroy_client_list(ClientList** head);

MessageList* create_message_list();

void add_message(MessageList** head, char* IP, int port, char* topic, char type, char* content);

void destroy_message_list(MessageList** head);

void subscribe_usage();

void unsubscribe_usage();

char* extract_topic(char* buffer);

int already_exists(ClientList* head, char* ID);

ClientList* get_client(ClientList* head, int socket);

TopicList* get_topic(TopicList* head, char* topic_name);

int is_subscribed(ClientList* client, char* topic);

char* get_type(char type);

char* compute_content(char type, char* content);

void send_stored_messages(MessageList** head, int socket);