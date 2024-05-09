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
#include <ctype.h>
#include <sys/wait.h>

#define MAX_SIZE 1024

int nrFisereCorupte = 0;

char vectorDistinct[10][100] = {""};
int size_vector = 0;

struct stat statBuffer;

void cloneSnapshot(char snapshot1[], int fd1, char snapshot2[], int fd2) {
    /*
    lseek muta cursorul asociat descriptorului de fisier la inceputul fisierului
    acest lucru este necesar pentru a citi fisierul de la inceput
    */
    lseek(fd1, 0, SEEK_SET);
    lseek(fd2, 0, SEEK_SET);
    char buffer[MAX_SIZE];
    int bytesRead = 0;
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
    char buffer1[MAX_SIZE] = "\0";
    char buffer2[MAX_SIZE] = "\0";
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
    snprintf scrie in bufferul aux informatiile despre fisierul dat ca argument
    write scrie bufferul aux in in fisierul asociat descriptorului fd.
    */
    char aux[MAX_SIZE] = "";
    int lengthBuf = snprintf(aux, sizeof(aux), "%s %ld %s\n", dirPath, statBuffer.st_size, ctime(&statBuffer.st_mtime));
    write(fd, aux, lengthBuf);
}

void createSnapshot(char *dirPath, int fd, char izolare[]) {
    int pid, wstatus;
    pid_t w;
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
                if((statBuffer.st_mode & (S_IRUSR | S_IWUSR | S_IXUSR 
                | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH)) == 0) {
                        int pipefd[2];
                        if(pipe(pipefd) < 0) {
                            perror("Error creating pipe");
                            exit(1);
                        }
                        if((pid = fork()) < 0) {
                            perror("Error creating process");
                            exit(1);
                        }
                        if(pid == 0) {
                            close(pipefd[0]);
                            /*
                            redirectam iesirea standard catre capatul de scriere al pipe-ului
                            */
                            dup2(pipefd[1], 1);
                            /*
                            Aceasta linie de cod va executa scriptul shell "script.sh", 
                            transmitandu-i calea "path" și argumentul "izolare".
                            */
                            execlp("./script.sh", "script.sh", path, NULL);
                            perror("Error creating child process");
                            exit(1);
                        }
                        else {
                            close(pipefd[1]);
                            char message[100] = "";
                            int count = read(pipefd[0], message, sizeof(message));
                            close(pipefd[0]);
                            if(count < 0) {
                                perror("Error reading from pipe");
                                exit(1);
                            }
                            message[strcspn(message , "\n")]='\0';
                            if(strcmp(message, "SAFE") != 0) {
                                nrFisereCorupte++;
                                printf("%s - are %d fisiere corupte\n", message, nrFisereCorupte);
                                char numeFis[150] = "";
                                char newPath[300] = "";
                                char *p = strrchr(path, '/');
                                if(p) {
                                    strcpy(numeFis, p + 1);
                                }
                                snprintf(newPath, sizeof(newPath), "%s/%s", izolare, numeFis);
                                rename(path, newPath);
                            }
                            w = wstatus = wait(&wstatus);
                            if(w == -1) {
                                perror("Error waiting for child process");
                                exit(1);
                            }
                        }
                }
                else {
                    saveSnapshot(path, fd);
                }
            }
            /*
            daca fisierul este un director, apelam recursiv functia createSnapshot
            */
            else if(S_ISDIR(statBuffer.st_mode) && S_ISLNK(statBuffer.st_mode) == 0) {
                createSnapshot(path, fd, izolare);
            }
            /*
            daca e director si nu e link simbolic, apelam recursiv functia createSnapshot
            */
        }
     
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    pid_t w;
    int wstatus, pid;
    if (argc > 12) {
        printf("Usage: %s <directory>\n", argv[0]);
        exit(1);
    }

    /*
    verificam daca nu am pus cumva diretoare cu acelasi nume deoarce daca 
    am pus acelasi nume de director de mai multe ori, nu are rost sa facem snapshot
    */
    for(int i = 5; i < argc; i++) {
        if(lstat(argv[i], &statBuffer) != 0) {
            perror("Error getting file status");
            continue;
        }
        if(S_ISDIR(statBuffer.st_mode) && S_ISLNK(statBuffer.st_mode) == 0) {
            int ok = 0;
            for(int j = 0; j < size_vector; j++) {
                if(strcmp(vectorDistinct[j], argv[i]) == 0) {
                    ok = 1;
                    break;
                }
            }
            if(ok == 0) {
                strcpy(vectorDistinct[size_vector++], argv[i]);
            }
        }
    }
    char dirOut[MAX_SIZE];
    strcpy(dirOut, argv[2]);
        for(int i = 0; i < size_vector; i++) {
            if(lstat(vectorDistinct[i], &statBuffer) != 0) {
                perror("Error getting file status");
                continue;
            }
            if(S_ISDIR(statBuffer.st_mode) == 0) {
                printf("Error: %s is not a directory\n", argv[i]);
                continue;
            }
            if((pid = fork()) < 0) {
                perror("Error creating process");
                exit(1);
            }
            if(pid == 0) {
                nrFisereCorupte = 0;
                char nr1[10] = "";
                int ino1 = statBuffer.st_ino;
                /*
                Se extrage numarul inode (identificatorul unic al fisierului) din structura statBuffer 
                si este stocat in variabila ino1.
                */
                sprintf(nr1, "%d", ino1);
                char numeSnapshot1[MAX_SIZE] = "";
                strcat(numeSnapshot1, dirOut);
                strcat(numeSnapshot1, "/snapshot_");
                strcat(numeSnapshot1, nr1);
                strcat(numeSnapshot1, "_1.txt");
                int fd1 = open(numeSnapshot1 , O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
                if(fd1 < 0) {
                    perror("Error opening file");
                    exit(1);
                }
                if(lstat(numeSnapshot1, &statBuffer) == 0) {
                    if(statBuffer.st_size == 0) {
                        createSnapshot(vectorDistinct[i], fd1, argv[4]);
                    }
                    char nr2[10] = "";
                    int ino2 = statBuffer.st_ino;
                    sprintf(nr2, "%d", ino2);
                    char numeSnapshot2[MAX_SIZE] = "";
                    strcat(numeSnapshot2, dirOut);
                    strcat(numeSnapshot2, "/snapshot_");
                    strcat(numeSnapshot2, nr2);
                    strcat(numeSnapshot2, "_2.txt");

                    int fd2 = open(numeSnapshot2 , O_RDWR | O_CREAT , S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
                    if(fd2 < 0) {
                        perror("Error opening file");
                        exit(1);
                    }

                    createSnapshot(vectorDistinct[i], fd2, argv[4]);
                    if(comparareSnapshot(numeSnapshot1, fd1, numeSnapshot2, fd2) == 0) {
                        printf("The snapshots are identical\n");
                        close(fd2);
                        unlink(numeSnapshot2);
                    }
                    else {
                        printf("The snapshots are different\n");
                        cloneSnapshot(numeSnapshot1, fd1, numeSnapshot2, fd2);
                        unlink(numeSnapshot2);
                    }
                }
                exit(nrFisereCorupte);
            }
        }
        for(int i = 0; i < size_vector; i++) {
            w = wait(&wstatus);
            if(w == -1) {
                perror("Error waiting for child process");
                exit(1);
            }
            if(WIFEXITED(wstatus)) {
                printf("Procesul Copil %d s-a incheiat cu PID-ul %d si cu %d fisiere cu potential periculos\n", i + 1 , w , WEXITSTATUS(wstatus));
            }
            else if(WIFSIGNALED(wstatus)) {
                printf("Procesul Copil %d a fost terminat fortat cu PID-ul %d si cu %d fisiere cu potential periculos\n", i + 1 , w , WTERMSIG(wstatus));
            }
            else if(WIFSTOPPED(wstatus)) {
                printf("Procesul Copil %d a fost oprit cu PID-ul %d si cu %d fisiere cu potential periculos\n", i + 1 , w , WSTOPSIG(wstatus));
            }
            else if(WIFCONTINUED(wstatus)) {
                printf("Procesul Copil %d a fost continuat cu PID-ul %d si cu %d fisiere cu potential periculos\n", i + 1 , w , WEXITSTATUS(wstatus));
            }
        }
    return 0;
}