#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


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

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    strcpy(host, argv[1]);
    strcpy(portString, argv[2]);
    port = atoi(argv[2]);

    status = getaddrinfo(host, portString, &hints, &serverInfo);

    // open socket and print error if failed
    socketStream = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0);
    if (socketStream < 0) {
        // error encountered while opening socket
        fprintf(stderr, "chatclient: ERROR opening socket with port %d\n", port);
        exit(1);
    }

    // connect to socket and print error if failed
    if (connect(socketStream, serverInfo->ai_addr, serverInfo->ai_addrlen)) {
        // error encountered while connecting to socket
        fprintf(stderr, "chatclient: ERROR connecting with port %d\n", port);
        exit(1);
    }

    char username[20];

    int getName = 1;
    while (getName) {
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
                getName = 0;
            } else {
                printf("Username may not contain any spaces.\n");
            }
        } else {
            printf("Username must be 10 characters or less.\n");
        }
    }

    char buffer[1000];
    int stayConnected = 1;

    while (stayConnected) {
        printf("%s> ", username);
        char input[500];
        fgets(input, 500, stdin);
        input[strlen(input) - 1] = '\0';

        if (strcmp(input, "\\quit") != 0) {
            /*
                Used my code from CS344 for sending/receiving
            */

            // clear out buffer array and set to input
            memset(buffer, '\0', 500);
            sprintf(buffer, "%s> %s", username, input);

            // send buffer to socket
            charsWritten = send(socketStream, buffer, strlen(buffer), 0);

            if (charsWritten < 0) {
                // error writing to socket
                printf("chatclient: ERROR sending to socket\n");
            }

            // clear buffer after sending
            memset(buffer, '\0', 500);

            // receive data from socket
            charsRead = recv(socketStream, buffer, 500, 0);
            if (charsRead < 0) {
                // error receiving from socket
                printf("chatclient: ERROR receiving from socket\n");
            }
            printf("%s\n", buffer);
        } else {
            // close socket
            close (socketStream);
            stayConnected = 0;
        }
    }

    // return 0 and exit
    return 0;
}
