#!/usr/bin/python3

#########################################################################
# CS372 Winter 2019 - Project 1 - Server
# David Mednikov
# Last Modified: 10/29/2019 1:55pm
#
# This project is a simple chat system with two users - a server
# and a client. The server listens to a port that is passed as a
# command line argument. The client attempts to connect to a server
# at the host and port specified via command line arguments. After
# asking the user for their username, the client sends a message to
# the server. The two hosts message back and forth until one of them
# kills the connection. After killing the connection, the client should
# stop running, but the server should go back to listenting on the port.
#
# This program is the server.
#########################################################################

# Much of this code has been adapted from the socket programming lecture:
# https://oregonstate.instructure.com/courses/1771948/files/76024149/download?wrap=1

import sys
from socket import socket, AF_INET, SOCK_STREAM, SOL_SOCKET, SO_REUSEADDR
from termios import tcflush, TCIOFLUSH

def openSocket(portNumber):
    """
    Opens a socket at the provided port number and listens for an incoming connection
    Params: portNumber on which server should open socket
    Pre-conditions: Socket at specified port must not be in use
    Post-conditions: Server is listening for connections at specified port
    """
    # create socket, set family and type
    chatSocket = socket(AF_INET, SOCK_STREAM)

    # set socket options to allow re-using the port. Excerpted from:
    # https://docs.python.org/3/library/socket.html
    chatSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)

    # bind to the specified port number and listen for 1 incoming connection
    chatSocket.bind(('', portNumber))
    chatSocket.listen(1)

    # print update to terminal and return opened socket to caller
    print(f"chatserve: listening on port {portNumber}...")
    return chatSocket


def closeSocket(chatSocket, portNumber):
    """
    Closes a socket at the provided port number
    Params: portNumber on which server should open socket
    Pre-conditions: Socket must have an active connection at specified port
    Post-conditions: Socket at specified port will not have an active connection
    """
    # print update to terminal and close socket
    print(f"chatserve: closing socket on port {portNumber}")
    chatSocket.close()


def sendMessage(chatSocket, message):
    """
    Sends a message to the provided socket
    Params: active socket connection, message to send to client
    Pre-conditions: Socket must have an active connection at specified port
    Post-conditions: Message will be sent to client at specified socket
    """
    # encode message from string to bytes and send to client
    chatSocket.send(message.encode('utf-8'))


def receiveMessage(chatSocket):
    """
    Receives a message from the provided socket
    Params: active socket connection
    Pre-conditions: Socket must have a pending message to be read by the server.
                    If no message, program should halt until one comes in.
    Post-conditions: Message will be read by the server
    """
    # read message from socket and decode from bytes to string
    return chatSocket.recv(1024).decode('utf-8')


def getInput(query):
    """
    Accepts input from the user
    Params: query to show the user when requesting input
    Pre-conditions: stdin should be empty
    Post-conditions: user's input will be accepted by the pgoram
    """
    # flush stdin so that any text entered out of turn is ignored
    tcflush(sys.stdin, TCIOFLUSH)
    # show user query, get input, and return
    return input(query)

# get port from runtime argument
chatServerPort = sys.argv[1]

# keep looping infinitely (until SIGINT received)
while True:
    # open socket and listen on the specified port
    chatServerSocket = openSocket(int(chatServerPort))

    # set connected flag to false
    connected = False

    # accept an incoming connection at the socket and set connected flag to true
    connectionSocket, address = chatServerSocket.accept()
    connected = True

    # while connection is open, receive and send messages
    while connected:
        # get message from socket (or wait for one if there isn't one)
        receivedMessage = receiveMessage(connectionSocket)

        # if received message is not blank, that means client sent a message
        # blank message means client closed connection
        if (receivedMessage != ''):
            # print received message to server terminal
            print(receivedMessage)

            # query user for input
            userInput = getInput('chatserve> ')

            # if not '\quit', send message to client
            if userInput != r'\quit':
                sendMessage(connectionSocket, "chatserve> " + userInput)
            else:
                # user entered '\quit', close connection and open up port for a new contact
                sendMessage(connectionSocket, r'\quit')
                closeSocket(chatServerSocket, chatServerPort)
                # set connected flag to false to break out of loop
                connected = False
        else:
            # client sent blank string, meaning they closed their connection
            print("chatserve: client has closed the connection")
            # close socket and set connected flag to false to break out of loop
            closeSocket(chatServerSocket, chatServerPort)
            connected = False