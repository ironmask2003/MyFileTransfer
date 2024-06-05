#include "../function.c"

// Variabili globali
int cont_cli = 0;   // Variabile che conta il numeri di client connessi
char* root_dir;     // Variabile in cui viene salvata la root_dir in cui verranno scritti o letti i file richiesti dall'utente 

// Funzione che se e' stata specificata la modalita' '-w' crea la directory e il file in cui salvare le informazioni
struct msg mod_write(struct msg cli_msg){
    // Variabile in cui viene salvato il percorso completo
    char path[BUFFER_SIZE];
    sprintf(path, "%s%s", cli_msg.path, cli_msg.file_name);
    // Controlla se il path esiste e se esiste anche il file all'interno della directory specificatas
    if(directory_exist(cli_msg.path) == 1 && directory_exist(path) == 1) cli_msg.file_name = strdup(create_file(cli_msg.file_name, cli_msg.path));
    // Se non esiste il percorso o il file crea il percorso dal punto in cui esiste e il file
    else{
        FILE* file;
        char* path_temp = strdup(cli_msg.path);     // Copia del percorso specificato
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

// Handler assegnato ad ogni client che esegue la connessione al server
void *handle_client(void *socket_desc) {
    // socket del client
    int new_socket = *(int*)socket_desc;
    // Variabile utilizzata per salvare i messaggi ricevuti dal client
    char buffer[BUFFER_SIZE];
    int read_size;
    int cont_msg = 0;       // Variabile utilizzata per contare i messaggi ricevuti dal client
    struct msg cli_msg;     // Struct utilizzata per salvare le informazioni per eseguire il trasferimento dei file
    FILE* file;
    FILE* fp;
    // Variabile in cui viene salvato il comando 'ls -la' con la directory specificata dal client
    char command_line[BUFFER_SIZE];
    // Ciclo che riceve i messaggi del client
    while((read_size = recv(new_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        // Primo messaggio (modalita' specificata dall'utente (lettura, scrittua o ls -la))
        if(cont_msg == 0) {
            if(strcmp(buffer, "read") == 0) cli_msg.mode = 1;
            else if(strcmp(buffer, "write") == 0) cli_msg.mode = 0;
            else cli_msg.mode = 2;
            printf("Client %d: Mod setted\n", cont_cli);
        }
        // Percorso da/in cui prendere/salvare i file nel server
        // E controlla se e' stato specificato il comando ls -la, nel caso salva il percorso di cui si vogliono le informazioni attraverso il comando
        if(cont_msg == 1){
            char* path = strdup(root_dir);
            char* temp = strtok(buffer, "/");
            cli_msg.path = malloc(BUFFER_SIZE);
            while(temp != NULL){
                sprintf(path, "%s/%s", path, temp);
                if(directory_exist(path) == 0) {mkdir(path, 0755);}
                temp = strtok(NULL, "/");
            }
            sprintf(cli_msg.path, "%s/", path);
            sprintf(command_line, "ls -la %s", path);
            fp = popen(command_line, "r");
            if(cli_msg.mode == 2) {cont_msg = 3;}
            else printf("Client %d: Directory setted\n", cont_cli);
        }
        // Nome del file da/in cui prendere/salvare il contenuto da inviare/ricevuto
        if(cont_msg == 2){
            cli_msg.file_name = strdup(buffer);
            char path[BUFFER_SIZE];
            sprintf(path, "%s%s", cli_msg.path, cli_msg.file_name);
            if(cli_msg.mode == 1) file = fopen(path, "r");
            else{
                cli_msg = mod_write(cli_msg);
                char path_after[BUFFER_SIZE];
                sprintf(path_after, "%s%s", cli_msg.path, cli_msg.file_name);
                file = fopen(path_after, "w");
            }
            if(file == NULL) strcpy(buffer, "fopen");
            printf("Client %d: File opened\n", cont_cli);
        }
        // Una volta presi tutte le informazioni gestisce cosa fare in base alla modalita' specificata
        if(cont_msg > 2){
            if(cli_msg.mode == 0 && strcmp(buffer, "end file") != 0) fprintf(file, "%s", buffer);
            else if(cli_msg.mode == 0) {fclose(file); printf("Client %d: File saved\n", cont_cli); }
            else if(cli_msg.mode == 1){
                if(fgets(buffer, BUFFER_SIZE, file) == NULL) {
                    memset(&buffer, 0, BUFFER_SIZE);
                    strcpy(buffer, "end file");
                    printf("Client %d: File sendend\n", cont_cli);
                    fclose(file);
                }
            }
            else if(cli_msg.mode == 2){
                if(fgets(buffer, sizeof(buffer)-1, fp) == NULL) {
                    memset(&buffer, 0, BUFFER_SIZE);
                    strcpy(buffer, "end file");
                    printf("Client %d: Command executed\n", cont_cli);
                    pclose(fp);
                }
            }
        }
        // Incrementa il numero di messaggi inviati al client
        cont_msg++;
        // Invio della risposta al client
        if(send(new_socket, buffer, strlen(buffer), 0) < 0) printf("Errore invio messaggio al client\n");
    }
    // Controlla se il client si e' disconesso o c'è stato un problema nella ricezione del messaggio da parte del client
    if(read_size == 0) {
        printf("Client disconnected\n");
        fflush(stdout);
    } else if(read_size == -1) {
        error("recv failed");
    }
    // Decrementa il numero di client connessi al momento al server
    cont_cli -= 1;
    // Chiude la socket e libera la memoria dei malloc
    free(cli_msg.path);
    close(new_socket);
    free(socket_desc);
    return NULL;        // Fine funzione
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
        cont_cli += 1;    // Incrementa il numero dei client connessi al mommento al server
        // Crea un thread per ogni client che si connette
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            return 1;
        }
        printf("Handler assigned\n");
    }
    // Controlla se il client e' riuscito a connettersi correttamente
    if (newsockfd < 0) {
        perror("Accept failed");
        close(sockfd);
        return 1;
    }
    // Chiude la comunicazione 
    close(sockfd);
    return 0;       // Fine funzione
}
