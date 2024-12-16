#ifndef __SS_HEADERS_H__
#define __SS_HEADERS_H__

#include "../common/common_headers.h"

extern int parsed_client_fd;
extern char* client_ip;
// main.c
void printClientInterface();
void readFile(char* file_path);
void getOperation(char* send_operation, int option);
int checkOption(int client_fd, int option);

//ss-cl opeerations
void readOperation(int client_fd, char* path);
void writeOperation(int client_fd, char* path);
void retrieveOperation(int client_fd, char* path);
void streamOperation(int client_fd, char* path);
#endif