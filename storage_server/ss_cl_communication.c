#include "headers.h"

// handles write operations to the storage server
// TODO: Asynchronous writes
char __global_file_path_for_write[MAX_FILE_PATH];
int handleWrite(int clientfd) {
    printf("[DEBUG]: coming to handleWrite with clientfd = %d\n", clientfd);
    char buffer[BUF_SIZE] = {0};
    int num_chunks = 0;
    // get number of chunks
    if(recv(clientfd, buffer, sizeof(buffer), 0) == -1) {
        printf("%sError receiving number of chunks%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return -1;
    }
    num_chunks = atoi(buffer);
    printf("[DEBUG]: Buffer: %s\n", buffer);
    printf("[DEBUG]: Number of chunks: %d\n", num_chunks);
    if(num_chunks == 0) {
        printf("%sError: Invalid number of chunks%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return -1;
    }

    printf("[DEBUG]: Number of chunks: %d\n", num_chunks);

    // process first chunk for path and asynchronous read/write priority
    if(recv(clientfd, buffer, sizeof(buffer), 0) == -1) {
        printf("%sError receiving chunk[1]%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return -1;
    }
    int count = 0;
    char** chunk_split = split(buffer, " ", &count);
    char* file_path = chunk_split[0];
    char* priority = chunk_split[1];
    strcpy(__global_file_path_for_write, file_path);
    if(file_path == NULL || priority == NULL) {
        printf("%sError: Invalid chunk[1]%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return -1;
    }
    printf("[DEBUG]: File path: %s, priority = %s\n", file_path, priority);
    FILE* f = fopen(file_path, "w");
    if(f == NULL) {
        printf("%sError opening file%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return -1;
    }
    char **buffers=(char**)malloc(num_chunks*sizeof(char*));
    for(int i=0;i<num_chunks;i++){
        buffers[i]=(char*)malloc(BUF_SIZE*sizeof(char));
    }
    for(int i = 1; i < num_chunks; i++) {
        if(recv(clientfd, buffers[i], sizeof(buffers[i]), 0) == -1) {
            printf("%sError receiving chunk[%d]%s\n", ANSI_COLOR_RED, i + 1, ANSI_COLOR_RESET);
            return -1;
        }
    }
    CHECK(send(clientfd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
    for(int i = 2; i < count; i++) {
        if(priority[0]=='0')sleep(15);//for debugging purposes
        fprintf(f, "%s ", chunk_split[i]);
    }
    for(int i = 1; i < num_chunks; i++) {
        if(priority[0]=='0')sleep(15);
        fprintf(f, "%s", buffers[i]);
    }
    printf("[DEBUG]: File written\n");
    fclose(f);
    return 0;
}

void handleStream(int clientfd, char* path) {
    // printf("Opening file: %s\n", path); // Debugging statement
    int filefd = open(path, O_RDONLY); 
    if (filefd == -1) {
        perror("Error opening file");
        return;
    }
    char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read= -1;
    while ((bytes_read = read(filefd, buffer, sizeof(buffer))) > 0) {
        ssize_t total_sent = 0;
        while (total_sent < bytes_read) {
            ssize_t bytes_sent = send(clientfd, buffer + total_sent, BUF_SIZE, MSG_NOSIGNAL);
            if (bytes_sent <= 0) {
                perror("Error sending file");
                goto cleanup;
            }
            total_sent += bytes_sent;
        }
    }
    if (bytes_read == -1) {
        perror("Error reading file");
    }

cleanup:
    close(filefd);
}

void handleRetrieve(int clientfd, char* path) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    } else if (pid == 0) {
        // Child process
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end of pipe
        close(pipefd[0]); // Close unused read end
        close(pipefd[1]);

        char *args[] = {"ls", "-l", path, NULL};
        execvp("ls", args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        close(pipefd[1]); // Close unused write end

        // Read all data from pipe into a dynamic buffer
        char *buffer = NULL;
        size_t total_size = 0;
        ssize_t bytes_read;
        char temp_buffer[BUF_SIZE];

        while ((bytes_read = read(pipefd[0], temp_buffer, sizeof(temp_buffer))) > 0) {
            char *new_buffer = realloc(buffer, total_size + bytes_read);
            if (new_buffer == NULL) {
                perror("Memory allocation failed");
                free(buffer);
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
                return;
            }
            buffer = new_buffer;
            memcpy(buffer + total_size, temp_buffer, bytes_read);
            total_size += bytes_read;
        }
        if (bytes_read == -1) {
            perror("Error reading from pipe");
        }
        close(pipefd[0]);
        waitpid(pid, NULL, 0); // Wait for child process to finish

        // Send all data at once
        if (total_size > 0) {
            ssize_t bytes_sent = send(clientfd, buffer, BUF_SIZE, 0);
            if (bytes_sent <= 0) {
                perror("Error sending data");
            }
        }

        free(buffer);
    }
}

// this function is for handling the client operations in a thread
void* SSReceiveClientOperations(void *arg) {
    int clientfd = *((int*) arg);
    free(arg);
    char operation[MAX_FILE_PATH];
    char operation_type[BUF_SIZE];
    memset(operation, 0, sizeof(operation));
    memset(operation_type, 0, sizeof(operation_type));
    printf("[DEBUG] Waiting for operation from the client\n");
    int bytes_received = recv(clientfd, operation_type, sizeof(operation_type), 0);
    printf("[DEBUG] Received operation from the client\n");
    printf("[DEBUG] Bytes received: %d\n", bytes_received);
    printf("[DEBUG] Operation type: %s\n", operation_type);
    const int randomAssVar=clientfd;
    printf("[DEBUG] clientfd: %d %d\n", clientfd, randomAssVar);
    if(strcmp(operation_type,"COPY_FILE_TO_SS")==0){
        char buf[BUF_SIZE];
        printf("lets see %d %d\n",clientfd,randomAssVar);
        CHECK(recv(clientfd,buf,sizeof(buf),0), -1, ERR_NETWORK_ERROR);
        printf("[DEBUG] Received file path: %s\n",buf);
        sendFileChunks(buf, clientfd);
        printf("[DEBUG] Sending ACK to the receiver \n");
        CHECK(send(clientfd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
        close(clientfd);
        return NULL;
    }
    if (bytes_received <= 0) {
        close(clientfd);
        return NULL;
    }
    int client_fd_for_ns;
    int count;
    sscanf(operation_type, "%d %[^\n]", &client_fd_for_ns, operation);
    printf("DEBUG: Operation received: %s\n", operation_type);
    printf("DEBUG: Client FD: %d\n", client_fd_for_ns);
    printf("DEBUG: Operation: %s\n", operation);
    printf("DEBUG: Length of operation: %lu\n", strlen(operation_type));
    char** operation_split = split(operation, " ", &count);
    printf("DEBUG: Operation split: %s\n", operation_split[0]);
    if(strcmp(operation_split[0], "READ") == 0) {
        sendFileChunks(operation_split[1], clientfd); // Sends file chunks and EOF marker
        printf("[DEBUG] Sending ACK to the receiver \n");
        CHECK(send(clientfd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
        CHECK(send(storage_server_fd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
        printf("[DEBUG] Sent ACK to the receiver\n");
    }
    else if(strcmp(operation_split[0], "WRITE") == 0) {
        char buffer[BUF_SIZE];
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%dKV", client_fd_for_ns);
        CHECK(send(storage_server_fd, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        handleWrite(clientfd);
        printf("[DEBUG] handlewrite done \n");
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "%dlol", client_fd_for_ns);
        CHECK(send(storage_server_fd, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        strcpy(buffer,__global_file_path_for_write);
        CHECK(send(storage_server_fd, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        printf("[DEBUG], sent the file path to the Naming server\n");
        printf("[DEBUG] Sending ACK to the receiver \n");
        CHECK(send(clientfd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
        printf("[DEBUG] Sent ACK to the receiver\n");
    }
    else if (strcmp(operation_split[0], "RETRIEVE") == 0) {
        handleRetrieve(clientfd, operation_split[1]);
        printf("[DEBUG] Sending ACK to the receiver \n");
        CHECK(send(clientfd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
        CHECK(send(storage_server_fd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
        printf("[DEBUG] Sent ACK to the receiver\n");
    }
    else if (strcmp(operation_split[0], "STREAM")==0){
        handleStream(clientfd, operation_split[1]);
        CHECK(send(storage_server_fd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
    }
    free(operation_split);
    close(clientfd); // Close the client socket after operations
    return NULL;
}

void* hearClientforSS(void *arg) {
    (void)arg;

    // Create a socket
    const int ss_cl_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(ss_cl_fd == -1) {
        perror("Failed to create socket");
        return NULL;
    }
    int opt = 1;
    CHECK(setsockopt(ss_cl_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), -1, ERR_NETWORK_ERROR);
    // Define the server address, using port 0 for the system to choose the port
    struct sockaddr_in ns_client_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(0), // 0 to let the OS choose the port
        .sin_addr = { .s_addr = INADDR_ANY }
    };

    // Bind the socket to the address
    CHECK(bind(ss_cl_fd, (struct sockaddr *)&ns_client_addr, sizeof(ns_client_addr)), -1, ERR_NETWORK_ERROR);
    CHECK(listen(ss_cl_fd, MAX_CONNECTIONS), -1, ERR_NETWORK_ERROR);

    // Retrieve and print the assigned port
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);
    CHECK(getsockname(ss_cl_fd, (struct sockaddr *)&server_addr, &len), -1, ERR_NETWORK_ERROR);
    int port_for_client = ntohs(server_addr.sin_port);
    printf("Listening for clients on port %i\n", port_for_client);
    CHECK(send(storage_server_fd, &port_for_client, sizeof(port_for_client), 0), -1, ERR_NETWORK_ERROR);
    // Start listening for incoming connections

    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr); // Move addr_size declaration here

    while (1) {
        // Accept an incoming connection from a client
        const int clientfd = accept(ss_cl_fd, (struct sockaddr *)&client_addr, &addr_size);
        CHECK(clientfd, -1, ERR_NETWORK_ERROR);

        // Print a message when a client connects
        printf("Client connected from IP to SS: %s, Port: %d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        pthread_t independent_client_operations; // allows the client to "detach" away from the initialization area and do operations independently
        int* clientfd_ptr = (int*) malloc(sizeof(int));
        *clientfd_ptr = clientfd;
        printf("[DEBUG] Created a new thread for the client %d\n", *clientfd_ptr);
        CHECK(pthread_create(&independent_client_operations, NULL, SSReceiveClientOperations, clientfd_ptr), -1, ERR_THREAD_CREATE);
        // Close the client connection (add further logic as needed)
        // CHECK(close(clientfd), -1);
    }

    // Close the server socket
    { CHECK(close(ss_cl_fd), -1, ERR_NETWORK_ERROR); } // Wrap CHECK macro call in braces
    return NULL;
}