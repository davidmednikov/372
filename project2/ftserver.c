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

int open_listen_port(char* port, struct addrinfo listen_hints, struct addrinfo *listen_res) {
    // create int for server socket file descriptor
    int socket_fd;

    // clear listen port address info struct and set fields for getting server info
    memset(&listen_hints, 0, sizeof listen_hints);
    listen_hints.ai_family = AF_UNSPEC;
    listen_hints.ai_socktype = SOCK_STREAM;
    listen_hints.ai_flags = AI_PASSIVE;

    // get localhost info and store to listen_res
    getaddrinfo(NULL, port, &listen_hints, &listen_res);

    // open socket based on info stored in listen_res
    socket_fd = socket(listen_res->ai_family, listen_res->ai_socktype, listen_res->ai_protocol);

    // bind server to socket
    bind(socket_fd, listen_res->ai_addr, listen_res->ai_addrlen);

    // print update to terminal and return socket number
    printf("Server open on %s\n\n", port);
    return socket_fd;
}

int open_data_port(char* host, char* port, struct addrinfo data_hints, struct addrinfo *data_res) {
    // create int for data socket file descriptor
    int socket_fd;

    // clear data port address intfo struct and set fields for getting client info
    memset(&data_hints, 0, sizeof data_hints);
    data_hints.ai_family = AF_UNSPEC;
    data_hints.ai_socktype = SOCK_STREAM;

    // get client's info and store to data_res
    getaddrinfo(host, port, &data_hints, &data_res);

    // open socket pointing to client using data stored in data_res sturect
    socket_fd = socket(data_res->ai_family, data_res->ai_socktype, data_res->ai_protocol);

    // connect to data socket and return socket numer
    connect(socket_fd, data_res->ai_addr, data_res->ai_addrlen);
    return socket_fd;
}

int accept_client_connection(int socket_fd, struct sockaddr_storage client_address, socklen_t address_size, char* client_host, char* client_name, char* service) {
    // listen to socket for client connection. only accepts 1 connection at a time
    listen(socket_fd, 1);

    // accept client connection and save to new socket number
    int new_fd = accept(socket_fd, (struct sockaddr *) &client_address, &address_size);

    // get info about client host and store to client_address
    getpeername(new_fd, (struct sockaddr *) &client_address, &address_size);

    // clear fields for client host, name, and service (port)
    memset(client_host, '\0', 100);
    memset(client_name, '\0', 10);
    memset(service, '\0', 10);

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

cmd get_command(int new_fd, char* buffer, char* command, char* filename, char* data_port) {
    // define variables used in function
    int bytes_sent, spaces, string_length;

    // clear strings for holding command info from client
    memset(buffer, '\0', 1000);
    memset(command, '\0', 10);
    memset(filename, '\0', 100);
    memset(data_port, '\0', 10);

    // receive data from the socket
    bytes_sent = recv(new_fd, buffer, 1000, 0);

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
                }

                // return list enum
                return list;
            } else if (spaces == 2 && strcmp(command, "-g") == 0) {
                // if 2 spaces if command is "-g", client requesting file

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

int get_directory(char* dir_string) {
    //https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program/17683417
    int files = 0;
    memset(dir_string, '\0', 1000);
    DIR* directory;
    struct dirent* file;
    directory = opendir("./");
    if (directory) {
        while((file = readdir(directory)) != NULL) {
            if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..")) {
                if (files == 0) {
                    strcpy(dir_string, file->d_name);
                } else {
                    strcat(dir_string, "\n");
                    strcat(dir_string, file->d_name);
                }
                files++;
            }
        }
    }
    return files;
}

int send_data(char* message, char* buffer, cmd cmd, int socket) {
    // set buffer to username + message
    sprintf(buffer, "%s\n%s", cmd == list ? "list" : "get", message);

    // send buffer to socket
    int charsWritten = send(socket, buffer, strlen(buffer), 0);

    // clear buffer after sending
    memset(buffer, '\0', strlen(buffer));
    return charsWritten;
}


int send_list(char* buffer, int data_fd) {
    char directory[1000];
    get_directory(&directory[0]);
    // clear out buffer
    memset(buffer, '\0', 1000);
    int return_value = send_data(&directory[0], buffer, list, data_fd);
    // clear buffer after sending
    memset(buffer, '\0', 1000);
    return return_value;
}

int send_file(char* file, char* buffer, int data_fd, size_t file_size) {
    // clear out buffer after sending
    memset(buffer, '\0', file_size + 11);
    int return_value = send_data(file, buffer, get, data_fd);
    // clear out buffer
    memset(buffer, '\0', file_size + 11);
    return return_value;
}

bool open_file(FILE* file, char* filename, char* save_string, size_t file_size) {
    memset(save_string, '\0', file_size + 11);
    file = fopen(filename, "r");
    if (file != NULL) {
        char temp[10000];
        fgets(temp, 9999, file);
        strncpy(save_string, temp, strlen(temp));
        memset(temp, '\0', 10000);
        while(fgets(temp, 9999, file)) {
            strncpy(temp, temp, strlen(temp));
            strcat(save_string, temp);
            memset(temp, '\0', 10000);
        }
        return true;
    }
    return false;
}

int send_error(int client_fd, char* print_message, char* send_message) {
    printf("%s\n", print_message);
    return send(client_fd, send_message, strlen(send_message), 0);
}

int main(int argc, char* argv[]) {
    bool keep_open = true;
    cmd cmd;
    char port[10], print_message[500], text_buffer[1000];
    char *file_buffer, *read_file;
    char client_host[100], client_name[10], command[10], filename[100], data_port[10], service[10];

    FILE* requested_file = NULL;

    int data_fd, new_fd, port_number, socket_fd;
    socklen_t address_size;
    struct addrinfo data_hints, listen_hints, *data_res = NULL, *listen_res = NULL;
    struct sockaddr_storage client_address;
    struct stat stat_struct;

    memset(port, '\0', 10);
    strcpy(port, argv[1]);

    port_number = atoi(port);
    if (port_number <= 1024 || port_number > 65535) {
        printf("%d is not a valid port number. Must be between 1025 and 65535.\n", port_number);
    } else {
        socket_fd = open_listen_port(port, listen_hints, listen_res);
        address_size = sizeof client_address;

        while(keep_open) {
            new_fd = accept_client_connection(socket_fd, client_address, address_size, &client_host[0], &client_name[0], &service[0]);

            cmd = get_command(new_fd, text_buffer, &command[0], &filename[0], &data_port[0]);

            if (cmd == list || cmd == get) {
                if (cmd == list) {
                    send(new_fd, "OK", 3, 0);
                    printf("List directory requested on port %s\n", data_port);
                    printf("Sending directory contents to %s:%s\n\n", client_name, data_port);
                    data_fd = open_data_port(&client_host[0], &data_port[0], data_hints, data_res);
                    send_list(text_buffer, data_fd);
                    close(data_fd);
                } else {
                    printf("File \"%s\" requested on port %s\n", filename, data_port);
                    stat(filename, &stat_struct);
                    size_t file_size = stat_struct.st_size;
                    read_file = (char*) calloc(file_size + 1, sizeof(char));
                    file_buffer = (char*) calloc(file_size + 11, sizeof(char));
                    if(open_file(requested_file, filename, &read_file[0], file_size)) {
                        send(new_fd, "OK", 3, 0);
                        data_fd = open_data_port(&client_host[0], &data_port[0], data_hints, data_res);
                        printf("Sending \"%s\" to %s:%s\n\n", filename, client_name, data_port);
                        send_file(&read_file[0], file_buffer, data_fd, file_size);
                        close(data_fd);
                    } else {
                        memset(print_message, '\0', 500);
                        sprintf(print_message, "File \"%s\" could not be found.\nSending error message to %s:%s\n", filename, client_name, port);
                        send_error(new_fd, print_message, "FILE NOT FOUND");
                    }
                }
            } else {
                memset(print_message, '\0', 500);
                sprintf(print_message, "Invalid Command.\n%s is not valid input.\nSending error message to %s:%s\n\n", text_buffer, client_name, port);
                send_error(new_fd, print_message, "INVALID COMMAND");
            }
        }

        close(new_fd);
        close(socket_fd);
    }
}