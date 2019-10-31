CFLAGS= -Wall

default: chatserve chatclient

chatserve: chatserve.py
	chmod +x chatserve.py

chatclient: chatclient.c
	clang -o chatclient -g chatclient.c $(CFLAGS)

all: chatserve chatclient
