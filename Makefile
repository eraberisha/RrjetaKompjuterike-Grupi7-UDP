CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lws2_32

TARGETS = server.exe admin.exe user.exe

all: $(TARGETS)

server.exe: server.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lpthread

admin.exe: client_admin.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

user.exe: client_user.c common.h
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)