# Network File System

## How to Run

```bash
    git clone https://github.com/OSN-Monsoon-2024/course-project-team_9.git
    cd course-project-team_9
    git checkout diff_ip
    git pull

    make                                                                      # Compiles the code

    ./ns.out                                                                  # Naming Server Executable
    ./ss.out <ip_address_of_ns>                                               # Storage Server Executable
    ./client.out <ip_address_of_ns>                                           # Client Executable
```

## File Structure

```Markdown
    .
    ├── common
    │   ├── common_headers.h
    │   ├── helpers.c
    │   ├── network_operations.c
    │   └── tree_operations.c
    |
    ├── client
    │   ├── headers.h
    │   ├── main.c
    │   └── serveroperations.c
    |
    ├── naming_server
    │   ├── cache.c
    │   ├── headers.h
    │   ├── init.c
    │   ├── main.c
    │   ├── ns_cl_communication.c
    │   ├── ns_ss_communication.c
    │   └── storageServerMetaData.c
    |
    ├── storage_server
    │   ├── headers.h
    │   ├── init.c
    │   ├── main.c
    │   ├── ss_cl_communication.c
    │   └── ss_ns_communication.c
    |
    └── Makefile
```

## Description of Files

### common

The `common` directory contains shared code and headers used by the client, naming server, and storage server. It includes utility functions, data structures, and definitions that are common across the project.

#### common_headers.h

This header file includes global headers, macros, constants, helper functions, data structures, and function declarations used throughout the project.

**Contents:**

- **Global Headers**: Includes standard C libraries and system headers such as `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<arpa/inet.h>`, `<unistd.h>`, `<pthread.h>`, and others.

- **Macros**:
  - **Error Checking Macro**: Defines `CHECK` macro for error handling.
  - **Color Codes**: ANSI color codes for terminal output formatting.

- **Error Codes**: Defines error codes for various client request errors and network errors (e.g., `ERR_FILE_NOT_FOUND`, `ERR_NETWORK_ERROR`).

- **Constants**: Defines constants for network ports (`NM_SS_PORT`, `NM_CL_PORT`), buffer sizes (`BUF_SIZE`), maximum lengths, and other configuration values.

- **Helper Functions**: Declares utility functions like `split` for string operations.

- **Data Structures**:
  - `fileInformation`: Stores information about files, such as whether it's a file or folder, lock status, and associated storage server index.
  - `directoryNode`: Represents nodes in the directory tree, including pointers to child nodes, file information, and storage server data.
  - `path`: Struct for handling file paths and type indicators.
  - `storageServer`: Contains information about storage servers, including IP address, port numbers, status, and file descriptors.
  - `ssDetailsForClient`: Holds details sent to clients regarding storage server information.

- **Function Declarations**:
  - **Network Operations**: Functions for connecting components (`connectXtoY`), sending and receiving data (`send_data_in_packets`, `receive_data_in_packets`), and file transfers (`sendFileChunks`, `receiveFileChunks`).
  - **Tree Operations**: Functions for initializing and manipulating the directory tree (`initDirectoryTree`, `addDirectoryPath`, `searchTree`, `deleteTree`).
  - **Error Handling**: Function `print_error` for printing formatted error messages.

#### helpers.c

This source file implements utility functions used across the project.

**Key Functions:**

- `split`:
  - **Purpose**: Splits a given string into tokens based on a specified delimiter.
  - **Usage**: Helps in parsing commands and input strings where multiple parameters are provided in a single line.

#### network_operations.c

This source file contains functions related to network communication between the client, naming server, and storage server.

**Key Functions:**

- `connectXtoY`:
  - **Purpose**: Establishes a TCP connection from one component to another using specified IP and port.
  - **Usage**: Used by clients and servers to connect to other services in the network.

- `sendFileChunks`:
  - **Purpose**: Reads a file and sends its contents over a socket in chunks.
  - **Usage**: Facilitates file transfer between storage servers and clients.

- `receiveFileChunks`:
  - **Purpose**: Receives file chunks from a socket and writes them to a file.
  - **Usage**: Used by clients and servers to receive files sent over the network.

- `send_data_in_packets` and `receive_data_in_packets`:
  - **Purpose**: Handles sending and receiving data over a network socket in fixed-size packets.
  - **Usage**: Ensures that large amounts of data are transmitted reliably in controlled packet sizes.

- `print_error`:
  - **Purpose**: Prints error messages with consistent formatting.
  - **Usage**: Used throughout the project for error reporting.

#### tree_operations.c

This source file implements operations on the directory tree structure, which represents the file system hierarchy within the naming server.

**Key Functions:**

- `initDirectoryTree`:
  - **Purpose**: Initializes the root of the directory tree.
  - **Usage**: Called during the initialization of the naming server to set up the file system structure.

- `addDirectoryPath`:
  - **Purpose**: Adds a file or directory path to the tree.
  - **Usage**: Updates the tree when new files or directories are created.

- `searchTree`:
  - **Purpose**: Searches for a given path in the tree and returns the storage server index if found.
  - **Usage**: Helps in locating where a file or directory is stored.

- `getDirectoryPath`:
  - **Purpose**: Retrieves the node corresponding to a given path.
  - **Usage**: Used for operations that require manipulating or querying specific files or directories.

- `deleteTreeFromPath`:
  - **Purpose**: Deletes a subtree starting from a specific path.
  - **Usage**: Used when files or directories are deleted.

- `deleteTree`:
  - **Purpose**: Recursively frees memory associated with the entire tree.
  - **Usage**: Called when shutting down the naming server or resetting the file system.

- `getAllAccessiblePaths`:
  - **Purpose**: Generates a list of all accessible paths from the current node.
  - **Usage**: Provides clients with a list of available files and directories.

- `checkSubtreeFilesBeingWritten`:
  - **Purpose**: Checks if any file in the subtree is currently being written.
  - **Usage**: Ensures data consistency and prevents conflicts during file operations.

- `callCopyFolderRecursive`:
  - **Purpose**: Recursively copies a folder from a source path to a destination path across storage servers.
  - **Usage**: Implements folder copy functionality requested by clients.

### client

The `client` directory contains the code for the client application that interacts with the naming server and storage server. It allows the user to perform operations such as reading, writing, retrieving information about files, streaming audio files, creating and deleting files and folders, copying files or folders, and listing all files and folders.

#### headers.h

This header file declares functions and external variables used by the client application.

**Contents:**

- **Includes**: The common headers from the `common` directory.
- **External Variables**:
  - `int parsed_client_fd`: Stores the parsed client file descriptor.
  - `char* client_ip`: Stores the client's IP address.
- **Function Declarations**:
  - **Main Functions**:
    - `void printClientInterface()`: Displays the options available to the user.
    - `void readFile(char* file_path)`: Prompts the user to enter the path of the file.
    - `void getOperation(char* send_operation, int option)`: Maps the user's choice to the corresponding operation string.
    - `int checkOption(int client_fd, int option)`: Validates the user's input and processes the selected option.
  - **Server Operations**:
    - `void readOperation(int client_fd, char* path)`: Handles reading a file from the storage server.
    - `void writeOperation(int client_fd, char* path)`: Handles writing to a file on the storage server.
    - `void retrieveOperation(int client_fd, char* path)`: Retrieves information about a file from the storage server.
    - `void streamOperation(int client_fd, char* path)`: Streams an audio file from the storage server.

#### main.c

This source file contains the main function and handles user interaction, sending requests to the naming server, and processing responses.

**Key Functions:**

- `printClientInterface`:
  - **Purpose**: Displays the menu of operations that the user can perform.
  - **Usage**: Called in the main loop to prompt the user for input.

- `readFile`:
  - **Purpose**: Prompts the user to enter the path of a file.
  - **Usage**: Used when an operation requires a file path input.

- `getOperation`:
  - **Purpose**: Maps the user's menu selection to the corresponding operation string used in requests.
  - **Usage**: Prepares the operation string before sending it to the naming server.

- `checkOption`:
  - **Purpose**: Validates the user's selected option and handles the operation.
  - **Usage**:
    - For options involving file operations (e.g., read, write), it interacts with the naming server to get storage server details.
    - Connects to the appropriate storage server and calls the relevant function from `serveroperations.c`.
    - Processes responses for operations like listing files.

- `main`:
  - **Purpose**: The entry point of the client application.
  - **Usage**:
    - Parses command-line arguments to get the IP address of the naming server.
    - Establishes a connection to the naming server.
    - Enters a loop to display the menu, get user input, and process the selected operation.
    - Exits the loop when the user chooses to quit.

#### serveroperations.c

This source file implements functions that handle communication with the storage server for various file operations.

**Key Functions:**

- `readOperation`:
  - **Purpose**: Handles the "READ" operation to read a file from the storage server.
  - **Usage**:
    - Sends a read request to the storage server with the file path.
    - Receives the file data in chunks and displays it to the user.

- `writeOperation`:
  - **Purpose**: Handles the "WRITE" operation to write data to a file on the storage server.
  - **Usage**:
    - Prompts the user for input data to write.
    - Sends the data in chunks to the storage server.

- `retrieveOperation`:
  - **Purpose**: Handles the "RETRIEVE" operation to get metadata about a file.
  - **Usage**:
    - Sends a retrieve request to the storage server with the file path.
    - Receives and displays file information such as permissions, owner, size, and last modified date.

- `streamOperation`:
  - **Purpose**: Handles the "STREAM" operation to stream an audio file from the storage server.
  - **Usage**:
    - Sends a stream request to the storage server with the file path.
    - Receives audio data and streams it using the `mpv` media player in low-latency mode.

### naming_server

The `naming_server` directory contains the code for the naming server component of the distributed file system. The naming server maintains the directory structure, handles client requests, communicates with storage servers, and manages metadata and caching.

#### headers.h

This header file declares functions, data structures, and external variables used by the naming server.

**Contents:**

- **Includes**: Common headers from the `common` directory.
- **Defines**:
  - **Constants**: `MAX_CACHE_SIZE` for cache management.
  - **Data Structures**:
    - `CacheEntry`: Represents entries in the cache, including path, index, file type, and pointers for a doubly linked list.
- **Function Prototypes**:
  - **Initialization Functions**:
    - `void initNamingServer()`: Initializes the naming server.
    - `void initCache()`: Initializes the cache.
  - **Client Handling**:
    - `void* NSReceiveClientOperations(void* arg)`: Thread function to receive and handle client operations.
    - `void handleClientOperation(int client_fd, char* operation)`: Processes a specific client operation.
  - **Storage Server Handling**:
    - `void* storageServerChecker(void* arg)`: Monitors the status of connected storage servers.
  - **Cache Functions**:
    - `int getFromCache(const char* path, int file_type)`: Retrieves an entry from the cache.
    - `void putInCache(const char* path, int index, int file_type)`: Adds an entry to the cache.
    - `void deleteFromCache(const char* path)`: Removes entries from the cache matching a path.
  - **Utility Functions**:
    - `int getIndexOfStorageServer(directoryNode* root, char* path, int file_type)`: Gets the index of the storage server for a given path.
    - `void addToLog(const char* format, ...)`: Logs messages to a log file.
- **Global Variables**:
  - `directoryNode* root`: Represents the root of the directory tree.
  - `pthread_mutex_t cacheMutex`: Mutex for synchronizing access to the cache.
  - `pthread_mutex_t directory_tree_mutex`: Mutex for synchronizing access to the directory tree.

#### init.c

This source file initializes the naming server, setting up the directory tree and cache.

**Key Functions:**

- `initNamingServer`:
  - **Purpose**: Initializes the naming server by setting up the directory tree and cache.
  - **Usage**:
    - Initializes the `directory_tree_mutex`.
    - Calls `initDirectoryTree` to create the root of the directory tree.
    - Calls `initCache` to set up the cache.
    - Logs the initialization message.

#### main.c

This source file contains the main function for the naming server and handles incoming connections from storage servers and clients.

**Key Components:**

- **Global Variables**:
  - `pthread_mutex_t directory_tree_mutex`: Mutex for synchronizing directory tree access.
  - `pthread_t _global_ns_ss_server_thread, _global_ns_cl_thread`: Threads for handling storage server and client communications.
- **Functions**:
  - `addToLog`:
    - **Purpose**: Appends formatted messages to a log file with a timestamp.
    - **Usage**: Used throughout the naming server to record significant events.
- **Main Function**:
  - **Purpose**: Entry point of the naming server application.
  - **Usage**:
    - Initializes the naming server by calling `initNamingServer`.
    - Creates threads to listen for storage server (`hearSSforNS`) and client (`hearClientforNS`) connections.
    - Waits for threads to finish (which is indefinite in this case).

#### cache.c

This source file implements an LRU (Least Recently Used) cache for the naming server to store path lookups.

**Key Components:**

- **Data Structures**:
  - `CacheEntry`: Represents a cache entry with path, index, file type, and pointers for a doubly linked list.
- **Global Variables**:
  - `CacheEntry* cacheHead, *cacheTail`: Pointers to the head and tail of the cache.
  - `int cacheSize`: Keeps track of the current size of the cache.
  - `pthread_mutex_t cacheMutex`: Mutex for thread-safe cache operations.
- **Functions**:
  - `initCache`:
    - **Purpose**: Initializes the cache by setting the head, tail, and size.
  - `getFromCache`:
    - **Purpose**: Retrieves the storage server index for a given path and file type from the cache.
    - **Usage**:
      - Searches the cache for a matching entry.
      - Moves the accessed entry to the front (MRU position).
      - Returns `-1` if not found.
  - `putInCache`:
    - **Purpose**: Adds a new entry to the cache.
    - **Usage**:
      - Inserts the new entry at the front.
      - Removes the least recently used entry if the cache exceeds `MAX_CACHE_SIZE`.
  - `deleteFromCache`:
    - **Purpose**: Removes entries from the cache whose paths start with a given prefix.
    - **Usage**: Used when a file or directory is deleted to invalidate related cache entries.
  - `getIndexOfStorageServer`:
    - **Purpose**: Retrieves the storage server index for a given path, checking the cache first.
    - **Usage**:
      - If the entry is in the cache, returns the index.
      - If not, searches the directory tree and updates the cache.
      - Checks if the file is currently being written to prevent conflicts.

#### ns_cl_communication.c

This source file handles communication between the naming server and clients.

**Key Functions:**

- `hearClientforNS`:
  - **Purpose**: Listens for client connections and creates a new thread for each client to handle operations.
- `NSReceiveClientOperations`:
  - **Purpose**: Receives and processes client operations.
  - **Usage**:
    - Reads client requests and calls `handleClientOperation`.
- `handleClientOperation`:
  - **Purpose**: Processes specific client operations such as READ, WRITE, STREAM, RETRIEVE, LIST, CREATE_FILE, CREATE_FOLDER, DELETE_FILE, DELETE_FOLDER, and COPY.
  - **Usage**:
    - Determines the operation and path from the client's request.
    - Interacts with the directory tree and cache.
    - Communicates with storage servers to perform the requested operation.
    - Handles error cases and sends appropriate responses to the client.

#### ns_ss_communication.c

This source file manages communication between the naming server and storage servers.

**Key Functions:**

- `hearSSforNS`:
  - **Purpose**: Listens for storage server connections and processes their initial data.
  - **Usage**:
    - Accepts incoming connections from storage servers.
    - Receives initial metadata including the number of paths, paths themselves, and client port number.
    - Adds the storage server to the list of storage servers.
    - Adds received paths to the directory tree.
    - Starts a thread (`storageServerChecker`) to monitor the storage server's connection.

#### storageServerMetaData.c

This source file handles metadata related to storage servers connected to the naming server.

**Key Components:**

- **Global Variables**:
  - `pthread_mutex_t storageServerMutex`: Mutex for synchronizing access to the storage server list.
  - `int numberOfStorageServers`: Tracks the number of connected storage servers.
  - `storageServer listOfStorageServers[100]`: Array holding details of connected storage servers.
- **Functions**:
  - `addStorageServer`:
    - **Purpose**: Adds a new storage server to the list.
    - **Usage**:
      - Checks if the storage server already exists.
      - If new, adds it to the list and increments `numberOfStorageServers`.
      - Logs the addition.
  - `findStorageServer`:
    - **Purpose**: Searches for a storage server in the list by IP address and port number.
    - **Usage**: Used to prevent duplicate entries.
  - `createStorageServer`:
    - **Purpose**: Creates a new `storageServer` struct with provided details.
    - **Usage**: Used internally by `addStorageServer`.

### storage_server

The `storage_server` directory contains the code for the storage server component of the distributed file system. The storage server handles file storage, manages communication with the naming server and clients, and performs file operations such as reading, writing, copying, and deleting files and directories.

#### headers.h

This header file declares functions, data structures, and external variables used by the storage server.

**Contents:**

- **Includes**: The common headers from the `common` directory.
- **External Variables**:
  - `int storage_server_fd`: File descriptor for the naming server connection.
  - `char* storage_server_ip`: Storage server's IP address.
- **Function Declarations**:
  - **Initialization Functions**:
    - `int initStorageServer()`: Initializes the storage server and connects to the naming server.
  - **Communication Functions**:
    - `void* SSReceiveClientOperations(void *arg)`: Manages client operations.
    - `void* SSReceiveNamingServerOperations(void* arg)`: Handles naming server operations.
    - `void* sendAliveMessages(void* arg)`: Periodically sends alive messages to the naming server.
  - **Operation Handlers**:
    - `void copy()`: Handles copy operations.
    - `void delete1()`: Handles delete operations.
    - `void create()`: Handles create operations.
    - `void copyFile(const char* source, const char* destination, int source_fd)`: Helper function for copying files.

#### main.c

This source file contains the main function for the storage server and sets up threads for communication with clients and the naming server.

**Key Components:**

- **Global Variables**:
  - `pthread_mutex_t directory_tree_mutex`: Mutex for synchronizing access to shared resources.
  - `pthread_t _global_ss_client_server_thread`: Thread for handling client communications.
  - `pthread_t _global_ss_ns_server_thread`: Thread for handling naming server communications.
  - `int storage_server_fd`: File descriptor for the naming server connection.
  - `char* storage_server_ip`: Storage server's IP address.
- **Main Function**:
  - **Purpose**: Entry point of the storage server application.
  - **Usage**:
    - Parses command-line arguments to get the IP address of the naming server.
    - Calls `initStorageServer()` to initialize the storage server and connect to the naming server.
    - Creates threads for communicating with the naming server (`SSReceiveNamingServerOperations`) and clients (`hearClientforSS`).
    - Waits for threads to finish execution.

#### init.c

This source file initializes the storage server and establishes a connection with the naming server.

**Key Functions:**

- `initStorageServer`:
  - **Purpose**: Initializes the storage server by setting up accessible directory paths and connecting to the naming server.
  - **Usage**:
    - Prompts the user to input the number of accessible directory paths and their details (whether they are files or directories).
    - Collects the paths and their types from the user.
    - Sends the directory information to the naming server.
    - Returns the file descriptor for the naming server connection.

#### ss_ns_communication.c

This source file manages communication between the storage server and the naming server, handling operations such as create, delete, and copy.

**Key Functions:**

- `SSReceiveNamingServerOperations`:
  - **Purpose**: Listens for commands from the naming server and handles them accordingly.
  - **Usage**:
    - Receives commands from the naming server in a continuous loop.
    - Parses each command and calls the appropriate handler (`create()`, `delete1()`, `copy()`).
  - **Operation Handlers**:
    - `create`:
      - **Purpose**: Handles create file or directory requests from the naming server.
      - **Usage**:
        - Receives the path and type (file or directory) from the naming server.
        - Creates the specified file or directory locally.
        - Sends an acknowledgment back to the naming server.
    - `delete1`:
      - **Purpose**: Handles delete file or directory requests from the naming server.
      - **Usage**:
        - Receives the path to delete from the naming server.
        - Deletes the specified file or directory.
        - Sends an acknowledgment back to the naming server.
    - `copy`:
      - **Purpose**: Handles copy requests from the naming server.
      - **Usage**:
        - Receives source and destination paths and storage server details.
        - Performs the copy operation, handling both files and directories.
    - `copyFile`:
      - **Purpose**: Helper function to copy a file from the source to the destination.
      - **Usage**:
        - Opens the source file and reads its contents.
        - Writes the contents to the destination file.
        - Handles data transfer in chunks to support large files.

#### ss_cl_communication.c

This source file handles communication between the storage server and clients, processing client operations such as read, write, retrieve, and stream.

**Key Functions:**

- `hearClientforSS`:
  - **Purpose**: Listens for client connections and creates a new thread for each client to handle operations.
  - **Usage**:
    - Sets up a socket and listens on a dynamically assigned port (selected by the OS).
    - Sends the port information to the naming server so clients can connect.
    - Accepts client connections and spawns threads to handle them concurrently.
- `SSReceiveClientOperations`:
  - **Purpose**: Receives and processes client operations.
  - **Usage**:
    - Receives the operation type and parameters from the client.
    - Calls the appropriate handler based on the operation (`handleWrite()`, `handleRetrieve()`, `handleStream()`).
- **Operation Handlers**:
  - `handleWrite`:
    - **Purpose**: Handles write operations from the client.
    - **Usage**:
      - Receives the file content in chunks from the client.
      - Writes the content to the specified file path.
      - Sends acknowledgments to the client and the naming server.
  - `handleRetrieve`:
    - **Purpose**: Handles retrieve operations to get file metadata.
    - **Usage**:
      - Retrieves file information such as permissions, owner, size, and last modified date.
      - Sends the information back to the client.
  - `handleStream`:
    - **Purpose**: Handles streaming of audio files to the client.
    - **Usage**:
      - Streams the audio file content to the client in chunks.
      - Manages the stream to support real-time playback.

## Summary of System Handling

- **Distributed Architecture**: The system is designed to operate over a network, allowing multiple clients to interact with multiple storage servers through a naming server.
- **Concurrency and Synchronization**:
  - Uses multithreading to handle multiple clients and storage servers concurrently.
  - Employs mutexes to manage access to shared resources like the directory tree and cache.
- **Caching Mechanism**: Implements an LRU cache in the naming server to optimize path lookups and reduce latency.
- **Error Handling**:
  - Utilizes standardized error codes and messages across the system.
  - Ensures robust error checking using the `CHECK` macro.
- **Network Communication**:
  - Relies on TCP sockets for reliable data transmission.
  - Uses defined protocols for communication between components.
- **File Management**:
  - Supports hierarchical directory structures.
  - Manages file locks to prevent conflicting operations.
- **Extensibility**:
  - Designed to allow the addition of more storage servers.
  - Can be scaled to support more clients and larger datasets.
- **Security Considerations**:
  - Assumes a trusted network environment.
  - Does not currently implement authentication or encryption.

## Assumptions

1. File/ Folder name cannot have spaces.
2. While initializing a storage server, you have to give all inputs before initializing another stroage server.
3. The NS sends a struct for details of SS for read write metadata and audio streaming which we consider as an ACK for them.
4. sudo apt-get install libmpv-dev must be run to include mpv.
5. You specify paths as provided by NS to the client as accessible paths, thus, if someone wants to create a folder `./folder`, when given the access of only `test`, the client cannot create the folder.
6. When a storage server joins for the first time, it must self report its index as -1. This will indicate the naming server to assign an index to it. Subsequenty, if this storage server reconnects after disconnecting, it must report its index as assigned before by the naming server on joining the first time. It asks for the path names, but would be ignored by the NFS.
7. Max number of clients that can connect to 1 storage server is 10. This can be changed by updating the macro SERVER_LOAD.
8. Max number of storage server that can connect to the naming server is 100. This can be changed by updating the macro MAX_STORAGE_SERVERS.
9. Max number of connections that can be queues to a particular port is 10. This can be changed by updating the macro MAX_CONNECTIONS.
10. In write, the user must enter each time whether they wish to perform a synchronous or an asynchronous write, instead of specifying a flag for the same.
11. Asynchronous write flushes to the persistant memory periodically every 15 seconds. This can be changed by updating the sleep time in the handleWrite function in storage_server/ss_cl_communication.c.
12. Stream audio does not open the mpv player, rather only plays the audio in the terminal.
