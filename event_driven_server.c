#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>
#include <dirent.h>


#define PORT 1234
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

struct proc_struct{
    int proc_id;
    char name[1024];
    int usertime;
    int kerneltime;
    int totaltime;
};

//Calculating the top 2 processes in terms of time taken
void top_two_processes(char* send_buff) {
    //Opening the proc directory
    DIR *processdir = opendir("/proc");
    struct dirent *processes;
    struct proc_struct parray[4096];                          //array to store data corresponding to each pid
    int cnt = 0;
    int i1 = 0, i2 = 0;                                       //Initialised indexes for selecting top 2 largest valued pids
    int l1 = -1, l2 = -1;
    while ((processes = readdir(processdir)) != NULL) {       //for ongoing processes in proc
        if (processes->d_type == DT_DIR) {                    //of directory type
            int pid = atoi(processes->d_name);
            if (pid > 0) {
                //opening stat corresponding to each pid
                struct proc_struct currproc;
                char pth[100];
                snprintf(pth, sizeof(pth), "/proc/%d/stat", pid);
                FILE *fil = fopen(pth, "r");
                //storing required fields in the struct created, note that * is used where we are ignoring the values
                fscanf(fil, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %d %d", &currproc.proc_id, currproc.name, &currproc.usertime, &currproc.kerneltime);
                currproc.totaltime = currproc.usertime + currproc.kerneltime;
                fclose(fil);
                parray[cnt++] = currproc;
                //calculating the largest and second largest total times and storing the indices
                if (currproc.totaltime > l1){
                    l1 = currproc.totaltime;
                    i1 = cnt - 1;
                }
                else if (currproc.totaltime > l2){
                    l2 = currproc.totaltime;
                    i2 = cnt - 1;
                }
            }
        }
    }
    closedir(processdir);
    //formatting the message in the buffer
    sprintf(send_buff,"1. PID: %d, Name: %s, User Time: %d, Kernel Time: %d\n2. PID: %d, Name: %s, User Time: %d, Kernel Time: %d",parray[i1].proc_id, parray[i1].name, parray[i1].usertime, parray[i1].kerneltime, parray[i2].proc_id, parray[i2].name, parray[i2].usertime, parray[i2].kerneltime);
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS], max_sd, sd;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Initialize client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 100) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d\n", PORT);

    int addrlen = sizeof(address);

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on any of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        // If something happened on the server socket, it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            // printf("New connection, socket fd is %d\n",new_socket);

            // Add new socket to the client_sockets array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    // printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Check all client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing, and also read the incoming message
                int valread = read(sd, buffer, BUFFER_SIZE);
                if (valread == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, IP %s, port %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Echo back the message to the client
                    buffer[valread] = '\0';
                    printf("Received: %s", buffer);
                    top_two_processes(buffer);
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }

    return 0;
}
