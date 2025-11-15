#include "common.h"
#include "file_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <io.h>

#ifdef _WIN32
    WSADATA wsa;
    #define access _access
#endif

client_info_t clients[MAX_CLIENTS];
int client_count = 0;
CRITICAL_SECTION clients_mutex;

void init_server_files() {
#ifdef _WIN32
    CreateDirectoryA(SERVER_FILES_DIR, NULL);
#else
    mkdir(SERVER_FILES_DIR, 0755);
#endif
}

int find_client(struct sockaddr_in *addr, socklen_t addr_len) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active &&
            memcmp(&clients[i].addr.sin_addr, &addr->sin_addr, sizeof(addr->sin_addr)) == 0 &&
            clients[i].addr.sin_port == addr->sin_port) {
            return i;
        }
    }
    return -1;
}

int add_client(struct sockaddr_in *addr, socklen_t addr_len, uint32_t client_id, int is_admin) {
    if (client_count >= MAX_CLIENTS) {
        printf("ERROR: Max clients reached (%d)\n", MAX_CLIENTS);
        return -1;
    }
    int idx = client_count++;
    clients[idx].addr = *addr;
    clients[idx].addr_len = addr_len;
    clients[idx].client_id = client_id;
    clients[idx].is_admin = is_admin;
    clients[idx].last_seen = time(NULL);
    clients[idx].msg_count = 0;
    clients[idx].bytes_in = 0;
    clients[idx].bytes_out = 0;
    clients[idx].active = 1;

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip_str, INET_ADDRSTRLEN);
    printf("NEW CLIENT: ID=%u | IP=%s:%d | Admin=%s\n",
           client_id, ip_str, ntohs(addr->sin_port), is_admin ? "YES" : "NO");
    return idx;
}

void update_client(int idx, int bytes_received) {
    clients[idx].last_seen = time(NULL);
    clients[idx].msg_count++;
    clients[idx].bytes_in += bytes_received;
}

// Timeout THREADI ktu

DWORD WINAPI timeout_thread(LPVOID arg) {
    (void)arg;
    while (1) {
        Sleep(5000);
        time_t now = time(NULL);
        EnterCriticalSection(&clients_mutex);
        for (int i = 0; i < client_count; i++) {
            if (clients[i].active && (now - clients[i].last_seen > TIMEOUT_SEC)) {
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clients[i].addr.sin_addr, ip_str, INET_ADDRSTRLEN);
                printf("TIMEOUT: Removing client ID=%u | IP=%s:%d\n",
                       clients[i].client_id, ip_str, ntohs(clients[i].addr.sin_port));
                clients[i].active = 0;
            }
        }
        LeaveCriticalSection(&clients_mutex);
    }
    return 0;
}

int main() {
    SOCKET sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    packet_t pkt;

#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    InitializeCriticalSection(&clients_mutex);
#endif

    init_server_files();

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        goto cleanup;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        goto cleanup;
    }

    printf("UDP File Server STARTED on 0.0.0.0:%d\n", PORT);
    printf("Timeout: %ds | Max clients: %d\n\n", TIMEOUT_SEC, MAX_CLIENTS);

    CreateThread(NULL, 0, timeout_thread, (LPVOID)sockfd, 0, NULL);
    CreateThread(NULL, 0, stats_logger_thread, NULL, 0, NULL);
}

 // PERSON 3: STATS command (admin only)
 if (clients[idx].is_admin && strncmp(pkt.command, "STATS", 5) == 0) {
    printf("\n=== QUICK STATS ===\n");
    printf("Active clients: %d\n", client_count);
    printf("Total messages: %d\n", clients[idx].msg_count);
    printf("==================\n\n");

    packet_t ack = {0};
    ack.is_ack = 1;
    strcpy(ack.command, "OK");
    sendto(sockfd, (char*)&ack, sizeof(ack), 0,
        (struct sockaddr*)&client_addr, addr_len);
         continue;
 }

// PERSON 3: /ping command
if (strncmp(pkt.command, "/ping", 5) == 0) {
    printf("PING from client ID=%u\n", clients[idx].client_id);

    packet_t pong = {0};
    pong.client_id = pkt.client_id;
    pong.seq_num = pkt.seq_num;
    pong.is_ack = 1;
    strcpy(pong.command, "/pong");

    sendto(sockfd, (char*)&pong, sizeof(pong), 0,
    (struct sockaddr*)&client_addr, addr_len);
    continue;
}