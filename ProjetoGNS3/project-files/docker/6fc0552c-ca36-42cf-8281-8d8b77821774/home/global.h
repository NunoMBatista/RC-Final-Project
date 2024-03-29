#ifndef GLOBAL_H
#define GLOBAL_H

#define BUFLEN 1024
#define MAX_REGISTERED_USERS 100

typedef struct user{
    char username[BUFLEN];
    char password[BUFLEN];
    char role[BUFLEN];
} User;

extern int registered_users_count;
extern User registered_users[MAX_REGISTERED_USERS];

#endif