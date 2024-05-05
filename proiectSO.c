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

void cloneSnapshot(char snapshot1[], int fd1, char snapshot2[], int fd2) {
    /*
    lseek muta cursorul asociat descriptorului de fisier la inceputul fisierului
    acest lucru este necesar pentru a citi fisierul de la inceput
    */
    lseek(fd1, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);
    char buffer[MAX_SIZE];
    int bytesRead;
    /*
    bytesread retine numarul de octeti cititi din fisierul snapshot1 in fiecare iteratie a buclei
    se citesc octeti din fisierul snapshot1 si se scriu in fisierul snapshot2
    */
    while((bytesRead = read(fd1, buffer, MAX_SIZE)) > 0) {
        write(fd2, buffer, bytesRead);
    }
}

int comparareSnapshot(char snapshot1[], int fd1, char snapshot2[], int fd2) {
    /*
    fstat returneaza informatii despre fisierul asociat descriptorului de fisier dat ca argument
    */
    fstat(fd1, &statBuffer);
    int size1 = statBuffer.st_size;
    fstat(fd2, &statBuffer);
    int size2 = statBuffer.st_size;
    if(size1 != size2) {
        return 1;
    }
    char buffer1[MAX_SIZE];
    char buffer2[MAX_SIZE];
    int bytesRead1;
    int bytesRead2;
    lseek(fd1, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);
    while((bytesRead1 = read(fd1, buffer1, MAX_SIZE)) > 0 && (bytesRead2 = read(fd2, buffer2, MAX_SIZE)) > 0) {
        if(strlen(buffer1) != strlen(buffer2)) {
            return 1;
        }
        for(int i = 0; buffer1[i]; i++) {
            if(buffer1[i] != buffer2[i]) {
                return 1;
            }
        }
    }
    return 0;
    /*
    daca dimensiunile fisierelor sunt diferite sau continutul lor este diferit, returnam 1
    altfel, returnam 0
    */
}

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
    struct dirent *dirCurrent;
    while((dirCurrent = readdir(dir)) != NULL) {
        /*
        ignoram fisierele . si ..
        */
        if(strcmp(dirCurrent->d_name, ".") == 0 || strcmp(dirCurrent->d_name, "..") == 0) {
            continue;
        }
        char path[MAX_SIZE];
        /*
        construim calea catre fisier si apelam functia saveSnapshot pentru fisierul gasit
        sau apelam recursiv functia createSnapshot pentru directorul gasit
        */
        snprintf(path, sizeof(path), "%s/%s", dirPath, dirCurrent->d_name);
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
    char numeDir1[MAX_SIZE] = "";
    strcat(numeDir1, "snapshot_");
    strcat(numeDir1, argv[1]);
    strcat(numeDir1, "_1.txt");
    int fd1 = open(numeDir1 , O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    if(fd1 < 0) {
        perror("Error opening file");
        exit(1);
    }
    if(lstat(argv[1], &statBuffer) == 0) {
        if(statBuffer.st_size == 0) {
            createSnapshot(argv[1], fd1);
        }
        char numeDir2[MAX_SIZE] = "";
        strcat(numeDir2, "snapshot_");
        strcat(numeDir2, argv[1]);
        strcat(numeDir2, "_2.txt");
        int fd2 = open(numeDir2, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
        if(fd2 < 0) {
            perror("Error opening file");
            exit(1);
        }
        /*
        cream un al doilea fisier snapshot_2.txt in care vom scrie informatii despre fisierele din directorul dat ca argument
        */
        createSnapshot(argv[1], fd2);
        if(comparareSnapshot(numeDir1, fd1, numeDir2, fd2) == 0) {
            printf("The snapshots are identical\n");
            unlink(numeDir2);
        }
        else {
            printf("The snapshots are different\n");
            cloneSnapshot(numeDir1, fd1, numeDir2, fd2);
            unlink(numeDir2);
        }
        /*
        unlink sterge fisierul snapshot_2.txt dupa ce am terminat de lucrat cu el
        */
    }
    else {
        perror("Error getting file status");
    }
    close(fd1);
    return 0;
}