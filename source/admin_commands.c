#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "global.h"

// LOGIN <username> <password>
int admin_login(char *username, char *password, int client_socket, struct sockaddr_in client_address, socklen_t client_address_len){
    char response[BUFLEN];
    //sprintf(response, "Received request to login as admin with username %s and password %s\n", username, password);
    //sendto(client_socket, response, strlen(response) + 1, 0, (struct sockaddr*) &client_address, client_address_len);

    // Check if the user is registered
    for(int i = 0; i < registered_users_count; i++){
        if(strcmp(username, registered_users[i].username) == 0 && strcmp(password, registered_users[i].password) == 0){
            // Send a message to the client indicating that the login was successful
            if(strcmp(registered_users[i].role, "administrador") == 0){
                sprintf(response, "OK\nLOGGED IN AS ADMIN\n");
                sendto(client_socket, response, strlen(response) + 1, 0, (struct sockaddr*) &client_address, client_address_len);
                return 1;
            }

            sprintf(response, "REJECTED, USER %s IS NOT AN ADMIN\n", username);
            sendto(client_socket, response, strlen(response) + 1, 0, (struct sockaddr*) &client_address, client_address_len);
            return 0;
        }
    }

    sprintf(response, "REJECTED, USER %s NOT FOUND\n", username);
    sendto(client_socket, response, strlen(response) + 1, 0, (struct sockaddr*) &client_address, client_address_len);
    return 0; 
}

// ADD_USER <username> <password> <role>
void add_user(char *username, char *password, char *role, int client_socket, struct sockaddr_in client_address, socklen_t client_address_len){
    char response[BUFLEN];
    sprintf(response, "Received request to add user %s with password %s and role %s\n", username, password, role);
    sendto(client_socket, response, strlen(response), 0, (struct sockaddr*) &client_address, client_address_len);
}

// DEL <username>
void remove_user(char *username, int client_socket, struct sockaddr_in client_address, socklen_t client_address_len){
    char response[BUFLEN];
    sprintf(response, "Received request to remove user %s\n", username);
    sendto(client_socket, response, strlen(response) + 1, 0, (struct sockaddr*) &client_address, client_address_len);
}

// LIST
void list_users(int client_socket, struct sockaddr_in client_address, socklen_t client_address_len){
    char response[BUFLEN];
    sprintf(response, "Received request to list users\n");
    sendto(client_socket, response, strlen(response) + 1, 0, (struct sockaddr*) &client_address, client_address_len);
}

// QUIT_SERVER
void shutdown_server(int client_socket, struct sockaddr_in client_address, socklen_t client_address_len){
    char response[BUFLEN];
    sprintf(response, "Received request to shutdown server\n");
    sendto(client_socket, response, strlen(response) + 1, 0, (struct sockaddr*) &client_address, client_address_len);
}
