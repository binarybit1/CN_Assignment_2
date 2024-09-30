#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

struct conn{
    int socket_descriptor;
    pthread_t thread_id;
    int connection_number;
};

void* c_task(void* arg) {
    struct conn* curr_data = (struct conn*)arg;

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[4096] = {0};

    //creating socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error %lu: Socket creation\n", curr_data->thread_id);
        return NULL;
    }
    curr_data->socket_descriptor = sock; // client socket descriptor set for current conn in the struct

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1234);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Error %lu: Invalid address/Address not supported\n", curr_data->thread_id);
        return NULL;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error %lu: Connection Failed\n", curr_data->thread_id);
        return NULL;
    }
    pid_t pid = getpid();
    //sending message to server
    char hello[200];
    snprintf(hello, sizeof(hello), "Hello from ThreadId: %lu and Process Id : %d\nValue of Socket desc.: %d The connection number: %d\n", curr_data->thread_id, pid, curr_data->socket_descriptor, curr_data->connection_number);
    send(sock, hello, strlen(hello), 0);
    // printf("Thread %lu: Hello message sent\n", curr_data->thread_id);

    //receiving from server
    read(sock, buffer, 4096);
    printf("Thread %lu: Message received from server: %s\n\n", curr_data->thread_id, buffer);

    close(sock);  //closing the connection
    return NULL;
}

int main(int argc, char** argv) {
    if (argc < 2) return -1;

    int n = atoi(argv[1]);
    struct conn thread_struct[n];

    // creating n threads
    for (int i = 0; i < n; i++) {
        //thread id and connection associated with each thread also get stored in respective structs
        if (pthread_create(&thread_struct[i].thread_id, NULL, c_task, (void*)&thread_struct[i]) != 0) {
            printf("Error- thread");
        }
        else{
            thread_struct[i].connection_number = i;
        }
    }
    // waiting for all threads
    for (int i = 0; i < n; i++) {
        pthread_join(thread_struct[i].thread_id, NULL);
    }

    return 0;
}

