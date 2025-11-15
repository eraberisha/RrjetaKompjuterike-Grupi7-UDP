#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define sleep(x) Sleep((x) * 1000)
#endif

#define CLIENT_ID 1001
#define IS_ADMIN 1

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

    printf("=== ADMIN CLIENT (ID: %u) ===\n", CLIENT_ID);
    printf("Available commands:\n");
    printf("  /list\n");
    printf("  /read <filename>\n");
    printf("  /download <filename>\n");
    printf("  /delete <filename>\n");
    printf("  /search <keyword>\n");
    printf("  /info <filename>\n");
    printf("  STATS\n");
    printf("  quit\n");
    printf("> ");

    char input[256];
    while (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) {
            printf("> ");
            continue;
        }

        if (strcmp(input, "quit") == 0) break;

        strcpy(pkt.command, input);
        pkt.is_ack = 0;

        int sent = sendto(sockfd, (char*)&pkt, sizeof(pkt), 0,
                          (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (sent == SOCKET_ERROR) {
            printf("sendto failed: %d\n", WSAGetLastError());
        } else {
            printf("[Sent] %s\n", input);

            char recv_buf[2048];
            struct sockaddr_in from;
            socklen_t from_len = sizeof(from);

            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

            int n = recvfrom(sockfd, recv_buf, sizeof(recv_buf), 0,
                             (struct sockaddr*)&from, &from_len);

            if (n > 0) {
                packet_t *resp = (packet_t*)recv_buf;
                printf("\n[SERVER] %s", resp->data);
                if (strncmp(resp->command, "PART_", 5) == 0) {
                    printf("   ← File part received\n");
                }
                if (strcmp(resp->command, "/pong") == 0) {
                    printf("\n");
                }
            } else {
                printf("\n[No reply from server]\n");
            }
            printf("\n");
        }

        pkt.seq_num++;
        printf("> ");
    }

    closesocket(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}