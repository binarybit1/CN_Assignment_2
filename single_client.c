#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int main() {

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[4096] = {0};

    //creating socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error %lu: Socket creation\n");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(1234);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Error %lu: Invalid address/Address not supported\n");
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error %lu: Connection Failed\n");
    }
    pid_t pid = getpid();
    //sending message to server
    char hello[200];
    snprintf(hello, sizeof(hello), "Hello from Process Id : %d\nValue of Socket desc.: %d\n", pid, sock);
    send(sock, hello, strlen(hello), 0);
    // printf("Thread %lu: Hello message sent\n", curr_data->thread_id);

    //receiving from server
    read(sock, buffer, 4096);
    printf("Message received from server: %s\n\n", buffer);

    close(sock);  //closing the connection

    return 0;
}

