// Dichiarazione delle librerie
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>
#include "header.h"

#define BUFFER_SIZE 1024

// Informazioni del client
struct client_inf cl;

// Stampa i messaggi di errore, con il relativo errore
void error(char *msg){
    perror(msg);
    exit(0);
}

// Funzione che estrare da una directory il nome del file e il percorso
char** take_file_path(char* complete_path, char* result[2]){
    char* file_name = &strrchr(complete_path, '/')[1];     // Prende il nome del file (prende la stringa dall'ultimo slash e seleziona dal carattere da index 1 in poi)
    size_t length_path = file_name - complete_path;        // Lunghezza della stringa in cui salvare il percorso del file, senza il nome del file
    char path[length_path];                               // Crea l'array di caratteri (stringa) in cui salvare il percordo del file
    // Controlla se l'ultimo slash e' stato preso correttamente
    if(file_name != NULL){
        strncpy(path, complete_path, length_path);        // Copia il percorso del file in results
        path[length_path] = '\0';                         // Aggiunge il terminatore, per indicare la fine della stringa
    } else {error("Error, missing slash in the path (try to run again the command and after -o or -f specify a correct path)");}
    result[0] = strdup(path);
    result[1] = strdup(file_name);
    return result;
}

// Funzione che controlla se la directory presa in input esiste o no
int directory_exist(char *path) {
    struct stat statbuf;
    // Controlla se il percorso esiste e ottiene le informazioni sul file
    if (stat(path, &statbuf) != 0) {
        // Il percorso non esiste o c'è stato un errore
        return 0;
    }
    // Restituisce 1 indicando che il percorso specificato esiste
    return 1;
}

// Funzione per contare i file con un nome specifico in una directory
int count_files_with_name(const char *directory, const char *filename) {
    DIR *dir;
    struct dirent *entry;
    int count = 0;
    // Apri la directory
    if ((dir = opendir(directory)) == NULL) {
        perror("opendir");
        return -1;
    }
    // Itera sui file nella directory
    while ((entry = readdir(dir)) != NULL) {
        // Confronta il nome del file
        if (strcmp(entry->d_name, filename) == 0) {
            count++;
        }
    }
    // Chiudi la directory
    closedir(dir);
    return count;
}

// Funzione che crea la directory e il file in cui viene salvato il file preso dal server
void mod_read(){
    char path[BUFFER_SIZE];
    sprintf(path, "%s%s", cl.loacl_path, cl.loacl_file_name);
    int count = 0;
    FILE* file;
    if(directory_exist(cl.loacl_path) == 1 && directory_exist(path) == 1){
        char path_cp[BUFFER_SIZE];
        while(true){
            count += count_files_with_name(cl.loacl_path, cl.loacl_file_name);
            if(count_files_with_name(cl.loacl_path, cl.loacl_file_name) == 0){break;}
            char* temp = strtok(cl.loacl_file_name, ".");
            char ccc[BUFFER_SIZE];
            sprintf(ccc, "%s(%d)", temp, count);
            temp = strtok(NULL, "");
            sprintf(ccc, "%s.%s", ccc, temp);
            cl.loacl_file_name = strdup(ccc);
            sprintf(path_cp, "%s%s", cl.loacl_path, cl.loacl_file_name);
        }
        file = fopen(path_cp, "w");
        if(file == NULL) error("File not found");
        fclose(file);
    }
    else{
        char* path_temp = strdup(cl.loacl_path);
        char* temp = strtok(path_temp, "/");
        char* cp_path_temp = strdup(temp);
        while(temp != NULL){
            if(directory_exist(cp_path_temp) == 0) {mkdir(cp_path_temp, 0755);}
            temp = strtok(NULL, "/");
            sprintf(cp_path_temp, "%s/%s", cp_path_temp, temp);
        }
        file = fopen(path, "w");
        if(file == NULL) error("File not found");
        fclose(file);
    }
}

// Funzione che controlla gli argomenti specificati e se sono corretti quelli inseriti
void check_command(int argc, char *argv[]){
    // Controllo della modalita' specificata dal client
    if (strcmp(argv[1], "-w") == 0) { cl.write = 1; cl.read = 0; }
    else if(strcmp(argv[1], "-r") == 0) { cl.read = 1; cl.write = 0; }
    else{error("Modalita' non specificata");}
    // Salva l'ip_address del server
    if(strcmp(argv[2], "-a") == 0){cl.ip_server = argv[3];}
    else{error("Indirizzo del server non specificato");}
    // Salva la porta del server
    if(strcmp(argv[4], "-p") == 0) {cl.portno_server = atoi(argv[5]);}
    else {error("Porta del server non specificata");}
    // Salva il local path 
    if(strcmp(argv[6], "-f") == 0) {
        char* prova[2];
        char** path = take_file_path(argv[7], prova);
        cl.loacl_path = path[0];
        cl.loacl_file_name = path[1];
    }
    else {error("Percorso file non specificato");}
    // Salva le informazioni del remote_path
    if (argc >= 9) {if(strcmp(argv[8], "-o") == 0){
        char* prova[2];
        char** path = take_file_path(argv[9], prova);
        cl.remote_path = path[0];
        cl.remote_file_name = path[1];
    }} else{
        char* prova[2];
        char** path = take_file_path(argv[7], prova);
        cl.remote_path = path[0];
        cl.remote_file_name = path[1];
    }
    if(cl.read == 1){mod_read();}
    return;
}

int main(int argc, char *argv[]){
    // Controlla se sono stati inseriti i dati sufficenti per poter comunicare con il server
    if(argc < 7){error("Insufficent arguments specified");}
    // Controlla se i dati inseriti sono corretti
    check_command(argc, argv);
    int sockfd;
    struct sockaddr_in serv_addr;
    // Buffer per i messaggi da inviare al server
    char buffer[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   // Settaggio del socket
    if (sockfd < 0) 
        error("ERROR opening socket");      // Controlla se il settaggio del socket e' andato a buon fine
    memset(&serv_addr, 0, sizeof(serv_addr));
    // Tipologia della famiglia del socket del server
    serv_addr.sin_family = AF_INET; 
    // Associa al server address la porta presa in input
    serv_addr.sin_port = htons(cl.portno_server);
    // Imposta l'indirizzo IP
    if(inet_pton(AF_INET, cl.ip_server, &serv_addr.sin_addr) <= 0){error("Errore nella conversione dell'indirizzo IP");}
    // Esegue la connessione, se la connessione non e' andata a buon fine stampa un errore
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    int i = 0;
    char path[BUFFER_SIZE];
    sprintf(path, "%s%s", cl.loacl_path, cl.loacl_file_name);
    FILE* file;
    while(1){
        memset(&buffer, 0, BUFFER_SIZE);
        memset(&server_reply, 0, BUFFER_SIZE);
        if(i == 0 && cl.read == true) {strcpy(buffer, "read"); file = fopen(path, "w");}
        else if(i == 0 && cl.write == true) {strcpy(buffer, "write"); file = fopen(path, "r");}
        else if(i == 1) {strcpy(buffer, cl.remote_path);}
        else if(i == 2) {strcpy(buffer, cl.remote_file_name);}
        else if(i == 3) {strcpy(buffer, "end information");}
        else{
            if(cl.write == 1){
                if(fgets(buffer, BUFFER_SIZE, file) == NULL) {strcpy(buffer, "end file");}
            }
            else{strcpy(buffer, "continua");}
        }
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        int read_size;
        if ((read_size = recv(sockfd, server_reply, BUFFER_SIZE, 0)) < 0) {
            perror("Recv failed");
            break;
        }
        if(cl.read == 1 && i > 3){
            if(strcmp(server_reply, "end file") != 0) {
                fprintf(file, "%s", server_reply);
                printf("%s\n", server_reply);
            }
        }
        if(strcmp(server_reply, "end file") == 0){
            fclose(file);
            break;
        }
        i += 1;
    }
    fclose(file);
    close(sockfd);
    return 0;
}