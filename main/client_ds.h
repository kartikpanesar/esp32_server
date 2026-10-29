#include <stdint.h>

#define USERNAME_MAX 32

// Max length of line , including '\n'
#define BUFF_MAX 4096

typedef struct client_s {
        uint16_t sockfd;
        uint16_t free_index;
        char username[USERNAME_MAX];
        char buffer[BUFF_MAX];
} client_t;


typedef struct client_node {
        client_t *client_p;
        struct client_node *next;
} client_n;

typedef struct linked_list{
        client_n *head;
        uint16_t count;
} client_list;


client_list *create_client_list();

bool add_client(client_list *clients, client_t *client);
bool remove_client(client_list *clients, uint16_t client_sockfd);

client_t *get_client(client_list *clients, uint16_t sockfd);
int get_client_sockfd(client_list *clients, char *username);

void destroy_client_list(client_list *clients);




