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
#include "function.c"

// Informazioni del client
struct client_inf cl;

// Funzione che estrare da una directory il nome del file e il percorso
char** take_file_path(char* complete_path, char* result[2]){
    char* file_name = &strrchr(complete_path, '/')[1];     // Prende il nome del file (prende la stringa dall'ultimo slash e seleziona dal carattere da index 1 in poi)
    size_t length_path = file_name - complete_path;        // Lunghezza della stringa in cui salvare il percorso del file, senza il nome del file
    char path[length_path];                               // Crea l'array di caratteri (stringa) in cui salvare il percordo del file
    // Controlla se l'ultimo slash e' stato preso correttamente
    if(file_name != NULL && strcmp(file_name, "") != 0){
        strncpy(path, complete_path, length_path);        // Copia il percorso del file in results
        path[length_path] = '\0';                         // Aggiunge il terminatore, per indicare la fine della stringa
    } else {error("Error, missing slash in the path or file name not specified (try to run again the command and after -o or -f specify a correct path)");}
    result[0] = strdup(path);
    result[1] = strdup(file_name);
    return result;
}

// Funzione che crea la directory e il file in cui viene salvato il file preso dal server
void mod_read(){
    char path[BUFFER_SIZE];
    sprintf(path, "%s%s", cl.loacl_path, cl.loacl_file_name);
    FILE* file;
    if(directory_exist(cl.loacl_path) == 1 && directory_exist(path) == 1){
      cl.loacl_file_name = strdup(create_file(cl.loacl_file_name, cl.loacl_path));  
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
    if (strcmp(argv[1], "-w") == 0) { cl.write = 1; cl.read = 0;}
    else if(strcmp(argv[1], "-r") == 0) { cl.read = 1; cl.write = 0;}
    else if(strcmp(argv[1], "-l") == 0) { cl.ls_la = 1; }
    else{error("Modalita' non specificata");}
    // Salva l'ip_address del server
    if(strcmp(argv[2], "-a") == 0){cl.ip_server = argv[3];}
    else{error("Indirizzo del server non specificato");}
    // Salva la porta del server
    if(strcmp(argv[4], "-p") == 0) {cl.portno_server = atoi(argv[5]);}
    else {error("Porta del server non specificata");}
    // Controlla se e' stato specificato il comando -l, allora ferma la funzione in quanto non esistono altri argomenti come il local_file_name
    if(cl.ls_la == 1) {
        if(argc > 7) cl.remote_path = strdup(argv[7]);
        else cl.remote_path = "/";
        return;
    }
    // Salva il local path 
    if(strcmp(argv[6], "-f") == 0) {
        char* prova[2];
        char** path = take_file_path(argv[7], prova);
        cl.loacl_path = path[0];
        cl.loacl_file_name = path[1];
    }
    else {error("Percorso file non specificato");}
    // Salva le informazioni del remote_path
    if (argc >= 9) {if(strcmp(argv[8], "-o") == 0 && cl.ls_la == 0){
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
}

int main(int argc, char *argv[]){
    // Controlla se sono stati inseriti i dati sufficenti per poter comunicare con il server
    if(argc < 5){error("Insufficent arguments specified");}
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
    if(cl.ls_la == 0) sprintf(path, "%s%s", cl.loacl_path, cl.loacl_file_name);
    FILE* file;
    while(1){
        memset(&buffer, 0, BUFFER_SIZE);
        memset(&server_reply, 0, BUFFER_SIZE);
        if(i == 0 && cl.read == true) {strcpy(buffer, "read"); file = fopen(path, "w");}
        else if(i == 0 && cl.write == true) {strcpy(buffer, "write"); file = fopen(path, "r");}
        else if(i == 0 && cl.ls_la == true) {strcpy(buffer, "ls -la");}
        else if(i == 1) {
            strcpy(buffer, cl.remote_path);
            if (cl.ls_la == true){
                i = 3;
            }
        }
        else if(i == 2) {strcpy(buffer, cl.remote_file_name);}
        else if(cl.ls_la == false && cl.write == true){
            if(fgets(buffer, BUFFER_SIZE, file) == NULL) {strcpy(buffer, "end file");}
        }
        else{strcpy(buffer, "continua");}
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
        if(cl.read == true && i > 2){
            if(strcmp(server_reply, "end file") != 0) {
                fprintf(file, "%s", server_reply);
            }
        }
        else if(cl.ls_la == true && i > 2 && strcmp(server_reply, "end file") != 0){
            printf("%s", server_reply);
        }
        else if(cl.ls_la == true && i > 2 && strcmp(server_reply, "end file") == 0) break;
        if(strcmp(server_reply, "end file") == 0){ fclose(file); break; }
        if(strcmp(server_reply, "fopen") == 0) error("Errore con l'apertura del file del server (Mettere un nome fi un file valido)");
        i += 1;
    }
    if (cl.ls_la == false) fclose(file);
    close(sockfd);
    return 0;
}
