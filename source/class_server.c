#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h> // socket(), bind(), recvfrom()
#include <unistd.h> // close()
#include <arpa/inet.h> // sockaddr_in, htons(), htonl(), inet_ntoa()

#include "admin_commands.h"
#include "client_commands.h"

#define BUFLEN 1024

void handle_udp(int udp_port);
void handle_tcp(int tcp_port);

/* 
    The main function should create a socket that can receive both TCP and UDP messages.

    Execution syntax: ./class_server <PORTO_TURMAS> <PORTO_CONFIG> <configuration file>
*/
int main(int argc, char *argv[]){
    // Check if the number of arguments is correct
    if(argc != 4){
        printf("<Invalid syntax>\n[Correct Usage: ./class_server <PORTO_TURMAS> <PORTO_CONFIG> <configuration file>]\n");
        return 1;
    }
    
    // Check port validity
    int PORTO_TURMAS = atoi(argv[1]);
    int PORTO_CONFIG = atoi(argv[2]);
    // Ports below 1024 are reserved for system services and ports above 65535 are invalid
    if(PORTO_TURMAS < 1024 || PORTO_TURMAS > 65535 || PORTO_CONFIG < 1024 || PORTO_CONFIG > 65535){
        printf("<Invalid port>\n[Port must be integers between 1024 and 65535]\n");
        return 1;
    }

    // Check if PORTO_TURMAS and PORTO_CONFIG are different
    if(PORTO_TURMAS == PORTO_CONFIG){
        printf("<Invalid port>\n[PORTO_TURMAS and PORTO_CONFIG must be different]\n");
        return 1;
    }

    // Check if the configuration file exists
    FILE *file = fopen(argv[3], "r");
    if(file == NULL){
        printf("<File not found>\n[File %s not found]\n", argv[3]);
        return 1;
    }

    // Start listening for UDP messages
    
    return 0; 
}

void handle_udp(int udp_port){
    // Create socket for UDP messages

    // AF_INET: IPv4 IP
    // SOCK_DGRAM: UDP socket type
    // 0: Use default protocol (UDP)
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // Check if the socket was created successfully
    if(udp_socket == -1){
        perror("Error creating UDP socket");
        exit(1);
    }

    // Create a sockaddr_in struct to store the server address and port
    struct sockaddr_in server_address;

    // Set every byte of server_address to 0 to ensure no garbage values are present
    memset(&server_address, 0, sizeof(server_address)); 
    // Set the server address family to IPv4
    server_address.sin_family = AF_INET;
    // Set the server port to the port passed as an argument
    server_address.sin_port = htons(udp_port); // htons() converts host's byte order (unspecified) to network's byte order (big-endian)
    // Set the server address to any available address in the system (INADDR_ANY)
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // htonl() is the same as htons() but for 32-bit values

    // Bind the socket to the server address
    if(bind(udp_socket, (struct sockddr*)&server_address, sizeof(server_address)) == -1){
        perror("Error binding UDP socket");
        close(udp_socket);
        exit(1);
    }

    // After setting up and binding the socket, start listening for messages

    // AINDA NÃO IMPLEMENTADO KAKAAAKAKAKAKKAKAKAKAKA 
}