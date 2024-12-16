#include "headers.h"

void* SSReceiveNamingServerOperations(void* arg){
    (void)arg;
    int toPrintIndex;
    recv(storage_server_fd,&toPrintIndex,sizeof(toPrintIndex),0);
    printf("You are index %d\n",toPrintIndex);
    while(1){
        char command[MAX_COMMAND_SIZE];
        CHECK(recv(storage_server_fd,command,MAX_COMMAND_SIZE,0), -1, ERR_NETWORK_ERROR);
        if(strlen(command) == 0){
            printf("Naming server died\n RIP\n");
            exit(0);
        }
        printf("Received command: %s\n",command);
        printf("Client FD: %d\n", storage_server_fd); // Print the client file descriptor
        handleNSCommands(command);
    }
}

void handleNSCommands(char* command){// This function will handle the commands received from the NS- Create, delete and copy
    if(strcmp(command,"CREATE")==0){
        create();
    }
    else if(strcmp(command,"DELETE")==0){
        delete1();
    }
    else if(strcmp(command,"COPY")==0){
        copy();
    }
}

void create(){
    printf("DEBUG: Create command received\n");
    path command;
    CHECK(recv(storage_server_fd,&command,sizeof(command),0), -1, ERR_NETWORK_ERROR);
    int clientofNSfd;int index=0;
    CHECK(recv(storage_server_fd,&clientofNSfd,sizeof(clientofNSfd),0), -1, ERR_NETWORK_ERROR);
    CHECK(recv(storage_server_fd,&index,sizeof(index),0), -1, ERR_NETWORK_ERROR);
    printf("DEBUG: Path received and storage_server_fd: %s %d\n",command.dirPath,clientofNSfd);
    char buffer1[BUF_SIZE];
    memset(buffer1, 0, BUF_SIZE);
    const char* message1 = "maskaracreates1";
    const char* message0 = "maskaracreates0";
    if(command.isFile){
        FILE *file = fopen(command.dirPath, "w");
        if (file) {
            printf("DEBUG: File created successfully\n");
            fclose(file);
            snprintf(buffer1, sizeof(buffer1), "%d%s", clientofNSfd, message1);
            printf("DEBUG: Response: %s\n",buffer1);
            CHECK(send(storage_server_fd, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
            CHECK(send(storage_server_fd,&command,sizeof(command),0),-1, ERR_NETWORK_ERROR);
            CHECK(send(storage_server_fd,&index,sizeof(index),0),-1, ERR_NETWORK_ERROR);
            printf("DEBUG: Response sent\n");
        } else {
            printf("ERROR: Failed to create file\n");
            buffer1[0] = '0';
            snprintf(buffer1, sizeof(buffer1), "%d%s", clientofNSfd, message0);
            CHECK(send(storage_server_fd, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        }
    } else {
        if (mkdir(command.dirPath, 0777) == 0) {
            printf("DEBUG: Directory created successfully\n");
            snprintf(buffer1, sizeof(buffer1), "%d%s", clientofNSfd, message1);
            CHECK(send(storage_server_fd, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
            CHECK(send(storage_server_fd,&command,sizeof(command),0),-1, ERR_NETWORK_ERROR);
            CHECK(send(storage_server_fd,&index,sizeof(index),0),-1, ERR_NETWORK_ERROR);
            printf("DEBUG: Response sent\n");
        } else {
            printf("ERROR: Failed to create directory\n");
            snprintf(buffer1, sizeof(buffer1), "%d%s", clientofNSfd, message0);
            CHECK(send(storage_server_fd, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        }
    }
}

int deleteDirectoryRecursively(const char* path) {
    struct stat path_stat;
    struct dirent *entry;
    DIR *dir;

    // Get file/directory information
    if (stat(path, &path_stat) != 0) {
        printf("%sError retrieving path information%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return -1;
    }

    // Check if it's a directory
    if (S_ISDIR(path_stat.st_mode)) {
        // Open the directory
        dir = opendir(path);
        if (!dir) {
            printf("%sError opening directory%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            return -1;
        }

        // Loop through directory entries
        while ((entry = readdir(dir)) != NULL) {
            // Skip "." and ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Construct full path for entry
            char fullPath[2*MAX_FILE_PATH];
            snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

            // Recursively delete
            if(deleteDirectoryRecursively(fullPath) == -1) {
                return -1;
            }
        }

        // Close the directory
        closedir(dir);

        // Remove the directory itself
        if(rmdir(path) != 0) {
            printf("%sError removing directory%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            return -1;
        }
    } 
    else {
        // It's a file; remove it
        if(remove(path) != 0) {
            printf("%sError removing file%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            return -1;
        }
    }
    return 0;
}

void delete1() {
    printf("DEBUG: Delete command received\n");

    char* delete_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
    CHECK(recv(storage_server_fd, delete_path, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
    printf("DEBUG: Path received: %s\n", delete_path);
    int clientofNSfd;
    CHECK(recv(storage_server_fd, &clientofNSfd, sizeof(clientofNSfd), 0), -1, ERR_NETWORK_ERROR);
    printf("DEBUG: Client FD received: %d\n", clientofNSfd);
    int count_storage_servers;
    CHECK(recv(storage_server_fd, &count_storage_servers, sizeof(count_storage_servers), 0), -1, ERR_NETWORK_ERROR);
    printf("DEBUG: Count of storage servers received: %d\n", count_storage_servers);
    int return_value = deleteDirectoryRecursively(delete_path);


    char buffer1[BUF_SIZE];
    memset(buffer1, 0, BUF_SIZE);
    const char* message1 = "maskaradeletes1";
    const char* message0 = "maskaradeletes0";
    if(return_value == -1) {
        printf("%sERROR: Failed to delete file / folder%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        buffer1[0] = '0';
        snprintf(buffer1, sizeof(buffer1), "%d%s", clientofNSfd, message0);
        CHECK(send(storage_server_fd, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
    }
    else {
        printf("%sFile/folder deleted successfully%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
        snprintf(buffer1, sizeof(buffer1), "%d%s", clientofNSfd, message1);
        CHECK(send(storage_server_fd, buffer1, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        CHECK(send(storage_server_fd, &count_storage_servers, sizeof(count_storage_servers), 0), -1, ERR_NETWORK_ERROR);
        CHECK(send(storage_server_fd, delete_path, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
        printf("DEBUG: Response sent\n");
    }
}

void copy(){
    printf("DEBUG: Copy command received\n");
    path command1,command2; // source and destination paths
    CHECK(recv(storage_server_fd,&command1,sizeof(command1),0), -1, ERR_NETWORK_ERROR);
    CHECK(recv(storage_server_fd,&command2,sizeof(command2),0), -1, ERR_NETWORK_ERROR);
    printf("DEBUG: Path received: %s\n",command1.dirPath);
    printf("DEBUG: Path to copy to: %s\n",command2.dirPath);
    ssDetailsForClient ssDetails;
    CHECK(recv(storage_server_fd,&ssDetails,sizeof(ssDetails),0), -1, ERR_NETWORK_ERROR);
    printf("DEBUG: Storage server details received: %s %d\n",ssDetails.ip_address,ssDetails.port_number);

    if(command2.isFile&&(command1.isFile)){
        close(open(command2.dirPath, O_CREAT | O_WRONLY, 0644));
        int client_fd = connectXtoY(ssDetails.port_number, ssDetails.ip_address);
        if(client_fd == -1){
            perror("ERROR: Failed to connect to the storage server");
            return;
        }
        copyFile(command1.dirPath,command2.dirPath,client_fd);
    }
    else if(command1.isFile&&(!command2.isFile)){
        int words=0;
        char** temp=split(command1.dirPath,"/",&words);
        char* temp1=(char*)malloc(sizeof(char)*MAX_FILE_PATH);
        strcpy(temp1,command2.dirPath);
        strcat(temp1,"/");
        strcat(temp1,temp[words-1]);
        close(open(temp1, O_CREAT | O_WRONLY, 0644));
        int client_fd = connectXtoY(ssDetails.port_number, ssDetails.ip_address);
        if(client_fd == -1){
            perror("ERROR: Failed to connect to the storage server");
            return;
        }
        copyFile(command1.dirPath,temp1,client_fd);
    }
    else if(!command1.isFile&&(!command2.isFile)){
        mkdir(command2.dirPath, 0777);
    }
}

void copyFile(const char* source, const char* destination, int source_fd) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    strcpy(buffer, "COPY_FILE_TO_SS");
    CHECK(send(source_fd, buffer,BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
    CHECK(send(source_fd, source, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
    int num_chunks;
    if (recv(source_fd, &num_chunks, sizeof(num_chunks), 0) <= 0) {
        printf("Error receiving number of chunks\n");
        return;
    }
    printf("Number of chunks to receive: %d\n", num_chunks);
    char* buffer1 = (char*) malloc (sizeof(char) * num_chunks * BUF_SIZE);
    // Receive the file data from the server
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < num_chunks; i++) {
        CHECK(recv(source_fd, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        // Print the received data to the terminal
        strcat(buffer1,buffer);
        // Clear buffer for the next iteration
        memset(buffer, 0, BUF_SIZE);
    }
    remove(destination);
    int filefd = open(destination, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(filefd, buffer1, strlen(buffer1));
    printf("%s was the buffer\n",buffer1);
    close(filefd);
    char ackbuffer[15];
    memset(ackbuffer, 0, sizeof(ackbuffer));
    CHECK(recv(source_fd, ackbuffer, sizeof(ackbuffer), 0), -1, ERR_NETWORK_ERROR);
    printf("Received: %s\n", ackbuffer);
    free(buffer1);
    close(source_fd);
}

// void* sendAliveMessages(void* arg) {
//     while (1) {
//         // send(storage_server_fd, "ALIVE", 15, 0);
//         sleep(10); // Send ALIVE message every 10 seconds
//     }
// }
