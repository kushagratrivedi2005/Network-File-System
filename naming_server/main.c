#include "headers.h"

// Mutex for synchronizing access to shared resources, such as a directory tree
pthread_mutex_t directory_tree_mutex;

// Thread variable for naming server
pthread_t _global_ns_ss_server_thread, _global_ns_cl_thread;

void addToLog(const char* format, ...) {
    FILE* logFile = fopen("log.txt", "a");
    if (logFile == NULL) {
        perror("Failed to open log file");
        return;
    }

    fseek(logFile, 0, SEEK_END);
    long fileSize = ftell(logFile);
    if (fileSize > 0) {
        va_list args;
        va_start(args, format);
        char buffer[256];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        // Check if the message is "Naming server initialized"
        if (strcmp(buffer, "Naming server initialized") == 0) {
            freopen("log.txt", "w", logFile); // Clear the file
        }
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);

    char timestamp[64];
    struct tm* tm_info = localtime(&tv.tv_sec);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Add milliseconds to timestamp
    fprintf(logFile, "[%s.%03ld] ", timestamp, tv.tv_usec / 1000);

    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    fprintf(logFile, "\n");
    fclose(logFile);
}

void* NSReceiveClientOperations(void* arg) {
    int client_fd = *((int*) arg);
    free(arg);
    char* operation = (char*) malloc (sizeof(char) * MAX_FILE_PATH * 3);
    for (int i = 0; i < MAX_FILE_PATH * 3; i++) {
        operation[i] = '\0';
    }
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_size);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    addToLog("Client connected: %s:%d", client_ip, client_port);

    while(true) {
        memset(operation, 0, MAX_FILE_PATH * 3);
        int recv_result = recv(client_fd, operation, MAX_FILE_PATH * 3, 0);
        CHECK(recv_result, -1, ERR_NETWORK_ERROR);
        if(recv_result == 0) {
            addToLog("Client disconnected: %s:%d", client_ip, client_port);
            break;
        }
        addToLog("Received request from client %s:%d: %s", client_ip, client_port, operation);
        handleClientOperation(client_fd, operation);
        addToLog("Processed operation for client %s:%d", client_ip, client_port);
        // CHECK(recv(client_fd, operation, MAX_FILE_PATH * 3, 0), -1);
        // printf("Operation received: %s\n", operation);

        // do further operations (send to SS, check for path etc) here!

    }
    // Close client connection (to be replaced by further logic if needed)
    CHECK(close(client_fd), -1, ERR_NETWORK_ERROR);
    return NULL;
}

void* storageServerChecker(void* arg) {
    int indexOfStorageServer = *((int*) arg);
    printf("[DEBUG] Storage server checker thread started with index %d\n", indexOfStorageServer);
    int ss_fd = listOfStorageServers[indexOfStorageServer].ss_file_descriptor;
    free(arg);  // Free the allocated memory
    printf("[DEBUG] Storage server checker thread started with ssfd %d\n", ss_fd);
    int clientfd[SERVER_LOAD];
    for(int i = 0; i < SERVER_LOAD; i++) {
        clientfd[i] = -1;
    }
    int storage_server_count_to_delete = 0;
    int storage_server_actual_delete = 0;
    struct sockaddr_in ss_addr;
    socklen_t addr_size = sizeof(ss_addr);
    getpeername(ss_fd, (struct sockaddr*)&ss_addr, &addr_size);
    char ss_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ss_addr.sin_addr, ss_ip, INET_ADDRSTRLEN);
    int ss_port = ntohs(ss_addr.sin_port);

    addToLog("Storage server checker thread started for server %s:%d", ss_ip, ss_port);

    while(1){
        char buffer1[BUF_SIZE];
        memset(buffer1, 0, BUF_SIZE);
        int recv_result = recv(ss_fd, buffer1, BUF_SIZE, 0);
        if(recv_result <= 0) {
            printf("[DEBUG]: Storage server disconnected: %s:%d:%d\n", ss_ip, ss_port,indexOfStorageServer);
            listOfStorageServers[indexOfStorageServer].status=0;
            printf("[DEBUG]:status of storage server %d\n",listOfStorageServers[indexOfStorageServer].status);
            addToLog("Storage server disconnected: %s:%d", ss_ip, ss_port);
            for(int i = 0; i < SERVER_LOAD; i++){
                if(clientfd[i] != -1){
                    char buffer[BUF_SIZE];
                    int number = -40;
                    snprintf(buffer, BUF_SIZE, "%d Server Crashed", number);
                    CHECK(send(clientfd[i], buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
                    printf("[DEBUG]: Sent the message to the client of async write\n");
                    clientfd[i] = -1;
                }
            }
            close(ss_fd);
            break;
        }
        addToLog("Received message from storage server %s:%d: %s", ss_ip, ss_port, buffer1);
        printf("[DEBUG]: Received from the storage server: %s\n", buffer1);

        int clientfdtemp;
        char buffer2[BUF_SIZE];
        memset(buffer2, 0, BUF_SIZE);
        sscanf(buffer1, "%d %[^\n]", &clientfdtemp, buffer2);

        printf("Client FD: %d\n", clientfdtemp);
        printf("message: %s\n",buffer2);
        if(strcmp(buffer2,"lol") == 0) {
            printf("[DEBUG]: Storage server is up %s\n", buffer2);
            char buffer[BUF_SIZE];
            int number = -40;
            snprintf(buffer, sizeof(buffer), "%d The operation is completed", number);
            CHECK(recv(ss_fd, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
            directoryNode* node = getDirectoryPath(root, buffer1, 1);
            pthread_mutex_lock(&node->file_information->file_lock);
            node->file_information->isbeingwritten = 0;
            printf("DEBUG: File unlocked\n");
            pthread_mutex_unlock(&node->file_information->file_lock);
            CHECK(send(clientfdtemp, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
            printf("[DEBUG]: Sent the message to the client of async write\n");
            for(int i = 0; i < SERVER_LOAD; i++){
                if(clientfd[i] == clientfdtemp){
                    clientfd[i] = -1;
                    break;
                }
            }
        }
        if(strcmp(buffer2,"KV") == 0) {
            for(int i=0;i<SERVER_LOAD;i++){
                if(clientfd[i]==-1){
                    clientfd[i]=clientfdtemp;
                    break;
                }
            }
        }
        if(strcmp(buffer2,"maskaracreates0") == 0) {
            printf("Error: Failed to create file/folder\n");
            addToLog("Error: Failed to create file/folder");
            CHECK(send(clientfdtemp, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        }
        if(strcmp(buffer2,"maskaracreates1") == 0) {
            struct path command;
            int index;
            CHECK(recv(ss_fd, &command, sizeof(command), 0), -1, ERR_NETWORK_ERROR);
            CHECK(recv(ss_fd, &index, sizeof(index), 0), -1, ERR_NETWORK_ERROR);
            addDirectoryPath(root, command.dirPath, command.isFile, index);
            printf("SUCCESS: File/Folder created successfully\n");
            addToLog("SUCCESS: File/Folder created successfully");
            CHECK(send(clientfdtemp, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        }
        if(strcmp(buffer2,"maskaradeletes0") == 0) {
            printf("Error: Failed to delete file/folder\n");
            addToLog("Error: Failed to delete file/folder");
            CHECK(send(clientfdtemp, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
            storage_server_count_to_delete++;
        }
        if(strcmp(buffer2,"maskaradeletes1") == 0) {
            int count_storage_servers;
            CHECK(recv(ss_fd, &count_storage_servers, sizeof(count_storage_servers), 0), -1, ERR_NETWORK_ERROR);
            char path[MAX_FILE_PATH];
            CHECK(recv(ss_fd, path, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            printf("SUCCESS: File/Folder deleted successfully\n");
            addToLog("SUCCESS: File/Folder deleted successfully");
            printf("DEBUG: Count of storage servers: %d\n", count_storage_servers);
            printf("DEBUG: Path received: %s\n", path);
            storage_server_count_to_delete++;
            storage_server_actual_delete++;
            printf("[DEBUG]: storage server count to delete: %d\n", storage_server_count_to_delete);
            printf("[DEBUG]: storage server actual delete: %d\n", storage_server_actual_delete);
            printf("[DEBUG]: count storage servers: %d\n", count_storage_servers);
            if(storage_server_count_to_delete == count_storage_servers) {
                if(storage_server_actual_delete == count_storage_servers) {
                    deleteTreeFromPath(root, path);
                    printf("[DEBUG]: Tree deleted successfully\n");
                    __debugPrintTree(root, "");
                    deleteFromCache(path);
                    printf("[DEBUG]: Cache deleted successfully\n");
                    printf("[DEBUG]: Files / folders deleted successfully\n");
                    char buffer10[BUF_SIZE];
                    strcpy(buffer10, "Files / folders deleted successfully");
                    printf("[DEBUG]: Sending buffer\n");
                    CHECK(send(clientfdtemp, buffer10, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
                    printf("[DEBUG]: sent buffer\n");
                }
                else {
                    char* buffer10 = (char*) malloc (sizeof(char) * BUF_SIZE);
                    snprintf(buffer10, BUF_SIZE, "%sError: Failed to delete files / folders %d%s\n", ANSI_COLOR_RED, ERR_UNEXPECTED_ERROR, ANSI_COLOR_RESET);
                    CHECK(send(clientfdtemp, buffer10, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
                }
                storage_server_count_to_delete = 0;
                storage_server_actual_delete = 0;
            }
        }
        if(strlen(buffer1) == 0) { //idk why this works but it works
            printf("Storage server disconnected with index %d\n", indexOfStorageServer);
            listOfStorageServers[indexOfStorageServer].status = 0;
            for(int i=0;i<SERVER_LOAD;i++){
                if(clientfd[i]!=-1){
                    char buffer[BUF_SIZE];
                    int number = -40;
                    snprintf(buffer, sizeof(buffer), "%d Server Crashed Troll Anup", number);
                    CHECK(send(clientfdtemp, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
                    printf("[DEBUG]:Sent the message to the client of async write\n");
                    clientfd[i]=-1;
                }
            }
            close(ss_fd);
            break;
        }
        printf("Recieved from the storage server: %s\n", buffer1);
        addToLog("Processed message from storage server %s:%d", ss_ip, ss_port);
    }

    addToLog("Storage server checker thread ended for server %s:%d", ss_ip, ss_port);
    return NULL;
}
int main() {
    // Initialize the naming server
    initNamingServer();

    addToLog("Naming server initialized");

    // Create the naming server thread
    if (pthread_create(&_global_ns_ss_server_thread, NULL, hearSSforNS, NULL) != 0) {
        addToLog("Error: Failed to create storage server thread");
        perror("Failed to create naming server thread");
        return EXIT_FAILURE;
    }
    if (pthread_create(&_global_ns_cl_thread, NULL,hearClientforNS, NULL) != 0) {
        addToLog("Error: Failed to create client thread");
        perror("Failed to create naming server thread");
        return EXIT_FAILURE;
    }

    // Wait for the naming server thread to finish
    if (pthread_join(_global_ns_ss_server_thread, NULL) != 0) {
        addToLog("Error: Failed to join storage server thread");
        perror("Failed to join naming server thread");
        return EXIT_FAILURE;
    }
    if (pthread_join(_global_ns_cl_thread, NULL) != 0) {
        addToLog("Error: Failed to join client thread");
        perror("Failed to join naming server thread");
        return EXIT_FAILURE;
    }

    addToLog("Naming server shut down");

    printf("Naming server thread finished.\n");
    pthread_mutex_destroy(&directory_tree_mutex);
    return EXIT_SUCCESS;
}
