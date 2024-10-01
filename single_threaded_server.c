# include <stdio.h>
# include <stdint.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <pthread.h>
# include <stdlib.h>
# include <string.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <dirent.h>

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

// void* thread_func(void* arg) {
//     int new_client_soc = (int)(intptr_t)arg;
//     char receive_message[1024];
//     //receiving message from client
//     if(recv(new_client_soc, (void*)receive_message, sizeof(receive_message), 0) == -1) {
//         perror("Error in receiving message in thread func.(server)");
//     };
//     printf("Message received from client: \n");
//     printf("%s\n", receive_message);
    
//     //sending message to client
//     char send_message[4096];
//     top_two_processes(send_message);
//     if(send(new_client_soc, (const void*)send_message, sizeof(send_message), 0) == -1) {
//         perror("Error in sending message in thread func.(server)");
//     }
//     close(new_client_soc); //closing the connection
// }

int main() {

    // create socket for listening
    int serv_listening_fd;
    struct sockaddr_in server_addr, client_addr[50];
    int server_addrlen = sizeof(server_addr);

    if ((serv_listening_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Socket Failed");
        return -1;
    }

    // create data structure holding ipv4 address and initialize it to zero
    memset(&server_addr, 0, sizeof(server_addr));
    for(int i = 0; i < 50; i++) {
        memset(&client_addr[i], 0, sizeof(client_addr[i]));
    }

    // add address and port where server is running
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // binding socket with server_addr
    if(bind(serv_listening_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in bind syscall(server)");
    }

    // listening for clients with upto 5 active listening but unaccepted connections
    if(listen(serv_listening_fd, 50) < 0) {
        perror("Error in listen syscall(server)");
    }

    int client_no = 0;
    while(1) {
        int new_client_sock;
        int client_addr_length = sizeof(client_addr[client_no]);
        //accepting and creating a new socket
        if((new_client_sock = accept(serv_listening_fd, (struct sockaddr*)&client_addr[client_no], (socklen_t *)&client_addr_length)) < 0) {
            perror("Error in accept syscall");
        }

        char receive_message[1024];
        //receiving message from client
        if(recv(new_client_sock, (void*)receive_message, sizeof(receive_message), 0) == -1) {
            perror("Error in receiving message in thread func.(server)");
        };
        printf("Message received from client: \n");
        printf("%s\n", receive_message);
    
        //sending message to client
        char send_message[4096];
        top_two_processes(send_message);
        if(send(new_client_sock, (const void*)send_message, sizeof(send_message), 0) == -1) {
            perror("Error in sending message in thread func.(server)");
        }

        close(new_client_sock);

        // create thread and run function on it
        // pthread_t thread_id;
        // if (pthread_create(&thread_id, NULL, thread_func, (void*)(intptr_t)(new_client_sock)) != 0) {
        //     perror("Thread creation failed");
        //     close(new_client_sock);
        // } else {
        //     pthread_detach(thread_id);
        // }
        client_no++;

    }


    
    close(serv_listening_fd);          //closing the file descriptor
    return 0;



}