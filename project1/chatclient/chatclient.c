/*************************************************************************
** CS372 Winter 2019 - Project 1 - Server
** David Mednikov
** Last Modified: 10/29/2019 1:55pm
**
** This project is a simple chat system with two users - a server
** and a client. The server listens to a port that is passed as a
** command line argument. The client attempts to connect to a server
** at the host and port specified via command line arguments. After
** asking the user for their username, the client sends a message to
** the server. The two hosts message back and forth until one of them
** kills the connection. After killing the connection, the client should
** stop running, but the server should go back to listenting on the port.
**
** This program is the server.
*************************************************************************/

// import modules
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


int openSocket(struct addrinfo *serverInfo, int port) {
    // open socket and print error if failed
    int result = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0);
    if (result < 0) {
        // error encountered while opening socket
        fprintf(stderr, "chatclient: ERROR opening socket with port %d\n", port);
        exit(1);
    }
    return result;
}

void connectToSocket(int socketStream, struct addrinfo *serverInfo, int port) {
    // connect to socket and print error if failed
    int result = connect(socketStream, serverInfo->ai_addr, serverInfo->ai_addrlen);
    if (result) {
        // error encountered while connecting to socket
        fprintf(stderr, "chatclient: ERROR connecting with port %d\n", port);
        exit(1);
    }
}

char* getName() {
    char* username = calloc(20, sizeof(char));
    while (1) {
        printf("Enter your username: ");
        memset(username, '\0', 20);
        fgets(username, 20, stdin);
        if (strlen(username) <= 11) {
            username[strlen(username) - 1] = '\0';
            int hasSpace = 0;
            for (int i = 0; i < strlen(username); i++) {
                if (isspace(username[i])) {
                    hasSpace = 1;
                    break;
                }
            }
            if (!hasSpace) {
                return username;
            } else {
                printf("Username may not contain any spaces.\n");
            }
        } else {
            printf("Username must be 10 characters or less.\n");
        }
    }
    return NULL;
}

/*
** main
** ----------------------------------------------------
** main function that creates the socket and attempts
** to connect to the server.
** ----------------------------------------------------
*/
int main (int argc, char* argv[]) {
    // if arg count is wrong, display message showing correct usage to user
    if (argc != 3) {
        printf("USAGE: chatserve <host> <port>\n");
        exit(1);
    }

    // initialize variables and structs
    int charsRead;
    int charsWritten;
    int port;
    int socketStream;
    int status;
    char host[100];
    char portString[10];

    struct addrinfo *serverInfo;
    struct addrinfo hints;

    /*
       Used Beej's Guide for this
       https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    */

    // clear out hints struct and set protocol family and socket type
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // get runtime arguments and store to variables
    strcpy(host, argv[1]);
    strcpy(portString, argv[2]);
    port = atoi(portString);

    // get info about server at specified hostname port combo
    status = getaddrinfo(host, portString, &hints, &serverInfo);

    socketStream = openSocket(serverInfo, port);
    connectToSocket(socketStream, serverInfo, port);

    char username[20];
    strcpy(username, getName());

    char input[512];
    char buffer[1000];
    int stayConnected = 1;

    while (stayConnected) {
        printf("%s> ", username);
        memset(input, '\0', 512);
        __fpurge(stdin);
        fgets(input, 512, stdin);
        input[strlen(input) - 1] = '\0';

        if (strcmp(input, "\\quit") != 0) {
            /*
                Used my code from CS344 for sending/receiving
            */

            // clear out buffer array and set to input
            memset(buffer, '\0', 1000);
            sprintf(buffer, "%s> %s", username, input);

            // send buffer to socket
            charsWritten = send(socketStream, buffer, strlen(buffer), 0);

            if (charsWritten < 0) {
                // error writing to socket
                printf("chatclient: ERROR sending to socket\n");
            }

            // clear buffer after sending
            memset(buffer, '\0', 1000);

            // receive data from socket
            charsRead = recv(socketStream, buffer, 512, 0);
            if (charsRead < 0) {
                // error receiving from socket
                printf("chatclient: ERROR receiving from socket\n");
            }
            if (strcmp(buffer, "\\quit") != 0) {
                printf("%s\n", buffer);
            } else {
                // close socket
                printf("chatclient: server has closed the connection\n");
                close (socketStream);
                stayConnected = 0;
            }
        } else {
            // close socket
            close (socketStream);
            stayConnected = 0;
        }
    }

    // return 0 and exit
    return 0;
}
