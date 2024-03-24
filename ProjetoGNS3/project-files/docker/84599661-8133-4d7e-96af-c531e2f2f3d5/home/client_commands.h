#ifndef CLIENT_COMMANDS_H
#define CLIENT_COMMANDS_H

void client_login(char *username, char *password);
void list_classes();

// Student only
void list_subscribed();
void subscribe_class(char *class_name);

// Professor only
void create_class(char *class_name, int capacity);
void send_message(char *class_name, char *message);

#endif