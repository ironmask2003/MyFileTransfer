#include "../header.h"

#define BUFFER_SIZE 1024

/* Per lanciarli: prima /server 127.0.0.1 123456 root_path/, poi ./client localhost 123456 */ 
int cont_cli = 0;
char* root_dir;   // Variabile in cui viene salvata la root_dir in cui verranno scritti o letti i file richiesti dall'utente 

// Stampa i messaggi di errore, con il relativo errore
void error(char *msg){
    perror(msg);
    exit(1);
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

struct msg mod_write(struct msg cli_msg){
    char path[BUFFER_SIZE];
    sprintf(path, "%s/%s", cli_msg.path, cli_msg.file_name);
    int count = 0;
    FILE* file;
    if(directory_exist(cli_msg.path) == 1 && directory_exist(path) == 1){
        char path_cp[BUFFER_SIZE];
        while(true){
            count += count_files_with_name(cli_msg.path, cli_msg.file_name);
            if(count_files_with_name(cli_msg.path, cli_msg.file_name) == 0){break;}
            char* temp = strtok(cli_msg.file_name, ".");
            char ccc[BUFFER_SIZE];
            sprintf(ccc, "%s(%d)", temp, count);
            temp = strtok(NULL, "");
            sprintf(ccc, "%s.%s", ccc, temp);
            cli_msg.file_name = strdup(ccc);
            sprintf(path_cp, "%s/%s", cli_msg.path, cli_msg.file_name);
        }
        file = fopen(path_cp, "w");
        if(file == NULL ) error("File not found");
        fclose(file);
    }
    else{
        char* path_temp = strdup(cli_msg.path);
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
    return cli_msg;
}

void *handle_client(void *socket_desc) {
    int new_socket = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;
    int cont_msg = 0;
    struct msg cli_msg;
    FILE* file;

    while((read_size = recv(new_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        if(cont_msg == 0) cli_msg.mode = strdup(buffer);
        if(cont_msg == 1){
            char* path = strdup(root_dir);
            char* temp = strtok(buffer, "/");
            while(temp != NULL){
                sprintf(path, "%s/%s", path, temp);
                if(directory_exist(path) == 0) {mkdir(path, 0755);}
                temp = strtok(NULL, "/");
            }
            cli_msg.path = strdup(path);
        }
        if(cont_msg == 2){
            cli_msg.file_name = strdup(buffer);
            char path[1024];
            sprintf(path, "%s/%s", cli_msg.path, cli_msg.file_name);
            printf("%s\n", path);
            if(strcmp(cli_msg.mode, "read") == 0) file = fopen(path, "r");
            else{
                cli_msg = mod_write(cli_msg);
                char path_after[BUFFER_SIZE];
                sprintf(path_after, "%s/%s", cli_msg.path, cli_msg.file_name);
                file = fopen(path_after, "w");
            }
            if(file == NULL) error("File not found");
        }
        if(cont_msg > 2){
            memset(&buffer, 0, BUFFER_SIZE);
            if(strcmp(cli_msg.mode, "write") == 0 && strcmp(buffer, "end file") != 0){
                fprintf(file, "%s", buffer);
            }
            else if(strcmp(cli_msg.mode, "write") == 0) fclose(file);
            else if(strcmp(cli_msg.mode, "read") == 0){
                if(fgets(buffer, BUFFER_SIZE, file) == NULL) {
                    memset(&buffer, 0, BUFFER_SIZE);
                    strcpy(buffer, "end file");
                    fclose(file);
                }
            }
        }
        printf("Client %d: %s\n", cont_cli, buffer);
        cont_msg++;
        send(new_socket, buffer, strlen(buffer), 0);
    }

    if(read_size == 0) {
        printf("Client disconnected\n");
        fflush(stdout);
    } else if(read_size == -1) {
        error("recv failed");
    }

    close(new_socket);
    free(socket_desc);
    return NULL;
}

int main(int argc, char *argv[]){
    int sockfd, newsockfd, portno, clilen, *new_sock;
    pthread_t thread_id;
    // Variabile utilizzata per il settare alcune funzione del socket (esempio SO_REUSEADDR) se il valore e' impostato a 1 allora la funzione indicata viene attivata
    int opt = 1;
    char* ip_address; // Variabile in cui viene salvato l'indirizzo IP in cui si collega il server
    struct sockaddr_in serv_addr, cli_addr;   // Server e client address
    // Controlla se gli argomenti specificati sono tutti
    if (argc < 4) {
        fprintf(stderr,"ERROR, no port (or ip_address or root_dir) no provided\n");
        exit(1);
    }
    ip_address = argv[1];    // Assegnazione dell'indirizzo IP specificato in input alla variabile ip_address
    portno = atoi(argv[2]);  // Assegnazione della porta in cui collegarsi specificato in input alla variabile portno
    root_dir = argv[3];  // Assegnazione della root_directory in cui salvare salvare o leggere i file su richiesta del client
    if(directory_exist(root_dir) == 2) error("Specificare una directory");
    // Se la directory non esiste viene creata
    else if(directory_exist(root_dir) == 0) mkdir(root_dir, 0755);
    // Libera la memoria dal puntatore della variabile serv_addr
    memset(&serv_addr, 0, sizeof(serv_addr)); /*  erase  the deta in the  memory writing zeros*/ 
    // Imposta il socker con la tipologia
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // Controlla se il socket e' stato creato correttamente
    if(sockfd < 0) error("ERROR opening socket");
    // Assegna la porta al socket
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    // Conversione dell'indirizzo IP in binario
    if(inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0) error("Errore nella conversione dell'indirizzo IP");
    // Imposta l'opzione SO_REUSEADDR per il socket in modo che e' possibile riutilizzare l'indirzzo che e' stato impostato
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) error("setsockopt");
    if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) error("ERROR on binding");
    // Marca il socket come passive, ovvero pronto a ricevere richieste mediante una accept
    listen(sockfd, 5);    // L'intero indica il numero massimo della coda
    clilen = sizeof(cli_addr);
    // Prende il messaggio che riceve da un client che si connette al server
    while ((newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen))) {
        printf("Connection accepted\n");
        new_sock = malloc(1);
        *new_sock = newsockfd;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            cont_cli += 1;
            perror("Could not create thread");
            free(new_sock);
            return 1;
        }
        printf("Handler assigned\n");
    }
    if (newsockfd < 0) {
        perror("Accept failed");
        close(sockfd);
        return 1;
    }
    close(sockfd);
    // Fine funzione
    return 0; 
}
