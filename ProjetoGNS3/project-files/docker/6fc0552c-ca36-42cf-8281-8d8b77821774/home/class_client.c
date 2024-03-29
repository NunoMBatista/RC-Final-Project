#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include "global.h"

// Socket file descriptor
int client_socket;

void handle_sigint(int sig);

int main(int argc, char *argv[]){
    signal(SIGINT, handle_sigint);
    // Server's ip address
    char endServer[100]; 
    // Server's address structure containing IP and port
    struct sockaddr_in server_address;
    // Host entry structure containing server's name and IP address
    struct hostent *hostPtr;

    // Check if the number of arguments is correct
    if(argc != 3){
        printf("<Invalid syntax>\n[Correct Usage: ./class_client <server_address> <PORTO_TURMAS>]\n");
        return 1;
    }

    // Check port validity
    int PORTO_TURMAS = atoi(argv[2]);
    // Ports below 1024 are reserved for system services and ports above 65535 are invalid
    if(PORTO_TURMAS < 1024 || PORTO_TURMAS > 65535){
        printf("<Invalid port>\n[Port must be integers between 1024 and 65535]\n");
        return 1;
    }

    strcpy(endServer, argv[1]); // Copy server's ip address to endServer
    if((hostPtr = gethostbyname(endServer)) == 0){
        printf("<Address not found>\n[Address %s not found]\n", endServer);
        return 1;
    }

    // Clear server_address structure
    bzero((void*) &server_address, sizeof(server_address)); 
    // Set the server_address family to IPv4
    server_address.sin_family = AF_INET;
    // Set the server's ip address
    server_address.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr_list[0]))->s_addr;
    // Set the server's port to the port passed as argument
    server_address.sin_port = htons((short) PORTO_TURMAS);
    
    // Create a socket and store its file descriptor in client_socket
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        printf("<Socket creation failed>\n");
        return 1;
    }

    // Connect to the server whose address is in server_address
    if(connect(client_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0){
        printf("<Connection failed>\n");
        return 1;
    }

    char message_sent[BUFLEN];
    char message_received[BUFLEN];

    int bytes_received;
   
    char *console_string = "> "; // Character to be displayed in the console
    int logged_in = 0; // 0 if not logged in, 1 if logged in as student, 2 if logged in as professor
    while(1){
        // Clear the message_received and message_sent buffers
        memset(message_received, 0, BUFLEN);
        memset(message_sent, 0, BUFLEN);

        // Read the message from the server, BUFLEN - 1 to leave space for the null character
        bytes_received = read(client_socket, message_received, BUFLEN - 1);

        // Check if the user has logged in and change the console string accordingly
        if(logged_in == 0){
            if(strcmp(message_received, "OK\nLOGGED IN AS STUDENT\n") == 0){
                logged_in = 1;
                console_string = "(student) $ ";
            }
            else if(strcmp(message_received, "OK\nLOGGED IN AS PROFESSOR\n") == 0){
                logged_in = 2;
                console_string = "(professor) $ ";
            }
        }

        if(bytes_received == 0){
            printf("The server has shut down\n");
            break;
        }
        message_received[bytes_received - 1] = '\0';

        printf("%s\n", message_received);

        // Get user input and send it to the server
        printf("%s", console_string);
        
        fgets(message_sent, BUFLEN - 1, stdin);
        message_sent[strlen(message_sent) - 1] = '\0';
        // + 1 to include the null character
        write(client_socket, message_sent, strlen(message_sent) + 1);
    }

    close(client_socket);
    return 0; 
}

void handle_sigint(int sig){
    if(sig == SIGINT){
        printf("\nSHUTTING DOWN CLIENT\n");
        close(client_socket);
        exit(0);
    }
}