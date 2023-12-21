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

#define OPCODE_RRQ      1
#define OPCODE_WRQ      2
#define OPCODE_DATA     3
#define OPCODE_ACK      4
#define OPCODE_ERROR    5
#define OPCODE_OACK     6

#define BLOCK_SIZE      65464
#define BUFFER_SIZE     BLOCK_SIZE + 2 // 2 bytes for opcode + 2 bytes for block id + BLOCK_SIZE bytes for data

// Fonction pour envoyer un fichier vers un serveur TFTP
void putFile(char *fileName, struct sockaddr *sock_addr, int sockfd) {
    // ...

    // Taille du paquet de requête WRQ
    int requestLength = (int)(2 + strlen(fileName) + 1 + 5 + 1 + 7 + 1);
    char buffer[BUFFER_SIZE];
    int sendSize;
    int recvSize;
    char opcode;
    int readFileSize;
    socklen_t addrSize;
    unsigned short blockId; // Entier non signé car il occupe 2 octets
    unsigned short blockCounter = 0;
    FILE *fileIn;

    struct sockaddr sock_in_addr;

    // Convertir BLOCK_SIZE en chaîne de caractères et l'ajouter à la requête
    sprintf(buffer, "%d", BLOCK_SIZE);
    requestLength += strlen(buffer);
    requestLength += 1;

    // Construction du paquet de requête WRQ
    buffer[0] = 0;
    buffer[1] = OPCODE_WRQ;
    strcpy(&buffer[2], fileName);
    strcpy(&buffer[2 + strlen(fileName) + 1], "octet");
    strcpy(&buffer[2 + strlen(fileName) + 1 + strlen("octet") + 1], "blksize");
    sprintf(&buffer[2 + strlen(fileName) + 1 + strlen("octet") + 1 + strlen("blksize") + 1], "%d", BLOCK_SIZE);

    int c = 0;
    printf("| ");
    while (buffer[c] != '\0' || c < requestLength) {
        if (c < 2) {
            printf("%d", buffer[c]);
        } else if (buffer[c] == 0) {
            printf(" | 0 | ");
        } else {
            printf("%c", buffer[c]);
        }
        c++;
    }

    printf("\n");

    // Ouverture du fichier en mode lecture
    if ((fileIn = fopen(fileName, "r")) == NULL) {
        printf("Error: unable to open file %s\n", fileName);
        return;
    }

    // Envoi du paquet de requête au serveur
    sendSize = sendto(sockfd, buffer, requestLength, 0, sock_addr, sizeof(struct sockaddr));
    if (sendSize == -1 || sendSize != requestLength) {
        printf("Error while sendto: %d\n", sendSize);
        fclose(fileIn);
        return;
    }

    // Réception et traitement de l'accusé de réception
    addrSize = sizeof(struct sockaddr);
    recvSize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, &sock_in_addr, &addrSize);
    if (recvSize == -1) {
        printf("Error while recvfrom: %d\n", recvSize);
        fclose(fileIn);
        return;
    }
    opcode = buffer[1];
    if (opcode == OPCODE_ERROR) {
        printf("Error opcode %s\n", &buffer[4]);
        fclose(fileIn);
        return;
    } else if (opcode != OPCODE_OACK) {
        printf("Error opcode %d\n", opcode);
        fclose(fileIn);
        return;
    }

    printf("Received ACK, sending file\n");

    blockCounter = 1;

    // Boucle pour lire et envoyer des blocs de données
    while (1) {
        readFileSize = fread(&buffer[4], 1, BLOCK_SIZE, fileIn);
        if (readFileSize <= 0) {
            printf("Error fread %d\n", readFileSize);
            fclose(fileIn);
            return;
        }

        // Construction du paquet de données
        buffer[0] = 0;
        buffer[1] = OPCODE_DATA;
        buffer[2] = (unsigned char)(blockCounter >> 8) & 0xff;
        buffer[3] = (unsigned char)(blockCounter & 0xff);

        // Envoi du paquet de données au serveur
        sendSize = sendto(sockfd, buffer, readFileSize + 4, 0, &sock_in_addr, sizeof(struct sockaddr));
        if (sendSize == -1 || sendSize != (readFileSize + 4)) {
            printf("Error while sendto: %d %d\n", sendSize, readFileSize + 4);
            fclose(fileIn);
            return;
        }

        printf("Block %d sent with %d bytes\n", blockCounter, readFileSize);

        // Réception de l'accusé de réception
        addrSize = sizeof(struct sockaddr);
        recvSize = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, &sock_in_addr, &addrSize);
        if (recvSize == -1) {
            printf("Error while recvfrom: %d\n", recvSize);
            fclose(fileIn);
            return;
        }
        opcode = buffer[1];
        if (opcode == OPCODE_ERROR) {
            printf("Error opcode %s\n", &buffer[4]);
            fclose(fileIn);
            return;
        }

        // Extraction de l'ID de bloc de l'accusé de réception
        blockId = (unsigned char)buffer[2] << 8 | (unsigned char)buffer[3];

        // Vérification si l'accusé de réception est un ACK et s'il correspond à l'ID de bloc attendu
        if (opcode != OPCODE_ACK /*|| blockId != blockCounter*/) {
            printf("Error opcode %d blockid: %d blockcounter: %d buff2: %x buff3: %x\n", opcode, blockId, blockCounter, (unsigned char)buffer[2], (unsigned char)buffer[3]);
            fclose(fileIn);
            return;
        }
        blockCounter++;

        // Vérification si le dernier bloc a moins de BLOCK_SIZE octets, indiquant la fin du fichier
        if (readFileSize != BLOCK_SIZE) {
            fclose(fileIn);
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
    char *token;
    char *host[2];
    char ipbuf[INET_ADDRSTRLEN];
    int sockfd = 0;

    // Initialisation des options réseau
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // Vérification des arguments de la ligne de commande
    if (argc != 3) {
        printf("Usage: putftp <host>:<port> <local file>\n");
        return 0;
    }

    host[1] = "69"; // Si le port n'est pas déterminé, utilisez le port par défaut 69

    token = strtok(args[1], ":");
    while (token != NULL) {
        host[sockfd++] = token;
        token = strtok(NULL, ":");
        if (sockfd == 2) {
            break;
        }
    }

    printf("Host: %s on port: %s, file: %s\n", host[0], host[1], args[2]);

    // Obtention des informations d'adresse pour l'hôte et le port spécifiés
    if ((s = getaddrinfo(host[0], host[1], &hints, &res)) != 0) {
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
    putFile(args[2], res->ai_addr, sockfd);

    // Nettoyage et fermeture de la socket
    close(sockfd);
    freeaddrinfo(res);
}
