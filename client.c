# include <stdio.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <pthread.h>
# include <stdlib.h>
# include <string.h>
# include <arpa/inet.h>
# include <unistd.h>

struct conn{
    int* socket_descriptor;
    pthread_t* thread_id;
    int* connection_number;
};

void* thread_func(void* arg) {
    struct conn* conn = (struct conn*)arg; // creating pointer to refer to conn passed as arg

    int* client_socket_descriptor = (*conn).socket_descriptor; // client socket descriptor unpacked from conn
    pthread_t thread_id = *(*conn).thread_id; // thread id unpacked from conn
    int connection_number = *(*conn).connection_number; // connection number unpacked from conn

    char send_message[400];

    pid_t pid = getpid();

    if(snprintf(send_message, 200, "Hello from client(processid) %d : This thread's id is %lu\tThe connection number is %d\n", pid, thread_id, connection_number) < 0) {
        perror("snprintf in threadfunc failed.(client)");
    }

    if(send(*client_socket_descriptor, (const void*)send_message, strlen(send_message)+1, 0) == -1) { // To include null character we have used +1 after strlen
        perror("Error in sending message(client)");
    }

    char receive_message[400];

    ssize_t num_received; // stores number of bytes received by client;

    if((num_received = recv(*client_socket_descriptor, (void*)receive_message, 400, 0)) == -1) {
        perror("Error in receiving message(client)");
    }

    printf("Message received from server : \n");

    for(int i = 0; i < num_received; i++) {// print the message received by client
        printf("%c", receive_message[i]);
    }
    printf("\n");

    //sleep(2);

    close(*client_socket_descriptor);
    free(client_socket_descriptor);
    free((void*)((*conn).connection_number));
    free((*conn).thread_id);
    free((void*)conn);


}

int main(int argc, char* argv[]) {

    int num_iterations = *(argv[1]) - '0';

    for(int i = 0; i < num_iterations; i++) {

        int* client_socket_descriptor = (int*)malloc(sizeof(int));

        *client_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;

        uint16_t port = 1234;
        server_addr.sin_port = htons(port);

        if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) == -1) { // convert ip address of server to binary form and store in sin_addr at proper place
            perror("Error in converting server ip address to binary format\n");
        }

        if(connect(*client_socket_descriptor, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("Error in establishing connection(client)\n");
        }

        pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t));

        // printf("Thread variable allocated dynamic memory\n");

        struct conn* conn = (struct conn*)malloc(sizeof(struct conn));// creating struct conn instance in heap so that the data remains valid even after iteration ends
        
        (*conn).socket_descriptor = client_socket_descriptor; // socket descriptor attached to conn

        int* connection_number = (int*)malloc(sizeof(int));
        (*conn).connection_number = connection_number; // connection number attached to conn\n");

        *((*conn).connection_number) = i; // connection number updated

        (*conn).thread_id = thread; // thread id attached to conn

        if (pthread_create(thread, NULL, thread_func, (void*)&(*conn)) != 0) {
            perror("Thread creation failed");
            close(*client_socket_descriptor);
            free((void*)client_socket_descriptor);
            free((void*)connection_number);
            //free((void*)thread);
            continue;
        } else {
            if(pthread_detach(*thread) != 0) {
                perror("pthread_detach failed");
            }

        }
    }

    sleep(10); // this is not robust as time to wait for threads is fixed. Track threadids of all threads created and wait for them to finish.

    return 0;


}