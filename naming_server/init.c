// initialization of the naming server
#include "headers.h"
directoryNode* root;


// initializes the naming server
void initNamingServer() {
    pthread_mutex_init(&directory_tree_mutex, NULL);
    pthread_mutex_lock(&directory_tree_mutex);
    root = initDirectoryTree();
    pthread_mutex_unlock(&directory_tree_mutex);
    
    initCache();
    addToLog("Naming server initialized");
}