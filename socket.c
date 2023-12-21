#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>


int main(int argc,char *args[]){
	    int s;
    struct addrinfo hints;
    struct addrinfo *res;
    char PORT[10];
    char ipbuf[INET_ADDRSTRLEN];
    int sockfd;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (argc != 2) {
        printf("Usage: <host>\n");
        return 0;
    }
    printf("Host: %s\n", args[1]);
    sprintf(PORT, "%d", 1069); // For now, manually set the port to use

    // Get address information for the specified host and port
    if ((s = getaddrinfo(args[1], PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // Convert the server address to a human-readable format
    inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, ipbuf, sizeof(ipbuf));
    printf("Connecting to server: %s\n", ipbuf);

    // Create a UDP socket
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        printf("Error while creating socket\n");
    }

}