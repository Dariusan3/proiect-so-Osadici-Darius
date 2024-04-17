#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX 1024

typedef struct {
    char nume[MAX];
    time_t ultimaModificare;
} metadata;

void takeSnapshot(const char *caleDir, const char *caleSnapshots) {
    DIR *dir;
    struct dirent *intrare;
    struct stat fileInfo;
    metadata entryMetadata;
    FILE *snapshot;
    dir = opendir(caleDir);
    if (dir == NULL) {
        perror("Eroare deschidere director");
        exit(EXIT_FAILURE);
    }
    char caleSnapshot[MAX];
    snprintf(caleSnapshot, sizeof(caleSnapshot), "%s/snapshot.txt", caleSnapshots);
    snapshot = fopen(caleSnapshot, "w");
    if (snapshot == NULL) {
        perror("Eroare deschidere snapshot");
        closedir(dir);
        exit(EXIT_FAILURE);
    }
    while ((intrare = readdir(dir)) != NULL) {
        if (strcmp(intrare->d_name, ".") != 0 && strcmp(intrare->d_name, "..") != 0) 
        {
            char caleIntrare[MAX];
            snprintf(caleIntrare, sizeof(caleIntrare), "%s/%s", caleDir, intrare->d_name);
            if (stat(caleIntrare, &fileInfo) == -1) {
                perror("Eroare la metadate");
                fclose(snapshot);
                closedir(dir);
                exit(EXIT_FAILURE);
            }

            strcpy(entryMetadata.nume, caleIntrare);
            entryMetadata.ultimaModificare = fileInfo.st_mtime;
            fprintf(snapshot, "%s\t%ld\n", entryMetadata.nume, (long)entryMetadata.ultimaModificare);
            if (S_ISDIR(fileInfo.st_mode)) {
                takeSnapshot(caleIntrare, caleSnapshots);
            }
        }
    }
    closedir(dir);
    fclose(snapshot);
}

int verificareDistincte(int n, char *argv[]) {
    for(int i = 1; i < n; i++) {
        for(int j = i + 1; j < n; j++) {
            if(strcmp(argv[i], argv[j]) == 0)
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) { 
    if (argc < 2 || argc > 11) {
        fprintf(stderr, "Usage: %s directory_path\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if(verificareDistincte(argc, argv) == 0) {
            printf("Exista argumente duplicate!");
            exit(-1);
    }

    for (int i = 3; i < argc; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Eroare la fork");
            return 1;
        } 
        else if (pid == 0) { 
        	const char *caleDir1= argv[i];
          if (access(caleDir1, F_OK) != -1) {
              takeSnapshot(caleDir1, argv[2]);
              printf("Snapshot pentru directorul cu path-ul %s a fost creat cu succes.\n", caleDir1);
            }
          else {
              printf("Directorul cu path-ul %s nu exista\n", caleDir1);
          }
          exit(0);
        }
    }
    int status;
    pid_t pidCopil;
    while ((pidCopil = wait(&status)) > 0) {
        printf("Procesul copil %d terminat cu PID %d si exit code %d.\n", (int)pidCopil, (int)pidCopil, status);
    }
    return 0;
}