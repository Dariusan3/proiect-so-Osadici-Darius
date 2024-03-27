#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void printNumeFilesOrDirectories(char numeDirectoare[100][50], int n){
  for(int i = 0; i < n; i++){
    printf("%s\n", numeDirectoare[i]);
  }
}

int main(int argc, char** argv){
  DIR *director = NULL;
  if((director = opendir(argv[1])) == NULL){
    perror("eroare deschidere director\n");
    exit(-1);
  }
  char numeDirectoare[100][50];
  struct dirent* dateDirector = (struct dirent*)malloc(sizeof(struct dirent));
  if(NULL == dateDirector){
    perror("eroare alocare director\n");
    exit(-1);
  }
  int i = 0;
  for( i = 0; i < 100 && (dateDirector = readdir(director)) != NULL; i++){
    strcpy(numeDirectoare[i], dateDirector->d_name);
   
  }
  printNumeFilesOrDirectories(numeDirectoare, i);
  closedir(director);
  return 0;
}


