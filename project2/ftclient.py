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
# This project is a simple file transfer system with two hosts - a server (ftserver)
# and a client (ftclient). The server listens to a port that is passed as a command
# line argument. The client attempts to connect to the server at the host and port
# specified via command line arguments. Before connecting, the client will validate
# that the runtime arguments are valid before sending the request to the server.
# The request contains additional arguments passed to the client at runtime
# which will be further validated by the server (such as filename).
#
# The message will contain a command of '-g' (get) or '-l' (list) that
# will either return a file or the contents of the current directory.
# If the command is valid, the server will open a new connection
# (at a port specified by the client) and send the directory or file
# contents there. If the server gets an invalid command, it sends an error
# message over the same connection the client sent the command on.
# After getting response(s) from the server, the client should stop running,
# but the server will go back to listenting on the port.
#
# This program is the client.
#########################################################################

# Much of this code has been adapted from the socket programming lecture:
# https://oregonstate.instructure.com/courses/1771948/files/76024149/download?wrap=1

# import necessary modules
import os
import sys
from socket import socket, AF_INET, SOCK_STREAM, SOL_SOCKET, SO_REUSEADDR
from termios import tcflush, TCIOFLUSH
from urllib.parse import urlparse

def get_open_socket():
    """
    Opens a socket and sets socket options
    Params:
        None
    Returns:
        opened socket
    Pre-conditions: Client needs an open socket
    Post-conditions: Socket is opened and port can be re-used
    """
    # create socket, set family and type
    new_socket = socket(AF_INET, SOCK_STREAM)

    # set socket options to allow re-using the port. Excerpted from:
    # https://docs.python.org/3/library/socket.html
    new_socket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)

    return new_socket


def connect_client_socket(open_socket, socket_host, socket_port):
    """
    Connects to the server host and port via open socket
    Params:
        open_socket (opened socket that is not connected)
        socket_host (server hostname to connect to)
        socket_port (server port to connect to)
    Returns:
        connected socket
    Pre-conditions: No existing connection to host and ip at socket
    Post-conditions: Connected to server via socket
    """
    # connect to host and port
    open_socket.connect((socket_host, socket_port))

    # return connected socket
    return open_socket


def listen_data_socket(data_socket, data_port):
    """
    Listens to the specified port at the provided data_socket
    Params:
        data_socket (opened socket that is not connected)
        data_port (port to listen)
    Returns:
        connected socket for listening
    Pre-conditions: No existing connection to host and ip at socket
    Post-conditions: Connected to server via socket
    """
    # bind to the specified port number and listen for 1 incoming connection
    data_socket.bind(('', data_port))
    data_socket.listen(1)

    # return listening socket
    return data_socket


def close_connection(open_socket):
    """
    Closes the connection to the socket
    Params:
        open_socket (connected socket)
    Pre-conditions: Client must be connected to socket
    Post-conditions: Connection closed and client terminated. I'll be back.
    """
    # close socket
    open_socket.close()


def send_command(open_socket, request):
    """
    Sends a command to the server
    Params:
        open_socket (active socket connection)
        request (command to send to server)
    Pre-conditions: Socket must have an active connection to server
    Post-conditions: Command will be sent to server
    """
    # if the request is for a file
    if (request['file'] != None):
        # format request for file (<COMMAND> <FILE> <DATA_PORT>)
        command = "{} {} {}".format(request['command'], request['file'], request['data_port'])
    else:
        # format request for directory (<COMMAND> <DATA_PORT>)
        command = "{} {}".format(request['command'], request['data_port'])
    # encode message from string to bytes and send to client
    open_socket.send(command.encode('utf-8'))


def receive_data(open_socket, is_response = False):
    """
    Receives data from the open socket
    Params:
        open_socket (active socket connection)
        is_response (bool indicating if data is response or requested data)
    Returns:
        data retreieved from server
    Pre-conditions: Client must have sent a command to the server
    Post-conditions: Client will receive data from server
    """
    # if response, get data, no chunking
    if (is_response):
        # read message from socket and decode from bytes to string then return
        return open_socket.recv(1024).decode('utf-8')

    # else loop until all data read
    # create data var and loop until all data read
    data = None
    while True:
        # if no data read so far
        if data is None:
            # copy received text into data var
            data = open_socket.recv(1024).decode('utf-8')
        else:
            # else reading more data, get segment and concat to data string
            segment = open_socket.recv(1024).decode('utf-8')
            data += segment

            # if no more text to read, break out of while loop
            if len(segment) == 0:
                break
    # return data
    return data


def invalid_input(error, bad_input = None):
    """
    Prints correct usage to the user
    Params:
        error (type of error: arguments, hostname, or port)
        bad_input (user's input that could not be recognized)
    Returns:
        None
    Pre-conditions: User entered invalid runtime arguments
    Post-conditions: Error message printed to terminal
    """
    # print error to terminal
    print("ftclient: ERROR - INVALID INPUT")
    # if error was # or type of arguments
    if error == 'arguments':
        # print accepted inputs
        print("Accepted inputs:")
        print("list: ./ftclient <SERVER_HOST> <SERVER_PORT> -l <DATA_PORT>")
        print("get: ./ftclient <SERVER_HOST> <SERVER_PORT> -g <FILENAME> <DATA_PORT>")
    elif error == 'hostname':
        # if error was invalid hostname, print valid hostnames
        print(f"{bad_input} is not a valid host. Must be one of 'flip1', 'flip2', or 'flip3'.")
    elif error == 'port':
        # if error was out-of-range port, print in-range ports
        print(f"{bad_input} is not a valid port number. Must be between 1025 and 65535 (inclusive).")
    else:
        # other error, print correct usage
        print("Correct usage:")
        print("list: ./ftclient <SERVER_HOST> <SERVER_PORT> -l <DATA_PORT>")
        print("get: ./ftclient <SERVER_HOST> <SERVER_PORT> -g <FILENAME> <DATA_PORT>")


def check_arguments(arguments):
    """
    Checks that the number of arguments are correct for the passed command before passing to validate_inputs
    Params:
        arguments (args passed at runtime)
    Returns:
        return value of validate_inputs (request object or None if invalid)
    Pre-conditions: Program needs to check arguments for validity
    Post-conditions: Program shows error message or passes args to next step of validation
    """
    # must be b args or 6
    if len(arguments) == 5 or len(arguments) == 6:
        # if command == '-g' and 6 args OR if command == '-l' and 5 args, # of args is valid
        if (arguments[3] == '-g' and len(arguments) == 6) or (arguments[3] == '-l' and len(arguments) == 5):
            # check remaining arguments for validity
            return validate_inputs(arguments)

    # invalid input, print error and return None
    invalid_input('arguments')
    return None


def validate_inputs(arguments):
    """
    Validates inputs for validity and returns a request object if valid
    Params:
        arguments (args passed at runtime)
    Returns:
        request object if valid or None if invalid
    Pre-conditions: Correct # of arguments passed into program
    Post-conditions: Args packaged into request object or shows error if invalid
    """
    # hostname, port, and command are always commands #2, 3, and 4 (index 1, 2, 3)
    hostname = arguments[1]
    port = int(arguments[2])
    command = arguments[3]

    # if 6 total args, '-g' is the command and filename is an arg
    if len(arguments) == 6:
        # get file and data_port args
        file = arguments[4]
        data_port = int(arguments[5])
    else:
        # '-l' is the command so no file
        # get data_port arg and set file to None
        data_port = int(arguments[4])
        file = None

    # if hostname is not one of the flip servers, show an error
    if hostname != 'flip1' and hostname != 'flip2' and hostname != 'flip3':
        invalid_input('hostname', hostname)
    elif port <= 1024 or port > 65535:
        # if client port is not in valid range, show an error
        invalid_input('port', port)
    elif data_port <= 1024 or data_port > 65535:
        # if data port is not in valid range, show an error
        invalid_input('port', data_port)
    else:
        # inputs are all valid, package into a request object and return
        request = {}
        request['data_port'] = data_port
        request['command'] = command
        request['file'] = file
        request['hostname'] = hostname
        request['port'] = port
        request['url'] = hostname + '.engr.oregonstate.edu'
        return request

    # invalid input so return None
    return None


def save_file(filename, lines):
    """
    Saves a string to a file using the provided filename. Renames the new file if it already exists in the destination.
    Params:
        filename (name of file to be saved)
        lines (string contents of file to be saved)
    Returns:
        name of newly saved file
    Pre-conditions: Server receives file from server and needs to save to directory
    Post-conditions: File saved to directory without re-writing any content
    """
    # print update to terminal
    print("Receiving \"{}\" from {}:{}".format(request['file'], request['hostname'], request['data_port']))

    # if file does not already exist
    # excerpted from https://www.geeksforgeeks.org/python-os-path-isfile-method/
    if not os.path.isfile('./' + filename):
        # create the file for writing
        new_file = open(filename, 'w')

        # loop through lines in string
        for line in lines:
            # write current line to file
            new_file.write(line + '\n')

        # close new file and return filename
        new_file.close()
        return filename
    else:
        # file exists, use counter to find unused name
        counter = 0

        # copy filename to test_name
        test_name = filename
        # while the file exists
        while os.path.isfile('./' + test_name):
            # increment counter by 1
            counter += 1
            # if file has ".txt" extension, remove extension before adding counter
            if filename.endswith(".txt"):
                # splice last 4 chars (".txt") from filename
                name = filename[:-4]

                # increment counter in test_name and see if file with new name already exists
                test_name = name + '_' + str(counter) + ".txt"
            else:
                # increment counter in test_name and see if file with new name already exists
                test_name = filename + '_' + str(counter)

        # unused file name found, create file for writing
        new_file = open(test_name, 'w')

        # to make sure there is no extra newline at the end, we need to know how many lines there are
        total_lines = 0

        # loop through lines and increment 1 per line
        for line in lines:
            total_lines += 1

        # keep track of current line
        line_num = 0

        # loop through lines
        for line in lines:
            # increment line counter
            line_num += 1

            # if not last line, write current line and a newline character
            if (line_num != total_lines):
                new_file.write(line + '\n')
            else:
                # on last line, so do not append newline when writing
                new_file.write(line)

        # close new_file and return saved file name
        new_file.close()
        return test_name


def print_directory(directory_list, request):
    """
    Prints the directory as passed by the server
    Params:
        directory_list (string containing directory from server)
        request (request object from runtime arguments)
    Returns:
        directory printed to terminal
    Pre-conditions: Client received directory from server at data port
    Post-conditions: Current server directory displayed in terminal
    """
    # print message to terminal
    print("Receiving directory substructure from {}:{}".format(request['hostname'], request['data_port']))

    # sort directory using case-insensitive sort
    # excerpted from https://stackoverflow.com/questions/10269701/case-insensitive-list-sorting-without-lowercasing-the-result
    directory_list = sorted(directory_list, key=str.casefold)

    # loop through directory and print each line
    for line in directory_list:
        print(line)


####################################################################################
#
# MAIN METHOD
#
####################################################################################

# get arguments and store to request object
request = check_arguments(sys.argv)

# if request is not None, args were valid
if (request != None):
    # get an open socket for connecting to the server
    client_socket = get_open_socket()

    # connect to the server at the provided socket, url, and port
    client_socket = connect_client_socket(client_socket, request['url'], request['port'])

    # send command to the server
    send_command(client_socket, request)

    # get response from server telling if command was valid
    is_valid_command = receive_data(client_socket, True).strip()

    # command was valid, open data connection
    if is_valid_command == "OK\0":

        # get new socket for data transfer
        data_socket = get_open_socket()

        # listen to the data_port passed as a runtime arg
        data_socket = listen_data_socket(data_socket, request['data_port'])

        # accept connection on the data_socket
        connected_socket, address = data_socket.accept()

        # get data from server, either containing a directory or a file
        response = receive_data(connected_socket, False)

        # split response by newline chars
        lines = response.split('\n')

        # if first line (command) == 'list', print each line
        if lines[0] == 'list':
            # remove 'list' from top of file
            lines.pop(0)

            # pass directory to print_directory for printing
            print_directory(lines, request)
        elif lines[0] == 'get':
            # remove 'get' from top of file
            lines.pop(0)

            # pass filename and file contents to save_file for saving
            save_name = save_file(request['file'], lines)

            # print success message
            print("File transfer complete. File saved as {}.".format(save_name))

        # close data connection
        close_connection(data_socket)

    # command not valid, print error to terminal
    else:
        print("{}:{} says\n{}".format(request['hostname'], request['port'], is_valid_command))

    # close socket whether successful or not
    close_connection(client_socket)
