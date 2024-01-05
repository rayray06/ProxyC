#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "./simpleSocketAPI.h"

#define SERVADDR "127.0.0.1" // Définition de l'adresse IP d'écoute
#define SERVPORT "12346"     // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1          // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024    // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64        // Taille d'un nom de machine
#define MAXPORTLEN 64        // Taille d'un numéro de port

int main()
{
    // Partie Serveur (La partie qui communique avec notre client (ftp 127.0.0.1 Port))
    int ecode;                      // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];    // Adresse du serveur
    char serverPort[MAXPORTLEN];    // Port du server
    int descSockRDV;                // Descripteur de socket de rendez-vous
    int descSockCOM;                // Descripteur de socket de communication
    int descSockData;               // Descripteur de socket de communication
    int descSockServer;             // Descripteur de la socket utiliser en tant que client
    int descSockServerData;         // Descripteur de la socket utiliser en tant que client
    struct addrinfo hints;          // Contrôle la fonction getaddrinfo
    struct addrinfo *res;           // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo; // Informations sur la connexion de RDV
    struct sockaddr_storage from;   // Informations sur le client connecté
    socklen_t len;                  // Variable utilisée pour stocker les
                                    // longueurs des structures de socket
    char writebuffer[MAXBUFFERLEN]; // Tampon de communication entre le client et le serveur
    char readbuffer[MAXBUFFERLEN];  // Tampon de communication entre le client et le serveur

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1)
    {
        perror("Erreur création socket RDV\n");
        exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;     // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_family = AF_INET;       // seules les adresses IPv4 seront présentées par
                                     // la fonction getaddrinfo

    // Récupération des informations du serveur
    ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
    if (ecode)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ecode));
        exit(1);
    }
    // Publication de la socket
    ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
    if (ecode == -1)
    {
        perror("Erreur liaison de la socket de RDV");
        exit(3);
    }
    // Nous n'avons plus besoin de cette liste chainée addrinfo
    freeaddrinfo(res);

    // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
    len = sizeof(struct sockaddr_storage);
    ecode = getsockname(descSockRDV, (struct sockaddr *)&myinfo, &len);
    if (ecode == -1)
    {
        perror("SERVEUR: getsockname");
        exit(4);
    }
    ecode = getnameinfo((struct sockaddr *)&myinfo, sizeof(myinfo), serverAddr, MAXHOSTLEN,
                        serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ecode != 0)
    {
        fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
        exit(4);
    }
    printf("L'adresse d'ecoute est: %s\n", serverAddr);
    printf("Le port d'ecoute est: %s\n", serverPort);

    // Definition de la taille du tampon contenant les demandes de connexion
    ecode = listen(descSockRDV, LISTENLEN);
    if (ecode == -1)
    {
        perror("Erreur initialisation buffer d'écoute");
        exit(5);
    }

    len = sizeof(struct sockaddr_storage);
    // Attente connexion du client
    // Lorsque demande de connexion, creation d'une socket de communication avec le client
    descSockCOM = accept(descSockRDV, (struct sockaddr *)&from, &len);
    if (descSockCOM == -1)
    {
        perror("Erreur accept\n");
        exit(6);
    }
    // Echange de données avec le client connecté

    /*****
     * Testez de mettre 220 devant BLABLABLA ...
     * **/
    strcpy(writebuffer, "220 Connexion au proxy\n");
    write(descSockCOM, writebuffer, strlen(writebuffer));

    /*******
     *
     * A vous de continuer !
     *
     * *****/

    /*******
     *
     * Recevoir la commande  USER nomlogin@nomserveur
     *
     * *****/
    ssize_t n;
    bool etatconnecter = true; // Variable pour voir l'état de la connexion client

    // boucle pour lire les données envoyées par le client tant qu'il est connecté
    // cette boucle est nessesaire pour traité les information lors de sa connection
    while (etatconnecter)
    {
        n = read(descSockCOM, readbuffer, MAXBUFFERLEN - 1);

        if (n == -1)
        {
            perror("Erreur de lecture du socket client");
            etatconnecter = false; // deconexion de la boucle si il y a un probleme
        }
        else if (n == 0)
        {
            printf("Le client a fermé la connexion\n");
            etatconnecter = false; // Mettre fin à la boucle si le client ferme la connexion
        }
        else
        {
            readbuffer[n] = '\0';

            if (strncmp(readbuffer, "USER", 4) == 0)
            {
                printf("Commande USER reçue : %s\n", readbuffer);
                char nomlogin[MAXBUFFERLEN];
                char nomserveur[MAXBUFFERLEN];

                sscanf(readbuffer, "USER %[^@]@%s", nomlogin, nomserveur);
                printf("Nom d'utilisateur : %s, Nom du serveur : %s\n", nomlogin, nomserveur);

                int newres = connect2Server(nomserveur, "21", &descSockServer);
                if (newres == -1)
                {
                    printf("Le serveur a fermé la connexion\n");
                    etatconnecter = false; // Mettre fin à la boucle si le client ferme la connexion
                }

                // Echange de donneés avec le serveur
                ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                if (ecode == -1)
                {
                    perror("Problème de lecture\n");
                    exit(3);
                }
                readbuffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);
                // write(descSockServer, writebuffer, strlen(writebuffer));

                strcpy(writebuffer, "USER ");
                strcat(writebuffer, nomlogin);
                strcat(writebuffer, "\r\n\0");

                printf("---> %s", writebuffer);
                write(descSockServer, writebuffer, strlen(writebuffer));

                // Echange de donneés avec le serveur
                ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                if (ecode == -1)
                {
                    perror("Problème de lecture\n");
                    exit(3);
                }
                readbuffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);
                write(descSockCOM, readbuffer, strlen(readbuffer));
            }
            else if (strncmp(readbuffer, "PASS", 4) == 0)
            {

                printf("Commande PASS reçue : PASS XXXX\n");

                write(descSockServer, readbuffer, strlen(readbuffer));

                // Echange de donneés avec le serveur
                ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                if (ecode == -1)
                {
                    perror("Problème de lecture\n");
                    exit(3);
                }
                readbuffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);
                write(descSockCOM, readbuffer, strlen(readbuffer));
            }
            else if (strncmp(readbuffer, "PORT", 4) == 0)
            {
                printf("Commande PORT reçue : %s\n", readbuffer);

                int add1, add2, add3, add4, port1, port2;
                char ip_address[INET_ADDRSTRLEN];
                char port_str[6];

                // Use proper format specifiers for integers in sscanf
                sscanf(readbuffer, "PORT %d,%d,%d,%d,%d,%d", &add1, &add2, &add3, &add4, &port1, &port2);

                snprintf(ip_address, sizeof(ip_address), "%d.%d.%d.%d", add1, add2, add3, add4);
                snprintf(port_str, sizeof(port_str), "%d", (port1 << 8) + port2);

                connect2Server(ip_address, port_str, &descSockData);

                strcpy(writebuffer, "PASV");
                strcat(writebuffer, "\r\n\0");

                printf("---> %s", writebuffer);
                write(descSockServer, writebuffer, strlen(writebuffer));

                // Echange de donneés avec le serveur
                ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                if (ecode == -1)
                {
                    perror("Problème de lecture\n");
                    exit(3);
                }
                readbuffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                // Use proper format specifiers for integers in sscanf
                sscanf(readbuffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &add1, &add2, &add3, &add4, &port1, &port2);

                snprintf(ip_address, sizeof(ip_address), "%d.%d.%d.%d", add1, add2, add3, add4);
                snprintf(port_str, sizeof(port_str), "%d", (port1 << 8) + port2);

                connect2Server(ip_address, port_str, &descSockServerData);

                strcpy(writebuffer, "200 PORT command successful");
                strcat(writebuffer, "\r\n\0");

                write(descSockCOM, writebuffer, strlen(writebuffer));
            }
            else if (strncmp(readbuffer, "LIST", 4) == 0)
            {
                printf("Commande LIST reçue : %s\n", readbuffer);

                // Send the LIST command to the server
                write(descSockServer, readbuffer, strlen(readbuffer));

                // Read the response from the server
                ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                if (ecode == -1)
                {
                    perror("Problème de lecture\n");
                    exit(3);
                }
                readbuffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                // Send the response to the client on the control connection
                write(descSockCOM, readbuffer, strlen(readbuffer));

                // Now, handle the data flow from the data connection (descSockServerData)
                while ((ecode = read(descSockServerData, readbuffer, MAXBUFFERLEN)) > 0)
                {
                    // Process the data as needed, you might want to send it to descSockCOM
                    printf("MESSAGE RECU DU SERVEUR DATA: %s", readbuffer);
                    write(descSockData, readbuffer, ecode);
                }
                printf("MESSAGE RECU DU SERVEUR DATA: %s", readbuffer);
                write(descSockData, readbuffer, ecode);

                if (ecode == -1)
                {
                    perror("Problème de lecture depuis la connexion de données\n");
                    exit(3);
                }

                // Optionally, read any additional response from the server
                ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                if (ecode == -1)
                {
                    perror("Problème de lecture\n");
                    exit(3);
                }
                readbuffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                // Send the additional response to the client on the control connection
                write(descSockCOM, readbuffer, strlen(readbuffer));
            }
            else
            {
                printf("Commande reçue : %s\n", readbuffer);

                write(descSockServer, readbuffer, strlen(readbuffer));

                // Echange de donneés avec le serveur
                ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                if (ecode == -1)
                {
                    perror("Problème de lecture\n");
                    exit(3);
                }
                readbuffer[ecode] = '\0';
                printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);
                write(descSockCOM, readbuffer, strlen(readbuffer));
            }
        }
    }

    // serveur www.debian.org/CD/http-ftp/#mirrors
    //  login : anonymous
    //  mdp:toto@tata.com

    // Fermeture de la connexion
    close(descSockCOM);
    close(descSockRDV);
}
