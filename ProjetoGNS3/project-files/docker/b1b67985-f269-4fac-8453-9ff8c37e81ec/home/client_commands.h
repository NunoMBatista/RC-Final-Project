#ifndef CLIENT_COMMANDS_H
#define CLIENT_COMMANDS_H

void client_login(char *username, char *password, int client_socket);
void list_classes(int client_socket);

// Student only
void list_subscribed(int client_socket);
void subscribe_class(char *class_name, int client_socket);

// Professor only
void create_class(char *class_name, int capacity, int client_socket);
void send_message(char *class_name, char *message, int client_socket);

#endif