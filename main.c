#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "timer.h"

char **theArray;          // Global string array
pthread_rwlock_t *locks;  // Read-write locks for each array position
int n;                    // Size of the array

// Thread function to handle client requests
void* handle_client(void* arg) {
    int client_fd = *((int*)arg);
    char buffer[COM_BUFF_SIZE]; // buffer to read and write
    read(client_fd, buffer, COM_BUFF_SIZE);

    // Parse client request
    ClientRequest req;
    ParseMsg(buffer, &req);
    // Validate if  array is within desierd position
    if (req.pos < 0 || req.pos >= n) {
        strcpy(buffer, "Invalid position");
        write(client_fd, buffer, COM_BUFF_SIZE);
        close(client_fd);
        return NULL;
    }

    // Measure latency (after parsing, before sending response)
    double start, finish, elapsed;  
    
    GET_TIME(start);  // set the timer to count the latency 
    /*
    our main goal was that This way, multiple reads on the same position can proceed concurrently,
    but writes are exclusive. For different positions, locks are independent, so no contention.
    
    If it's a read:
        - Acquire read lock for pos.
        - Call getContent.
        - Release read lock.
        - Send the result back.
    */
    if (req.is_read) {
        // Acquire read lock
        pthread_rwlock_rdlock(&locks[req.pos]);
        getContent(buffer, req.pos, theArray);
        pthread_rwlock_unlock(&locks[req.pos]);
    } else {
        /*
        If it's a write:

            - Acquire write lock for pos.
            - Call setContent with the new msg.
            - Call getContent to get the updated value.
            - Release write lock.
            - Send the result back.
        */
        pthread_rwlock_wrlock(&locks[req.pos]);
        setContent(req.msg, req.pos, theArray);
        getContent(buffer, req.pos, theArray);
        pthread_rwlock_unlock(&locks[req.pos]);
    }  
   
    GET_TIME(finish);
    elapsed = finish - start; 
    saveTimes(&elapsed, 1);

    write(client_fd, buffer, COM_BUFF_SIZE);
    close(client_fd);
    free(arg); // Free client socket pointer
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <array_size> <server_ip> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    n = atoi(argv[1]); // Array size from command line
    struct sockaddr_in server_addr;
    int server_fd, client_fd; // initilize server and client file descriptor 
    pthread_t thread;

    theArray = (char**)malloc(n * sizeof(char*));    // Initialize theArray with initial values

    for (int i = 0; i < n; i++) {
        theArray[i] = (char*)malloc(COM_BUFF_SIZE * sizeof(char));
        sprintf(theArray[i], "String %d: the initial value", i); //fill in the initial values for theArray
    }

    locks = (pthread_rwlock_t*)malloc(n * sizeof(pthread_rwlock_t)); // Initialize read-write locks for each array position

    for (int i = 0; i < n; i++) {
        pthread_rwlock_init(&locks[i], NULL);
    }

    // Configure server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[2]); // IP  in our case "127.0.0.1" from command line
    server_addr.sin_port = htons(atoi(argv[3]));       // Port from command line

/*   example code for bind function
if(bind(serverFileDescriptor,(struct sockaddr*)&sock_var,sizeof(sock_var))>=0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor,2000); 
        while(1)        //loop infinity
        {
            for(i=0;i<20;i++)      //can support 20 clients at a time
            {
                clientFileDescriptor=accept(serverFileDescriptor,NULL,NULL);
                printf("Connected to client %d\n",clientFileDescriptor);
                pthread_create(&t[i],NULL,ServerEcho,(void *)(long)clientFileDescriptor);
            }
        }
        close(serverFileDescriptor);
    }
*/
    // Bind and listen
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    listen(server_fd, 2000);
    printf("Server listening on %s:%s\n", argv[2], argv[3]);

    // Accept client connections and spawn threads
    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        int *client_ptr = malloc(sizeof(int));
        *client_ptr = client_fd;
        pthread_create(&thread, NULL, handle_client, (void*)client_ptr);
        pthread_detach(thread); // Detach to avoid memory leaks
    }

    // Cleanup (unreachable in this loop, but included for completeness)
    for (int i = 0; i < n; i++) {
        pthread_rwlock_destroy(&locks[i]);
        free(theArray[i]);
    }
    free(theArray);
    free(locks);
    close(server_fd);
    return 0;
}