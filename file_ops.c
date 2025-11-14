#include "file_ops.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

//List files
void list files(int client_sock){
    char response[2048]={0};
    DIR *dir=opendir(FILE_FOLDER);

    if(!dir){
        strcpy(response,"Error: Cannot open folder.\n");
        send(client_sock, response, strlen(response), 0);
        return;
    }

    strcpy(response,"FILES: \n");

    struct dirent *entry;
    while((entry=readdir(dir))!=NULL){
        if(entry->d_name[0]=='.') continue;

        strcat(response, entry->d_name);
        strcat(response,"\n";)
    }

    closedir(dir);
    send(client_sock, response, strlen(response),0);
}

//Read file
void read_file(int client_sock, char *filename){
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_FOLDER, filename);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        send(client_sock, "ERROR: File not found.\n", 24, 0);
        return;
    }

    char buffer[CHUNK_SIZE];
    int bytes;

    while ((bytes = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        send(client_sock, buffer, bytes, 0);
    }

    fclose(fp);
}

//File info
void file_info(int client_sock, char *filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", FILE_FOLDER, filename);

    struct stat st;
    if (stat(filepath, &st) != 0) {
        send(client_sock, "ERROR: File not found.\n", 24, 0);
        return;
    }

    char response[512];
    snprintf(response, sizeof(response),
             "Size: %ld bytes\nCreated: %ld\nModified: %ld\n",
             st.st_size, st.st_ctime, st.st_mtime);

    send(client_sock, response, strlen(response), 0);
}