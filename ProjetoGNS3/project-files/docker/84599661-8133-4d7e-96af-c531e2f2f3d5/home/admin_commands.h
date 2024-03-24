#ifndef ADMIN_COMMANDS_H
#define ADMIN_COMMANDS_H

void add_user(char *username, char *password, char *role);
void remove_user(char *username);
void list_users();
void shutdown_server();
void admin_login(char *username, char *password);

#endif