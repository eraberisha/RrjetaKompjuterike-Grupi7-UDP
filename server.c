#include "common.h"

#ifdef _WIN32
    WSADATA wsa;
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