// Ky fajll eshte header per modulin "file_ops.c", deklaron funksionet per menaxhimin 
// e fajllave dhe komandave qe vijne nga klientet."
 #ifndef FILE_OPS_H
 #define FILE_OPS_H 
 
 #include "common.h"

 #define CHUNK_SIZE 1024 //Madhesia e bllokut per dergim/lexim fajlli
 #define FILE_FOLDER "./server_files/" //Folderi ku ruhen fajllat ne server

//Funksione per menaxhimin e fajllave nga klientet
void list_files(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq);

void read_file(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *filename);

void delete_file(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *filename);

void file_info(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *filename);

void search_files(SOCKET sockfd, client_info_t *c, struct sockaddr_in *addr, uint32_t seq, const char *keyword);

 #endif