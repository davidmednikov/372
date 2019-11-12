CFLAGS= -Wall

default: ftclient ftserver

ftclient: ftclient.py
	chmod +x ftclient.py

ftserver: ftserver.c
	clang -o ftserver -g ftserver.c $(CFLAGS)

all: ftclient ftserver
