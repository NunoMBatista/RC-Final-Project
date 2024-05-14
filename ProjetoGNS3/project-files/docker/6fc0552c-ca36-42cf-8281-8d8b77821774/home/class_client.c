#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>

#include "global.h"

// Socket file descriptor
int current_subscribed_classes = 0;
int client_socket;
int multicast_exit = 0;

// Flag to check if the user is logged in
int logged_in = 0; // 0 if not logged in, 1 if logged in as student, 2 if logged in as professor

char multicast_ips[MAX_SUBSCRIBED_CLASSES][16];
unsigned int last_assigned_port = FIRST_MULTICAST_PORT;

int join_multicast_group(char *multicast_address);
void *receive_multicast_messages(void *multicast_address);

void handle_sigint(int sig);

int main(int argc, char *argv[]){
    signal(SIGINT, handle_sigint);
    // Server's ip address
    char endServer[100]; 
    // Server's address structure containing IP and port
    struct sockaddr_in server_address;
    // Host entry structure containing server's name and IP address
    struct hostent *hostPtr;

    pthread_t class_threads[MAX_SUBSCRIBED_CLASSES];

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

        // Check if the message is a multicast message
        char *token = strtok(message_received, " ");
        if(strcmp(token, "ACCEPTED") == 0){
            // Get the multicast address
            token = strtok(NULL, "<");
            token[strlen(token) - 2] = '\0'; // -2 to account the newline and the '<'
            // Join the multicast group
            int multicast_socket = join_multicast_group(token);
            printf("-> Joined multicast group %s <-\n\n", token);

            // Create a thread to receive multicast messages
            pthread_create(&class_threads[current_subscribed_classes - 1], NULL, receive_multicast_messages, (void*) &multicast_socket);
        }

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

int join_multicast_group(char *multicast_address){
    // Create a socket for the multicast group
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){
        printf("<Socket creation failed>\n");
        exit(1);
    }


    // Let the socket reuse the address
    int reuse = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        perror("<setsockopt failed>\n");
        close(sockfd);
        exit(1);
    }

    unsigned int multicast_address_int = inet_addr(multicast_address);
    // Set the multicast_address_struct to the multicast address and port
    struct sockaddr_in multicast_address_struct;

    memset(&multicast_address_struct, 0, sizeof(multicast_address_struct));
    multicast_address_struct.sin_family = AF_INET; // IPv4
    multicast_address_struct.sin_addr.s_addr = htonl(INADDR_ANY); // Multicast address
    multicast_address_struct.sin_port = htons(FIRST_MULTICAST_PORT + multicast_address_int % 1000); // Port

    //multicast_address_struct.sin_port = htons(last_assigned_port); // Port
    
    last_assigned_port++; // Increment last_assigned_port for the next multicast group

    // Bind the socket to the multicast address
    if(bind(sockfd, (struct sockaddr*)&multicast_address_struct, sizeof(multicast_address_struct)) < 0){
        perror("<Bind failed>\n");
        close(sockfd);
        exit(1);
    }

    // Set the multicast group to join
    struct ip_mreq multicast_group;
    multicast_group.imr_multiaddr.s_addr = inet_addr(multicast_address);
    multicast_group.imr_interface.s_addr = htonl(INADDR_ANY);

    // Join the multicast group
    if(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_group, sizeof(multicast_group)) < 0){
        printf("<Join failed>\n");
        close(sockfd);
        exit(1);
    } 

    //multicast_ips[current_subscribed_classes] = multicast_address;
    strcpy(multicast_ips[current_subscribed_classes], multicast_address);
    current_subscribed_classes++;

    return sockfd;
}

void *receive_multicast_messages(void *multicast_address){
    int sockfd = *(int*) multicast_address;
    char message[BUFLEN];
    struct sockaddr_in sender_address;
    socklen_t sender_address_length = sizeof(sender_address);

    while(!multicast_exit){
        memset(message, 0, BUFLEN);
        if(recvfrom(sockfd, message, BUFLEN - 1, 0, (struct sockaddr*)&sender_address, &sender_address_length) < 0){
            perror("Failed to receive multicast group message\n");
            close(sockfd);
            exit(1);
        }
        printf("\n\n!!! MESSAGE RECEIVED FROM MULTICAST GROUP %s!!!\n  -> [%s]\n", (char*) multicast_address, message);
    
        if(logged_in == 1){
            printf("(student) $ ");
        }
        else if(logged_in == 2){
            printf("(professor) $ ");
        }
    }
}

void handle_sigint(int sig){
    // CLOSE MULTICAST GROUPS

    if(sig == SIGINT){
        printf("\nSHUTTING DOWN CLIENT\n");
        close(client_socket);
        exit(0);
    }
}