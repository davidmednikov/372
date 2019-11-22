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
#include <unistd.h>

typedef enum { false, true } bool;

typedef enum { err, list, get } cmd;

int open_listen_port(char* port, struct addrinfo listen_hints, struct addrinfo *listen_res) {
    int socket_fd;
    memset(&listen_hints, 0, sizeof listen_hints);
    listen_hints.ai_family = AF_UNSPEC;
    listen_hints.ai_socktype = SOCK_STREAM;
    listen_hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, port, &listen_hints, &listen_res);

    socket_fd = socket(listen_res->ai_family, listen_res->ai_socktype, listen_res->ai_protocol);
    bind(socket_fd, listen_res->ai_addr, listen_res->ai_addrlen);

    printf("Server open on %s\n\n", port);
    return socket_fd;
}

int open_data_port(char* host, char* port, struct addrinfo data_hints, struct addrinfo *data_res) {
    int socket_fd;
    memset(&data_hints, 0, sizeof data_hints);
    data_hints.ai_family = AF_UNSPEC;
    data_hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(host, port, &data_hints, &data_res);

    socket_fd = socket(data_res->ai_family, data_res->ai_socktype, data_res->ai_protocol);
    connect(socket_fd, data_res->ai_addr, data_res->ai_addrlen);
    return socket_fd;
}

int accept_client_connection(int socket_fd, struct sockaddr_storage client_address, socklen_t address_size, char* client_host, char* client_name, char* service) {
    listen(socket_fd, 1);

    int new_fd = accept(socket_fd, (struct sockaddr *) &client_address, &address_size);

    getpeername(new_fd, (struct sockaddr *) &client_address, &address_size);

    memset(client_host, '\0', 100);
    memset(client_name, '\0', 10);
    memset(service, '\0', 10);
    getnameinfo((struct sockaddr *) &client_address, address_size, client_host,
                    sizeof (char[100]), service, sizeof (char[10]), 0);
    strncpy(client_name, client_host, 6);
    client_name[5] = '\0';
    printf("Connection from %s:%s.\n", client_name, service);
    return new_fd;
}

cmd get_command(int new_fd, char* buffer, char* command, char* filename, char* data_port) {
    int bytes_sent, spaces, string_length;

    memset(command, '\0', 10);
    memset(filename, '\0', 100);
    memset(data_port, '\0', 10);

    memset(buffer, '\0', 10000);
    bytes_sent = recv(new_fd, buffer, 1000, 0);
    if (bytes_sent != 0) {
        string_length = strlen(buffer);
        if (string_length >= 2) {
            spaces = 0;
            for (int i = 0; i < string_length; i++) {
                if (buffer[i] == ' ') {
                    spaces++;
                }
            }
            strncpy(command, buffer, 3);
            command[2] = '\0';
            if (spaces == 1 && strcmp(command, "-l") == 0) {
                char* token = strtok(buffer, " ");
                if (token != NULL) {
                    token = strtok(NULL, " ");
                    strcpy(data_port, token);
                }
                return list;
            } else if (spaces == 2 && strcmp(command, "-g") == 0) {
                char* token = strtok(buffer, " ");
                if (token != NULL) {
                    token = strtok(NULL, " ");
                    strcpy(filename, token);
                    if (token != NULL) {
                        token = strtok(NULL, " ");
                        strcpy(data_port, token);
                    }
                }
                return get;
            }
        }
    }
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
    // clear out buffer array and set to username + message
    memset(buffer, '\0', 10000);
    sprintf(buffer, "%s\n%s", cmd == list ? "list" : "get", message);

    // send buffer to socket
    int charsWritten = send(socket, buffer, strlen(buffer), 0);
    if (charsWritten < 0) {
        // error writing to socket
        printf("chatclient: ERROR sending to socket\n");
    }

    // clear buffer after sending
    memset(buffer, '\0', 10000);
    return charsWritten;
}


int send_list(char* buffer, int data_fd) {
    char directory[1000];
    get_directory(&directory[0]);
    return send_data(&directory[0], buffer, list, data_fd);
}

int send_file(char* file, char* buffer, int data_fd) {
    return send_data(file, buffer, get, data_fd);
}

bool open_file(FILE* file, char* filename, char* save_string) {
    memset(save_string, '\0', 10000);
    file = fopen(filename, "r");
    if (file != NULL) {
        fgets(save_string, 1000, file);
        while(fgets(save_string, 1000, file)) {
            char temp[1000];
            memset(temp, '\0', 1000);
            strcat(save_string, temp);
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
    char buffer[10000], read_file[10000], port[10], print_message[500];
    char client_host[100], client_name[10], command[10], filename[100], data_port[10], service[10];

    FILE* requested_file = NULL;

    int data_fd, new_fd, socket_fd;
    socklen_t address_size;
    struct addrinfo data_hints, listen_hints, *data_res = NULL, *listen_res = NULL;
    struct sockaddr_storage client_address;

    memset(port, '\0', 10);
    strcpy(port, argv[1]);

    socket_fd = open_listen_port(port, listen_hints, listen_res);
    address_size = sizeof client_address;

    while(keep_open) {
        new_fd = accept_client_connection(socket_fd, client_address, address_size, &client_host[0], &client_name[0], &service[0]);

        cmd = get_command(new_fd, buffer, &command[0], &filename[0], &data_port[0]);

        if (cmd == list || cmd == get) {
            if (cmd == list) {
                send(new_fd, "OK", 3, 0);
                printf("List directory requested on port %s\n", data_port);
                printf("Sending directory contents to %s:%s\n\n", client_name, data_port);
                data_fd = open_data_port(&client_host[0], &data_port[0], data_hints, data_res);
                send_list(buffer, data_fd);
                close(data_fd);
            } else {
                printf("File \"%s\" requested on port %s\n", filename, data_port);
                memset(read_file, '\0', 10000);
                if(open_file(requested_file, filename, &read_file[0])) {
                    send(new_fd, "OK", 3, 0);
                    data_fd = open_data_port(&client_host[0], &data_port[0], data_hints, data_res);
                    printf("Sending \"%s\" to %s:%s\n\n", filename, client_name, data_port);
                    send_file(read_file, buffer, data_fd);
                    close(data_fd);
                } else {
                    memset(print_message, '\0', 500);
                    sprintf(print_message, "File \"%s\" could not be found.\nSending error message to %s:%s\n", filename, client_name, service);
                    send_error(new_fd, print_message, "FILE NOT FOUND");
                }
            }
        } else {
            memset(print_message, '\0', 500);
            sprintf(print_message, "Invalid Command.\n%s is not valid input.\nSending error message to %s:%s\n\n", buffer, client_name, service);
            send_error(new_fd, print_message, "INVALID COMMAND");
        }
    }

    close(new_fd);
    close(socket_fd);
}