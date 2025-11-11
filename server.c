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