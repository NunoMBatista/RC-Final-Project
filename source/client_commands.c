#include <stdio.h>

void client_login(char *username, char *password){
    printf("Received request to login as client with username %s and password %s\n", username, password);
}

void list_classes(){
    printf("Received request to list classes\n");
}

// Student Only
void list_subscribed(){
    printf("Received request to list subscribed classes\n");
}

void subscribe_class(char *class_name){
    printf("Received request to subscribe to class %s\n", class_name);
}

// Professor Only
void create_class(char *class_name){
    printf("Received request to create class %s\n", class_name);
}

void send_message(char *class_name, char *message){
    printf("Received request to send message %s to class %s\n", message, class_name);
}