// Dichiarazione delle librerie utilizzate
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

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
};
#endif

// Struct utilizzata dal server per salvare la modalita', il path specificato e il nome del file
#ifndef SERVER_INFORMATION
#define SERVER_INFORMATION

// Dichiarazione della struct
struct msg{
    char* path;                          // Percorso specificato
    char* file_name;                     // Nome del file
    char* mode;                          // Modalita' (scrittura o lettura)
};
#endif