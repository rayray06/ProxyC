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
    int descSockData;               // Descripteur de socket de communication D'echange de donnéee
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

    /*******
     *
     * Gestion des connexions
     *
     * *****/
    while (true)
    {

        // Attente connexion du client
        // Lorsque demande de connexion, creation d'une socket de communication avec le client
        descSockCOM = accept(descSockRDV, (struct sockaddr *)&from, &len);
        if (descSockCOM == -1)
        {
            perror("Erreur accept\n");
            exit(6);
        }
        // Création d'un processus enfant pour la nouvelle connexion
        pid_t pid = fork();

        if (pid == 0)
        {
            // Initialisation du message de bienvenue pour le client
            strcpy(writebuffer, "220 Connexion au proxy\n");
            write(descSockCOM, writebuffer, strlen(writebuffer));

            ssize_t n;
            bool etatconnecter = true; // Variable pour suivre l'état de la connexion client

            // Boucle pour lire les données envoyées par le client tant qu'il est connecté
            // Cette boucle est nécessaire pour traiter les informations lors de sa connexion
            while (etatconnecter)
            {
                n = read(descSockCOM, readbuffer, MAXBUFFERLEN - 1);

                if (n == -1)
                {
                    perror("Erreur de lecture du socket client");
                    etatconnecter = false; // Déconnexion de la boucle en cas de problème
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
                        // La commande USER est utilisée pour s'identifier auprès du serveur FTP.
                        // Elle est généralement suivie du nom d'utilisateur et du nom du serveur de la forme "USER username@servername".
                        // Cette section du code extrait le nom d'utilisateur et le nom du serveur à partir de la commande USER,
                        // puis tente de se connecter au serveur FTP spécifié avec le nom du serveur.
                        // Les échanges de données avec le serveur, tels que la lecture et l'envoi de messages, sont également gérés ici.
                        // En cas de problème de connexion au serveur, le programme affiche un message indiquant que le serveur a fermé la connexion,
                        // et la boucle de traitement des commandes du client est interrompue.

                        printf("Commande USER reçue : %s\n", readbuffer);
                        // Affiche la réception de la commande USER avec les données du client

                        char nomlogin[MAXBUFFERLEN];
                        char nomserveur[MAXBUFFERLEN];

                        // Extraction du nom d'utilisateur et du nom du serveur à partir de la commande USER
                        sscanf(readbuffer, "USER %[^@]@%s", nomlogin, nomserveur);
                        printf("Nom d'utilisateur : %s, Nom du serveur : %s\n", nomlogin, nomserveur);

                        // Tentative de connexion au serveur avec le nom du serveur extrait
                        int newres = connect2Server(nomserveur, "21", &descSockServer);
                        if (newres == -1)
                        {
                            printf("Le serveur a fermé la connexion\n");
                            etatconnecter = false; // Mettre fin à la boucle si le client ferme la connexion
                        }

                        // Échange de données avec le serveur après la connexion
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';
                        printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                        // Construction de la commande USER avec le nom d'utilisateur extrait
                        strcpy(writebuffer, "USER ");
                        strcat(writebuffer, nomlogin);
                        strcat(writebuffer, "\r\n\0");

                        printf("---> %s", writebuffer);
                        write(descSockServer, writebuffer, strlen(writebuffer));

                        // Échange de données avec le serveur après l'envoi de la commande USER
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';
                        printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                        // Envoi de la réponse du serveur au client sur la connexion de contrôle
                        write(descSockCOM, readbuffer, strlen(readbuffer));
                    }
                    else if (strncmp(readbuffer, "PASS", 4) == 0)
                    {

                        // La commande PASS est utilisée pour envoyer le mot de passe au serveur FTP après la commande USER.
                        // Dans cette section du code, le programme envoie la commande PASS au serveur FTP avec des caractères masqués,
                        // puis gère les échanges de données avec le serveur, tels que la lecture et l'envoi de messages.
                        // La réponse du serveur est également transmise au client sur la connexion de contrôle.
                        // Cette section est responsable de la gestion de la phase d'authentification avec le serveur FTP.

                        printf("Commande PASS reçue : PASS XXXX\n");
                        // Affiche la réception de la commande PASS avec des caractères masqués (XXXX)

                        // Envoi de la commande PASS au serveur
                        write(descSockServer, readbuffer, strlen(readbuffer));

                        // Échange de données avec le serveur après l'envoi de la commande PASS
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';
                        printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                        // Envoi de la réponse du serveur au client sur la connexion de contrôle
                        write(descSockCOM, readbuffer, strlen(readbuffer));
                    }
                    else if (strncmp(readbuffer, "PORT", 4) == 0)
                    {
                        // La commande PORT est utilisée pour spécifier l'adresse IP et le port sur lequel le client écoute pour les données en mode actif.
                        // Cette section du code extrait les composants de l'adresse IP et du port à partir de la commande PORT,
                        // puis établit une connexion avec le serveur de données en utilisant ces informations.
                        // Ensuite, elle envoie la commande PASV au serveur de contrôle, lit et traite la réponse,
                        // puis établit une connexion avec le serveur de données en mode passif.
                        // Finalement, une réponse de succès est envoyée au client sur la connexion de contrôle.

                        printf("Commande PORT reçue : %s\n", readbuffer);
                        // Affiche la réception de la commande PORT avec les informations de l'adresse IP et du port

                        int add1, add2, add3, add4, port1, port2;
                        char ip_address[INET_ADDRSTRLEN];
                        char port_str[6];

                        // Extraction des composants de l'adresse IP et du port de la commande PORT
                        sscanf(readbuffer, "PORT %d,%d,%d,%d,%d,%d", &add1, &add2, &add3, &add4, &port1, &port2);

                        // Construction de l'adresse IP et du port
                        snprintf(ip_address, sizeof(ip_address), "%d.%d.%d.%d", add1, add2, add3, add4);
                        snprintf(port_str, sizeof(port_str), "%d", (port1 << 8) + port2);

                        // Connexion au serveur de données en utilisant l'adresse IP et le port extraits
                        connect2Server(ip_address, port_str, &descSockData);

                        // Envoi de la commande PASV au serveur de contrôle
                        strcpy(writebuffer, "PASV");
                        strcat(writebuffer, "\r\n\0");

                        printf("---> %s", writebuffer);
                        write(descSockServer, writebuffer, strlen(writebuffer));

                        // Lecture de la réponse du serveur de contrôle après l'envoi de la commande PASV
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';
                        printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                        // Extraction des informations de l'adresse IP et du port du mode passif depuis la réponse du serveur
                        sscanf(readbuffer, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &add1, &add2, &add3, &add4, &port1, &port2);

                        // Construction de l'adresse IP et du port pour la connexion au serveur de données en mode passif
                        snprintf(ip_address, sizeof(ip_address), "%d.%d.%d.%d", add1, add2, add3, add4);
                        snprintf(port_str, sizeof(port_str), "%d", (port1 << 8) + port2);

                        // Connexion au serveur de données en mode passif
                        connect2Server(ip_address, port_str, &descSockServerData);

                        // Envoi de la réponse réussie au client sur la connexion de contrôle
                        strcpy(writebuffer, "200 PORT command successful");
                        strcat(writebuffer, "\r\n\0");

                        write(descSockCOM, writebuffer, strlen(writebuffer));
                    }
                    else if (strncmp(readbuffer, "LIST", 4) == 0)
                    {
                        // La commande LIST est utilisée pour demander au serveur FTP la liste des fichiers dans le répertoire courant.
                        // Cette section du code envoie la commande LIST au serveur de contrôle, lit et transmet la réponse du serveur au client sur la connexion de contrôle.
                        // Elle gère également le flux de données depuis la connexion de données (descSockServerData) vers la connexion de données du client (descSockData),
                        // permettant ainsi au client de recevoir la liste des fichiers. En cas d'erreur de lecture depuis la connexion de données,
                        // le programme affiche un message d'erreur et quitte avec le code d'erreur 3.
                        // En fin de traitement, une réponse supplémentaire du serveur (le cas échéant) est lue et transmise au client sur la connexion de contrôle.

                        printf("Commande LIST reçue : %s\n", readbuffer);
                        // Affiche la réception de la commande LIST

                        // Envoi de la commande LIST au serveur de contrôle
                        write(descSockServer, readbuffer, strlen(readbuffer));

                        // Lecture de la réponse du serveur de contrôle
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';
                        printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                        // Envoi de la réponse au client sur la connexion de contrôle
                        write(descSockCOM, readbuffer, strlen(readbuffer));

                        // Traitement du flux de données depuis la connexion de données (descSockServerData)
                        while ((ecode = read(descSockServerData, readbuffer, MAXBUFFERLEN)) > 0)
                        {
                            // Traitement des données selon les besoins, vous voudrez peut-être les envoyer à descSockCOM
                            readbuffer[ecode] = '\0';
                            printf("MESSAGE RECU DU SERVEUR DATA: %s\n|", readbuffer);
                            write(descSockData, readbuffer, strlen(readbuffer));
                        }

                        // Gestion d'une éventuelle erreur de lecture depuis la connexion de données
                        if (ecode == -1)
                        {
                            perror("Problème de lecture depuis la connexion de données\n");
                            exit(3);
                        }

                        // Envoi d'un CRLF supplémentaire pour indiquer la fin de la transmission des données sur descSockData
                        strcpy(readbuffer, "\r\n\0");
                        printf("MESSAGE RECU DU SERVEUR DATA: %s", readbuffer);
                        write(descSockData, readbuffer, strlen(readbuffer));

                        // Fermeture des connexions de données
                        close(descSockServerData);
                        close(descSockData);

                        // Lecture éventuelle de toute réponse supplémentaire du serveur de contrôle
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';

                        // Envoi de la réponse supplémentaire au client sur la connexion de contrôle
                        strcpy(writebuffer, readbuffer);
                        printf("MESSAGE RECU DU SERVEUR: %s", writebuffer);
                        write(descSockCOM, writebuffer, strlen(writebuffer));
                    }
                    else if (strncmp(readbuffer, "QUIT", 4) == 0)
                    {
                        printf("Commande QUIT reçue : %s\n", readbuffer);
                        // Affiche la réception de la commande QUIT

                        // La commande QUIT est utilisée pour demander la déconnexion du client du serveur FTP.
                        // Cette section du code envoie la commande QUIT au serveur de contrôle, lit et transmet la réponse du serveur au client sur la connexion de contrôle.
                        // Ensuite, elle ferme les connexions avec le serveur et le client, met fin à la boucle de traitement des commandes du client (etatconnecter = false),
                        // et termine le processus enfant. Cette commande permet une déconnexion propre du client du serveur FTP.

                        // Envoi de la commande QUIT au serveur de contrôle
                        write(descSockServer, readbuffer, strlen(readbuffer));

                        // Échange de données avec le serveur après l'envoi de la commande QUIT
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';
                        printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                        // Envoi de la réponse du serveur au client sur la connexion de contrôle
                        write(descSockCOM, readbuffer, strlen(readbuffer));

                        // Fermeture des connexions avec le serveur
                        close(descSockServer);
                        close(descSockCOM);

                        // Met fin à la boucle de connexion avec le client
                        etatconnecter = false;
                    }
                    else
                    {
                        printf("Commande reçue : %s\n", readbuffer);
                        // Affiche la réception d'une commande non traitée spécifiquement

                        // Cette section du code gère les commandes qui ne sont pas traitées explicitement dans les sections précédentes.
                        // Elle envoie la commande au serveur de contrôle, lit et transmet la réponse du serveur au client sur la connexion de contrôle.
                        // Cette partie du code est destinée à traiter les commandes génériques qui ne nécessitent pas un traitement spécifique.

                        // Envoi de la commande au serveur de contrôle
                        write(descSockServer, readbuffer, strlen(readbuffer));

                        // Échange de données avec le serveur après l'envoi de la commande non spécifiée
                        ecode = read(descSockServer, readbuffer, MAXBUFFERLEN);
                        if (ecode == -1)
                        {
                            perror("Problème de lecture\n");
                            exit(3);
                        }
                        readbuffer[ecode] = '\0';
                        printf("MESSAGE RECU DU SERVEUR: %s", readbuffer);

                        // Envoi de la réponse du serveur au client sur la connexion de contrôle
                        write(descSockCOM, readbuffer, strlen(readbuffer));
                    }
                }
            }
            close(descSockCOM);
            close(descSockRDV);
            exit(0);
        }
    }

    // Fermeture de la connexion
    close(descSockCOM);
    close(descSockRDV);
}
