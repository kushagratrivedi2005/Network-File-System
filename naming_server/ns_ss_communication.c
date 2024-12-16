#include "headers.h"

//listens for connections from storage servers to the naming server
void* hearSSforNS(void * arg) {
    (void)arg;
    const int ns_ss_fd = socket(AF_INET, SOCK_STREAM, 0);
    CHECK(ns_ss_fd, -1, ERR_NETWORK_ERROR); 
    int opt = 1;
    CHECK(setsockopt(ns_ss_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)), -1, ERR_NETWORK_ERROR);
    struct sockaddr_in ns_server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(NM_SS_PORT),
        .sin_addr = { .s_addr = INADDR_ANY }
    };
    CHECK(bind(ns_ss_fd, (struct sockaddr *)&ns_server_addr, sizeof(ns_server_addr)), -1, ERR_NETWORK_ERROR);
    CHECK(listen(ns_ss_fd, MAX_CONNECTIONS), -1, ERR_NETWORK_ERROR);
    printf("Listening for storage servers on port %d\n", NM_SS_PORT);
    struct sockaddr_in client_addr;
    while(1){
        socklen_t addr_size = sizeof(client_addr);
        // Accept an incoming connection
        const int clientfd = accept(ns_ss_fd, (struct sockaddr *)&client_addr, &addr_size);
        CHECK(clientfd, -1, ERR_NETWORK_ERROR);
        // Print a message when a client connects
        printf("SS connected from IP: %s, Port: %d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        addToLog("Storage server connected from IP: %s, Port: %d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        pthread_t storage_server_down_thread; // allows the client to "detach" away from the initialization area and do operations independently
        int *clientfd_ptr = malloc(sizeof(int)); // Allocate memory for clientfd_ptr
        *clientfd_ptr = clientfd; // Copy the value of clientfd to clientfd_ptr
        int number_of_paths;
        int new_ss_index=-1;
        CHECK(recv(clientfd, &new_ss_index, sizeof(new_ss_index), 0), -1, ERR_NETWORK_ERROR);
        CHECK(recv(clientfd, &number_of_paths, sizeof(number_of_paths), 0), -1, ERR_NETWORK_ERROR);
        printf("Received: %d\n", number_of_paths);

        struct path* pathsgot = (struct path*) malloc(number_of_paths * sizeof(struct path));
        CHECK(recv(clientfd, pathsgot, number_of_paths * sizeof(struct path), 0), -1, ERR_NETWORK_ERROR);
        int clientport=0;
        CHECK(recv(clientfd, &clientport, sizeof(clientport), 0), -1, ERR_NETWORK_ERROR);
        printf("Received: %d\n", clientport);
        if(new_ss_index==-1){
        new_ss_index=addStorageServer(inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),clientport, 1, clientfd);
        
        // printf("DEBUG: New storage server added at index %d\n", new_ss_index);

        for (int i = 0; i < number_of_paths; i++) {
            addToLog("Received path from storage server %s:%d: %s (isFile: %d)", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), pathsgot[i].dirPath, pathsgot[i].isFile);
            char* path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            strcpy(path, pathsgot[i].dirPath);
            // printf("[DEBUG]: Added path %s of type %d and storage server %d\n", path, pathsgot[i].isFile, new_ss_index);
            pthread_mutex_lock(&directory_tree_mutex);
            printf("[DEBUG]: Adding path %s of type %d and storage server %d\n", path, pathsgot[i].isFile, new_ss_index);
            addDirectoryPath(root, path, pathsgot[i].isFile, new_ss_index);
            pthread_mutex_unlock(&directory_tree_mutex);
            // printf("[DEBUG]: search: %d\n", searchTree(root, path));
            // strcpy(path, "lmfao/omk.txt");
            // printf("[DEBUG]: search: %d\n", searchTree(root, path));
        }
        // debugging tree after
        pthread_mutex_lock(&directory_tree_mutex);
        char* root_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
        strcpy(root_path, "./");

        __debugPrintTree(root, root_path);
        
        free(root_path);
        pthread_mutex_unlock(&directory_tree_mutex);
        free(pathsgot); // Free the allocated memory
        printf("[DEBUG] the SS_FDis %d\n", clientfd);
    }
        else{
            printf("SS id:- %d reconnected!!! with ip:-%s and port:-%d\n",new_ss_index,inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            listOfStorageServers[new_ss_index].status=1;
            listOfStorageServers[new_ss_index].ss_file_descriptor=clientfd;
            listOfStorageServers[new_ss_index].ss_port_number=ntohs(client_addr.sin_port);
            listOfStorageServers[new_ss_index].cl_port_number=clientport;
            listOfStorageServers[new_ss_index].ip_address=inet_ntoa(client_addr.sin_addr);
        }
        CHECK(send(clientfd,&new_ss_index,sizeof(new_ss_index),0), -1, ERR_NETWORK_ERROR);
        int* index_ptr = malloc(sizeof(int)); // Allocate memory for index_ptr
        *index_ptr = new_ss_index; // Copy the value of new_ss_index to index_ptr
        CHECK(pthread_create(&storage_server_down_thread, NULL, storageServerChecker, index_ptr), -1, ERR_THREAD_CREATE);
        // free(clientfd_ptr); // Free the allocated memory
    }
    CHECK(close(ns_ss_fd), -1, ERR_NETWORK_ERROR);
    return NULL;

}