#ifndef __SS_HEADERS_H__
#define __SS_HEADERS_H__

#include "../common/common_headers.h"
// for passing this information to the NS threads for client operations
extern int storage_server_fd;
extern char* storage_server_ip;
// init.c
int initStorageServer();
void* SSReceiveClientOperations(void *arg);
void* SSReceiveNamingServerOperations(void* arg);
void handleNSCommands(char* command);
void* sendAliveMessages(void* arg);
// ss_ns_communication.c
void copy();
void delete1();
void create();
void copyFile(const char* source, const char* destination, int source_fd);
#endif