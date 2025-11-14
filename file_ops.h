// Ky fajll eshte header per modulin "file_ops.c", deklaron funksionet per menaxhimin 
// e fajllave dhe komandave qe vijne nga klientet."
 #ifndef FILE_OPS_H
 #define FILE_OPS_H 
 
 #include <stdbool.h>

 #define CHUNK_SIZE 1024
 #define FILE_FOLDER "./server_files/"

//Funksionet per komanda 
 void list_files(int client_sock);
 void read_file(int client_sock, char *filename);
 void file_info(int client_sock, char *filename); 
 void search_files(int client_sock, char *text);

 //Upload dhe download
 void handle_upload(int client_sock, char *filename, bool is_admin);
 void handle_download(int client_sock, char *filename);

 //Delete
 void delete_file(int client_sock, char *filename, bool is_admin);
 
 #endif