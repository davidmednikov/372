/*************************************************************************
** CS372 Intro to Networks
** Winter 2019
**
** Project 1 - Client (chatclient)
** David Mednikov
** Last Modified: 10/30/2019 3:30pm
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
** This program is the client.
*************************************************************************/

// import modules
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


/*************************************************************************
* function openSocket
* Opens a socket using the provided addrinfo struct
* Params:
*   struct addrinfo *serverInfo (contains server info)
*   int port (port number for printing)
* Returns:
*   int pointing to socketFD
* Pre-conditions: No existing socket to host at serverInfo
* Post-conditions: Socket opened but not connected to host at serverInfo
*************************************************************************/
int openSocket(struct addrinfo *serverInfo, int port) {
    /*
        Used my code from CS344 for opening the socket
        Also used Beej's guide: https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    */

    // open socket and store socket FD to int result
    int result = socket(serverInfo->ai_family, serverInfo->ai_socktype, 0);
    if (result < 0) {
        // error encountered while opening socket, print error and exit
        fprintf(stderr, "chatclient: ERROR opening socket with port %d\n", port);
        exit(1);
    }

    // no error, return socket FD to caller
    return result;
}


/*************************************************************************
* function connectToSocket
* Creates a connection at the provided socketFD using provided serverInfo
* Params:
*   int socketStream (reference to socketFD)
*   struct addrinfo *serverInfo (contains server info)
*   int port (port number for printing)
* Pre-conditions: Socket to host at serverInfo exists but not connected
* Post-conditions: Client connected to server at serverInfo
*************************************************************************/
void connectToSocket(int socketStream, struct addrinfo *serverInfo, int port) {
    /*
        Used my code from CS344 for connecting to the socket
        Also used Beej's guide: https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    */

    // connect to socket and print error if failed
    int result = connect(socketStream, serverInfo->ai_addr, serverInfo->ai_addrlen);
    if (result) {
        // error encountered while connecting to socket
        fprintf(stderr, "chatclient: ERROR connecting with port %d\n", port);
        exit(1);
    }
}


/*************************************************************************
* function getName
* Gets a valid username from the user. To be a valid username, the user's
* input must be 10 characters or less and not contain any spaces.
* Returns:
*   char* pointing to valid username
* Pre-conditions: Client does not have the user's name
* Post-conditions: Client has a valid username: <=10 chars and no spaces
*************************************************************************/
char* getName() {
    // allocate memory for username
    char* username = calloc(20, sizeof(char));

    // loop infinitely, until user enters valid input
    while (1) {
        // query user for username
        printf("Enter your username: ");

        // clear username string and get input from user
        memset(username, '\0', 20);
        fgets(username, 20, stdin);

        // if input is 11 chars or less (including newline), check for spaces
        if (strlen(username) == 1) {
            printf("Username may not be empty.\n");
        } else if (strlen(username) <= 11) {
            // strip newline character from username
            username[strlen(username) - 1] = '\0';

            // flag to denote if username has a space
            int hasSpace = 0;

            // loop through username, checking if each char is a space
            for (int i = 0; i < strlen(username); i++) {
                if (isspace(username[i])) {
                    // if char is a space, set flag to true and break out of loop
                    hasSpace = 1;
                    break;
                }
            }

            // if end of loop reached and hasSpace is still false, return username
            if (!hasSpace) {
                return username;
            } else {
                // found space in user input, try again
                printf("Username may not contain any spaces.\n");
            }
        } else {
            // input is too long, try again
            printf("Username must be 10 characters or less.\n");
        }
    }
    // this line should never be run
    return NULL;
}


/*************************************************************************
* function flushStdin
* Flushes any buffered data from stdin. This fixes a bug caused by the
* client sending messages out-of-turn.
* Pre-conditions: None
* Post-conditions: stdin will be emptied
*************************************************************************/
void flushStdin() {
    /*
        This code adapted from https://pubs.opengroup.org/onlinepubs/009695399/functions/poll.html
    */

    // set up struct data for poll() function
    struct pollfd fds[1];

    // poll stdin (represented by 0)
    fds[0].fd = 0;
    // poll any data (represented by POLLRDNORM)
    fds[0].events = POLLRDNORM;

    // poll stdin and store to result
    int result = poll(fds, 1, 0);

    // if result not 0, data needs to be flushed from stdin
    while(result != 0) {
        // create temporary variable to flush stdin into
        char trash[1000];

        // get data from stdin and store to trash
        fgets(trash, 1000, stdin);

        // poll new result to see if more data needs to be cleared
        result = poll(fds, 1, 0);
    }
}


/*************************************************************************
* function getInput
* Gets input from user and stores to char* inputString param
* Params:
*   char* username (for displaying the query)
*   char* inputString (to store the user input to)
* Returns:
*   stores input to char* inputString param
* Pre-conditions: Client is waiting for user input
* Post-conditions: Client gets user input and passes back to main function
*************************************************************************/
void getInput(char* username, char* inputString) {
    // flush stdin so that bad data is not sent to server
    flushStdin();

    // print username and query user for input
    printf("%s> ", username);

    // clear inputString and stdin
    memset(inputString, '\0', 512);

    // get input from user and store to inputString
    fgets(inputString, 512, stdin);

    // drop newline character from inputString
    inputString[strlen(inputString) - 1] = '\0';
}


/*************************************************************************
* function sendMessage
* Sends provided message to the socket pointed to by socketStream
* Params:
*   char* message
*   char* username (for appending to the message before sending)
*   char* buffer (for sending the actual message)
*   int socketStream (points to socket connection)
* Pre-conditions: Client has user input and is ready to send it
* Post-conditions: Client sends user input and then waits for a response
*************************************************************************/
void sendMessage(char* message, char* username, char* buffer, int socketStream) {
    /*
        Used my code from CS344 for sending messages
    */

    // clear out buffer array and set to username + message
    memset(buffer, '\0', 1000);
    sprintf(buffer, "%s> %s", username, message);

    // send buffer to socket
    int charsWritten = send(socketStream, buffer, strlen(buffer), 0);
    if (charsWritten < 0) {
        // error writing to socket
        printf("chatclient: ERROR sending to socket\n");
    }

    // clear buffer after sending
    memset(buffer, '\0', 1000);
}


/*************************************************************************
* function receiveMessage
* Receives a message from the designated socket and returns a status
* that indicates if the connection is still open
* Params:
*   char* buffer
*   int socketStream (points to socket connection)
* Returns:
*   1 if connection is still open, 0 if connection is closed
* Pre-conditions: Client has sent a message and is waiting for a response
* Post-conditions: Query input to send to server or quit if server quit
*************************************************************************/
int receiveMessage(char* buffer, int socketStream) {
    /*
        Used my code from CS344 for receiving messages
    */

    // clear out buffer array
    memset(buffer, '\0', 1000);

    // receive data from socket
    int charsRead = recv(socketStream, buffer, 512, 0);
    if (charsRead < 0) {
        // error receiving from socket
        printf("chatclient: ERROR receiving from socket\n");
    }

    // if buffer not equal to '\quit', server has not closed connection
    if (strcmp(buffer, "\\quit") != 0 && strcmp(buffer, "") != 0) {
        // print contents of buffer
        printf("%s\n", buffer);

        // clear out buffer
        memset(buffer, '\0', 1000);

        // return 1 indicating connection still open
        return 1;
    } else {
        // server has quit the connection, close socket
        printf("chatclient: server has closed the connection\n");

        // clear out buffer
        memset(buffer, '\0', 1000);

        // return 0 indicating connection has been closed
        return 0;
    }
}

/*************************************************************************
* main method
* Opens a socket connection with the host and port provided as runtime
* arguments. After opening the connection, the client will ask the user
* for their username, and then send an initial message to the server.
* The client will then wait until it receives a response from the server
* and print it out. This will repeat until one of the server or client
* quit the connection.
*************************************************************************/
int main (int argc, char* argv[]) {
    // if arg count is wrong, display message showing correct usage to user
    if (argc != 3) {
        printf("USAGE: chatserve <host> <port>\n");
        exit(1);
    }

    // initialize variables and structs
    int port;
    int socketStream;
    int status;
    char host[100];
    char portString[10];

    struct addrinfo *serverInfo;
    struct addrinfo hints;

    /*
       Used Beej's Guide for this networking stuff
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

    // open socket and connect to it using serverInfo returned by getaddrinfo
    socketStream = openSocket(serverInfo, port);
    connectToSocket(socketStream, serverInfo, port);

    // get the username
    char username[20];
    strcpy(username, getName());

    // variables for input and sending/receiving data
    char input[512];
    char buffer[1000];
    int stayConnected = 1;

    // repeat until connection closed
    while (stayConnected) {
        // get input from user and copy to input var
        getInput(username, input);

        // if user entered something other than '\quit', append the username to the message and send to the server
        if (strcmp(input, "\\quit") != 0) {
            // send message to the server
            sendMessage(input, username, buffer, socketStream);

            // receiveMessage waits for response. returns 1 if connection is still alive, 0 if server closed connection
            if(!receiveMessage(buffer, socketStream)) {
                // server closed connection, close socket on client and break out of loop
                close(socketStream);
                stayConnected = 0;
            }
            // receiveMessage returned 1, loop back to top of loop and query user for input
        } else {
            // user entered quit, close socket and break out of loop
            close(socketStream);
            stayConnected = 0;
        }
    }

    // return 0 and exit
    return 0;
}
