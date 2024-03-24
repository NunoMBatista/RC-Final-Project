#include <stdio.h>

void add_user(char *username, char *password, char *role){
    printf("Received request to add user %s with password %s and role %s\n", username, password, role);
}

void remove_user(char *username){
    printf("Received request to remove user %s\n", username);
}

void list_users(){
    printf("Received request to list users\n");
}

void shutdown_server(){
    printf("Received request to shutdown server\n");
}

void admin_login(char *username, char *password){
    printf("Received request to login as admin with username %s and password %s\n", username, password);
}