#include "headers.h"

void readOperation(int storage_server_fd,char* send_operation1) {
    char* send_operation= strdup(send_operation1);
    // printf("DEBUG: Reading file %s\n", send_operation);
    int count;
    char** operation= split(send_operation, " ", &count);
    // printf("DEBUG: Reading file %s\n", operation[1]);
    // char* path = operation[1];
    char buffer3[BUF_SIZE];
    snprintf(buffer3, sizeof(buffer3), "%d %s", parsed_client_fd, send_operation1);
    CHECK(send(storage_server_fd, buffer3, strlen(buffer3), 0), -1, ERR_NETWORK_ERROR);    // printf("Sent path: %s\n", send_operation1);
    printf("[debug] Sent client_fd and buffer:%d %s\n",parsed_client_fd, buffer3);
    free(operation); // Free the duplicated string
    // printf("Sent operation: %s\n", send_operation);
    // Receive the file data from the server
   // Receive the number of chunks from the server
    int num_chunks;
    if (recv(storage_server_fd, &num_chunks, sizeof(num_chunks), 0) <= 0) {
        perror("recv");
        free(send_operation);
        return;
    }
    printf("Number of chunks to receive: %d\n", num_chunks);
    // Receive the file data from the server
    char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < num_chunks; i++) {
        CHECK(recv(storage_server_fd, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
        // Print the received data to the terminal
        printf("Received: %s\n", buffer); 
        // Clear buffer for the next iteration
        memset(buffer, 0, BUF_SIZE);
    }
    char ackbuffer[15];
    memset(ackbuffer, 0, sizeof(ackbuffer));
    CHECK(recv(storage_server_fd, ackbuffer, sizeof(ackbuffer), 0), -1, ERR_NETWORK_ERROR);
    printf("Received: %s\n", ackbuffer);
}

void writeOperation(int storage_server_fd, char* path) {
    printf("[DEBUG] Writing file %s\n", path);
    while(getchar() != '\n'); // clear the buffer

    // writing the path to the file temp.txt
    FILE* f = fopen("temp.txt", "w");
    int path_size = strlen(path);
    for(int i = 6; i < path_size; i++) { // ignore write and space
        fputc(path[i], f);
    }
    fputc(' ', f);
    fclose(f);

    // synchronous write priority?
    printf("Do you want to have priority synchronous write? (1/0): ");
    int priority;
    if(scanf("%d", &priority) != 1) {
        printf("%sInvalid input%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
    if(priority == 0) {
        f = fopen("temp.txt", "a");
        fputc('0', f);
        fputc(' ', f);
        fclose(f); 
    }
    else if(priority == 1) {
        f = fopen("temp.txt", "a");
        fputc('1', f);
        fputc(' ', f);
        fclose(f);
    }
    while(getchar() != '\n'); // clear the buffer

    // writing data to temp.txt
    printf("Enter the data to be written to the file: ");
    char c;
    while((c = getchar()) != '\n' && c != EOF) {
        f = fopen("temp.txt", "a");
        fputc(c, f);
        fclose(f);
    } 
    f = fopen("temp.txt", "a");
    fputc('\n', f);
    fclose(f);
    
    int num_characters = 0;
    f = fopen("temp.txt", "r");
    while(fgetc(f) != EOF) {
        num_characters++;
    }
    fclose(f);

    printf("[DEBUG] num_characters: %d\n", num_characters);
    int num_chunks = (num_characters + BUF_SIZE - 1) / BUF_SIZE;
    printf("[DEBUG] num_chunks: %d\n", num_chunks);
    char buffer[BUF_SIZE] = {0};
    snprintf(buffer, sizeof(buffer), "%d WRITE", parsed_client_fd);
    CHECK(send(storage_server_fd, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
    printf("[DEBUG] Sent operation: %s\n", buffer);
    // memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%d", num_chunks);
    CHECK(send(storage_server_fd, buffer, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);
    printf("[DEBUG] Sent num_chunks: %s\n", buffer);
    for(int i = 0; i < num_chunks; i++) {
        char buffer[BUF_SIZE];
        memset(buffer, 0, sizeof(buffer));
        f = fopen("temp.txt", "r");
        fseek(f, i * BUF_SIZE, SEEK_SET);
        fread(buffer, 1, BUF_SIZE, f);
        fclose(f);
        CHECK(send(storage_server_fd, buffer, sizeof(buffer), 0), -1, ERR_NETWORK_ERROR);
    }
    char ackbuffer[BUF_SIZE];
    memset(ackbuffer, 0, sizeof(ackbuffer));
    CHECK(recv(storage_server_fd, ackbuffer, sizeof(ackbuffer), 0), -1, ERR_NETWORK_ERROR);
    printf("Received: %s\n", ackbuffer);
    remove("temp.txt"); // hitler temp.txt
}

void streamOperation(int storage_server_fd, char* path) {
    // Construct the operation string "STREAM <path>"
    char send_operation[MAX_FILE_PATH];
    memset(send_operation, 0, sizeof(send_operation));

    // Replace backslashes with forward slashes in the path
    for(int i = 0; path[i] != '\0'; i++) {
        if(path[i] == '\\') {
            path[i] = '/';
        }
    }

    snprintf(send_operation, sizeof(send_operation), "%s", path);
    // printf("Sending operation: %s\n", send_operation); // Debugging statement

    // Send the operation to the server
    char buffer3[BUF_SIZE];
    memset(buffer3, 0, sizeof(buffer3)); // Zero out the buffer
    snprintf(buffer3, sizeof(buffer3), "%d %s", parsed_client_fd, send_operation);
    CHECK(send(storage_server_fd, buffer3, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);    // printf("Sent path: %s\n", send_operation1);
    printf("[debug] Sent client_fd and buffer: %d %s\n", parsed_client_fd, buffer3);
    // Receive the file data from the server and stream to mpv
    char buffer[BUF_SIZE];
    ssize_t bytes_received;

    // Set buffer to unbuffered mode
    setvbuf(stdout, NULL, _IONBF, 0);
    
    // Open mpv with improved parameters
    FILE* mpv = popen("mpv --profile=low-latency --no-cache --no-terminal --audio-display=no --audio-channels=stereo -", "w");
    if (mpv == NULL) {
        perror("popen");
        printf("Make sure mpv is installed (try: sudo apt install mpv)\n");
        return;
    }

    // Set pipe to unbuffered mode
    setvbuf(mpv, NULL, _IONBF, 0);

    // Stream the audio data to mpv
    while ((bytes_received = recv(storage_server_fd, buffer, sizeof(buffer), 0)) > 0) {
        if (fwrite(buffer, 1, bytes_received, mpv) != (size_t)bytes_received) {
            perror("Error writing to mpv");
            break;
        }
    }

    // Ensure final data is written
    fflush(mpv);
    fsync(fileno(mpv));

    if (bytes_received == -1) {
        perror("Error receiving data");
    }

    // Close the mpv pipe
    pclose(mpv);
    printf("ACKNOWLEDGEMENT RECEIVED: Streamed file successfully\n");
}

void retrieveOperation(int storage_server_fd, char* path){
    char send_operation[MAX_FILE_PATH];
    memset(send_operation, 0, sizeof(send_operation));

    // Replace backslashes with forward slashes in the path
    for(int i = 0; path[i] != '\0'; i++) {
        if(path[i] == '\\') {
            path[i] = '/';
        }
    }

    snprintf(send_operation, sizeof(send_operation), "%s", path);
    // printf("Sending operation: %s\n", send_operation); // Debugging statement

    // Send the operation to the server
    char buffer3[BUF_SIZE];
    memset(buffer3, 0, sizeof(buffer3)); // Zero out the buffer
    snprintf(buffer3, sizeof(buffer3), "%d %s", parsed_client_fd, send_operation);
    CHECK(send(storage_server_fd, buffer3, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);    // printf("Sent path: %s\n", send_operation1);
    printf("[debug] Sent client_fd and buffer: %d %s\n", parsed_client_fd, buffer3);

    // Receive the file data from the server
    char buffer[BUF_SIZE];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_received= recv(storage_server_fd, buffer, sizeof(buffer), 0);
    if (bytes_received == -1) {
        perror("Error receiving data");
        return;
    }
    int count;
    char** tokens= split(buffer, " ", &count);
    if (tokens== NULL) {
        printf("Error: split failed\n");
        return;
    }

    printf("Permissions: %s\n", tokens[0]+1);
    printf("Owner: %s\n", tokens[2]);
    printf("Group: %s\n", tokens[3]);
    printf("Size: %s bytes\n", tokens[4]);
    printf("Last modified: %s %s %s\n", tokens[5], tokens[6], tokens[7]);
    for (int i = 8; i < count; i++) {
        free(tokens[i]);
    }
    free(tokens);
    char ackbuffer[15];
    memset(ackbuffer, 0, sizeof(ackbuffer));
    CHECK(recv(storage_server_fd, ackbuffer, sizeof(ackbuffer), 0), -1, ERR_NETWORK_ERROR);
    printf("Received: %s\n", ackbuffer);
}