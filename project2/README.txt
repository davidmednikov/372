PROJECT 2 README

By David Mednikov

How to compile:
    1. Make sure the following 3 source files are in the same directory:
        * ftserver.c
        * ftclient.py
        * Makefile
    2. Navigate to that directory and run 'make' in terminal.
        This should give ftclient.py the execute permission and compile an executable for ftserver.c

How to run:
    1. On one FLIP server, run this command to start the server, passing in your own port number:
        ./ftserver [SERVER_PORT]
    2. On another FLIP server, run this command to start the client, passing in the following parameters:
        - host and port of the server
        - command and filename (filename only if necessary)
        - data_port for server to send response on

        ./ftclient.py [SERVER_HOST] [SERVER_PORT] [COMMAND] [FILENAME] [DATA_PORT]

Validation:
    The program must pass the following validation checks:
        1. The server_port on ftserver must be in the range 1025 <= server_port <= 65535.
        2. The server_port on ftclient must be in the range 1025 <= server_port <= 65535.
        3. The data_port on ftclient must be in the range 1025 <= data_port <= 65535.
        4. The server_host must be one of "flip", "flip2", or "flip3".
        5. The command must be one of "-l" or "-g".
        6. If the command is "-g", there must be a filename argument and 6 total arguments.
        7. If the command is "-l", there must be no filename argument and 5 total arguments.
        8. The specified filename must exist on the server or else an error will be returned.