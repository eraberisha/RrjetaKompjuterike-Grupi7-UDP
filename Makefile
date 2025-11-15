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

	clean:
	-del /f $(TARGETS) 2>nul
	if exist server_files rmdir /s /q server_files
	if exist server_stats.txt del server_stats.txt

run-server:
	start cmd /c server.exe

run-admin:
	start cmd /c admin.exe

run-user:
	start cmd /c user.exe

.PHONY: all clean run-server run-admin run-user