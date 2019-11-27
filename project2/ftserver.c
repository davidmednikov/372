/*************************************************************************
** CS372 Intro to Networks
** Winter 2019
**
** Project 2 - File Transfer Server (ftserver)
** David Mednikov
** Last Modified: 11/26/2019 12:30pm
**
** This project is a simple file transfer system with two hosts -
** a server (ftserver) and a client (ftclient). The server listens to
** a port that is passed as a command line argument. The client attempts
** to connect to the server at the host and port specified via command
** line arguments. Before connecting, the client will validate that the
** runtime arguments are valid before sending the request to the server.
** The request contains additional arguments passed to the client at runtime
** which will be further validated by the server (such as filename).
**
** The message will contain a command of '-g' (get) or '-l' (list) that
** will either return a file or the contents of the current directory.
** If the command is valid, the server will open a new connection
** (at a port specified by the client) and send the directory or file
** contents there. If the server gets an invalid command, it sends an error
** message over the same connection the client sent the command on.
** After getting response(s) from the server, the client should stop running,
** but the server will go back to listenting on the port.
**
** This program is the server.
*************************************************************************/

// import all necessary modules
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

// define bool enums
typedef enum { false, true } bool;

// define command enums
typedef enum { err, list, get } cmd;


/*************************************************************************
* function open_listen_port
* Opens a socket for listening using the provided addrinfo struct
* Params:
*   struct addrinfo listen_hints (contains client host info)
*   struct addrinfo *listen_res (to store client socket info)
*   int port (port number to listen on)
* Returns:
*   int pointing to socket_fd
* Pre-conditions: No existing socket to client at listen_res
* Post-conditions: Socket opened but not connected to host at listen_res
*************************************************************************/
int open_listen_port(char* port, struct addrinfo listen_hints, struct addrinfo *listen_res) {
    // create int for server socket file descriptor
    int socket_fd;

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#bind
    // clear listen port address info struct and set fields for getting server info
    memset(&listen_hints, 0, sizeof listen_hints);
    listen_hints.ai_family = AF_UNSPEC;
    listen_hints.ai_socktype = SOCK_STREAM;
    listen_hints.ai_flags = AI_PASSIVE;

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    // get localhost info and store to listen_res
    getaddrinfo(NULL, port, &listen_hints, &listen_res);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#socket
    // open socket based on info stored in listen_res
    socket_fd = socket(listen_res->ai_family, listen_res->ai_socktype, listen_res->ai_protocol);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#bind
    // bind server to socket
    bind(socket_fd, listen_res->ai_addr, listen_res->ai_addrlen);

    // print update to terminal and return socket number
    printf("Server open on %s\n\n", port);
    return socket_fd;
}


/*************************************************************************
* function open_data_port
* Opens a socket for sending data using the provided addrinfo struct
* Params:
*   struct addrinfo data_hints (contains client data socket info)
*   struct addrinfo *data_res (to store client data socket host info)
*   int port (port number to connect to)
* Returns:
*   int pointing to data_fd
* Pre-conditions: No existing socket to client at data_res
* Post-conditions: Socket opened and connected to host at data_res
*************************************************************************/
int open_data_port(char* host, char* port, struct addrinfo data_hints, struct addrinfo *data_res) {
    // create int for data socket file descriptor
    int data_fd;

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#connect
    // clear data port address intfo struct and set fields for getting client info
    memset(&data_hints, 0, sizeof data_hints);
    data_hints.ai_family = AF_UNSPEC;
    data_hints.ai_socktype = SOCK_STREAM;

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    // get client's info and store to data_res
    getaddrinfo(host, port, &data_hints, &data_res);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#connect
    // open socket pointing to client using data stored in data_res sturect
    data_fd = socket(data_res->ai_family, data_res->ai_socktype, data_res->ai_protocol);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#connect
    // connect to data socket and return socket numer
    connect(data_fd, data_res->ai_addr, data_res->ai_addrlen);
    return data_fd;
}


/*************************************************************************
* function accept_client_connection
* Accept a client connection at the provided listen socket
* Params:
*   int socket_fd (int pointing to open but not connected listen socket)
*   struct sockaddr_storage client_address (struct to store info about the client_address)
*   socklen_t address_size (struct to store info about the size of the address socket)
*   char* client_host (string to hold client host)
*   char* client_name (string to hold client name)
*   char* service (string to hold client service (port #))
* Returns:
*   int new_fd pointing to new data connection
* Pre-conditions: Socket with client opened but not connected
* Post-conditions: Connection exists between server and client
*************************************************************************/
int accept_client_connection(int socket_fd, struct sockaddr_storage client_address, socklen_t address_size, char* client_host, char* client_name, char* service) {
    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#listen
    // listen to socket for client connection. only accepts 1 connection at a time
    listen(socket_fd, 1);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#acceptthank-you-for-calling-port-3490.
    // accept client connection and save to new socket number
    int new_fd = accept(socket_fd, (struct sockaddr *) &client_address, &address_size);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#getpeernameman
    // get info about client host and store to client_address
    getpeername(new_fd, (struct sockaddr *) &client_address, &address_size);

    // clear fields for client host, name, and service (port)
    memset(client_host, '\0', 100);
    memset(client_name, '\0', 10);
    memset(service, '\0', 10);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#getnameinfoman
    // get client hostname and port, store to client_host and service
    getnameinfo((struct sockaddr *) &client_address, address_size, client_host,
                    sizeof (char[100]), service, sizeof (char[10]), 0);

    // copy first 6 chars of hostname to get host server
    strncpy(client_name, client_host, 6);

    // add null terminator to end string
    client_name[5] = '\0';

    // print update to terminal and return data socket to caller
    printf("Connection from %s.\n", client_name);
    return new_fd;
}


/*************************************************************************
* function get_command
* Get the command from the client
* Params:
*   int listen_fd (int pointing to connected listen socket)
*   char* buffer (buffer string for sending)
*   char* command (string to hold command from client)
*   char* filename (string to hold requested filename from client)
*   char* filename (string to hold data port provided by client)
* Returns:
*   cmd enum containing type of command ('list', 'get', or 'err')
* Pre-conditions: Connected socket waiting for a message
* Post-conditions: Server receives message, stores command to local strings, and returns enum
*************************************************************************/
cmd get_command(int listen_fd, char* buffer, char* command, char* filename, char* data_port) {
    // define variables used in function
    int bytes_sent, spaces, string_length;

    // clear strings for holding command info from client
    memset(buffer, '\0', 1000);
    memset(command, '\0', 10);
    memset(filename, '\0', 100);
    memset(data_port, '\0', 10);

    // Code excerpted from Beej's Guide: http://beej.us/guide/bgnet/html/#sendrecv
    // receive data from the socket
    bytes_sent = recv(listen_fd, buffer, 1000, 0);

    // if data received
    if (bytes_sent != 0) {
        // get string length and check that there are at least 2 characters (minimum command length)
        string_length = strlen(buffer);
        if (string_length >= 2) {
            // if at least 2 chars, count spaces so that we know what the client is sending
            spaces = 0;
            for (int i = 0; i < string_length; i++) {
                if (buffer[i] == ' ') {
                    spaces++;
                }
            }

            // copy command from buffer to var and set end of string
            strncpy(command, buffer, 3);
            command[2] = '\0';

            // if just one space and command is "-l", client requesting directory
            if (spaces == 1 && strcmp(command, "-l") == 0) {
                // get first token (command)
                char* token = strtok(buffer, " ");

                // if not null, get next token and copy to data_port
                if (token != NULL) {
                    token = strtok(NULL, " ");
                    strcpy(data_port, token);
                } else {
                    // not enough tokens, invalid input - return error enum
                    return err;
                }

                // return list enum
                return list;
            } else if (spaces == 2 && strcmp(command, "-g") == 0) {
                // if 2 spaces and command is "-g", client requesting file

                // get first token (command)
                char* token = strtok(buffer, " ");

                // if not null, get next token and copy to filename
                if (token != NULL) {
                    token = strtok(NULL, " ");
                    strcpy(filename, token);
                    // if not null, get next token and copy to data_port
                    if (token != NULL) {
                        token = strtok(NULL, " ");
                        strcpy(data_port, token);
                    } else {
                        // not enough tokens, invalid input - return error enum
                        return err;
                    }
                } else {
                    // not enough tokens, invalid input - return error enum
                    return err;
                }

                // return get enum
                return get;
            }
        }
    }
    // return error enum
    return err;
}


/*************************************************************************
* function get_directory
* Gets the contents of the local directory and stores it to dir_string
* Params:
*   char* dir_string (string where contents of directory should be saved)
* Returns:
*   int files (# of files in directory)
* Pre-conditions: Client requested directory list from server, dir_string is empty
* Post-conditions: dir_string contains directory contents
*************************************************************************/
int get_directory(char* dir_string) {
    // Code excerpted from https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program/17683417

    // var to count number of files
    int files = 0;

    // clear out directory string
    memset(dir_string, '\0', 1000);

    // initialize directory structs and objects
    DIR* directory;
    struct dirent* file;

    // open current directory
    directory = opendir("./");

    // if directory was opened
    if (directory) {
        // loop through files in folder until there are no more
        while((file = readdir(directory)) != NULL) {
            // only count files that are not shortscuts to current directory or parent directory
            if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..")) {
                // if no files so far, copy current name to directory string
                if (files == 0) {
                    strcpy(dir_string, file->d_name);
                } else {
                    // if 1 or more files listed, concat filename to directory string
                    strcat(dir_string, "\n");
                    strcat(dir_string, file->d_name);
                }
                // increment number of files
                files++;
            }
        }
    }
    // return number of files
    return files;
}


/*************************************************************************
* function send_data
* Appends the text in message to the command, then copies that into the
* buffer and sends the buffer over the provided socket
* Params:
*   char* message (string containing message to send to client)
*   char* buffer (string where text to be sent to client will be stored)
*   cmd cmd (enum holding type of command sent by client)
*   int socket (int pointing to connected data socket)
* Returns:
*   int containing # of bytes sent
* Pre-conditions: Data ready to be sent to client, buffer is empty
* Post-conditions: Buffer filled and data sent to client
*************************************************************************/
int send_data(char* message, char* buffer, cmd cmd, int socket) {
    // set buffer to username + message
    sprintf(buffer, "%s\n%s", cmd == list ? "list" : "get", message);

    // send buffer to socket and return bytes written
    return send(socket, buffer, strlen(buffer), 0);
}


/*************************************************************************
* function send_list
* Gets the local directory contents into a string and sends that to the client
* Params:
*   char* buffer (string where text to be sent to client will be stored)
*   int data_fd (int pointing to connected data socket)
* Returns:
*   int containing # of bytes sent
* Pre-conditions: Client requested directory list from server, need to get directory
* Post-conditions: Directory sent to client
*************************************************************************/
int send_list(char* buffer, int data_fd) {
    // create string to hold directory and pass to get_directory function
    char directory[1000];
    get_directory(&directory[0]);

    // clear out buffer
    memset(buffer, '\0', 1000);

    // send directory to client and store bytes sent to return_value
    int return_value = send_data(&directory[0], &buffer[0], list, data_fd);

    // clear buffer after sending and return # bytes sent
    memset(buffer, '\0', 1000);
    return return_value;
}


/*************************************************************************
* function send_file
* Sends the requested file to the client
* Params:
*   char* file (string holding contents of file to be sent)
*   char* buffer (string where text to be sent to client will be stored)
*   int data_fd (int pointing to connected data socket)
*   size_t file_size (size of file so that buffer is correctly cleared)
* Returns:
*   int containing # of bytes sent
* Pre-conditions: Client requested valid file from server that is stored in char* file
* Post-conditions: File sent to client
*************************************************************************/
int send_file(char* file, char* buffer, int data_fd, size_t file_size) {
    // clear out buffer
    memset(buffer, '\0', file_size + 11);

    // pass file and buffer to send_data function for sending
    int return_value = send_data(&file[0], &buffer[0], get, data_fd);

    // clear out buffer and return # bytes sent
    memset(buffer, '\0', file_size + 11);
    return return_value;
}


/*************************************************************************
* function open_file
* Opens the requested file and saves its contents to the save_string parameter
* Params:
*   FILE* file (FILE pointer pointing to the file descriptor)
*   char* filename (string holding the requested file's name)
*   char* save_string (string where the contents of the file will be stored)
*   size_t file_size (size of file so that save_string is correctly cleared)
* Returns:
*   bool if file was opened (true) or not (false)
* Pre-conditions: Client requested file from server
* Post-conditions: Server either opens file and stores to string or returns false
*************************************************************************/
bool open_file(FILE* file, char* filename, char* save_string, size_t file_size) {
    // clear out string to hold file text data
    memset(save_string, '\0', file_size + 11);

    // open file for reading
    file = fopen(filename, "r");

    // if file exists and was opened successfully
    if (file != NULL) {
        // create 10000 char temp string to read through file
        char temp[10000];

        // get 9999 characters (plus null terminator appended by fgets)
        fgets(temp, 9999, file);

        // copy chars from temp variable to save_string
        strncpy(save_string, temp, strlen(temp));

        // clear out temp string to all null terminators
        memset(temp, '\0', 10000);

        // while text is still being read from the file
        while(fgets(temp, 9999, file)) {
            // remove null terminator from temp
            strncpy(temp, temp, strlen(temp));

            // concat temp string to save_string
            strcat(save_string, temp);

            // clear out temp string
            memset(temp, '\0', 10000);
        }

        // all reading done, file text stored to save_string and return true
        return true;
    }

    // open failed, return false
    return false;
}


/*************************************************************************
* function send_error
* Sends an error to the client at the provided socket and also prints an
* error message to the terminal window
* Params:
*   int client_fd (int pointing to client socket)
*   char* print_message (string holding message to print to terminal)
*   char* send_message (string holding message to send to client)
* Returns:
*   int (# of bytes sent)
* Pre-conditions: Server encountered error while parsing command (bad input or missing file)
* Post-conditions: Error printed to terminal and sent to client
*************************************************************************/
int send_error(int client_fd, char* print_message, char* send_message) {
    // print error message to terminal
    printf("%s\n", print_message);

    // send error message to client via listening socket
    return send(client_fd, send_message, strlen(send_message), 0);
}


/*************************************************************************
* main method
* Listens on a socket connection for an incoming connection from the client.
* Once connected to a client, waits for a command. If the command is invalid,
* send back an error on the already-opened connection. If the command is
* valid, open up a new data connection and send the requested resource (list
* or file) to the client at the specified data port. After receiving data, the
* client will close the connection and the server will go back to listening
* for new connections.
*************************************************************************/
int main(int argc, char* argv[]) {
    // If # of args is not 2 (./ftserver <SERVER_PORT>) then print an error and quit
    if (argc != 2) {
        printf("Invalid input. Server must be started using following command:\n./ftserver <SERVER_PORT>\n");
        return -1;
    }

    // keep accepting client connections until SIGINT
    bool keep_open = true;

    // enum to store command received from server
    cmd cmd;

    // static size strings for use by server
    char port[10], print_message[500], text_buffer[1000];

    // unknown size strings for use by server
    char *file_buffer, *read_file;

    // static size strings for info about client and command
    char client_host[100], client_name[10], command[10], filename[100], data_port[10], service[10];

    // file to be sent if client passes "-g"
    FILE* requested_file = NULL;

    // ints to store socket #s and listen port number
    int data_fd, new_fd, port_number, socket_fd;

    // struct to hold size of client address
    socklen_t address_size;

    // structs for networking to hold client info
    struct addrinfo data_hints, listen_hints, *data_res = NULL, *listen_res = NULL;
    struct sockaddr_storage client_address;

    // struct to store info about file size
    struct stat stat_struct;

    // clear port string and retrieve from arguments
    memset(port, '\0', 10);
    strcpy(port, argv[1]);

    // convert port number to int and check that it is in valid range
    port_number = atoi(port);
    if (port_number <= 1024 || port_number > 65535) {
        // port is invalid, print error and quit to terminal
        printf("%d is not a valid port number. Must be between 1025 and 65535.\n", port_number);
        return -1;
    }

    // port is valid, open and store socket # to socket_fd
    socket_fd = open_listen_port(port, listen_hints, listen_res);

    // get address size for accepting client connection
    address_size = sizeof client_address;

    // keep looping until SIGINT
    while(keep_open) {
        // listen for and accept client connection, store socket # to new_fd
        new_fd = accept_client_connection(socket_fd, client_address, address_size, &client_host[0], &client_name[0], &service[0]);

        // receive command from server
        cmd = get_command(new_fd, text_buffer, &command[0], &filename[0], &data_port[0]);

        // if command is 'list' or 'get', go to the next step
        if (cmd == list || cmd == get) {
            // if command is 'list'
            if (cmd == list) {
                // send OK message to client on control socket
                send(new_fd, "OK", 3, 0);

                // print message about request to terminal
                printf("List directory requested on port %s\n", data_port);

                // open data port at port number requested by client
                data_fd = open_data_port(&client_host[0], &data_port[0], data_hints, data_res);

                // print to terminal that directory contents being sent to client
                printf("Sending directory contents to %s:%s\n\n", client_name, data_port);

                // send directory to data connection and close data socket
                send_list(text_buffer, data_fd);
                close(data_fd);
            } else {
                // else if command is 'get'

                // print message about request
                printf("File \"%s\" requested on port %s\n", filename, data_port);

                // get stats about file, then get file size
                // adapted from https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
                stat(filename, &stat_struct);
                size_t file_size = stat_struct.st_size;

                // allocate memory for read_file string and file_buffer to be big enough for entire file
                read_file = (char*) calloc(file_size + 1, sizeof(char));
                file_buffer = (char*) calloc(file_size + 11, sizeof(char));

                // if file opened successfully, send to client
                if(open_file(requested_file, filename, &read_file[0], file_size)) {
                    // send OK message to client on control socket
                    send(new_fd, "OK", 3, 0);

                    // open data port at port number requested by client
                    data_fd = open_data_port(&client_host[0], &data_port[0], data_hints, data_res);

                    // print to terminal that file being sent to client
                    printf("Sending \"%s\" to %s:%s\n\n", filename, client_name, data_port);

                    // send file to data connection and close data socket
                    send_file(&read_file[0], file_buffer, data_fd, file_size);
                    close(data_fd);
                } else {
                    // error opening file, send error message to client

                    // clear print_message string and format with error message
                    memset(print_message, '\0', 500);
                    sprintf(print_message, "File \"%s\" could not be found.\nSending error message to %s:%s\n", filename, client_name, port);

                    // print message to terminal and send "FILE NOT FOUND" to client
                    send_error(new_fd, print_message, "FILE NOT FOUND");
                }
            }
        } else {
            // invalid command (not 'list' or 'get'), send error message to client

            // clear print_message string and format with error message
            memset(print_message, '\0', 500);
            sprintf(print_message, "Invalid Command.\n%s is not valid input.\nSending error message to %s:%s\n\n", text_buffer, client_name, port);

            // print message to terminal and send "INVALID COMMAND" to client
            send_error(new_fd, print_message, "INVALID COMMAND");
        }

        // close client connection on server side
        close(new_fd);
    }

    // close connection socket and listen to socket for new connection
    close(socket_fd);

    // return 0 at exit (should never happen)
    return 0;
}