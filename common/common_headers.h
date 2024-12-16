#ifndef __COMMON_HEADERS_H__
#define __COMMON_HEADERS_H__

// global headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdbool.h>
#include <fcntl.h>
#include <mpv/client.h>
#include <dirent.h>
#include <stdarg.h>
#include <sys/time.h>

// helper function defines
// Define a custom error-checking macro with red error messages
#define CHECK(x, y, err_code) if ((x) == (y)) { \
    print_error(#x " failed"); \
    exit(err_code); \
}

// defining colours
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_ORANGE  "\x1b[38;5;208m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// Error codes for client requests
#define ERR_FILE_NOT_FOUND              1  // File does not exist
#define ERR_FILE_BUSY                   2  // File is currently being written by another client
#define ERR_STORAGE_SERVER_UNAVAILABLE  3  // Storage server unavailable
#define ERR_NETWORK_ERROR               4  // Network error occurred
#define ERR_OPERATION_NOT_SUPPORTED     5  // Operation not supported
#define ERR_THREAD_CREATE               6  // Thread creation failed
#define ERR_UNEXPECTED_ERROR            7  // Unexpected error occurred

// common defines
#define NM_SS_PORT 42069 // naming server to storage server port
#define NM_CL_PORT 42070 // naming server to client port
#define MAX_CONNECTIONS 10 // maximum number of connections that can be queued to any port
#define MAX_FILE_NAME 100 // maximum length of the file name
#define MAX_FILE_PATH 1024 // maximum length of the file path
#define MAX_STORAGE_SERVERS 100 // maximum number of storage servers that can be connected to the naming server
#define MAX_IP_SIZE 20 // maximum length of the IP address
#define TEMP_IP "127.0.0.1" // temporary IP address for testing
#define BUF_SIZE 1024 // buffer size for reading and writing data
#define MAX_STR_LEN 1024 // maximum length of a string
#define MAX_TREE_SIZE 4096 // maximum size of the directory tree
#define MAX_ASCII_VALUE 256
#define MAX_COMMAND_SIZE 1024 // maximum length of the command
#define SERVER_LOAD 10 // maximum number of clients that can be connected to a storage server

// helper functions
char** split(const char* str, const char* delimiter, int* count);

// directory information
typedef struct fileInformation { // stores the information about the file
    int is_file;
    int isbeingwritten;
    pthread_mutex_t file_lock;
    int storage_server_index; // index of the storage server where the file is stored
} fileInformation;

typedef struct directoryNode { // stores the information about the directory structure (as a trie!)
    fileInformation* file_information; // only true if is_file flag is set to 1
    int is_file;
    int is_folder;
    int* storage_servers; // list of storage servers that contain the file / folder
    struct directoryNode* next_characters[MAX_ASCII_VALUE]; // 256 characters in the ASCII table
} directoryNode;

// network_operations.c common headers
int connectXtoY(int port, char* ip);
void* hearSSforNS(void * arg); //listens for connections from storage servers to the naming server
int SSconnectsToNS(); //connects the storage server to the naming server
int connectClientToNS();//connects the client to the naming server
void* hearClientforSS(void * arg); //listens for connections from client servers to the storage server
void* hearClientforNS(void * arg); //listens for connections from client servers to the naming server
void print_error(const char *msg); //general error printing function
void send_data_in_packets(void *buffer, const int sockfd, int buffer_length);
void receive_data_in_packets(void *buffer, const int sockfd, int buffer_length);
void receiveFileChunks(const char* file_path, int socketfd); 
void sendFileChunks(const char* file_path, int socketfd); 

typedef struct path {
    char dirPath[MAX_FILE_PATH];
    int isFile;
} path;

extern int numberOfStorageServers;
typedef struct storageServer{
    char* ip_address;
    int ss_port_number;
    int cl_port_number;
    int ss_file_descriptor;
    int status;
    struct storageServer* backup;
}storageServer;
extern storageServer listOfStorageServers[MAX_STORAGE_SERVERS]; // list of storage servers
int addStorageServer(char* ip_address, int port_number,int clientport, int status, int client_file_descriptor); // adds the storage server to the list of storage servers, also returns index of the latest added server
int findStorageServer(char* ip_address, int port_number); // finds the storage server with the given IP address and port number, returns -1 if not found
storageServer* createStorageServer(char* ip_address, int port_number,int clientport, int status, int client_file_descriptor); // creates a storage server with the given IP address, port number and status


typedef struct ssDetailsForClient{
    char ip_address[MAX_IP_SIZE];
    int port_number;
    int status;
}ssDetailsForClient;

// tree_operations.c common headers
directoryNode*  initDirectoryTree(); // initializes the directory tree
directoryNode*  addDirectoryPath(directoryNode* root, char* directory_path, int is_file, int storage_server_index); // adds the directory path to the directory tree structure
int             searchTree(directoryNode* root, char* full_directory_path, int is_file); // searches the tree for the specified file / folder and returns the storage server index
directoryNode*  getDirectoryPath(directoryNode* root, char* full_directory_path, int is_file); // gets the directory path from the root
void            deleteTreeFromPath(directoryNode* root, char* full_directory_path); // deletes the tree from the specified path   
int*            getStorageServersInSubtree(directoryNode* node); // gets the list of all the storage servers that contain any file / folder in the subtree
void            deleteTree(directoryNode* root); // deletes the tree
char*           getAllAccessiblePaths(directoryNode* node, char* already_accessible_paths); // gets all the accessible paths from the root
int             checkSubtreeFilesBeingWritten(directoryNode* node); // checks if any file in the subtree is being written
void            callCopyFolderRecursive(directoryNode* node, char* source_path, char* destination_path, int client_fd, int storage_server_index, storageServer* ss_list, directoryNode* root, int next_slash_compulsory); // recursive function to copy the folder
void            __debugPrintTree(directoryNode* root, char* parent_directory_path); // debugging function to print the tree

#include "../client/headers.h"
#include "../naming_server/headers.h"
#include "../storage_server/headers.h"

#endif