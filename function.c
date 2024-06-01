// Header in cui sono deifinite le funzioni da implementare
#include "header.h"

// Stampa i messaggi di errore, con il relativo errore
void error(char *msg){
    perror(msg);
    exit(0);
}

// Funizone che prende in input un percorso e controlla se tale percorso esiste
int directory_exist(char* path){
  struct stat stat_path;
  // Controlla se il percorso esiste
  if(stat(path, &stat_path) != 0) return 0;
  return 1;
}

// Funzione che prende in input un percorso e un numo di un file e controlla quanti file esiste nel percorso specificato con lo stesso nome del file_name passato in input
int count_file(char* path, char*file_name){
  DIR *dir;
  struct dirent *entry;
  int count = 0;
  // Apre la directory
  if((dir = opendir(path)) == NULL){
    perror("opendir");
    return -1;
  }
  // Itera sui file nella directory
  while((entry = readdir(dir)) != NULL){
    // Controlla il nome del file
    if(strcmp(entry->d_name, file_name) == 0) count++;
  }
  // Chiude la directory e restitusice il numero dei file con il nome del file_name passato in input
  closedir(dir);
  return count;
}

// Funzione che prende una directory e un file name e crea il file con il nome passato in input
char* create_file(char* file_name, char* path){
  char path_cp[BUFFER_SIZE];
  char* temp_name = strdup(file_name);
  int count = 0;
  while(true){
    char* org_file_name = strdup(file_name);
    count += count_file(path, temp_name);
    if(count_file(path, temp_name) == 0) break;
    char* temp = strtok(org_file_name, ".");
    sprintf(temp_name, "%s(%d)", temp, count);
    temp = strtok(NULL, "");
    sprintf(temp_name, "%s.%s", temp_name, temp);
  }
  file_name = strdup(temp_name);
  sprintf(path_cp, "%s%s", path, file_name);
  FILE* file = fopen(path_cp, "w");
  if(file == NULL) perror("fopen");
  fclose(file);
  return file_name;
}