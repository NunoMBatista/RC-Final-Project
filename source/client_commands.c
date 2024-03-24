#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFLEN 1024

// LOGIN <username> <password>
void client_login(char *username, char *password, int client_socket){
    char response[BUFLEN];
    sprintf(response, "Received request to login as client with username %s and password %s\n", username, password);
    write(client_socket, response, strlen(response));
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