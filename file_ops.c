// file_ops.c
// Implementim i funksioneve për menaxhimin e fajllave (Windows + UDP)

#include "file_ops.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>

#ifdef _WIN32
    #include <windows.h>
    #define mkdir(path, mode) _mkdir(path)
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#define FILE_FOLDER "./server_files/" // Folderi ku ruhen fajllat

// Ndihmëse: dërgon një paketë përgjigjeje tek klienti
static void send_resp(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr,
                      uint32_t seq, const char *cmd, const char *msg) {
    packet_t p = {0};
    p.client_id = c->client_id;
    p.seq_num = seq;
    p.is_ack = 1;
    strncpy(p.command, cmd, sizeof(p.command)-1);
    strncpy(p.data, msg, PACKET_DATA_SIZE-1);
    sendto(sockfd, (char*)&p, sizeof(p), 0, (struct sockaddr*)addr, sizeof(*addr));
    c->bytes_out += sizeof(p); // Përditëson numrin e byte të dërguara
}

// Liston fajllat në folderin e serverit
void list_files(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq) {
    WIN32_FIND_DATAA fd;
    HANDLE hFind;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\*", FILE_FOLDER);

    char response[PACKET_DATA_SIZE] = "FILES:\n";
    char *ptr = response + 7;

    hFind = FindFirstFileA(path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        send_resp(sockfd, c, addr, seq, "ERROR", "No files or folder error");
        return;
    }

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            int len = snprintf(ptr, PACKET_DATA_SIZE - (ptr - response), "  %s\n", fd.cFileName);
            if (len > 0) ptr += len;
        }
    } while (FindNextFileA(hFind, &fd) && (ptr - response) < PACKET_DATA_SIZE - 100);

    FindClose(hFind);
    send_resp(sockfd, c, addr, seq, "LIST_OK", response); // Dërgon listën tek klienti
}

// Lexon dhe dërgon përmbajtjen e fajllit te klienti
void read_file(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *filename) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\%s", FILE_FOLDER, filename);

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        send_resp(sockfd, c, addr, seq, "ERROR", "File not found");
        return;
    }

    char buffer[PACKET_DATA_SIZE - 100];
    size_t n;
    uint32_t part = 0;

    while ((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        packet_t p = {0};
        p.client_id = c->client_id;
        p.seq_num = seq;
        p.is_ack = 1;
        sprintf(p.command, "PART_%u", part++); // Numëron pjesët e fajllit
        memcpy(p.data, buffer, n);
        sendto(sockfd, (char*)&p, sizeof(p), 0, (struct sockaddr*)addr, sizeof(*addr));
        c->bytes_out += sizeof(p);
    }
    fclose(fp);
    send_resp(sockfd, c, addr, seq, "FILE_END", "Transfer complete"); // Sinjalizon përfundimin
}

// Fshin fajllin nëse klienti është admin
void delete_file(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *filename) {
    if (!c->is_admin) {
        send_resp(sockfd, c, addr, seq, "ERROR", "Admin only");
        return;
    }
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\%s", FILE_FOLDER, filename);
    if (DeleteFileA(path))
        send_resp(sockfd, c, addr, seq, "OK", "File deleted"); // Fshihet me sukses
    else
        send_resp(sockfd, c, addr, seq, "ERROR", "Delete failed");
}

// Jep informacion mbi fajllin (emër, madhësi)
void file_info(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *filename) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\%s", FILE_FOLDER, filename);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(path, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        send_resp(sockfd, c, addr, seq, "ERROR", "File not found"); // Karakter i gabuar i zëvendësuar
        return;
    }
    FindClose(h);

    char info[512];
    snprintf(info, sizeof(info),
             "Name: %s\nSize: %lu bytes\n", filename, fd.nFileSizeLow);
    send_resp(sockfd, c, addr, seq, "INFO", info); // Dërgon informacionin
}

// Kërkon fajlla që përmbajnë keyword të caktuar
void search_files(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *keyword) {
    WIN32_FIND_DATAA fd;
    HANDLE hFind;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\*", FILE_FOLDER);

    char result[PACKET_DATA_SIZE] = "Search results:\n";
    char *ptr = result + 16;
    int found = 0;

    hFind = FindFirstFileA(path, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                if (strstr(fd.cFileName, keyword)) { // Nëse emri përmban keyword
                    int len = snprintf(ptr, PACKET_DATA_SIZE - (ptr - result), "  %s\n", fd.cFileName);
                    if (len > 0) ptr += len;
                    found = 1;
                }
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

    if (!found) strcpy(result, "No matches found.\n");
    send_resp(sockfd, c, addr, seq, "SEARCH", result); // Dërgon rezultatin e kërkimit
}
