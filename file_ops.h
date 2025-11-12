// Ky fajll eshte header per modulin "file_ops.c, deklaron funksionet per menaxhimin 
// e fajllave dhe komandave qe vijne nga klientet."
 #ifndef FILE_OPS_H
 #define FILE_OPS_H 
 
 #include <stdio.h> 
 
 void list_files(char *response); 
 void read_file(const char *filename, char *response); 
 void file_info(const char *filename, char *response); 
 int delete_file(const char *filename); 
 int search_file(const char *keyword, char *response); 
 int upload_file(const char *filename, const char *data); 
 int download_file(const char *filename, char *response); 
 
 #endif