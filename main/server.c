#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "client_ds.h"

#define PORT 5050
#define MAX_CLIENTS 15

static const char *TAG = "Chat Server";

static struct pollfd fds[MAX_CLIENTS + 1];
// max_clients + 1 because fds[0] is for server_sockfd.

static nfds_t nfds = 0;


int get_listener(){
        int server_sockfd =  socket(AF_INET, SOCK_STREAM, 0);
        if(server_sockfd == -1){
                fprintf(stderr, "Couldn't creat socket for server.\n");
                return -1;
        }

        int opt = 1;
        setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        printf("Server Socket created successfully.\n");

        struct sockaddr_in server_sockaddr = {0};
        server_sockaddr.sin_family = AF_INET;
        server_sockaddr.sin_port = htons(PORT);
        server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);


        if(bind(server_sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))==-1){
                fprintf(stderr, "Unable to bind the socket to the port.\n");
                close(server_sockfd);
                return -1;
        }

        int backlog = 10;

        if(listen(server_sockfd, backlog)!= 0){
                fprintf(stderr, "listen syscall failed.\n");
                close(server_sockfd);
                return -1;
        }

        printf("Listening on PORT: %d  ......\n", PORT);

        return server_sockfd;
}

void remove_fd(client_list *clients, int index){

        ESP_LOGE(TAG, "Client fd %d left.\n", fds[index].fd);

        // This also closes the fd.
        remove_client(clients, fds[index].fd);

        // This updates the pollfd_array correctly .
        // replaces with the last client, since order doesn't matter.
        fds[index] = fds[nfds-1];
        nfds--;
}


int get_newline_index(char *buffer, size_t start, size_t end){
        if(end < start){
                return -1;
        }

        for(ssize_t i = (ssize_t) end; i>= (ssize_t) start; i--){
                if(buffer[i] == '\n'){
                        return i;
                }
        }

        return -1;
}

void send_message( int client_sockfd, const char *buffer, size_t len){

        size_t total = 0;
        ssize_t n = 0;

        while(total<len){
                n = send(client_sockfd, buffer+total, len-total, 0);
                if(n < 0){
                        ESP_LOGE(TAG, "Error in send().\n");
                        return ;
                }

                total += n;
        }

        return ;
}

int broadcast(client_list *clients, client_t *current, size_t bytes){

        client_n *this = clients->head;

        while(this != NULL){
                if(this->client_p->sockfd == current->sockfd){
                        this = this->next;
                        continue;
                }

                size_t total = 0;
                ssize_t n = 0;

                while(total < bytes){
                        n = send(this->client_p->sockfd, current->buffer + total, bytes-total, 0);
                        if(n == -1){
                                ESP_LOGE(TAG, "Error in send().\n");
                                break;
                        }
                        total += n;
                }
                this = this->next;
        }

        return 1;
}


int read_message(client_t *current, client_list *clients){

        // returns -1 if an error occured in recv, 0 if the client disconnects
        // 1 otherwise.

        ssize_t n = 0;
        size_t old = current->free_index ;

        // Index of new_line character
        ssize_t nl = -1;

        n = recv(current->sockfd, current->buffer+old, BUFF_MAX-old, 0);
        if(n==0){
                // No data to sent , the peer socket has shutdown.
                return 0;
        }

        if(n<0){
                return -1;
        }

        size_t total_count = old + n;

        nl = get_newline_index(current->buffer, old, total_count-1);

        // if no newline character found , no message sent.
        if(nl == -1 ){
                // if the buffer is full, reset the free_index .
                if(total_count == BUFF_MAX){
                        current->free_index = 0;
                        return 1;
                }

                current->free_index = total_count;
                return 1;
        }


        broadcast(clients, current, nl+1);

        size_t leftover = total_count - (nl + 1);

        // move the leftover bytes to the start of the array .
        memmove(current->buffer, current->buffer + nl + 1, leftover);

        current->free_index = leftover;

        return 1;
}


int handle_incoming(client_list *clients, int client_fd){

        client_t *current = get_client(clients, client_fd);

        // if there is no such client with this client_fd.
        if(current==NULL){
                return 0;
        }

        int n = read_message(current, clients);

        if(n==0){
                return 0;
        }

        if(n<0){
                return -1;
        }

        // if successful.
        return 1;
}


void server_task(void *args){
        int server_sockfd = get_listener();

        if(server_sockfd < 0){
                vTaskDelete(NULL);
                return ;
        }

        fds[0] = (struct pollfd) {.fd = server_sockfd, .events = POLLIN };
        nfds = 1;

        client_list *clients = create_client_list();

        for(;;){
                int ready = poll(fds, nfds, -1);

                if(ready<0){
                        ESP_LOGE(TAG, "poll errno %d\n", errno);
                        break;

                }

                if(fds[0].revents & POLLIN){
                        int client_sockfd = accept(fds[0].fd, NULL, NULL);

                        if(client_sockfd >= 0){
                                if(nfds<=MAX_CLIENTS){

                                        client_t new = {.sockfd = client_sockfd, .free_index = 0};

                                        fds[nfds++] = (struct pollfd){.fd = client_sockfd, .events = POLLIN};
                                        add_client(clients, &new);
                                }
                                else{
                                        send_message(client_sockfd, "Server is full.\n", 16);
                                        close(client_sockfd);
                                }
                        }
                }

                for(int i=nfds-1; i>0; i--){
                        int r = fds[i].revents;

                        if(!r)
                                continue;

                        if(r & (POLLERR | POLLHUP | POLLNVAL) ){
                                // removes the fd from pollfd and also remove the client from client_list.
                                // And closes the fd.
                                remove_fd(clients, i);
                                continue;
                        }

                        if(r & POLLIN){
                                if(handle_incoming(clients, fds[i].fd)<= 0){
                                        remove_fd(clients, i);
                                }
                        }

                }
        }

        destroy_client_list(clients);
        close(server_sockfd);
        vTaskDelete(NULL);
}



