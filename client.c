// Dichiarazione delle librerie
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
    // Funzione in cui viene salvato il percorso completo con il nome del file per poterlo aprire con il comando fopen
    char path[BUFFER_SIZE];
    sprintf(path, "%s%s", cl.loacl_path, cl.loacl_file_name);
    FILE* file;
    // Controlla se il percorso e la directory e esistono, se non esistono vengono create
    if(directory_exist(cl.loacl_path) == 1 && directory_exist(path) == 1){
      cl.loacl_file_name = strdup(create_file(cl.loacl_file_name, cl.loacl_path));      // Nel caso in cui il percorso esiste controlla se esiste il file e nel caso ne crea un altro con lo stesso nome
    }
    else{
        char* path_temp = strdup(cl.loacl_path);        // Copia il percorso
        char* temp = strtok(path_temp, "/");            // Eseuge lo split del '/'
        char* cp_path_temp = strdup(temp);              // Copia la prime parte del percorso
        // Controlla se e' arrivato alla fine del percorso
        while(temp != NULL){
            if(directory_exist(cp_path_temp) == 0) {mkdir(cp_path_temp, 0755);}     // Controlla se la directory esiste, nel caso la crea
            temp = strtok(NULL, "/");       // Effettua di nuovo lo split per passare al pezzo dopo
            sprintf(cp_path_temp, "%s/%s", cp_path_temp, temp);     // Viene aggiunto alla variabile in cui alla fine sara' salvato il percorso completo
        }
        // Apre il file e lo chiude per effettuare la creazione del file fisico
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
        if(argc > 7) cl.remote_path = strdup(argv[7]);      // Se e' stato specificata la cartella in cui usare il comando ls -la lo salva nella struct
        else cl.remote_path = "/";                          // Altrimenti salva '/' per indicare che il comando ls -la deve essere usato nella root dir
        return; // Fine funzione
    }
    // Salva il local path 
    if(strcmp(argv[6], "-f") == 0) {
        char* prova[2];     // Variabile in cui viene salvato il percorso e il nome del file
        char** path = take_file_path(argv[7], prova);
        cl.loacl_path = path[0];
        cl.loacl_file_name = path[1];
    }
    else {error("Percorso file non specificato");}
    // Salva le informazioni del remote_path
    if (argc >= 9) {if(strcmp(argv[8], "-o") == 0 && cl.ls_la == 0){
        char* prova[2];                // Variabile in cui viene salvato il percorso e il nome del file
        char** path = take_file_path(argv[9], prova);
        cl.remote_path = path[0];
        cl.remote_file_name = path[1];
    }} else{
        char* prova[2];
        char** path = take_file_path(argv[7], prova);
        cl.remote_path = path[0];
        cl.remote_file_name = path[1];
    }
    // Controlla se e' stato chiamato la modalita' '-r' nel caso fa in modo di creare il percorso e il file in cui salvare le informazioni se non esistenti
    if(cl.read == 1){mod_read();}
}

int main(int argc, char *argv[]){
    // Controlla se sono stati inseriti i dati sufficenti per poter comunicare con il server
    if(argc < 5){error("Insufficent arguments specified");}
    // Controlla se i dati inseriti sono corretti
    check_command(argc, argv);
    int sockfd;
    struct sockaddr_in serv_addr;
    // Buffer per i messaggi da inviare e da ricevere dal server
    char buffer[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];
    // Socket che crea la comunicazione con il server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);   // Settaggio del socket
    if (sockfd < 0) 
        error("ERROR opening socket");      // Controlla se il settaggio del socket e' andato a buon fine
    // Resetta la memoria della variabile utilizzata per salvare le informazioni del server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    // Tipologia della famiglia del socket del server
    serv_addr.sin_family = AF_INET; 
    // Associa al server address la porta presa in input
    serv_addr.sin_port = htons(cl.portno_server);
    // Imposta l'indirizzo IP
    if(inet_pton(AF_INET, cl.ip_server, &serv_addr.sin_addr) <= 0){error("Error converting IP address");}
    // Esegue la connessione, se la connessione non e' andata a buon fine stampa un errore
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    // Contatore utilizzato per indicare a quale messaggio e' arrivato il client
    int i = 0;
    // Variabile utilizzata per prendere il percorso completo del file
    char path[BUFFER_SIZE];
    // Controlla se e' stato specificato il comando -l
    if(cl.ls_la == 0) sprintf(path, "%s%s", cl.loacl_path, cl.loacl_file_name);
    FILE* file;
    // Ciclo per l'invio dei messaggi al server
    while(1){
        // Resetta la memoria delle variabili in cui vengono salvate il messaggio di invio al server e il messaggio di risposta
        memset(&buffer, 0, BUFFER_SIZE);
        memset(&server_reply, 0, BUFFER_SIZE);
        // Controlla quale modalita' e' stata specificata, nel caso invia il nome della modalita' al server
        if(i == 0 && cl.read == true) {strcpy(buffer, "read"); file = fopen(path, "w"); printf("File opened with mode -w\n");}
        else if(i == 0 && cl.write == true) {strcpy(buffer, "write"); file = fopen(path, "r"); printf("File opened woth mode -r\n");}
        else if(i == 0 && cl.ls_la == true) {strcpy(buffer, "ls -la"); printf("Command sent\n");}
        // Invia il percorso del server in/da cui prendere/salvare le informazioni
        else if(i == 1) {
            strcpy(buffer, cl.remote_path);
            printf("Remote path sent\n");
            if (cl.ls_la == true) { i = 3;        // Se e' stato specificato il comando ls -la allora al prossimo messaggio salta al ricevere direttamente le informazioni del comando ls -la
            printf("------------------------------\n");
            }  
      }
        // Invia al server il nome del file in/da cui prendere/salvare informazioni
        else if(i == 2) {strcpy(buffer, cl.remote_file_name); printf("Remote file_name sent\n");}
        // Controlla se e' stato specificato il comando '-w' nel caso manda il contenuto del file da inviare al server
        else if(cl.ls_la == false && cl.write == true){
            if(fgets(buffer, BUFFER_SIZE, file) == NULL) {strcpy(buffer, "end file"); printf("File sent\n");}
        }
        // Altrimenti invia un messaggio di risposta al server con 'continua' che indica che il client sta ricevendo informaizoni dal server in quanto e' in modalita' -r
        else{strcpy(buffer, "continua");}
        // Invia il messaggio al server e verifica se ci sono stati errori
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        // Riceve il messaggio di risposta dal server e controlla se ci sono stati errori
        int read_size;
        if ((read_size = recv(sockfd, server_reply, BUFFER_SIZE, 0)) < 0) {
            perror("Recv failed");
            break;
        }
        // Controlla se sono stati passati prima le informazioni necessarie per continuare il trasferimento in modalita' -r
        if(cl.read == true && i > 2){
            if(strcmp(server_reply, "end file") != 0) fprintf(file, "%s", server_reply);
        }
        // Controlla se sono stati passati tutte le informazioni necessarie per continuare e mostrare il contenute della directory specificata con il comando ls -la
        else if(cl.ls_la == true && i > 2 && strcmp(server_reply, "end file") != 0) printf("%s", server_reply);
        // Controlla se sono state stampate tutte le informazioni generate dal comando ls -la
        else if(cl.ls_la == true && i > 2 && strcmp(server_reply, "end file") == 0) break;
        // Controlla se e' finito il trasferimento del file
        if(strcmp(server_reply, "end file") == 0) { fclose(file); printf("File downloaded/uploaded\n"); break; }
        // Controlla se il server ha avuto problemi ad aprire il file
        if(strcmp(server_reply, "fopen") == 0) error("Errore con l'apertura del file del server (Mettere un nome fi un file valido)");
        // Incrementa il numero di messaggi
        i += 1;
    }
    // Chiude la comunicazione con il server
    close(sockfd);
    return 0;       // Fine funzione
}
