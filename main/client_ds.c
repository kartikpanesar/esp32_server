#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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


client_list *create_client_list(){
        client_list *clients = malloc(sizeof(client_list));

        if(clients==NULL){
                fprintf(stderr, "Couldn't allocte memory for creating client_list.\n");
        }

        clients->head = NULL;
        clients->count = 0;

        return clients;
}


bool add_client(client_list *clients, client_t *client){
        client_t *new = malloc(sizeof(client_t));
        if(new == NULL){
                fprintf(stderr, "Couldn't allocate memory for adding a client.\n");
                return false;
        }

        new->sockfd = client->sockfd;
        strcpy(new->username, client->username);

        client_n *new_node = malloc(sizeof(client_n));
        if(new_node == NULL){
                fprintf(stderr, "Couldn't allocate memory for new node.\n");
                free(new);
                return false;
        }

         new_node -> client_p = new;
         new_node ->next = NULL;

         if(clients->head == NULL){
                 clients->head = new_node;
         }

         else{
                 client_n *current = clients->head;
                 while(current->next != NULL){
                         current = current->next;
                 }

                 current->next = new_node;
         }

         clients->count++;

         return true;
}


int32_t get_client_sockfd(client_list *clients, char *username){
        client_n *current = clients->head;

        while(current->next != NULL){
                if(strcmp(username, current->client_p->username)==0){
                        return current->client_p->sockfd;
                }
                current = current->next;
        }

        if(strcmp(current->client_p->username, username)==0){
                return current->client_p->sockfd;
        }

        // if username doesn't exist;
        fprintf(stderr, "The username you searched for, doesn't exist.\n");
        return -1;
}


bool remove_client(client_list *clients, uint16_t client_sockfd){

        client_n *current = clients->head;
        client_n *prev = NULL;

        while(current!=NULL){
                if(current->client_p->sockfd == client_sockfd){
                        if(prev==NULL){
                                clients->head = current->next;
                        }
                        else{
                                prev->next = current->next;
                        }
                        free(current->client_p);
                        free(current);
                        return true;
                }
                prev = current;
                current = current->next;
        }


        // no such client exists.
        return false;
}



