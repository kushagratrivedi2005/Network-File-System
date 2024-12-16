#include "headers.h"

// Mutex for synchronizing access to shared resources, such as a directory tree
pthread_mutex_t directory_tree_mutex = PTHREAD_MUTEX_INITIALIZER;

// Thread variable for naming server
pthread_t _global_ss_client_server_thread,_global_ss_ns_server_thread,_global_ss_alive_thread;
int storage_server_fd;
char* storage_server_ip;

int main(int argc, char* argv[]) {
    if(argc != 2) {
        printf("%sUsage: %s <ip_address>%s\n", ANSI_COLOR_RED, argv[0], ANSI_COLOR_RESET);
        return EXIT_FAILURE;
    }
    storage_server_ip = (char*) malloc (sizeof(char) * MAX_IP_SIZE);
    strcpy(storage_server_ip, argv[1]);

    storage_server_fd=initStorageServer();
    CHECK(storage_server_fd, -1, ERR_NETWORK_ERROR);
    if(pthread_create(&_global_ss_ns_server_thread, NULL,SSReceiveNamingServerOperations, NULL) != 0) {
        perror("Failed to create naming server thread");
        return EXIT_FAILURE;
    }
    if (pthread_create(&_global_ss_client_server_thread, NULL,hearClientforSS, NULL) != 0) {
        perror("Failed to create naming server thread");
        return EXIT_FAILURE;
    }
    // if(pthread_create(&_global_ss_alive_thread, NULL,sendAliveMessages, NULL) != 0) {
    //     perror("Failed to create naming server thread");
    //     return EXIT_FAILURE;
    // }
    if (pthread_join(_global_ss_ns_server_thread, NULL) != 0) {
        perror("Failed to join naming server thread");
        return EXIT_FAILURE;
    }
    // Wait for the naming server thread to finish
    if (pthread_join(_global_ss_client_server_thread, NULL) != 0) {
        perror("Failed to join naming server thread");
        return EXIT_FAILURE;
    }
    // if (pthread_join(_global_ss_alive_thread, NULL) != 0) {
    //     perror("Failed to join naming server thread");
    //     return EXIT_FAILURE;
    // }
    return 0;
}