#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define sleep(x) Sleep((x) * 1000)
#endif

#define CLIENT_ID 2001
#define IS_ADMIN 0

int main() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
#endif

    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    packet_t pkt = {0};
    pkt.client_id = CLIENT_ID;
    pkt.is_admin = IS_ADMIN;
    pkt.seq_num = 0;

    printf("=== USER CLIENT (ID: %u) ===\n", CLIENT_ID);
    printf("Connected to server at 127.0.0.1:%d\n", PORT);

    closesocket(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}