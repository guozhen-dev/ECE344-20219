#include <stdlib.h>
#include <errno.h>
#include "common.h"
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


// #define DEBUG
/* make sure to use syserror() when a system call fails. see common.h */

void
usage() {
    fprintf(stderr, "Usage: cpr srcdir dstdir\n");
    exit(1);
}

void copyFile(char *src_file, char *dest_file) {
    int src_fd = open(src_file, O_RDONLY);
    if (src_fd < 0){
        syserror(open, src_file);
    }
    struct stat *fileStat = malloc(sizeof(struct stat));
    if (stat(src_file, fileStat)) {
        syserror(stat, src_file);
    }
    printf("File size: %ld\n", fileStat -> st_size);
    char *buffer = malloc(fileStat -> st_size);
    read(src_fd, buffer, fileStat -> st_size);
    int dest_fd = creat(dest_file, fileStat->st_mode);
    if (dest_fd  < 0 ){
        syserror(creat, dest_file);
    }
    if (write(dest_fd, buffer, fileStat -> st_size) < 0){
        syserror(write, dest_file);
    } 
    close(src_fd);
    close(dest_fd);
    free(buffer);
    free(fileStat);
    return;
}

void copyDir(char *src_dir, char *dest_dir) {



    DIR *dirstream = opendir(src_dir);
    if(!dirstream){
        syserror(opendir, src_dir);
    }
    struct stat *dirstat = malloc(sizeof(struct stat));
    if (stat(src_dir, dirstat)){
        syserror(stat, src_dir);
    }
    if(mkdir(dest_dir, 0777)) {
        syserror(mkdir, dest_dir);
    }
    if (!dirstream) {
        syserror(opendir, src_dir);
    } else {
        struct dirent *curfile = NULL ;
        while((curfile = readdir(dirstream))) {
            if (curfile -> d_type == DT_DIR) {
                if (strcmp(curfile -> d_name, ".") == 0 || strcmp(curfile -> d_name, "..") == 0)
                    continue;
                // printf("Length: %ld\n", sizeof(char) * (10 + strlen(src_dir) + strlen(curfile -> d_name)));
                char *new_src = malloc(sizeof(char) * (10 + strlen(src_dir) + strlen(curfile -> d_name)));
                memset(new_src, 0, sizeof(char) * (10 + strlen(src_dir) + strlen(curfile -> d_name)));
                strcat(new_src, src_dir);
                strcat(new_src, "/");
                strcat(new_src, curfile -> d_name);
                char *new_dest = malloc(sizeof(char) * (10 + strlen(dest_dir) + strlen(curfile -> d_name)));
                memset(new_dest, 0, sizeof(char) * (10 + strlen(dest_dir) + strlen(curfile -> d_name)));

                strcat(new_dest, dest_dir);
                strcat(new_dest, "/");
                strcat(new_dest, curfile -> d_name);
                printf("%s : is a folder\nDoing copy %s -> %s\n", curfile -> d_name, new_src, new_dest);
                copyDir(new_src, new_dest);
                free(new_dest);
                free(new_src);
            } else if (curfile -> d_type == DT_REG) {
                char *new_src = malloc(sizeof(char) * (10 + strlen(src_dir) + strlen(curfile -> d_name)));
                memset(new_src, 0, sizeof(char) * (10 + strlen(src_dir) + strlen(curfile -> d_name)));
                strcat(new_src, src_dir);
                strcat(new_src, "/");
                strcat(new_src, curfile -> d_name);
                char *new_dest = malloc(sizeof(char) * (10 + strlen(dest_dir) + strlen(curfile -> d_name)));
                memset(new_dest, 0, sizeof(char) * (10 + strlen(dest_dir) + strlen(curfile -> d_name)));
                strcat(new_dest, dest_dir);
                strcat(new_dest, "/");
                strcat(new_dest, curfile -> d_name);
                printf("%s : is a reg file\nDoing copy %s -> %s\n", curfile -> d_name, new_src, new_dest);
                copyFile(new_src, new_dest);
                free(new_dest);
                free(new_src);

            }
        }
    }

    chmod (dest_dir, dirstat -> st_mode);
    free(dirstat);
    return;
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage();
    }
    char *src = argv[1];
    char *dest = argv[2];
    if (src[strlen(src) - 1 ] == '/') src[strlen(src) - 1 ] = '\x0';
    if (dest[strlen(dest) - 1 ] == '/') dest[strlen(dest) - 1 ] = '\x0';
    DIR *source_dir = opendir(src);
    if (!source_dir) {
        syserror(opendir, src);
    } else {
        copyDir(src, dest);
    }

    return 0;
}
