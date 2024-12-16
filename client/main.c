#include "headers.h"

int parsed_client_fd;
char* client_ip;

void printClientInterface() {
    printf("You have the following options:\n");
    printf("%s1 - read a file%s\n", ANSI_COLOR_ORANGE, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s2 - write to a file%s\n", ANSI_COLOR_ORANGE, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s3 - retrieve information about a file%s\n", ANSI_COLOR_ORANGE, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s4 - streaming audio files%s\n", ANSI_COLOR_RESET, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s5 - creating a file%s\n", ANSI_COLOR_RESET, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s6 - deleting a file%s\n", ANSI_COLOR_BLUE, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s7 - creating a folder%s\n", ANSI_COLOR_RESET, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s8 - deleting a folder%s\n", ANSI_COLOR_RESET, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s9 - copying%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s10 - listing all files and folders%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
    usleep(100000);
    printf("%s11 - exit%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
    usleep(100000);
}

void readFile(char* file_path) {
    printf("Enter the path of the file: ");
    scanf("%s", file_path);
}

void getOperation(char* send_operation, int option) {
    if(option == 1) {
        strcpy(send_operation, "READ");
    }
    else if(option == 2) {
        strcpy(send_operation, "WRITE");
    }
    else if(option == 3) {
        strcpy(send_operation, "RETRIEVE");
    }
    else if(option == 4) {
        strcpy(send_operation, "STREAM");
    }
    else if(option == 5) {
        strcpy(send_operation, "CREATE_FILE");
    }
    else if(option == 6) {
        strcpy(send_operation, "DELETE_FILE");
    }
    else if(option == 7) {
        strcpy(send_operation, "CREATE_FOLDER");
    }
    else if(option == 8) {
        strcpy(send_operation, "DELETE_FOLDER");
    }
    else if(option == 9) {
        strcpy(send_operation, "COPY");
    }
    else if(option == 10) {
        strcpy(send_operation, "LIST");
    }
}

int checkOption(int client_fd, int option) {
    if(option < 1 || option > 11) {
        return -1;
    }
    char* send_operation = (char*) malloc (sizeof(char) * MAX_FILE_PATH * 3);
    getOperation(send_operation, option);
    // read file path for all the given operations
    if(option != 10) {
        char* file1_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        if(option == 9) {
            printf("(Source): ");
        }
        readFile(file1_path);
        strcat(send_operation, " ");
        strcat(send_operation, file1_path);
        if(option == 9) {
            if(option == 9) {
                printf("(Destination): ");
            }
            char* file2_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            readFile(file2_path);
            strcat(send_operation, " ");
            strcat(send_operation, file2_path);
            free(file2_path);
        }
        free(file1_path);
    }
    char bufferack[BUF_SIZE];
    char message[BUF_SIZE];
    // Set a timeout for the recv function
    struct timeval timeout;
    timeout.tv_sec = 0;  // 1 second timeout
    timeout.tv_usec = 10;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    recv(client_fd, bufferack, sizeof(bufferack), 0);
    // Split the buffer into parsed_client_fd and the rest of the message
    sscanf(bufferack, "%d %[^\n]", &parsed_client_fd, message);
    if(parsed_client_fd==-40) {
        printf("[Debug] Parsed Client FD: %d\n", parsed_client_fd);
        printf("[Debug] Message: %s\n", message);
    }
    // Reset the socket options to default (no timeout)
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    // send the operation to the naming server
    CHECK(send(client_fd, send_operation,MAX_FILE_PATH * 3, 0), -1, ERR_NETWORK_ERROR);
    while (1) {
        CHECK(recv(client_fd, bufferack, sizeof(bufferack), 0), -1, ERR_NETWORK_ERROR);
        // Split the buffer into parsed_client_fd and the rest of the message
        sscanf(bufferack, "%d %[^\n]", &parsed_client_fd, message);
        if(parsed_client_fd==-40){
            printf("[Debug] Parsed Client FD: %d\n", parsed_client_fd);
            printf("[Debug] Message: %s\n", message);
            }
        // Break the loop if parsed_client_fd is -40
        if (parsed_client_fd != -40) {
            break;
        }
    }
    if(option >= 1 && option <= 4) {
        ssDetailsForClient ssDetails;
        CHECK(recv(client_fd, &ssDetails, sizeof(ssDetails), 0), -1, ERR_NETWORK_ERROR);
        if(ssDetails.status == -1) {
            printf("%sFile not found%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            return 0;
        }
        if(ssDetails.status == -2){
             printf("%s File is being written sorry%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            return 0;
        }
        if(ssDetails.status == -3){
             printf("%s Server is down contact Anup kalbalinga%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            return 0;
        }
        else {
            printf("File found at storage server %s%s%s:%s%d%s\n", ANSI_COLOR_GREEN, ssDetails.ip_address, ANSI_COLOR_RESET, ANSI_COLOR_BLUE, ssDetails.port_number, ANSI_COLOR_RESET);
            int storage_server_fd = connectXtoY(ssDetails.port_number, ssDetails.ip_address);
            
            CHECK(storage_server_fd, -1, ERR_NETWORK_ERROR);
            if(option == 1) {
                printf("%s\n",send_operation);
                readOperation(storage_server_fd, send_operation);
            }
            else if(option == 2) {
                writeOperation(storage_server_fd, send_operation);
            }
            else if(option == 3) {
                retrieveOperation(storage_server_fd, send_operation);
            }
            else if(option == 4) {
                streamOperation(storage_server_fd, send_operation);
            }
        }
    }
    else if(option == 5 || option == 7) { // create
        char* ackbuffer = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        CHECK(recv(client_fd, ackbuffer, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
        printf("Received: %s\n", ackbuffer);
    }
    else if(option == 6 || option == 8) { // delete
        char* ackbuffer = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        CHECK(recv(client_fd, ackbuffer, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
        printf("Received: %s\n", ackbuffer);
    }
    else if(option == 9) { // copy
    
        char* ackbuffer = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        CHECK(recv(client_fd, ackbuffer, MAX_FILE_PATH, 0), -1, ERR_NETWORK_ERROR);
        printf("Received: %s\n", ackbuffer);
    }
    else if(option == 10) {
        char* get_whole_tree = (char*) malloc (sizeof(char) * MAX_TREE_SIZE);
        CHECK(recv(client_fd, get_whole_tree, MAX_TREE_SIZE, 0), -1, ERR_NETWORK_ERROR);
        printf("%s\n", get_whole_tree);
        free(get_whole_tree);
        char ackbuffer[15];
        memset(ackbuffer, 0, sizeof(ackbuffer));
        CHECK(recv(client_fd, ackbuffer, sizeof(ackbuffer), 0), -1, ERR_NETWORK_ERROR);
        printf("Received: %s\n", ackbuffer);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("%sUsage: %s <ip_address>%s\n", ANSI_COLOR_RED, argv[0], ANSI_COLOR_RESET);
        return EXIT_FAILURE;
    }
    client_ip = (char*) malloc (sizeof(char) * MAX_IP_SIZE);
    strcpy(client_ip, argv[1]);
    int client_fd = connectXtoY(NM_CL_PORT, client_ip);
    CHECK(client_fd, -1, ERR_NETWORK_ERROR);
    while(1) { 
        printClientInterface();
        int option;
        if(scanf("%d", &option) != 1) {
            printf("%sInvalid input\n%s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            while(getchar() != '\n');
            continue;
        }
        if(option == 11) { // exit the code
            break;
        }
        if(checkOption(client_fd, option) == -1) {
            printf("%sInvalid option\n%s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            continue;
        }
    }
    CHECK(close(client_fd), -1, ERR_UNEXPECTED_ERROR);
    return 0;
}
//just a comment for testing purpose