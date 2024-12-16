// does operations on network for connections between client, naming server and storage server
#include "common_headers.h"

#define CHECK(x, y, err_code) if ((x) == (y)) { \
    print_error(#x " failed"); \
    exit(err_code); \
}

// connects a cl/ns/ss to a cl/ns/ss
int connectXtoY(int port, char* ip) {
    const int x_y_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(x_y_fd, -1, ERR_NETWORK_ERROR);
    int opt = 1;
    CHECK(setsockopt(x_y_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), -1, ERR_NETWORK_ERROR);
    struct sockaddr_in y_client_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { .s_addr = inet_addr(ip) }
    };
    CHECK(connect(x_y_fd, (struct sockaddr *)&y_client_addr, sizeof(y_client_addr)), -1, ERR_NETWORK_ERROR);
    return x_y_fd;
}

void print_error(const char *msg) {
    // ANSI escape code for red text: "\033[1;31m" and reset color "\033[0m"
    fprintf(stderr, "\033[1;31mError: %s\033[0m\n", msg);
}

// Function to divide the file into chunks and sends it to socketfd
// params: file_path - path of the file
//         socketfd - socket file descriptor
// returns: void

void sendFileChunks(const char* file_path, int socketfd) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    int filefd = open(file_path, O_RDONLY);
    // printf("[DEBUG]: the file path is %s\n", file_path);
    if (filefd == -1) {
        perror("open");
        return;
    }

    // Get the file size
    struct stat file_stat;
    if (fstat(filefd, &file_stat) == -1) {
        perror("fstat");
        close(filefd);
        return;
    }
    off_t file_size = file_stat.st_size;

    // Calculate the number of chunks
    int num_chunks = (file_size + BUF_SIZE - 1) / BUF_SIZE;

    // Send the number of chunks to the receiver
    send_data_in_packets((char*)&num_chunks, socketfd, sizeof(num_chunks));

    ssize_t bytesRead;
    while ((bytesRead = read(filefd, buffer, sizeof(buffer))) > 0) {
        printf("[DEBUG]: sent buffer: %s\n", buffer);
        send_data_in_packets(buffer, socketfd, sizeof(buffer)); // Send only the bytes read
        memset(buffer, 0, sizeof(buffer));
    }

    if (bytesRead == -1) {
        perror("read");
    }
    close(filefd);
}

// Function to receive the file chunks and write it to the file
// params: file_path - path of the file
//         socketfd - socket file descriptor
// returns: void

void receiveFileChunks(const char* file_path, int socketfd) {
    int filefd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (filefd == -1) {
        print_error("Error opening file");
        return;
    }
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE);
    ssize_t bytes_received;
    while ((bytes_received = recv(socketfd, buffer, sizeof(buffer), 0)) > 0) {
        if (write(filefd, buffer, bytes_received) == -1) {
            print_error("Error writing to file");
            close(filefd);
            return;
        }
        memset(buffer, 0, BUF_SIZE);
    }
    if (bytes_received == -1) {
        print_error("Error receiving data");
    }
    close(filefd);
}

// Function to send data in packets
void send_data_in_packets(void *buffer, const int sockfd, int buffer_length) {
    int numpackets = buffer_length / MAX_STR_LEN;

    // Send full-sized packets
    for (int i = 0; i < numpackets; i++) {
        CHECK(send(sockfd, buffer + MAX_STR_LEN * i, MAX_STR_LEN, 0), -1, ERR_NETWORK_ERROR);
    }

    // Send remaining bytes (if any)
    if (buffer_length % MAX_STR_LEN != 0) {
        int remaining = buffer_length % MAX_STR_LEN;
        CHECK(send(sockfd, buffer + MAX_STR_LEN * numpackets, remaining, 0), -1, ERR_NETWORK_ERROR);
    }
}


// Function to receive data in packets
void receive_data_in_packets(void *buffer, const int sockfd, int buffer_length)
{
  int numpackets = buffer_length/MAX_STR_LEN;
  for (int i=0; i<numpackets; i++)
  {
    CHECK(recv(sockfd, buffer + MAX_STR_LEN*i, MAX_STR_LEN, 0), -1, ERR_NETWORK_ERROR);
    memset(buffer, 0, BUF_SIZE);
  }
  if (buffer_length % MAX_STR_LEN != 0)
  {
    CHECK(recv(sockfd, buffer + MAX_STR_LEN*numpackets, buffer_length % MAX_STR_LEN, 0), -1, ERR_NETWORK_ERROR);
    memset(buffer, 0, BUF_SIZE);
  }
//   printf("[DEBUG]: buffer received: %s\n", (char*)buffer);
}