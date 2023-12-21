#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    // Vérification du nombre d'arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s hostname\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Extraction du nom d'hôte à résoudre
    const char *hostname = argv[1];

    //Initialisation des structures pour la résolution DNS
    //structures addrinfo sont utilisées pour stocker des informations sur les adresses réseau
    //La structure hints est utilisée pour spécifier les critères de recherche lors de la résolution DNS.
    //Elle est initialisée avec memset pour s'assurer que toutes les valeurs sont initialisées à zéro.
    
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; //ai_family spécifie la famille d'adresses, ici AF_INET pour les adresses IPv4.
    // Utilisez AF_INET6 également pour IPv6
    hints.ai_socktype = SOCK_STREAM; //ai_socktype spécifie le type de socket, ici SOCK_STREAM pour un socket orienté connexion.

    // Résolution du nom d'hôte en adresses IP
    //getaddrinfo est appelée pour résoudre le nom d'hôte en une liste d'adresses réseau. Si la résolution échoue, le programme affiche un message d'erreur et se termine avec un code d'échec.
    if (getaddrinfo(hostname, NULL, &hints, &result) != 0) {
        fprintf(stderr, "Error while resolving server address\n");
        exit(EXIT_FAILURE);
    }

    // Parcours des résultats de la résolution
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        // Extraction de l'adresse IP
        struct sockaddr_in *addr = (struct sockaddr_in *)rp->ai_addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
        
        // Affichage de l'adresse IP
        printf("Adresse IP de %s: %s\n", hostname, ip);
    }

    // Libération de la mémoire allouée pour les résultats de la résolution
    freeaddrinfo(result);

    // Fin du programme avec succès
    return 0;
}
