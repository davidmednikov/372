import SocketServer
import sys

class ChatServer(SocketServer.BaseRequestHandler):
    def handle(self):
        print(self)

port = sys.argv[1]
print(port)

server = SocetServier.TCPServer("0.0.0.0", port, ChatServer)

server.serve_forever()