/*
Q.1. Write a client-server socket program in C. The client process and the server process
should run on separate VMs (or containers) and communicate with each other. Use
“taskset” to pin the process to specific CPUs. This helps measure the performance. Your
program should have the following features.
*/

# include <stdio.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <pthread.h>
# include <stdlib.h>
# include <string.h>
# include <arpa/inet.h>
# include <unistd.h>

void* thread_func(void* arg) {
    printf("Before thread_func entered in server\n");
    int connection_socket_descriptor = *((int*)arg);
    char send_message[] = "hello from server\n";
    char receive_message[400];

    ssize_t num_received; // stores number of bytes received by server

    if((num_received = recv(connection_socket_descriptor, (void*)receive_message, 400, 0)) == -1) {
        perror("Error in receiving message in thread func.(server)");
    };

    printf("Message received from client: \n");
    printf("%s", receive_message);
    printf("\n");

    if(send(connection_socket_descriptor, (const void*)send_message, sizeof(send_message), 0) == -1) {
        perror("Error in sending message in thread func.(server)");
    }

}

int main() {

    // create socket for listening
    int server_listening_socket_descriptor;
    server_listening_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    // create data structure holding ipv4 address and initialize it to zero
    struct sockaddr_in server_addr, client_addr[50];
    memset(&server_addr, 0, sizeof(server_addr));
    
    for(int i = 0; i < 50; i++) {
        memset(&client_addr[i], 0, sizeof(client_addr[i]));
    }

    // add address and port where server is running
    server_addr.sin_family = AF_INET;

    uint16_t port = 1234;
    server_addr.sin_port = htons(port);

    server_addr.sin_addr.s_addr = INADDR_ANY;

    // binding socket with server_addr
    if(bind(server_listening_socket_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Error in bind syscall(server)");
    }

    // listening for clients with upto 5 active listening but unaccepted connections
    if(listen(server_listening_socket_descriptor, 50) == -1) {
        perror("Error in listen syscall(server)");
    }

    int client_no = 0;

    while(1) {
        int* server_connection_socket_descriptor = malloc(sizeof(int));

        socklen_t client_addr_length = sizeof(client_addr[client_no]);

        if((*server_connection_socket_descriptor = accept(server_listening_socket_descriptor, (struct sockaddr*)&client_addr[client_no], &client_addr_length)) == -1) {
            perror("Error in accept syscall");
        }

        // create thread and run function on it
        pthread_t thread;
        if (pthread_create(&thread, NULL, thread_func, (void*)(server_connection_socket_descriptor)) != 0) {
            perror("Thread creation failed");
            close(*server_connection_socket_descriptor);
            free(server_connection_socket_descriptor);
            continue;
        } else {
            pthread_detach(thread);

        }

        client_no++;

    }

    return 0;



}