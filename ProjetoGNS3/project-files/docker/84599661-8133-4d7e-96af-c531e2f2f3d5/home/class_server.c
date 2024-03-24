#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h> // socket(), bind(), recvfrom()
#include <unistd.h> // close()
#include <arpa/inet.h> // sockaddr_in, htons(), htonl(), inet_ntoa()
#include <pthread.h>
#include <signal.h>

#include "admin_commands.h"
#include "client_commands.h"

#define BUFLEN 1024

int tcp_socket;
int udp_socket;

// void pointer return type is used to run the functions in threads
void* handle_udp(void* udp_port_ptr);
void* handle_tcp(void* tcp_port_ptr);
void process_client(int client_socket);
void handle_sigint(int sig);

/* 
    The main function should create a socket that can receive both TCP and UDP messages.

    Execution syntax: ./class_server <PORTO_TURMAS> <PORTO_CONFIG> <configuration file>
*/
int main(int argc, char *argv[]){
    signal(SIGINT, handle_sigint);

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

    pthread_t udp_thread, tcp_thread;
    // Start listening for UDP messages
    pthread_create(&udp_thread, NULL, handle_udp, &PORTO_CONFIG);
    
    // Start listening for TCP messages
    pthread_create(&tcp_thread, NULL, handle_tcp, &PORTO_TURMAS);
    
    pthread_join(udp_thread, NULL);
    pthread_join(tcp_thread, NULL);

    return 0; 
}

void* handle_tcp(void* tcp_port_ptr){
    int tcp_port = *((int*) tcp_port_ptr);

    // Create socket for TCP messages
    // AF_INET: IPv4 IP
    // SOCK_STREAM: TCP socket type
    // 0: Use default protocol (TCP)
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Check if the socket was created successfully
    if(tcp_socket == -1){
        perror("Error creating TCP socket");
        exit(1);
    }

    // Create a sockaddr_in struct to store the server address and port
    struct sockaddr_in server_address;

    // Set every byte of server_address to 0 to ensure no garbage values are present
    memset(&server_address, 0, sizeof(server_address));

    // Set the server address family to IPv4
    server_address.sin_family = AF_INET;
    // Set the server port to the port passed as an argument
    server_address.sin_port = htons(tcp_port); // htons() converts host's byte order (unspecified) to network's byte order (big-endian)
    // Set the server address to any available address in the system (INADDR_ANY)
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // htonl() is the same as htons() but for 32-bit values

    // Bind the socket to the server address and port, which are stored in a sockaddr struct
    if(bind(tcp_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1){
        perror("Error binding TCP socket");
        close(tcp_socket);
        exit(1);
    }

    // Start listening for incoming connections
    if(listen(tcp_socket, 5) == -1){
        perror("Error listening for incoming connections");
        close(tcp_socket);
        exit(1);
    }

    char buffer[BUFLEN];
    while(1){
        // Clear the buffer before receiving a new message
        memset(buffer, 0, BUFLEN);

        // Accept a new connection
        // These variables must be declared inside the loop in TCP to avoid using the same memory address for different clients
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        // Create a new socket for the client and store the client's address in client_address
        int client_socket = accept(tcp_socket, (struct sockaddr*) &client_address, &client_address_len);
        if(client_socket == -1){
            perror("Error accepting connection");
            break;
        }

        // Fork a new process to handle the client
        pid_t pid = fork();
        if(pid == -1){
            perror("Error forking process");
            break;
        }

        if(pid == 0){
            // Close the copy of the server socket in the child process
            close(tcp_socket);

            // Process a client
            process_client(client_socket);

            // Exit the child process
            exit(0);
        }
        else{
            // Close the copy of the client socket in the parent process
            close(client_socket);
        }
    }
    close(tcp_socket);
    return NULL;
}

void process_client(int client_socket){
    int bytes_received = 0;
    char buffer[BUFLEN];
    // Clear the buffer before receiving a new message
    memset(buffer, 0, BUFLEN);

    while((bytes_received = read(client_socket, buffer, BUFLEN - 1)) > 0){
        buffer[bytes_received - 1] = '\0'; // Add null terminator to the end of the message
        if(bytes_received == -1){
            perror("Error reading message");
            break;
        }
        printf("Message received - %s\n", buffer);
        // Clear the buffer before receiving a new message
        memset(buffer, 0, BUFLEN);
    }
    if(bytes_received == 0){
        printf("Client disconnected\n");
    }
    else{
        perror("Error reading message");
    }

    close(client_socket);
}

void* handle_udp(void *udp_port_ptr){
    // Cast the void pointer to an integer
    int udp_port = *((int*) udp_port_ptr);

    // Create socket for UDP messages
    // AF_INET: IPv4 IP
    // SOCK_DGRAM: UDP socket type
    // 0: Use default protocol (UDP)
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

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

    // Bind the socket to the server address and port, which are stored in a sockaddr struct
    if(bind(udp_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1){
        perror("Error binding UDP socket");
        close(udp_socket);
        exit(1);
    }

    // After setting up and binding the socket, start listening for messages
    char buffer[BUFLEN];
    // Create a sockaddr_in struct to store the client's address
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    while(1){
        // Clear the buffer before receiving a new message
        memset(buffer, 0, BUFLEN);

        // Receive a message from the client and store the client's address in client_address
        // recvfrom() is a blocking function, meaning the program will wait until a message is received
        // BUFFLEN - 1 to leave space for the null terminator
        int bytes_received = recvfrom(udp_socket, buffer, BUFLEN - 1, 0, (struct sockaddr*) &client_address, &client_address_len);
        buffer[bytes_received-1] = '\0'; // Remove the newline character from the message
        if(bytes_received == -1){
            perror("Error receiving message");
            break;
        }
        
        printf("Message from %s:%d - %s\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), buffer); 

        // devolver ao cliente udp uma mensagem de log in, caso seja admin, deve poder aceder ao resto das funções (talvez isto tenha de ser feito fora do loop)
    }

    // Close the socket
    close(udp_socket);
    return NULL;
}

void handle_sigint(int sig){
    close(tcp_socket);
    close(udp_socket);
    printf("SHUTTING DOWN SERVER\n");
    exit(0);
}