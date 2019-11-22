#!/usr/bin/python3

#########################################################################
# CS372 Intro to Networks
# Winter 2019
#
# Project 2 - File Transfer Client (ftclient)
# David Mednikov
# Last Modified: 11/30/2019 3:30pm
#
#   TO-DO: rewrite below
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
from urllib.parse import urlparse

def open_client_socket():
    """
    Opens a socket
    Params:
        None
    Returns:
        opened socket
    Pre-conditions: Program needs an open socket
    Post-conditions: Opened socket
    """
    # create socket, set family and type
    new_socket = socket(AF_INET, SOCK_STREAM)
    return new_socket


def connect_client_socket(open_socket, socket_host, socket_port):
    """
    Connects to the socket at the provided socket
    Params:
        socket opened socket that is not connected
    Returns:
        connection to socket
    Pre-conditions: No existing connection to host and ip at socket
    Post-conditions: Connected to socket
    """
    open_socket.connect((socket_host, socket_port))
    return open_socket


def open_data_socket():
    """
    Opens a socket at the provided port number and listens for incoming data
    Params:
        data_port on which server should open socket
    Returns:
        opened data_port
    Pre-conditions: Socket at specified port must not be in use
    Post-conditions: Server is listening for connections at specified port
    """
    # create socket, set family and type
    data_socket = socket(AF_INET, SOCK_STREAM)

    # set socket options to allow re-using the port. Excerpted from:
    # https://docs.python.org/3/library/socket.html
    data_socket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)

    return data_socket


def listen_data_socket(data_socket, data_port):
     # bind to the specified port number and listen for 1 incoming connection
    data_socket.bind(('', data_port))
    data_socket.listen(1)

    return data_socket


def close_connection(open_socket):
    """
    Closes the connection to the socket
    Params:
        open_socket (connected socket)
    Pre-conditions: Client must be connected to socket
    Post-conditions: Connection closed and client terminated. I'll be back.
    """
    open_socket.close()


def send_command(open_socket, request):
    """
    Sends a command to the server
    Params:
        active socket connection
        command to send to server
    Pre-conditions: Socket must have an active connection to server
    Post-conditions: Command will be sent to server
    """
    # encode message from string to bytes and send to client
    if (request['file'] != None):
        command = "{} {} {}".format(request['command'], request['file'], request['data_port'])
    else:
        command = "{} {}".format(request['command'], request['data_port'])
    open_socket.send(command.encode('utf-8'))


def receive_data(open_socket):
    """
    Receives data from the open socket
    Params:
        active socket connection
    Returns:
        data retreieved from server
    Pre-conditions: Client must have sent a command to the server
    Post-conditions: Client will receive output from server
    """
    # read message from socket and decode from bytes to string
    return open_socket.recv(10000).decode('utf-8')


def invalid_input(error, bad_input = None):
    """
    Prints correct usage to the user
    Params:
        None
    Returns:
        None
    Pre-conditions: User entered invalid runtime arguments
    Post-conditions: Error message printed to terminal
    """
    print(f"ftclient: ERROR - INVALID INPUT")
    if error == 'arguments':
        print(f"Accepted inputs:")
        print("list: ./ftclient <SERVER_HOST> <SERVER_PORT> -l <DATA_PORT>")
        print("get: ./ftclient <SERVER_HOST> <SERVER_PORT> -g <FILENAME> <DATA_PORT>")
    elif error == 'hostname':
        print(f"{bad_input} is not a valid host. Must be one of 'flip1', 'flip2', or 'flip3'.")
    elif error == 'port':
        print(f"{bad_input} is not a valid port number. Must be between 0 and 65535.")
    else:
        print(f"Correct usage:")
        print("list: ./ftclient <SERVER_HOST> <SERVER_PORT> -l <DATA_PORT>")
        print("get: ./ftclient <SERVER_HOST> <SERVER_PORT> -g <FILENAME> <DATA_PORT>")


def check_arguments(arguments):
    if len(arguments) == 5 or len(arguments) == 6:
        if (arguments[3] == '-g' and len(arguments) == 6) or (arguments[3] == '-l' and len(arguments) == 5):
            return validate_inputs(arguments)

    invalid_input('arguments')
    return None


def validate_inputs(arguments):
    hostname = arguments[1]
    port = int(arguments[2])
    command = arguments[3]

    if len(arguments) == 6:
        file = arguments[4]
        data_port = int(arguments[5])
    else:
        file = None
        data_port = int(arguments[4])

    if hostname != 'flip1' and hostname != 'flip2' and hostname != 'flip3':
        invalid_input('hostname', hostname)
    elif port < 1 or port > 65535:
        invalid_input('port', port)
    elif data_port < 1 or data_port > 65535:
        invalid_input('port', data_port)
    else:
        request = {}
        request['data_port'] = data_port
        request['command'] = command
        request['file'] = file
        request['hostname'] = hostname
        request['port'] = port
        request['url'] = hostname + '.engr.oregonstate.edu'
        return request

    return None


def save_file(file):
    """
    Sends a file to the provided server
    Params:
        active server connection
        file to send to server
    Pre-conditions: Client must have an active connection to server
    Post-conditions: File will be sent to server
    """


def print_directory(directory_list, request):
    """
    Gets the directory from the server
    Params:
        active server connection
    Returns:
        directory retrieved from server
    Pre-conditions: Connection to server and -l flag passed at runtime
    Post-conditions: Displays list of files from server
    """
    print("Receiving directory substructure from {}:{}".format(request['hostname'], request['data_port']))
    #https://stackoverflow.com/questions/10269701/case-insensitive-list-sorting-without-lowercasing-the-result
    directory_list = sorted(directory_list, key=str.casefold)
    for line in directory_list:
        print(line)

request = check_arguments(sys.argv)

if (request != None):
    # open socket and listen on the specified port
    client_socket = open_client_socket()

    # accept an incoming connection at the socket and set connected flag to true
    client_socket = connect_client_socket(client_socket, request['url'], request['port'])

    send_command(client_socket, request)

    is_valid_command = receive_data(client_socket).strip()

    if is_valid_command == "OK\0":

        data_socket = open_data_socket()

        data_socket = listen_data_socket(data_socket, request['data_port'])

        connected_socket, address = data_socket.accept()

        response = receive_data(connected_socket)

        lines = response.split('\n')

        if lines[0] == 'list':
            lines.pop(0)
            print_directory(lines, request)
        elif lines[0] == 'get':
            print("Receiving \"{}\" from {}:{}".format(request['file'], request['hostname'], request['data_port']))
            lines.pop(0)
            save_file(lines)
            print(lines)

    else:
        print("{}:{} says\n{}".format(request['hostname'], request['port'], is_valid_command))

    close_connection(client_socket)
