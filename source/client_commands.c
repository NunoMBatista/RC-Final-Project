#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "global.h"

#define BUFLEN 1024

// LOGIN <username> <password>
User* client_login(char *username, char *password, int client_socket){
    char response[BUFLEN];
    User *user = (User*)malloc(sizeof(User));
    user->role[0] = '\0';

    //sprintf(response, "Received request to login as client with username %s and password %s\n", username, password);
    //write(client_socket, response, strlen(response));
    
    for(int i = 0; i < registered_users_count; i++){
        if(strcmp(username, registered_users[i].username) == 0 && strcmp(password, registered_users[i].password) == 0){
            strcpy(user->username, registered_users[i].username);
            strcpy(user->password, registered_users[i].password);
            strcpy(user->role, registered_users[i].role);
            break;
        }
    }
    
    if(user->role[0] == '\0'){
        sprintf(response, "REJECTED\n");
        write(client_socket, response, strlen(response) + 1);
    }
    else if(strcmp(user->role, "administrador") == 0){
        // Only students and professors can login using the TCP client
        sprintf(response, "<This user is an administrator>\nUse and UDP client to access the admin commands\n");
        write(client_socket, response, strlen(response) + 1);

        // Return an empty string to indicate that a role has not been assigned
        User *empty = (User*)malloc(sizeof(User));
        empty->role[0] = '\0';
        return empty;
    }

    // Return student role
    else if(strcmp(user->role, "aluno") == 0){
        sprintf(response, "OK\nLOGGED IN AS STUDENT\n");
        write(client_socket, response, strlen(response) + 1);
    }

    // Return professor role
    else if(strcmp(user->role, "professor") == 0){
        sprintf(response, "OK\nLOGGED IN AS PROFESSOR\n");
        write(client_socket, response, strlen(response) + 1);
    }

    return user;
}
 
// LIST_CLASSES
void list_classes(int client_socket){
    char response[BUFLEN];
    sprintf(response, "Received request to list classes\n");
    write(client_socket, response, strlen(response) + 1);
}

// Student Only

// LIST_SUBSCRIBED
void list_subscribed(int client_socket){
    char response[BUFLEN];
    sprintf(response, "Received request to list subscribed classes\n");
    write(client_socket, response, strlen(response) + 1);
}

// SUBSCRIBE <class_name>
void subscribe_class(char *class_name, int client_socket){
    char response[BUFLEN];
    sprintf(response, "Received request to subscribe to class %s\n", class_name);
    write(client_socket, response, strlen(response) + 1);
}

// Professor Only

// CREATE_CLASS <class_name> <capacity>
void create_class(char *class_name, int capacity, int client_socket){
    char response[BUFLEN];
    sprintf(response, "Received request to create class %s with capacity %d\n", class_name, capacity);
    write(client_socket, response, strlen(response) + 1);
}

// SEND <class_name> <message>
void send_message(char *class_name, char *message, int client_socket){
    char response[BUFLEN];
    sprintf(response, "Received request to send message %s to class %s\n", message, class_name);
    write(client_socket, response, strlen(response) + 1);
}