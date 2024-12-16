#include "headers.h"
CacheEntry* cacheHead = NULL;
CacheEntry* cacheTail = NULL;

int cacheSize = 0;
pthread_mutex_t cacheMutex = PTHREAD_MUTEX_INITIALIZER;

void initCache() {
    cacheHead = NULL;
    cacheTail = NULL;
    cacheSize = 0;
}

int getFromCache(const char* path, int file_type) {
    CacheEntry* curr = cacheHead;
    while (curr != NULL) {
        if (strcmp(curr->path, path) == 0 && curr->file_type == file_type) {
            // Move to front
            if (curr != cacheHead) {
                // Remove from current position
                if (curr->prev) curr->prev->next = curr->next;
                if (curr->next) curr->next->prev = curr->prev;
                if (curr == cacheTail) cacheTail = curr->prev;
                // Insert at front
                curr->next = cacheHead;
                curr->prev = NULL;
                cacheHead->prev = curr;
                cacheHead = curr;
            }
            addToLog("Cache hit for path '%s' with index %d", path, curr->index);
            return curr->index;
        }
        curr = curr->next;
    }
    return -1; // Not found
}

void putInCache(const char* path, int index, int file_type) {
    CacheEntry* newEntry = (CacheEntry*) malloc(sizeof(CacheEntry));
    newEntry->path = strdup(path);
    newEntry->index = index;
    newEntry->file_type = file_type;
    newEntry->prev = NULL;
    newEntry->next = cacheHead;
    if (cacheHead != NULL)
        cacheHead->prev = newEntry;
    cacheHead = newEntry;
    if (cacheTail == NULL)
        cacheTail = newEntry;
    cacheSize++;
    // Remove LRU if cache exceeds size
    if (cacheSize > MAX_CACHE_SIZE) {
        CacheEntry* lru = cacheTail;
        cacheTail = cacheTail->prev;
        cacheTail->next = NULL;
        free(lru->path);
        free(lru);
        cacheSize--;
    }
    addToLog("Added to cache: path '%s', index %d", path, index);
}

void deleteFromCache(const char* path) {
    CacheEntry* curr = cacheHead;

    printf("[DEBUG]: Before delete\n");
    while(curr != NULL) {
        printf("Path: %s\n", curr->path);
        curr = curr->next;
    }

    curr = cacheHead;

    while(curr != NULL) {
        if(strncmp(curr->path, path, strlen(path)) == 0) {
            if (curr != cacheHead) {
                // Remove from current position
                if (curr->prev) curr->prev->next = curr->next;
                if (curr->next) curr->next->prev = curr->prev;
                if (curr == cacheTail) cacheTail = curr->prev;
            }
            else {
                cacheHead = curr->next;
                if (cacheHead) cacheHead->prev = NULL;
            }
            cacheSize--;
        }
        curr = curr->next;
    }

    addToLog("Deleted from cache paths starting with '%s'", path);

    printf("[DEBUG]: After delete\n");
    curr = cacheHead;
    while(curr != NULL) {
        printf("Path: %s\n", curr->path);
        curr = curr->next;
    }
}

int getIndexOfStorageServer(directoryNode* root, char* path, int file_type) {
    int index;
    pthread_mutex_lock(&cacheMutex);
    index = getFromCache(path, file_type);
    // printf("Index: %d\n", index); // Debug
    pthread_mutex_unlock(&cacheMutex);
    if(index == -1){
        index = searchTree(root, path, file_type); // Ensure to change this (add file_type parameter for getCache)
        if(index != -1) {
            pthread_mutex_lock(&cacheMutex);
            putInCache(path, index, file_type);
            pthread_mutex_unlock(&cacheMutex);
        }
    }
    printf("[CONC DEBUG]:I am in CACHE Index: %d\n", index);
    if(file_type == 1) {
        directoryNode* temp = getDirectoryPath(root, path, file_type);
        if(temp==NULL){
            return -1;
        }
        printf("[CONC DEBUG]: File is being writtens value is%d\n",temp->file_information->isbeingwritten);
        if(temp->file_information->isbeingwritten==1){
            printf("[CONC DEBUG]: File is being written i am in cache\n");
            index=-2;
        }
    }
    printf("[CONC DEBUG]:I am in CACHE Index: %d\n", index);
    return index;
}