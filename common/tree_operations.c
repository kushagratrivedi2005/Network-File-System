// does operations on the directory tree that has been created
// everything, unless specified, returns the root of the tree
#include "common_headers.h"

// initializes the initial directory structure
directoryNode* initDirectoryTree() {
    directoryNode* root = (directoryNode*) malloc (sizeof(directoryNode));
    root->file_information = NULL;
    root->is_file = 0;
    root->is_folder = 0;
    root->storage_servers = (int*) malloc (sizeof(int) * MAX_STORAGE_SERVERS);
    for(int i = 0; i < MAX_STORAGE_SERVERS; i++) { // no storage servers contain this right now
        root->storage_servers[i] = 0;
    }
    for(int i = 0; i < MAX_ASCII_VALUE; i++) {
        root->next_characters[i] = NULL;
    }
    return root;
}

// adds a directory path to the directory tree
directoryNode* addDirectoryPath(directoryNode* root, char* full_directory_path, int is_file, int storage_server_index) {
    directoryNode* current_directory = root;
    int path_size = strlen(full_directory_path);
    for(int i = 0; i < path_size; i++) {
        if(current_directory->next_characters[(int)(full_directory_path[i])] == NULL) { // if child is not existing, create child until we get to the path
            directoryNode* child_directory = (directoryNode*) malloc (sizeof(directoryNode));
            child_directory->file_information = NULL;
            child_directory->is_file = 0;
            child_directory->is_folder = 0;
            child_directory->storage_servers = (int*) malloc (sizeof(int) * MAX_STORAGE_SERVERS);
            for(int j = 0; j < MAX_STORAGE_SERVERS; j++) {
                child_directory->storage_servers[j] = 0;
            }
            for(int j = 0; j < MAX_ASCII_VALUE; j++) {
                child_directory->next_characters[j] = NULL;
            }
            current_directory->next_characters[(int)(full_directory_path[i])] = child_directory;
        }
        if(i != 0 && full_directory_path[i] == '/') { // add directory if its not existing
            if(current_directory->is_folder == 0) {
                if(current_directory->is_file == 1) {
                    return root;
                }
                current_directory->is_folder = 1;
                fileInformation* folder_information = (fileInformation*) malloc (sizeof(fileInformation));
                folder_information->is_file = 0;
                folder_information->storage_server_index = storage_server_index;
                current_directory->file_information = folder_information;
            }
        }

        // if curent directory is a folder or a file, then add it to storage_server_index
        if(current_directory->is_file == 1 || current_directory->is_folder == 1) {
            current_directory->storage_servers[storage_server_index] = 1;
        }

        // copy the storage servers from the parent to the child
        for(int j = 0; j < MAX_STORAGE_SERVERS; j++) {
            current_directory->next_characters[(int)(full_directory_path[i])]->storage_servers[j] |= current_directory->storage_servers[j];
        }
        current_directory = current_directory->next_characters[(int)(full_directory_path[i])];
    }
    if(is_file == 1) {
        if(current_directory->is_file == 1 || current_directory->is_folder == 1) {
            if(current_directory->is_file == 1) {
                current_directory->storage_servers[storage_server_index] = 1;
            }
            return root;
        }
        fileInformation* file_information = (fileInformation*) malloc (sizeof(fileInformation));
        file_information->is_file = 1;
        file_information->storage_server_index = storage_server_index;
        file_information->isbeingwritten=0;
        // Initialize the mutex lock
        pthread_mutex_init(&file_information->file_lock, NULL); 
        current_directory->file_information = file_information;
        current_directory->is_file = 1;
        for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
            current_directory->storage_servers[i] = 0;
        }
        current_directory->storage_servers[storage_server_index] = 1;
    }
    else {
        if(current_directory->is_folder == 1 || current_directory->is_file == 1) {
            if(current_directory->is_folder == 1) {
                current_directory->storage_servers[storage_server_index] = 1;
            }
            return root;
        }
        fileInformation* folder_information = (fileInformation*) malloc (sizeof(fileInformation));
        folder_information->is_file = 0;
        folder_information->storage_server_index = storage_server_index;
        current_directory->file_information = folder_information;
        current_directory->is_folder = 1;
        for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
            current_directory->storage_servers[i] = 0;
        }
        current_directory->storage_servers[storage_server_index] = 1;
    }
    return root;
}

// searches the tree for the specified file / folder and returns the storage server index
int searchTree(directoryNode* root, char* full_directory_path, int is_file) {
    // printf("[DEBUG]: searchTree with path: %s\n", full_directory_path);
    directoryNode* current_directory = root;
    int path_size = strlen(full_directory_path);
    for(int i = 0; i < path_size; i++) {
        if(current_directory->next_characters[(int)(full_directory_path[i])] == NULL) {
            return -1;
        }
        current_directory = current_directory->next_characters[(int)(full_directory_path[i])];
    }
    if(is_file == 1) {
        if(current_directory->is_file == 1) {
            return current_directory->file_information->storage_server_index;
        }
    }
    else {
        if(current_directory->is_folder == 1) {
            return current_directory->file_information->storage_server_index;
        }
    }
    return -1;
}

// gets the directory path from the root
directoryNode* getDirectoryPath(directoryNode* root, char* full_directory_path, int is_file) {
    directoryNode* current_directory = root;
    int path_size = strlen(full_directory_path);
    for(int i = 0; i < path_size; i++) {
        if(current_directory->next_characters[(int)(full_directory_path[i])] == NULL) {
            return NULL;
        }
        current_directory = current_directory->next_characters[(int)(full_directory_path[i])];
    }
    if(is_file == 1) {
        if(current_directory->is_file == 1) {
            return current_directory;
        }
    }
    else {
        if(current_directory->is_folder == 1) {
            return current_directory;
        }
    }
    return NULL;
}

// deletes the tree from the specified path
void deleteTreeFromPath(directoryNode* root, char* full_directory_path) {
    directoryNode* current_directory = root;
    printf("[DEBUG]: Deleting tree from path: %s\n", full_directory_path);
    int path_size = strlen(full_directory_path);
    for(int i = 0; i < path_size; i++) {
        if(current_directory->next_characters[(int)(full_directory_path[i])] == NULL) {
            return;
        }
        if(i == path_size - 1) {
            if(current_directory->next_characters[(int)(full_directory_path[i])]->is_file == 1) {
                deleteTree(current_directory->next_characters[(int)(full_directory_path[i])]);
                current_directory->next_characters[(int)(full_directory_path[i])] = NULL;
            }
            else {
                current_directory = current_directory->next_characters[(int)(full_directory_path[i])];
                current_directory->is_folder = 0;
                deleteTree(current_directory->next_characters[(int)('/')]);
                current_directory->next_characters[(int)('/')] = NULL;
            }
        }
        current_directory = current_directory->next_characters[(int)(full_directory_path[i])];
    }
}

// gets the list of all the storage servers that contain any file / folder in the subtree
int* getStorageServersInSubtree(directoryNode* node) {
    int* storage_servers = (int*) malloc (sizeof(int) * MAX_STORAGE_SERVERS);
    for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
        storage_servers[i] = 0;
    }
    if(node == NULL) {
        return storage_servers;
    }
    if(node->is_file == 1 || node->is_folder == 1) {
        storage_servers[node->file_information->storage_server_index] = 1;
    }
    for(int i = 0; i < MAX_ASCII_VALUE; i++) {
        int* child_storage_servers = getStorageServersInSubtree(node->next_characters[i]);
        for(int j = 0; j < MAX_STORAGE_SERVERS; j++) {
            if(child_storage_servers[j] == 1) {
                storage_servers[j] = 1;
            }
        }
    }
    return storage_servers;
}

// deletes (and frees) the entire tree
void deleteTree(directoryNode* current_directory) {
    if(current_directory == NULL) {
        return;
    }
    for(int i = 0; i < MAX_ASCII_VALUE; i++) {
        if(current_directory->next_characters[i] != NULL) {
            deleteTree(current_directory->next_characters[i]);
            current_directory->next_characters[i] = NULL;
        }
    }
    if(current_directory->file_information != NULL) {
        free(current_directory->file_information);
        current_directory->file_information = NULL;
    }
    if(current_directory->storage_servers != NULL) {
        free(current_directory->storage_servers);
        current_directory->storage_servers = NULL;
    }
    free(current_directory);
}

// function to get all the accessible paths of the tree and store it in a string
char* getAllAccessiblePaths(directoryNode* node, char* parent_directory_path) {
    directoryNode* current_directory = node;
    char* return_value = (char*) malloc (sizeof(char) * MAX_TREE_SIZE);
    if(current_directory == NULL) {
        return "";
    }
    if(current_directory->is_file == 1) {
        snprintf(return_value, MAX_TREE_SIZE, "%s%s%s\n", ANSI_COLOR_GREEN, parent_directory_path, ANSI_COLOR_RESET);
    }
    if(current_directory->is_folder == 1) {
        snprintf(return_value, MAX_TREE_SIZE, "%s%s%s\n", ANSI_COLOR_BLUE, parent_directory_path, ANSI_COLOR_RESET);
    }
    for(int i = 0; i < MAX_ASCII_VALUE; i++) {
        if(current_directory->next_characters[i] != NULL) {
            char* new_directory_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(new_directory_path, MAX_FILE_PATH, "%s%c", parent_directory_path, (char)(i));
            char* child_return_value = getAllAccessiblePaths(current_directory->next_characters[i], new_directory_path);
            strcat(return_value, child_return_value);
        }
    }
    return return_value;
}

// function to check whether any files in the subtree is being written or not
int checkSubtreeFilesBeingWritten(directoryNode* node) {
    if(node == NULL) {
        return 0;
    }
    if(node->is_file == 1) {
        if(node->file_information->isbeingwritten == 1) {
            return 1;
        }
    }
    for(int i = 0; i < MAX_ASCII_VALUE; i++) {
        if(node->next_characters[i] != NULL) {
            if(checkSubtreeFilesBeingWritten(node->next_characters[i]) == 1) {
                return 1;
            }
        }
    }
    return 0;
}

// recursive function to copy the folder
void callCopyFolderRecursive(directoryNode* node, char* source_path, char* destination_path, int client_fd, int storage_server_index, storageServer* ss_list, directoryNode* root, int next_slash_compulsory) {
    printf("[DEBUG]: Coming to callCopyFolderRecursive!\n");
    if(node == NULL) {
        return;
    }
    if(node->is_file == 1 || node->is_folder == 1) {
        printf("[DEBUG]: Copying %s to %s\n", source_path, destination_path);
        char* send_operation = (char*) malloc (sizeof(char) * BUF_SIZE);
        strcpy(send_operation, "COPY");

        int ss_file_descriptor = ss_list[storage_server_index].ss_file_descriptor;
        storageServer cl = ss_list[node->file_information->storage_server_index];

        CHECK(send(ss_file_descriptor, send_operation, BUF_SIZE, 0), -1, ERR_NETWORK_ERROR);

        struct path source_path_struct, destination_path_struct;
        strcpy(source_path_struct.dirPath, source_path);
        source_path_struct.isFile = node->is_file;

        strcpy(destination_path_struct.dirPath, destination_path);
        destination_path_struct.isFile = node->is_file;

        CHECK(send(ss_file_descriptor, &source_path_struct, sizeof(source_path_struct), 0), -1, ERR_NETWORK_ERROR);
        CHECK(send(ss_file_descriptor, &destination_path_struct, sizeof(destination_path_struct), 0), -1, ERR_NETWORK_ERROR);

        ssDetailsForClient ssDetails;
        ssDetails.port_number = cl.cl_port_number;
        strcpy(ssDetails.ip_address, cl.ip_address);
        ssDetails.status = 1;

        CHECK(send(ss_file_descriptor, &ssDetails, sizeof(ssDetails), 0), -1, ERR_NETWORK_ERROR);

        addDirectoryPath(root, destination_path, node->is_file, storage_server_index);

        free(send_operation);
    }

    for(int i = 0; i < MAX_ASCII_VALUE; i++) {
        if(node->next_characters[i] != NULL && (next_slash_compulsory == 0 || i == (int)('/'))) {
            char* new_source_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            char* new_destination_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(new_source_path, MAX_FILE_PATH, "%s%c", source_path, (char)(i));
            snprintf(new_destination_path, MAX_FILE_PATH, "%s%c", destination_path, (char)(i));
            callCopyFolderRecursive(node->next_characters[i], new_source_path, new_destination_path, client_fd, storage_server_index, ss_list, root, 0);
            free(new_source_path);
            free(new_destination_path);
        }
    }
}

// debugging code -- prints the whole tree
void __debugPrintTree(directoryNode* current_directory, char* parent_directory_path) {
    if(current_directory->is_file == 1) {
        printf("%s%s%s\n", ANSI_COLOR_GREEN, parent_directory_path, ANSI_COLOR_RESET);
        printf("List of storage servers that contain the above file: ");
        for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
            if(current_directory->storage_servers[i] == 1) {
                printf("%d ", i);
            }
        }
        printf("\n");
    }
    else if(current_directory->is_folder == 1) {
        printf("%s%s%s\n", ANSI_COLOR_BLUE, parent_directory_path, ANSI_COLOR_RESET);
        printf("List of storage servers that contain the above folder: ");
        for(int i = 0; i < MAX_STORAGE_SERVERS; i++) {
            if(current_directory->storage_servers[i] == 1) {
                printf("%d ", i);
            }
        }
        printf("\n");
    }

    for(int i = 0; i < MAX_ASCII_VALUE; i++) {
        if(current_directory->next_characters[i] != NULL) {
            char* new_directory_path = (char*) malloc (sizeof(char) * MAX_FILE_PATH);
            snprintf(new_directory_path, MAX_FILE_PATH, "%s%c", parent_directory_path, (char)(i));
            __debugPrintTree(current_directory->next_characters[i], new_directory_path);
            free(new_directory_path);
        }
    }
}