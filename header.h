// Dichiarazione delle librerie utilizzate
#include <stdio.h>
#include <sys/socket.h>         // Usato per la comunicazione con il server
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>         
#include <strings.h>            // Usato per lavorare con le stringhe
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdbool.h>            // Usato per usare i booleani
#include <unistd.h>
#include <pthread.h>            // Usato per istanziare Thread
#include <dirent.h>             // Usato per lavorare con le directory

#define BUFFER_SIZE 1024

// Struct in cui salvo l'indirizzo del client, il path specificato, il nome del file, la modalita' (se lettura o scrittua)
#ifndef CLIENT_INFORMATION
#define CLIENT_INFORMTAION

// Dichiarazione della struct
struct client_inf{
    char* ip_server;                      // Indirizzo IP del server
    int portno_server;                    // Porta del server
    // Percorso in cui devono essere presi o salvati i file in locale
    char* loacl_path;
    char* loacl_file_name;
    // Percoso in cui devono essere salvati o presi i file nel server
    char* remote_path;
    char* remote_file_name;
    // Modalita': lettura o scrittura
    bool read;
    bool write;
    // Commando ls -la
    bool ls_la;
};
#endif

// Struct utilizzata dal server per salvare la modalita', il path specificato e il nome del file
#ifndef SERVER_INFORMATION
#define SERVER_INFORMATION

// Dichiarazione della struct
struct msg{
    char* path;                          // Percorso specificato
    char* file_name;                     // Nome del file
    int mode;                            // Modalita' (0-scrittura o 1-lettura e 2-comando ls -la)
};
#endif

// Funzioni utilizzate sia in server.c e client.c

// Funzione che dato un percorso determina se tale percorso esiste
int directory_exist(char* path);
// 
// Funzione che dato un percorso e un nome di un file determina quanti file esistono nel percorso con quel nome
int count_file(char* path, char* file_name);
//
// Funzione che dato un percorso e un file crea il file nel percorso speficiato
char* create_file(char* file_name, char* path);