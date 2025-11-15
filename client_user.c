#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USER_ID     2001   // or 2002, 2003... just change when running multiple
#define IS_ADMIN    0

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &srv.sin_addr);

    packet_t pkt = {0};
    pkt.client_id = USER_ID;
    pkt.is_admin = IS_ADMIN;
    pkt.seq_num = 0;

    printf("=== USER CLIENT (ID: %u) – READ ONLY ===\n", USER_ID);
    printf("Allowed: /list | /read file.txt | /ping | quit\n");
    printf("Blocked: upload, delete, STATS\n");

    char line[256];
    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        if (strcmp(line, "quit") == 0) break;

       
        if (strstr(line, "/upload") || strstr(line, "/delete") ||
            strcmp(line, "STATS") == 0) {
            printf("Permission denied! Users cannot use this command.\n");
            continue;
        }

        strcpy(pkt.command, line);
        pkt.seq_num++;
        sendto(sock, (char*)&pkt, sizeof(pkt), 0,
               (struct sockaddr*)&srv, sizeof(srv));
        printf("[Sent] %s\n", line);

        char recv_buf[2048];
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        int n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
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

    closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}