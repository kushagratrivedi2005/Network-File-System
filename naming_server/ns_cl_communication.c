#include "headers.h"

void* hearClientforNS(void * arg){ //listens for connections from client servers to the naming server
    (void)arg;
    const int ns_cl_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(ns_cl_fd, -1, ERR_NETWORK_ERROR); 
    int opt = 1;
    CHECK(setsockopt(ns_cl_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), -1, ERR_NETWORK_ERROR);
    struct sockaddr_in ns_server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(NM_CL_PORT),
        .sin_addr = { .s_addr = INADDR_ANY }
    };
    CHECK(bind(ns_cl_fd, (struct sockaddr *)&ns_server_addr, sizeof(ns_server_addr)), -1, ERR_NETWORK_ERROR);
    CHECK(listen(ns_cl_fd, MAX_CONNECTIONS), -1, ERR_NETWORK_ERROR);
    printf("Listening for client on port %d\n", NM_CL_PORT);
    struct sockaddr_in client_addr;
    while(1) {
        socklen_t addr_size = sizeof(client_addr);
        // Accept an incoming connection
        const int clientfd = accept(ns_cl_fd, (struct sockaddr *)&client_addr, &addr_size);
        CHECK(clientfd, -1, ERR_NETWORK_ERROR);
        // Print a message when a client connects
        printf("Client connected from IP: %s, Port: %d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        addToLog("Accepted client connection from %s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        pthread_t independent_client_operations; // allows the client to "detach" away from the initialization area and do operations independently
        int* clientfd_ptr = (int*) malloc(sizeof(int));
        *clientfd_ptr = clientfd;
        CHECK(pthread_create(&independent_client_operations, NULL, NSReceiveClientOperations, clientfd_ptr), -1, ERR_THREAD_CREATE);
    }
    CHECK(close(ns_cl_fd), -1, ERR_NETWORK_ERROR);
    return NULL;

}

// this function is used to handle the client operations queired by the client

void handleClientOperation(int client_fd, char* operation1){
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_size);
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);

    addToLog("Handling operation '%s' from client %s:%d", operation1, client_ip, client_port);
    char* op = strdup(operation1);
    char* operation = strtok(op," ");
    char* path = strtok(NULL," ");
    ssDetailsForClient ssDetails;
    char bufferack[BUF_SIZE];
    memset(bufferack, 0, sizeof(bufferack));
    sprintf(bufferack, "%d Got a request for ", client_fd);
    strcat(bufferack, operation);
    printf("[DEBUG] send %s\n", bufferack);
    CHECK(send(client_fd, bufferack, sizeof(bufferack), 0), -1, ERR_NETWORK_ERROR);
    if((strcmp(operation,"READ")==0||strcmp(operation,"WRITE")==0||strcmp(operation,"STREAM")==0||strcmp(operation,"RETRIEVE")==0)){
        addToLog("Processed %s operation for path '%s' from client %s:%d", operation, path, client_ip, client_port);
        int index = getIndexOfStorageServer(root, path, 1); // add file_type to this function
        printf("[DEBUG] index: %d %d\n", index,listOfStorageServers[index].status);
        if(index==-1){
            ssDetails.status=-1;
            ssDetails.port_number=-1;
            strcpy(ssDetails.ip_address,"");
            CHECK(send(client_fd,&ssDetails,sizeof(ssDetails),0), -1, ERR_NETWORK_ERROR);
        }
        else if(index==-2){
            ssDetails.status=-2;
            ssDetails.port_number=-1;
            strcpy(ssDetails.ip_address,"");
            CHECK(send(client_fd,&ssDetails,sizeof(ssDetails),0), -1, ERR_NETWORK_ERROR);
        }
        else if(listOfStorageServers[index].status==0){
            printf("[DEBUG] Storage server is down\n");
            ssDetails.status=-3;
            ssDetails.port_number=-1;
            strcpy(ssDetails.ip_address,"");
            send(client_fd,&ssDetails,sizeof(ssDetails),0);
            return;
        }
        else if(listOfStorageServers[index].status==1){
            ssDetails.status=1;
            ssDetails.port_number=listOfStorageServers[index].cl_port_number;   
            strcpy(ssDetails.ip_address,listOfStorageServers[index].ip_address);
            printf("[DEBUG] ss_details: %s %d\n", ssDetails.ip_address, ssDetails.port_number);
            CHECK(send(client_fd,&ssDetails,sizeof(ssDetails),0), -1, ERR_NETWORK_ERROR);
            if(strcmp(operation,"WRITE")==0){
                directoryNode* node = getDirectoryPath(root, path, 1);
                pthread_mutex_lock(&node->file_information->file_lock);
                node->file_information->isbeingwritten = 1;
                printf("[CONC DEBUG] File is being written%d\n",node->file_information->isbeingwritten);
                pthread_mutex_unlock(&node->file_information->file_lock);
            }
        }
        else{
            //handle the case where the storage server is down check backup
        }
    }
    else if(strcmp(operation,"LIST")==0){
        addToLog("Client requested LIST operation");
        char* get_whole_tree = (char*) malloc (sizeof(char) * MAX_TREE_SIZE);
        char* root_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        strcpy(root_path, "./");
        strcpy(get_whole_tree, getAllAccessiblePaths(root, root_path));
        free(root_path);
        get_whole_tree[strlen(get_whole_tree)] = '\0'; // remove last '/n'
        CHECK(send(client_fd, get_whole_tree, MAX_TREE_SIZE, 0), -1, ERR_NETWORK_ERROR);
        // printf("[DEBUG] Sending ACK to the receiver \n");
        CHECK(send(client_fd,"HRISHIRAj ACK", 15, 0), -1, ERR_NETWORK_ERROR);
        // printf("[DEBUG] Sent ACK to the receiver\n");
    }
    else if(strcmp(operation, "CREATE_FILE") == 0 || strcmp(operation, "CREATE_FOLDER") == 0) {
        addToLog("Client requested %s operation on path %s from %s:%d", operation, path, client_ip, client_port);
        printf("[DEBUG]: path received: %s\n", path);
        struct path command;
        strcpy(command.dirPath, path);
        if(strcmp(operation, "CREATE_FILE") == 0) {
            command.isFile = 1;
        }
        else {
            command.isFile = 0;
        }
        char* path_without_last_slash = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        strcpy(path_without_last_slash, path);
        while(strlen(path_without_last_slash) != 0 && path_without_last_slash[strlen(path_without_last_slash) - 1] != '/') {
            path_without_last_slash[strlen(path_without_last_slash) - 1] = '\0';
        }
        if(strlen(path_without_last_slash) != 0) {
            path_without_last_slash[strlen(path_without_last_slash) - 1] = '\0';
        }
        int index = getIndexOfStorageServer(root, path_without_last_slash, 0);
        int index_new = -1;
        if(index != -1) {
            if(command.isFile == 1) {
                index_new = getIndexOfStorageServer(root, path, 1);
            }
            else {
                index_new = getIndexOfStorageServer(root, path, 0);
            }
        }
        printf("[DEBUG]: index: %d, index_new: %d\n", index, index_new);
        printf("[DEBUG]: path_without_last_slash: %s\n", path_without_last_slash);
        printf("[DEBUG]: path: %s\n", path);
        if(index == -1 || index_new != -1) {
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sError: Path not found / File already exists %d%s\n", ANSI_COLOR_RED, ERR_FILE_NOT_FOUND, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            return;
        }
        else if(listOfStorageServers[index].status == 0) {
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sError: Storage server down%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            return;
        }
        else {
            printf("[DEBUG]: I can add the folder now!\n");
            int ss_file_descriptor = listOfStorageServers[index].ss_file_descriptor;
            printf("[DEBUG]: ss_file_descriptor: %d\n", ss_file_descriptor);
            char* send_operation = (char*) malloc (sizeof(char) * BUF_SIZE);
            strcpy(send_operation, "CREATE");
            CHECK(send(ss_file_descriptor, send_operation, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
            CHECK(send(ss_file_descriptor, &command, sizeof(command), 0), -1, ERR_NETWORK_ERROR);
            CHECK(send(ss_file_descriptor,&client_fd,sizeof(client_fd),0),-1, ERR_NETWORK_ERROR);
            CHECK(send(ss_file_descriptor,&index,sizeof(index),0),-1, ERR_NETWORK_ERROR);
            printf("[DEBUG]: waiting for response from ss KV\n");
            // char bufferack[BUF_SIZE];
            // recv(ss_file_descriptor, &bufferack,BUF_SIZE, 0);
            // printf("[DEBUG]: response received from ss: %s\n", bufferack);
            // if(strcmp(bufferack, "0") == 0) {
            //     char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            //     snprintf(error_message, MAX_FILE_PATH, "%sError: Failed to create file/folder%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            //     CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
            // }
            // else {
            //     addDirectoryPath(root, path, command.isFile, index);
            //     char* success_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            //     snprintf(success_message, MAX_FILE_PATH, "%sSuccess: File/folder created%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
            //     printf("[DEBUG]: success message: %s\n", success_message);
            //     CHECK(send(client_fd, success_message, MAX_FILE_PATH, 0), -1);
            // }
            free(send_operation);
        }
    }
    else if(strcmp(operation, "DELETE_FILE") == 0 || strcmp(operation, "DELETE_FOLDER") == 0) {
        addToLog("Client requested %s operation on path %s from %s:%d", operation, path, client_ip, client_port);
        printf("[DEBUG]: path received: %s\n", path);
        int* storage_servers_list;
        directoryNode* node;
        printf("[DEBUG]: operation: %s\n", operation);
        if(strcmp(operation, "DELETE_FILE") == 0) {
            printf("[DEBUG]: deleting file\n");
            node = getDirectoryPath(root, path, 1);
            if(node == NULL) {
                char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
                snprintf(error_message, MAX_FILE_PATH, "%sError: File not found %d%s\n", ANSI_COLOR_RED, ERR_FILE_NOT_FOUND, ANSI_COLOR_RESET);
                CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
                free(error_message);
                return;
            }
            printf("[DEBUG]: node->file_information->isbeingwritten: %d\n", node->file_information->isbeingwritten);
            if(node->file_information->isbeingwritten == 1) {
                char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
                snprintf(error_message, MAX_FILE_PATH, "%sError: File is being written %d%s\n", ANSI_COLOR_RED, ERR_FILE_BUSY, ANSI_COLOR_RESET);
                CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
                free(error_message);
                return;
            }
        }
        else {
            node = getDirectoryPath(root, path, 0);
            if(node == NULL) {
                char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
                snprintf(error_message, MAX_FILE_PATH, "%sError: Folder not found %d%s\n", ANSI_COLOR_RED, ERR_FILE_NOT_FOUND, ANSI_COLOR_RESET);
                CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
                free(error_message);
                return;
            }
            if(checkSubtreeFilesBeingWritten(node) == 1) {
                char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
                snprintf(error_message, MAX_FILE_PATH, "%sError: File is being written %d%s\n", ANSI_COLOR_RED, ERR_FILE_BUSY, ANSI_COLOR_RESET);
                CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
                free(error_message);
                return;
            }
        }
        storage_servers_list = getStorageServersInSubtree(node);
        int count_storage_servers = 0;
        for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
            if(storage_servers_list[i] == 1) {
                count_storage_servers += 1;
                if(listOfStorageServers[i].status == 0) {
                    char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
                    snprintf(error_message, MAX_FILE_PATH, "%sError: Storage server down %d%s\n", ANSI_COLOR_RED, ERR_STORAGE_SERVER_UNAVAILABLE, ANSI_COLOR_RESET);
                    CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
                    free(error_message);
                    return;
                }
            }
        }
        printf("[DEBUG]: count_storage_servers: %d\n", count_storage_servers);
        for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
            if(storage_servers_list[i] == 1) {
                printf("[DEBUG]: sending delete request to storage server %d\n", i);
                int ss_file_descriptor = listOfStorageServers[i].ss_file_descriptor;
                char* send_operation = (char*) malloc (sizeof(char) * BUF_SIZE);
                strcpy(send_operation, "DELETE");
                CHECK(send(ss_file_descriptor, send_operation, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
                CHECK(send(ss_file_descriptor, path, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
                CHECK(send(ss_file_descriptor, &client_fd, sizeof(client_fd), 0), -1, ERR_NETWORK_ERROR);
                CHECK(send(ss_file_descriptor, &count_storage_servers, sizeof(count_storage_servers), 0), -1, ERR_NETWORK_ERROR);
            }
        }
    }
    else if(strcmp(operation, "COPY") == 0) {
        char* source_path = path;
        char* destination_path = strtok(NULL, " ");
        printf("[DEBUG]: source_path: %s, destination_path: %s\n", source_path, destination_path);
        printf("[DEBUG]: coming here\n");
        addToLog("Client requested COPY operation on path %s from %s:%d", path, client_ip, client_port);
        if(strcmp(source_path, destination_path) == 0 || (strncmp(destination_path, source_path, strlen(destination_path)) == 0 && destination_path[strlen(source_path)] == '/')) {
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sError: Destination path cannot be a part of the source path %d%s\n", ANSI_COLOR_RED, ERR_OPERATION_NOT_SUPPORTED, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
            return;
        }

        printf("[DEBUG]: source_path: %s, destination_path: %s\n", source_path, destination_path);

        directoryNode* source_node = getDirectoryPath(root, source_path, 1);
        if(source_node == NULL) {
            source_node = getDirectoryPath(root, source_path, 0);
        }
        directoryNode* destination_node = getDirectoryPath(root, destination_path, 1);
        if(destination_node == NULL) {
            destination_node = getDirectoryPath(root, destination_path, 0);
        }

        if(source_node == NULL || destination_node == NULL) {
            if(source_node == NULL) {
                printf("[DEBUG]: Source path not found\n");
            }
            if(destination_node == NULL) {
                printf("[DEBUG]: Destination path not found\n");
            }
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sError: Source or destination path not found %d%s\n", ANSI_COLOR_RED, ERR_FILE_NOT_FOUND, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
            return;
        }

        int is_source_file = source_node->file_information->is_file;
        int is_destination_file = destination_node->file_information->is_file;
        printf("[DEBUG]: is_source_file: %d, is_destination_file: %d\n", is_source_file, is_destination_file);

        if(is_source_file == 0 && is_destination_file == 1) {
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sError: Cannot copy a folder to a file %d%s\n", ANSI_COLOR_RED, ERR_OPERATION_NOT_SUPPORTED, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
            return;
        }
        
        if(listOfStorageServers[destination_node->file_information->storage_server_index].status == 0) {
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sError: Destination storage server down %d%s\n", ANSI_COLOR_RED, ERR_STORAGE_SERVER_UNAVAILABLE, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
            return;
        }

        if(checkSubtreeFilesBeingWritten(source_node) == 1) {
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sError: File is being written %d%s\n", ANSI_COLOR_RED, ERR_FILE_BUSY, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
            return;
        }

        for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
            if(source_node->storage_servers[i] == 1) {
                if(listOfStorageServers[i].status == 0) {
                    char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
                    snprintf(error_message, MAX_FILE_PATH, "%sError: Source storage server down %d%s\n", ANSI_COLOR_RED, ERR_STORAGE_SERVER_UNAVAILABLE, ANSI_COLOR_RESET);
                    CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
                    free(error_message);
                    return;
                }
            }
        }

        // now i can do operations freely!
        if(is_source_file == 1 && is_destination_file == 1) {
            char* send_operation = (char*) malloc (sizeof(char) * BUF_SIZE);
            strcpy(send_operation, "COPY");
            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, send_operation, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);

            struct path source_path_struct, destination_path_struct;
            strcpy(source_path_struct.dirPath, source_path);
            source_path_struct.isFile = 1;

            strcpy(destination_path_struct.dirPath, destination_path);
            destination_path_struct.isFile = 1;

            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, &source_path_struct, sizeof(source_path_struct), 0), -1, ERR_NETWORK_ERROR);
            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, &destination_path_struct, sizeof(destination_path_struct), 0), -1, ERR_NETWORK_ERROR);
            
            ssDetailsForClient ssDetails;
            ssDetails.port_number = listOfStorageServers[source_node->file_information->storage_server_index].cl_port_number;
            strcpy(ssDetails.ip_address, listOfStorageServers[source_node->file_information->storage_server_index].ip_address);
            ssDetails.status = 1;

            printf("[DEBUG]: ssDetails: %s %d\n", ssDetails.ip_address, ssDetails.port_number);
            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, &ssDetails, sizeof(ssDetails), 0), -1, ERR_NETWORK_ERROR);
            // no need to do addDirectoryPath
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sSUCCESS: files copied successfully%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
        }
        else if(is_source_file == 1 && is_destination_file == 0) {
            // file to folder copy
            struct timeval tv;
            gettimeofday(&tv, NULL);
            strcat(destination_path, "/");
            char timestamp[100];
            sprintf(timestamp, "%ld_%06d", tv.tv_sec, tv.tv_usec);
            strcat(destination_path, timestamp); // change destination path so that it is now a file -> file copy

            char* send_operation = (char*) malloc (sizeof(char) * BUF_SIZE);
            strcpy(send_operation, "COPY");
            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, send_operation, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);

            struct path source_path_struct, destination_path_struct;
            strcpy(source_path_struct.dirPath, source_path);
            source_path_struct.isFile = 1;

            strcpy(destination_path_struct.dirPath, destination_path);
            destination_path_struct.isFile = 1;

            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, &source_path_struct, sizeof(source_path_struct), 0), -1, ERR_NETWORK_ERROR);
            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, &destination_path_struct, sizeof(destination_path_struct), 0), -1, ERR_NETWORK_ERROR);

            ssDetailsForClient ssDetails;
            ssDetails.port_number = listOfStorageServers[source_node->file_information->storage_server_index].cl_port_number;
            strcpy(ssDetails.ip_address, listOfStorageServers[source_node->file_information->storage_server_index].ip_address);
            ssDetails.status = 1;

            printf("[DEBUG]: ssDetails: %s %d\n", ssDetails.ip_address, ssDetails.port_number);
            CHECK(send(listOfStorageServers[destination_node->file_information->storage_server_index].ss_file_descriptor, &ssDetails, sizeof(ssDetails), 0), -1, ERR_NETWORK_ERROR);

            addDirectoryPath(root, destination_path, 1, destination_node->file_information->storage_server_index);
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sSUCCESS: file copied successfully%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
        }
        else {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            strcat(destination_path, "/");
            char timestamp[100];
            sprintf(timestamp, "%ld_%06d", tv.tv_sec, tv.tv_usec);
            strcat(destination_path, timestamp); // change destination path so that it is now a file -> file copy
            printf("[DEBUG] New time stamped path: %s\n", destination_path);
            storageServer* ss_copy = (storageServer*) malloc (sizeof(storageServer) * MAX_STORAGE_SERVERS);
            if(ss_copy == NULL) {
                perror("malloc failed");
                return;
            }
            for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
                ss_copy[i].ip_address = (char*) malloc (sizeof(char) * MAX_IP_SIZE);
                if (listOfStorageServers[i].ip_address) {
                    strcpy(ss_copy[i].ip_address, listOfStorageServers[i].ip_address);
                } else {
                    ss_copy[i].ip_address[0] = '\0'; // Set as empty string
                }
                ss_copy[i].cl_port_number = listOfStorageServers[i].cl_port_number;
                ss_copy[i].status = listOfStorageServers[i].status;
                ss_copy[i].ss_file_descriptor = listOfStorageServers[i].ss_file_descriptor;
                ss_copy[i].backup = NULL;
            }
            printf("[DEBUG]: Lmfao I am here!\n");

            callCopyFolderRecursive(source_node, source_path, destination_path, client_fd, destination_node->file_information->storage_server_index, ss_copy, root, 1);

            // directoryNode* actual_node = source_node->next_characters[(int)('/')];

            // if(actual_node == NULL) {
            //     callCopyFolderRecursive(source_node, source_path, destination_path, client_fd, destination_node->file_information->storage_server_index, ss_copy, root, 1);
            // }
            // // add '/' in the source path
            // else {
            //     char* source_path_with_slash = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            //     strcpy(source_path_with_slash, source_path);
            //     strcat(source_path_with_slash, "/");
            //     callCopyFolderRecursive(actual_node, source_path_with_slash, destination_path, client_fd, destination_node->file_information->storage_server_index, ss_copy, root, 1);
            // }
            char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(error_message, MAX_FILE_PATH, "%sSUCCESS: folder copied successfully%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
            CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
            free(error_message);
            // callCopyFolderRecursive(actual_node, source_path, destination_path, client_fd, destination_node->file_information->storage_server_index, ss_copy, root);
            for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
                free(ss_copy[i].ip_address);
            }
            free(ss_copy);
        }
        

        // if(strcmp(operation, "COPY_FILE") == 0) {
        //     addToLog("Client requested COPY_FILE operation on path %s from %s:%d", path, client_ip, client_port);
        //     if(strcmp(source_path, destination_path) == 0) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Destination path cannot be a part of the source path%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }
        //     directoryNode* source_node = getDirectoryPath(root, source_path, 1);
        //     directoryNode* destination_node = getDirectoryPath(root, destination_path, 1);
        //     if(source_node == NULL || destination_node == NULL) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Source or destination path not found%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }
        //     int source_storage_server_index = source_node->file_information->storage_server_index;
        //     int destination_storage_server_index = destination_node->file_information->storage_server_index;
        //     if(listOfStorageServers[source_storage_server_index].status == -1 || listOfStorageServers[destination_storage_server_index].status == -1) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Source or destination storage server down%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }
        //     if(source_node->file_information->isbeingwritten == 1 || destination_node->file_information->isbeingwritten == 1) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Source or destination file is being written%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }
        //     char* send_operation = (char*) malloc (sizeof(char) * BUF_SIZE);
        //     strcpy(send_operation, "COPY");
        //     CHECK(send(listOfStorageServers[destination_storage_server_index].ss_file_descriptor, send_operation, BUF_SIZE, 0), -1);

        //     struct path source_path_struct, destination_path_struct;
        //     strcpy(source_path_struct.dirPath, source_path);
        //     source_path_struct.isFile = 1;

        //     strcpy(destination_path_struct.dirPath, destination_path);
        //     destination_path_struct.isFile = 1;

        //     CHECK(send(listOfStorageServers[destination_storage_server_index].ss_file_descriptor, &source_path_struct, sizeof(source_path_struct), 0), -1);
        //     CHECK(send(listOfStorageServers[destination_storage_server_index].ss_file_descriptor, &destination_path_struct, sizeof(destination_path_struct), 0), -1);
            
        //     ssDetailsForClient ssDetails;
        //     ssDetails.port_number = listOfStorageServers[source_storage_server_index].cl_port_number;
        //     strcpy(ssDetails.ip_address, listOfStorageServers[source_storage_server_index].ip_address);
        //     ssDetails.status = 1;

        //     printf("[DEBUG]: ssDetails: %s %d\n", ssDetails.ip_address, ssDetails.port_number);
        //     CHECK(send(listOfStorageServers[destination_storage_server_index].ss_file_descriptor, &ssDetails, sizeof(ssDetails), 0), -1);
        //     // no need to do addDirectoryPath
        // }
        // else {
        //     addToLog("Client requested COPY_FOLDER operation on path %s from %s:%d", path, client_ip, client_port);
        //     if(strncmp(source_path, destination_path, strlen(source_path)) == 0) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Destination path cannot be a part of the source path%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }

        //     directoryNode* source_node = getDirectoryPath(root, source_path, 0);
        //     directoryNode* destination_node = getDirectoryPath(root, destination_path, 0);
        //     if(source_node == NULL || destination_node == NULL) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Source or destination path not found%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }
        //     if(listOfStorageServers[destination_node->file_information->storage_server_index].status == -1) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Source or destination storage server down%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }
        //     for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
        //         if(source_node->storage_servers[i] == 1) {
        //             if(listOfStorageServers[i].status == -1) {
        //                 char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //                 snprintf(error_message, MAX_FILE_PATH, "%sError: Storage server of source is down%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //                 CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //                 free(error_message);
        //                 return;
        //             }
        //         }
        //     }
        //     if(checkSubtreeFilesBeingWritten(source_node) == 1) {
        //         char* error_message = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        //         snprintf(error_message, MAX_FILE_PATH, "%sError: Files under source node are being written%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        //         CHECK(send(client_fd, error_message, MAX_FILE_PATH, 0), -1);
        //         free(error_message);
        //         return;
        //     }

        // }
    }
    else{
        printf("%sInvalid operation%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        addToLog("Invalid client operation");
    }
}