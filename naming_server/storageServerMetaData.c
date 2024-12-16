#include "headers.h"

pthread_mutex_t storageServerMutex = PTHREAD_MUTEX_INITIALIZER;
int numberOfStorageServers = 0;
storageServer listOfStorageServers[100];
int addStorageServer(char* ip_address, int port_number,int clientport, int status, int client_file_descriptor){ // adds the storage server to the list of storage servers, also returns index of the the added server even if already present
    storageServer* newStorageServer = createStorageServer(ip_address, port_number,clientport ,status, client_file_descriptor);
    int index=-1;
    if((index=findStorageServer(ip_address,port_number)) == -1){
        if(numberOfStorageServers>MAX_STORAGE_SERVERS){
            return -1;
        }
        pthread_mutex_lock(&storageServerMutex);
        listOfStorageServers[numberOfStorageServers] = *newStorageServer;
        index = numberOfStorageServers;
        numberOfStorageServers++;
        pthread_mutex_unlock(&storageServerMutex);
        addToLog("Added new storage server: IP %s, Port %d, Client Port %d", ip_address, port_number, clientport);
    } else {
        addToLog("Storage server already exists: IP %s, Port %d", ip_address, port_number);
    }
    return index;
}
int findStorageServer(char* ip_address, int port_number){ // finds the storage server with the given IP address and port number, returns -1 if not found
    for(int i = 0; i < numberOfStorageServers; i++){
        if(strcmp(listOfStorageServers[i].ip_address, ip_address) == 0 && listOfStorageServers[i].ss_port_number == port_number){
            return i;
        }
    }
    return -1;
}
storageServer* createStorageServer(char* ip_address, int port_number,int clientport, int status, int client_file_descriptor){// creates a storage server with the given IP address, port number and status
    storageServer* newStorageServer = (storageServer*) malloc(sizeof(storageServer));
    newStorageServer->ip_address = strdup(ip_address);
    newStorageServer->ss_port_number = port_number;
    newStorageServer->cl_port_number=clientport;
    newStorageServer->ss_file_descriptor = client_file_descriptor;
    newStorageServer->status = status;
    newStorageServer->backup = NULL;
    return newStorageServer;
}