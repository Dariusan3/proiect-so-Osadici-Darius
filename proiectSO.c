#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define MAX_SIZE 1024

struct stat statBuffer;

void saveSnapshot(char *dirPath, int fd) {
    /*
    scriem in fisierul snapshot.txt numele fisierului, dimensiunea si data ultimei modificari
    */
    if(lstat(dirPath, &statBuffer) == 0) {
        if(S_ISREG(statBuffer.st_mode)) {
            dprintf(fd, "%s %ld %s\n", dirPath, statBuffer.st_size, ctime(&statBuffer.st_mtime));
        }
    }
    else {
        perror("Error getting file status");
    }
}

void createSnapshot(char *dirPath, int fd) {
    DIR *dir = opendir(dirPath);
    if(dir == NULL) {
        perror("Error opening directory");
        return;
    }
    /*
    parcurgem directorul si apelam functia saveSnapshot pentru fiecare fisier gasit
    */
    struct dirent *dir_current;
    while((dir_current = readdir(dir)) != NULL) {
        /*
        ignoram fisierele . si ..
        */
        if(strcmp(dir_current->d_name, ".") == 0 || strcmp(dir_current->d_name, "..") == 0) {
            continue;
        }
        char path[MAX_SIZE];
        /*
        construim calea catre fisier si apelam functia saveSnapshot pentru fisierul gasit
        sau apelam recursiv functia createSnapshot pentru directorul gasit
        */
        snprintf(path, sizeof(path), "%s/%s", dirPath, dir_current->d_name);
        if(lstat(path, &statBuffer) == 0) {
            /*
            daca fisierul este un fisier obisnuit, apelam functia saveSnapshot
            */
            if(S_ISREG(statBuffer.st_mode)) {
                saveSnapshot(path, fd);
            }
            /*
            daca fisierul este un director, apelam recursiv functia createSnapshot
            */
            else if(S_ISDIR(statBuffer.st_mode)) {
                createSnapshot(path, fd);
            }
        }
     
    }
    closedir(dir);
}

int main(int argc, char *argv[]) { 
    if (argc != 2) {
        printf("Usage: %s <directory>\n", argv[0]);
        exit(1);
    }
    if(S_ISDIR(statBuffer.st_mode) != 0) {
        printf("Error: %s is not a directory\n", argv[1]);
        exit(1);
    }
    /*
    cream un fisier snapshot.txt in care vom scrie informatii despre fisierele din directorul dat ca argument
    este deschis pentru citire si scriere, daca fisierul nu exista, 
    este creat cu drepturi de citire, scriere si executie pentru user, 
    citire si scriere pentru grup si altii
    */
    int fd = open("snapshot.txt" , O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if(fd < 0) {
        perror("Error opening file");
        exit(1);
    }

    createSnapshot(argv[1], fd);
    close(fd);
    return 0;
}