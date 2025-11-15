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


void print_stats_terminal() {
    EnterCriticalSection(&clients_mutex);
    printf("\n=== LIVE STATS ==================================\n");
    printf("Active connections : %d / %d\n", client_count, MAX_CLIENTS);
    long total_in = 0, total_out = 0;
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clients[i].addr.sin_addr, ip, INET_ADDRSTRLEN);
            printf(" [%u] %-15s:%-5d Msg:%-4d In:%-8ld Out:%-8ld %s\n",
                   clients[i].client_id, ip, ntohs(clients[i].addr.sin_port),
                   clients[i].msg_count, clients[i].bytes_in, clients[i].bytes_out,
                   clients[i].is_admin ? "ADMIN" : "USER");
            total_in += clients[i].bytes_in;
            total_out += clients[i].bytes_out;
        }
    }
    printf("Total traffic : %ld bytes in, %ld bytes out\n", total_in, total_out);
    printf("================================================\n\n");
    LeaveCriticalSection(&clients_mutex);
}

void write_stats_to_file() {
    FILE *f = fopen("server_stats.txt", "a");
    if (!f) return;
    time_t now = time(NULL);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&now));
    EnterCriticalSection(&clients_mutex);
    fprintf(f, "=== STATS: %s ===\n", timestr);
    fprintf(f, "Active: %d\n", client_count);
    fprintf(f, "Clients: ");
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active) {
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clients[i].addr.sin_addr, ip, INET_ADDRSTRLEN);
            fprintf(f, "%s:%d ", ip, ntohs(clients[i].addr.sin_port));
        }
    }
    long total_in = 0, total_out = 0;
    for (int i = 0; i < client_count; i++) {
        total_in += clients[i].bytes_in;
        total_out += clients[i].bytes_out;
    }
    fprintf(f, "\nTraffic: %ld B in, %ld B out\n\n", total_in, total_out);
    LeaveCriticalSection(&clients_mutex);
    fclose(f);
}

DWORD WINAPI stats_logger_thread(LPVOID arg) {
    (void)arg;
    while (1) {
        Sleep(STATS_INTERVAL * 1000);
        print_stats_terminal();
        write_stats_to_file();
    }
    return 0;
}

void handle_stats(SOCKET sockfd, client_info_t *c, struct sockaddr_in *client_addr) {
    if (!c->is_admin) {
        packet_t resp = {0};
        resp.client_id = c->client_id;
        resp.is_ack = 1;
        strcpy(resp.command, "ERROR");
        strcpy(resp.data, "Only admin can use STATS");
        sendto(sockfd, (char*)&resp, sizeof(resp), 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
        return;
    }
    print_stats_terminal();
    packet_t resp = {0};
    resp.client_id = c->client_id;
    resp.is_ack = 1;
    strcpy(resp.command, "STATS_OK");
    strcpy(resp.data, "Full stats printed in server console");
    sendto(sockfd, (char*)&resp, sizeof(resp), 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
}

void handle_ping(SOCKET sockfd, client_info_t *c, packet_t *req, struct sockaddr_in *client_addr) {
    printf("PING from client ID=%u\n", c->client_id);
    packet_t pong = {0};
    pong.client_id = c->client_id;
    pong.seq_num = req->seq_num;
    pong.is_ack = 1;
    strcpy(pong.command, "/pong");
    strcpy(pong.data, "Server is alive!");
    sendto(sockfd, (char*)&pong, sizeof(pong), 0, (struct sockaddr*)client_addr, sizeof(*client_addr));
    c->bytes_out += sizeof(pong);
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

    while (1) {
        int n = recvfrom(sockfd, (char*)&pkt, sizeof(pkt), 0,
                         (struct sockaddr*)&client_addr, &addr_len);
        if (n == SOCKET_ERROR) {
            printf("recvfrom error: %d\n", WSAGetLastError());
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);

        EnterCriticalSection(&clients_mutex);

        int idx = find_client(&client_addr, addr_len);
        if (idx == -1 && pkt.client_id != 0) {
            idx = add_client(&client_addr, addr_len, pkt.client_id, pkt.is_admin);
        }

        if (idx != -1 && clients[idx].active) {
            update_client(idx, n);
            printf("FROM ID=%u (%s:%d): CMD='%s' | seq=%u\n",
                   clients[idx].client_id, ip_str, ntohs(client_addr.sin_port),
                   pkt.command, pkt.seq_num);
  
            if (clients[idx].is_admin && strncmp(pkt.command, "STATS", 5) == 0) {
                handle_stats(sockfd, &clients[idx], &client_addr);
            }
            else if (strncmp(pkt.command, "/ping", 5) == 0) {
                handle_ping(sockfd, &clients[idx], &pkt, &client_addr);
            }

            else if (strncmp(pkt.command, "/list", 5) == 0) {
                list_files(sockfd, &clients[idx], &client_addr, pkt.seq_num);
            }
            else if (strncmp(pkt.command, "/read ", 6) == 0) {
                char *fname = pkt.command + 6;
                while (*fname == ' ') fname++;
                read_file(sockfd, &clients[idx], &client_addr, pkt.seq_num, fname);
            }
            else if (strncmp(pkt.command, "/delete ", 8) == 0) {
                char *fname = pkt.command + 8;
                while (*fname == ' ') fname++;
                delete_file(sockfd, &clients[idx], &client_addr, pkt.seq_num, fname);
            }
            else if (strncmp(pkt.command, "/info ", 6) == 0) {
                char *fname = pkt.command + 6;
                while (*fname == ' ') fname++;
                file_info(sockfd, &clients[idx], &client_addr, pkt.seq_num, fname);
            }
            else if (strncmp(pkt.command, "/search ", 8) == 0) {
                char *kw = pkt.command + 8;
                while (*kw == ' ') kw++;
                search_files(sockfd, &clients[idx], &client_addr, pkt.seq_num, kw);
            }
            else {
                packet_t err = {0};
                err.client_id = clients[idx].client_id;
                err.seq_num = pkt.seq_num;
                err.is_ack = 1;
                strcpy(err.command, "ERROR");
                strcpy(err.data, "Unknown command");
                sendto(sockfd, (char*)&err, sizeof(err), 0,
                       (struct sockaddr*)&client_addr, addr_len);
            }
        }
        else {
            printf("IGNORED: Unknown or inactive client from %s:%d\n", ip_str, ntohs(client_addr.sin_port));
        }

        LeaveCriticalSection(&clients_mutex);
    }
cleanup:
    closesocket(sockfd);
#ifdef _WIN32
    DeleteCriticalSection(&clients_mutex);
    WSACleanup();
#endif
    return 0;
}