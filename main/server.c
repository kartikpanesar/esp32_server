#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#define PORT 5050
#define MAX_CLIENTS 15
#define BUFF_MAX 1024

static const char *TAG = "Chat Server";

static struct pollfd fds[MAX_CLIENTS + 1];
// max_clients + 1 because fds[0] is for server_sockfd.

static nfds_t nfds = 0;


int16_t get_listener(){
        int32_t server_sockfd =  socket(AF_INET, SOCK_STREAM, 0);
        if(server_sockfd == -1){
                fprintf(stderr, "Couldn't creat socket for server.\n");
                return -1;
        }

        int8_t opt = 1;
        setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        printf("Server Socket created successfully.\n");

        struct sockaddr_in server_sockaddr = {0};
        server_sockaddr.sin_family = AF_INET;
        server_sockaddr.sin_port = htons(PORT);
        server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);


        if(bind(server_sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))==-1){
                fprintf(stderr, "Unable to bind the socket to the port.\n");
                return -1;
        }

        uint8_t backlog = 10;

        if(listen(server_sockfd, backlog)== 0){
                fprintf(stderr, "listen syscall failed.\n");
                return -1;
        }

        printf("Listening on PORT: %d  ......\n", PORT);

        return server_sockfd;
}

void remove_client(uint16_t index){

        ESP_LOGE(TAG, "Client fd %d left.\n", fds[index].fd);
        close(fds[index].fd);  // close client socket fd.
        fds[index] = fds[nfds-1];   // replace with the last client, since order doesn't matter.
}

int32_t read_all(uint16_t client_fd, char *buffer, uint16_t max){
        // reads till '\n' and if the buffer overflows , it returns -1;

        max = max - 1;
        // leaving space for '\n' .

        uint16_t n = 0;
        uint16_t total_count = 0;
        while(total_count<max){
                n = recv(client_fd, buffer+total_count, max-total_count, 0);
                if(n==0){
                        break;
                }

                if(n<0){
                        char *e_msg = "Server: Couldn't send your previous msg.\n";
                        send_all(client_fd, e_msg, strlen(e_msg));
                        return ;
                }

                total_count += n;

                for(int i=total_count-1; i>=0; i--){

                }

        }

}

void handle_incoming(uint16_t client_fd, char *buffer){

        int32_t n = read_all(client_fd, buffer, BUFF_MAX);
        if(n<0){
                const char *e_msg = "Server: Message not sent, write a shorter message or send in chunks.\n";
                send_all(client_fd, e_msg, strlen(e_msg));
                return ;
        }
        buffer[n++] = '\n';
        broadcast(buffer, n);

        return ;
}

void server_task(void *args){
        int16_t server_sockfd = get_listener();
        fds[0] = (struct pollfd) {.fd = server_sockfd, .events = POLLIN };
        nfds = 1;

        char buffer[BUFF_MAX];

        for(;;){
                int16_t ready = poll(fds, nfds, NULL);

                if(ready<0){
                        ESP_LOGE(TAG, "poll errno %d\n", errno);
                        break;

                }

                if(fds[0].revents & POLLIN){
                        int16_t client_sockfd = accept(fds[0].fd, NULL, NULL);

                        if(client_sockfd >= 0){
                                if(nfds<=MAX_CLIENTS){
                                        fds[nfds++] = (struct pollfd){.fd = client_sockfd, .events = POLLIN};
                                }
                                else{
                                        send(client_sockfd, "Server is full.\n", 16, 0);
                                        close(client_sockfd);
                                }
                        }
                }

                for(int i=nfds-1; i>0; i--){
                        int8_t r = fds[i].revents;

                        if(!r)
                                continue;

                        if(r & (POLLERR | POLLHUP | POLLNVAL) ){
                                remove_client(i);
                                continue;
                        }

                        if(r & POLLIN){
                                // handle_incoming(fds[i].fd);
                        }

                }
        }

        close(server_sockfd);
        vTaskDelete(NULL);
}




