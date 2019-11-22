PROJECT 2 README

By David Mednikov

How to compile:
    1. Make sure the following 3 source files are in the same directory:
        * ftserver.c
        * ftclient.py
        * Makefile
    2. Navigate to that directory and run 'make' in terminal.
        This should give ftclient.py the execute permission and compile an executable for ftserver.c

How to execute:
    1. On one FLIP server, run this command to start the server, passing in your own port number:
        ./ftserver [PORTNUM]
    2. On another FLIP server, run this command to start the client, passing in the following parameters:
        - host and port of the server
        - command and filename (if necessary)
        - data_port for server to send response on

        ./ftclient.py [HOSTNAME] [PORTNUM] [COMMAND] [FILENAME] [DATA_PORT]

How to control:
    1. The client will ask you to enter your username. Must be 10 characters or less and contain no white space.
    2. After entering username, the client will query you for a message to send to the server. Limit 500 chars.
    3. Hit Enter to send the message to the server. The client will pause, waiting for a response from the server.
    4. On the server side, you should see the message from the client appear as "{username} > {message}".
    5. The server will query the user for a message to send to the client. Press Enter to send. Max 500 characters.
    6. The client will display the message as "chatserve > {message}" and query the user for a message to send back.
    7. Steps 2-6 will repeat until one of the programs enters an input of '\quit' or sends a SIGINT signal.
    8. If the client enters '\quit' or receives a SIGINT, the client will quit but the server will still listen to the port.
    9. If the server enters '\quit', it will close the connection with the client and listen for another connection.
    10. If the server receives a SIGINT, both the server and the client will quit.