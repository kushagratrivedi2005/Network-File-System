#ifndef __NS_HEADERS_H__
#define __NS_HEADERS_H__

// include files
#include "../common/common_headers.h"

// global defines
#define MAX_CACHE_SIZE 16

typedef struct CacheEntry {
    char* path;
    int index;
    int file_type;
    struct CacheEntry* prev;
    struct CacheEntry* next;
} CacheEntry;

// function prototypes
void initNamingServer();
void* NSReceiveClientOperations(void* arg);
void* storageServerChecker(void* arg);
void handleClientOperation(int client_fd, char* operation);
void initCache();
int getFromCache(const char* path, int file_type);
void putInCache(const char* path, int index, int file_type);
void deleteFromCache(const char* path);
int getIndexOfStorageServer(directoryNode* root, char* path, int file_type);
void addToLog(const char* format, ...);

// global variables
extern directoryNode* root; // root of the directory structure
extern pthread_mutex_t cacheMutex;
extern pthread_mutex_t directory_tree_mutex;

#endif