// initialization of the storage server
#include "headers.h"

// initializes the storage server
int initStorageServer() {
    // initialize the directory tree
    int num_directory_paths;
    int initialindex=-1;
    printf("Enter your Index -1 if you connect for the first time\n");
    scanf("%d",&initialindex);
    int naming_server_fd=connectXtoY(NM_SS_PORT, storage_server_ip);
    send(naming_server_fd,&initialindex,sizeof(initialindex),0);
    while(true) {
        printf("Add list of accessible paths to storage server\n");
        if(scanf("%d", &num_directory_paths) == 1 && num_directory_paths > 0) {
            break;
        }
        else {
            printf("%sError: Invalid input...try again!\n%s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            while(getchar() != '\n');
        }
    }
    printf("start with a \"1 \" for a file, \"0 \" for a directory\n");
    path* paths = (path*) malloc(num_directory_paths * sizeof(struct path));
    CHECK(send(naming_server_fd, &num_directory_paths, sizeof(num_directory_paths), 0), -1, ERR_NETWORK_ERROR);
    for(int i = 0; i < num_directory_paths; i++) {
        int isFile;
        if(scanf("%d %s",&isFile, paths[i].dirPath) == 2 && isFile >= 0 && isFile <= 1) {
            paths[i].isFile = isFile;
        }
        else {
            printf("%sError: Invalid input...try again!\n%s", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            i--;
            while(getchar() != '\n');
            continue;
        }
    }
    CHECK(send(naming_server_fd, paths, num_directory_paths * sizeof(struct path), 0), -1, ERR_NETWORK_ERROR);
    free(paths); // Free the allocated memory

    return naming_server_fd;
}