# https://oregonstate.instructure.com/courses/1771948/files/76024149/download?wrap=1


import sys
from socket import *

def openSocket(portNumber):
    chatSocket = socket(AF_INET, SOCK_STREAM)
    # https://docs.python.org/3/library/socket.html
    chatSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
    chatSocket.bind(('', portNumber))
    chatSocket.listen(1)
    print(f"chatserve listening on port {portNumber}...")
    return chatSocket

def closeSocket(chatSocket, portNumber):
    print(f"closing connection on port {portNumber}")
    chatSocket.close()

def sendMessage(chatSocket, message):
    chatSocket.send(message.encode('utf-8'))

def receiveMessage(chatSocket):
    return chatSocket.recv(1024).decode('utf-8')

def getInput(query):
    return input(query)

chatServerPort = sys.argv[1]

while True:
    chatServerSocket = openSocket(int(chatServerPort))
    connected = False
    connectionSocket, address = chatServerSocket.accept()
    connected = True
    while connected:
        receivedMessage = receiveMessage(connectionSocket)
        print(receivedMessage)
        if (receivedMessage != ''):
            userInput = getInput('chatserve> ')
            if userInput != r'\quit':
                sendMessage(connectionSocket, "chatserve> " + userInput)
            else:
                closeSocket(chatServerSocket, chatServerPort)
                connected = False
        else:
            closeSocket(chatServerSocket, chatServerPort)
            connected = False