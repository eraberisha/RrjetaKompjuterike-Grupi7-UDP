#ifndef COMMON_H
#define COMMON_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PORT                8080
#define MAX_CLIENTS         10
#define BUFFER_SIZE         1500
#define TIMEOUT_SEC         30
#define STATS_INTERVAL      10
#define PACKET_DATA_SIZE    1400
#define SERVER_FILES_DIR    "server_files"

typedef struct {
    struct sockaddr_in addr;
    socklen_t addr_len;
    uint32_t client_id;
    int is_admin;
    time_t last_seen;
    int msg_count;
    long bytes_in, bytes_out;
    int active;
} client_info_t;

typedef struct {
    uint32_t client_id;
    uint32_t seq_num;
    uint8_t  is_ack;
    uint8_t  is_admin;
    char     command[32];
    char     data[PACKET_DATA_SIZE];
} packet_t;

#endif