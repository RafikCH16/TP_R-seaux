#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

// Définition des opcodes du protocole TFTP
#define OPCODE_RRQ      1
#define OPCODE_WRQ      2
#define OPCODE_DATA     3
#define OPCODE_ACK      4
#define OPCODE_ERROR    5

// Taille du tampon pour stocker les données
#define BUFFER_SIZE     516

// Fonction pour récupérer un fichier depuis un serveur TFTP
void getFile(char *fileName, struct sockaddr *sock_addr, int sockfd) {
    // ...

    // Taille du paquet de requête RRQ
    int requestLength = (int)(2 + strlen(fileName) + 1 + 5 + 1);
    char buffer[BUFFER_SIZE];
    int sendSize;
    int recvSize;
    char opcode;
    socklen_t addrSize;
    unsigned short blockId; // Entier non signé car il occupe 2 octets
    unsigned short blockCounter = 0;
    FILE *fileOut;

    struct sockaddr sock_in_addr;

    // Construction du paquet de requête RRQ
    buffer[0] = 0;
    buffer[1] = OPCODE_RRQ;
    strcpy(&buffer[2], fileName);
    strcpy(&buffer[2 + strlen(fileName) + 1], "octet");

    // Ouverture du fichier en mode écriture et lecture
    if ((fileOut = fopen(fileName, "w+")) == NULL) {
        printf("Error: unable to open file %s\n", fileName);
        return;
    }

    // Envoi du paquet de requête au serveur
    sendSize = sendto(sockfd, buffer, requestLength, 0, sock_addr, sizeof(struct sockaddr));
    if (sendSize == -1 || sendSize != requestLength) {
        printf("Error while sendto: %d\n", sendSize);
        return;
    }

    // Réception et traitement des paquets de données
    while (1) {
        addrSize = sizeof(struct sockaddr);
        recvSize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, &sock_in_addr, &addrSize);
        if (recvSize == -1) {
            printf("Error while recvfrom: %d\n", recvSize);
        }
        opcode = buffer[1];

        // Traitement des paquets d'erreur
        if (opcode == OPCODE_ERROR) {
            printf("Error opcode %s\n", &buffer[4]);
            return;
        }

        // Extraction de l'ID de bloc du paquet reçu
        blockId = (unsigned short)buffer[2] | (unsigned short)buffer[3];

        requestLength = 4; // Définir la longueur de la requête à 2 octets

        // Construction et envoi d'un paquet ACK
        buffer[0] = 0;
        buffer[1] = OPCODE_ACK;
        sendSize = sendto(sockfd, buffer, requestLength, 0, &sock_in_addr, sizeof(struct sockaddr));

        printf("Received block id %d: %d bytes\n", blockId, recvSize - 4);

        // Écriture des données dans le fichier si les conditions sont remplies
        if ((recvSize - 4 > 0) && (blockId > blockCounter)) {
            fwrite(&buffer[4], 1, recvSize - 4, fileOut);
            blockCounter++;
        }

        // Vérification si le dernier bloc a moins de 512 octets, indiquant la fin du fichier
        if (recvSize - 4 < 512) {
            fclose(fileOut);
            return;
        }
    }
}

// Fonction principale
int main(int argc, char *args[]) {
    // ...
    int s;
    struct addrinfo hints;
    struct addrinfo *res;
    char PORT[10];
    char ipbuf[INET_ADDRSTRLEN];
    int sockfd;

    // Initialisation des options réseau
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // Vérification des arguments de la ligne de commande
    if (argc != 3) {
        printf("Usage: getftp <host> <local file>\n");
        return 0;
    }

    printf("Host: %s, File: %s\n", args[1], args[2]);
    sprintf(PORT, "%d", 1069); // Pour l'instant, le port est défini manuellement

    // Obtention des informations d'adresse pour l'hôte et le port spécifiés
    if ((s = getaddrinfo(args[1], PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // Conversion de l'adresse du serveur en format lisible par l'homme
    inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, ipbuf, sizeof(ipbuf));
    printf("Connecting to server: %s\n", ipbuf);

    // Création d'une socket UDP
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        printf("Error while creating socket\n");
    }

    // Exécution du transfert de fichier
    getFile(args[2], res->ai_addr, sockfd);

    // Nettoyage et fermeture de la socket
    close(sockfd);
    freeaddrinfo(res);
}
