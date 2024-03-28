#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <sys/socket.h> // socket(), bind(), recvfrom()
#include <unistd.h> // close()
#include <arpa/inet.h> // sockaddr_in, htons(), htonl(), inet_ntoa()
#include <pthread.h>
#include <signal.h>

#include "admin_commands.h"
#include "client_commands.h"
#include "global.h"

User registered_users[MAX_REGISTERED_USERS];
int registered_users_count = 0; 
int admin_logged_in = 0; // Boolean to check if an admin is logged in
pid_t main_process_id;

int tcp_socket;
int udp_socket;

// void pointer return type is used to run the functions in threads
void* handle_udp(void* udp_port_ptr);
void* handle_tcp(void* tcp_port_ptr);
void process_client(int client_socket);
void handle_sigint(int sig);
void interpret_client_command(char* command, int client_socket, User** user_info);
void interpret_admin_command(char* command, int client_socket, struct sockaddr_in client_address, socklen_t client_address_len);
int read_configuration_file(char* file_name);

/* 
    The main function should create a socket that can receive both TCP and UDP messages.
    Execution syntax: ./class_server <PORTO_TURMAS> <PORTO_CONFIG> <configuration file>
*/
int main(int argc, char *argv[]){
    signal(SIGINT, handle_sigint);
    main_process_id = getpid();

    // Check if the number of arguments is correct
    if(argc != 4){
        printf("<Invalid syntax>\n[Correct Usage: ./class_server <PORTO_TURMAS> <PORTO_CONFIG> <configuration file>]\n");
        return 1;
    }
    
    // Check port validity
    int PORTO_TURMAS = atoi(argv[1]);
    int PORTO_CONFIG = atoi(argv[2]);
    // Ports below 1024 are reserved for system services and ports above 65535 are invalid
    if(PORTO_TURMAS < 1024 || PORTO_TURMAS > 65535 || PORTO_CONFIG < 1024 || PORTO_CONFIG > 65535){
        printf("<Invalid port>\n[Port must be integers between 1024 and 65535]\n");
        return 1;
    }

    // Check if PORTO_TURMAS and PORTO_CONFIG are different
    if(PORTO_TURMAS == PORTO_CONFIG){
        printf("<Invalid port>\n[PORTO_TURMAS and PORTO_CONFIG must be different]\n");
        return 1;
    }

    // Check if the configuration file exists
    FILE *file = fopen(argv[3], "r");
    if(file == NULL){
        printf("<File not found>\n[File %s not found]\n", argv[3]);
        return 1;
    }
    fclose(file);

    registered_users_count = read_configuration_file(argv[3]);

    pthread_t udp_thread, tcp_thread;
    // Start listening for UDP messages
    pthread_create(&udp_thread, NULL, handle_udp, &PORTO_CONFIG);
    
    // Start listening for TCP messages
    pthread_create(&tcp_thread, NULL, handle_tcp, &PORTO_TURMAS);
    
    pthread_join(udp_thread, NULL);
    pthread_join(tcp_thread, NULL);

    return 0; 
}

void* handle_tcp(void* tcp_port_ptr){
    int tcp_port = *((int*) tcp_port_ptr);

    // Create socket for TCP messages
    // AF_INET: IPv4 IP
    // SOCK_STREAM: TCP socket type
    // 0: Use default protocol (TCP)
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Check if the socket was created successfully
    if(tcp_socket == -1){
        perror("Error creating TCP socket");
        exit(1);
    }

    // Create a sockaddr_in struct to store the server address and port
    struct sockaddr_in server_address;

    // Set every byte of server_address to 0 to ensure no garbage values are present
    memset(&server_address, 0, sizeof(server_address));

    // Set the server address family to IPv4
    server_address.sin_family = AF_INET;
    // Set the server port's to the port passed as argument
    server_address.sin_port = htons(tcp_port); // htons() converts host's byte order (unspecified) to network's byte order (big-endian)
    // Set the server address to any available address in the system (INADDR_ANY)
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // htonl() is the same as htons() but for 32-bit values

    // Bind the socket to the server address and port, which are stored in a sockaddr struct
    if(bind(tcp_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1){
        perror("Error binding TCP socket");
        close(tcp_socket);
        exit(1);
    }

    // Start listening for incoming connections
    if(listen(tcp_socket, 5) == -1){
        perror("Error listening for incoming connections");
        close(tcp_socket);
        exit(1);
    }

    char buffer[BUFLEN];
    while(1){
        // Clear the buffer before receiving a new message
        memset(buffer, 0, BUFLEN);

        // Accept a new connection
        // These variables must be declared inside the loop in TCP to avoid using the same memory address for different clients
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        // Create a new socket for the client and store the client's address in client_address
        int client_socket = accept(tcp_socket, (struct sockaddr*) &client_address, &client_address_len);
        if(client_socket == -1){
            perror("Error accepting connection");
            break;
        }

        // Fork a new process to handle the client
        pid_t pid = fork();
        if(pid == -1){
            perror("Error forking process");
            break;
        }

        if(pid == 0){
            // Close the copy of the server socket in the child process
            close(tcp_socket);

            // Process a client
            process_client(client_socket);

            // Exit the child process
            exit(0);
        }
        else{
            // Close the copy of the client socket in the parent process
            close(client_socket);
        }
    }
    close(tcp_socket);
    return NULL;
}

void process_client(int client_socket){
    write(client_socket, "-- Welcome to the class server, login as a student or a professor before using the commands --\n", 96);

    //char user_role[15]; // Store the user's role
    //user_role[0] = '\0';
    User *user_info = (User*) malloc(sizeof(User));
    user_info->role[0] = '\0';

    int bytes_received = 0;
    char buffer[BUFLEN];
    char response[BUFLEN + 1]; // + 1 to account for the "\n" suffix on the client's end

    // Clear the buffer and response before receiving a new message
    memset(buffer, 0, BUFLEN);
    memset(response, 0, BUFLEN);

    // Read until the client disconnects
    while((bytes_received = read(client_socket, buffer, BUFLEN - 1)) > 0){
        buffer[bytes_received - 1] = '\0'; // Add null terminator to the end of the message
        if(bytes_received == -1){
            perror("Error reading message\n");
            break;
        }

        printf("Message received - %s\n", buffer);

        // Interpret the command
        interpret_client_command(buffer, client_socket, &user_info);
        printf("user role after int: %s\n", user_info->role);
        
        // Clear the buffer and response before receiving a new message
        memset(buffer, 0, BUFLEN);
        memset(response, 0, BUFLEN);
    }
    if(bytes_received == 0){
        printf("Client disconnected\n");
    }
    else{
        perror("Error reading message\n");
    }

    close(client_socket);
}

void* handle_udp(void *udp_port_ptr){
    // Cast the void pointer to an integer
    int udp_port = *((int*) udp_port_ptr);
    int bytes_received = 0;

    // Create socket for UDP messages
    // AF_INET: IPv4 IP
    // SOCK_DGRAM: UDP socket type
    // 0: Use default protocol (UDP)
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // Check if the socket was created successfully
    if(udp_socket == -1){
        perror("Error creating UDP socket");
        exit(1);
    }

    // Create a sockaddr_in struct to store the server address and port
    struct sockaddr_in server_address;

    // Set every byte of server_address to 0 to ensure no garbage values are present
    memset(&server_address, 0, sizeof(server_address)); 
    // Set the server address family to IPv4
    server_address.sin_family = AF_INET;
    // Set the server port to the port passed as an argument
    server_address.sin_port = htons(udp_port); // htons() converts host's byte order (unspecified) to network's byte order (big-endian)
    // Set the server address to any available address in the system (INADDR_ANY)
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // htonl() is the same as htons() but for 32-bit values

    // Bind the socket to the server address and port, which are stored in a sockaddr struct
    if(bind(udp_socket, (struct sockaddr*) &server_address, sizeof(server_address)) == -1){
        perror("Error binding UDP socket");
        close(udp_socket);
        exit(1);
    }

    // After setting up and binding the socket, start listening for messages
    char buffer[BUFLEN];
    memset(buffer, 0, BUFLEN);

    // Create a sockaddr_in struct to store the client's address
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);

    while(1){
        // Clear the buffer before receiving a new message
        memset(buffer, 0, BUFLEN);

        // Receive a message from the client and store the client's address in client_address
        // recvfrom() is a blocking function, meaning the program will wait until a message is received
        // BUFFLEN - 1 to leave space for the null terminator
        bytes_received = recvfrom(udp_socket, buffer, BUFLEN - 1, 0, (struct sockaddr*) &client_address, &client_address_len);
        buffer[bytes_received-1] = '\0'; // Remove the newline character from the message
        if(bytes_received == -1){
            perror("Error receiving message");
            break;
        }

        printf("Message from %s:%d - %s\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port), buffer); 
        
        // sendto(udp_socket, "Message received\n", 17, 0, (struct sockaddr*) &client_address, client_address_len); // Send a message to the client to confirm the message was received

        // Interpret the command
        interpret_admin_command(buffer, udp_socket, client_address, client_address_len);
        // devolver ao cliente udp uma mensagem de log in, caso seja admin, deve poder aceder ao resto das funções (talvez isto tenha de ser feito fora do loop)
    }

    // Close the socket
    close(udp_socket);
    return NULL;
}

void handle_sigint(int sig){
    if(sig == SIGINT){
        close(tcp_socket);
        close(udp_socket);

        // If the process is the main process, print a message before exiting
        if(getpid() == main_process_id){
            printf("\nSHUTTING DOWN SERVER\n");
        }

        exit(0);
    }
}

// Double pointer is used to change the value of the user_info pointer in the main function when the user logs in
void interpret_client_command(char* command, int client_socket, User **user_info){
    char* token = strtok(command, " ");
    if(token == NULL){
        write(client_socket, "<Empty command>\n", 17);
        return;
    }
    
    if(strcmp("LOGIN", token) == 0){
        // Check if the user is already logged in
        if((*user_info)->role[0] != '\0'){
            write(client_socket, "<Already logged in>\n", 21);
            return;
        }

        char *error_message = "<Invalid command>\n Correct Usage: LOGIN <username> <password>\n\n";
        // Check if the command has the correct number of arguments
        char* username = strtok(NULL, " ");
        char* password = strtok(NULL, " ");
        if(username == NULL || password == NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        // Check if there are no more arguments
        if(strtok(NULL, " ") != NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        
        //User* user;

        *user_info = client_login(username, password, client_socket);
        //user_info = user; 
        return;
    }

    // Check if the user is logged in before executing any other command
    else if((*user_info)->role[0] == '\0'){
        write(client_socket, "<Not logged in>\n", 17);
        return;
    }

    if(strcmp("LIST_CLASSES", token) == 0){
        char *error_message = "<Invalid command>\n Correct Usage: LIST_CLASSES\n";
        // Check if the command has the correct number of arguments
        if(strtok(NULL, " ") != NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        list_classes(client_socket);
        return;
    }

    if(strcmp("LIST_SUBSCRIBED", token) == 0){
        if(strcmp((*user_info)->role, "student") != 0){
            write(client_socket, "<Unauthorized command>\nYou need to be a student to list subscribed classes\n", 74);
            return;
        }
        char *error_message = "<Invalid command>\n Correct Usage: LIST_SUBSCRIBED\n";
        // Check if the command has the correct number of arguments
        if(strtok(NULL, " ") != NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        list_subscribed(client_socket);
        return;
    }

    if(strcmp("SUBSCRIBE", token) == 0){
        if(strcmp((*user_info)->role, "student") != 0){
            write(client_socket, "<Unauthorized command>\nYou need to be a student to subscribe to a class\n", 70);
            return;
        }

        char *error_message = "<Invalid command>\n Correct Usage: SUBSCRIBE <class_name>\n";
        // Check if the command has the correct number of arguments
        char* class_name = strtok(NULL, " ");
        if(class_name == NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        // Check if there are no more arguments
        if(strtok(NULL, " ") != NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        subscribe_class(class_name, client_socket);
        return;
    }

    if(strcmp("CREATE_CLASS", token) == 0){
        if(strcmp((*user_info)->role, "professor") != 0){
            write(client_socket, "<Unauthorized command>\nYou need to be a professor to create a class\n", 69);
            return;
        }

        char *error_message = "<Invalid command>\n Correct Usage: CREATE_CLASS <class_name> <capacity>\n";
        // Check if the command has the correct number of arguments
        char* class_name = strtok(NULL, " ");
        char* capacity_str = strtok(NULL, " ");
        if(class_name == NULL || capacity_str == NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        // Check if there are no more arguments
        if(strtok(NULL, " ") != NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        int capacity = atoi(capacity_str);
        create_class(class_name, capacity, client_socket);
        return;
    }

    if(strcmp("SEND", token) == 0){
        if(strcmp((*user_info)->role, "professor") != 0){
            write(client_socket, "<Unauthorized command>\nYou need to be a professor to send a message to a class\n", 80);
            return;
        }
        char *error_message = "<Invalid command>\n Correct Usage: SEND <class_name> <message>\n";
        // Check if the command has the correct number of arguments
        char* class_name = strtok(NULL, " ");
        char* message = strtok(NULL, "");
        if(class_name == NULL || message == NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        // Check if there are no more arguments
        if(strtok(NULL, " ") != NULL){
            write(client_socket, error_message, strlen(error_message));
            return;
        }
        send_message(class_name, message, client_socket);
        return;
    }
    
    write(client_socket, "<Invalid command>\n", 19);
}

void interpret_admin_command(char* command, int client_socket, struct sockaddr_in client_address, socklen_t client_address_len){
    printf("Command: %s\n", command);   
    char *token = strtok(command, " ");
    if(token == NULL){
        sendto(client_socket, "<Invalid command>\n", 20, 0, (struct sockaddr*) &client_address, client_address_len);
        return;
    }

    if(strcmp("LOGIN", token) == 0){
        // Check if the admin is already logged in
        if(admin_logged_in == 1){
            sendto(client_socket, "<Already logged in as admin>\n", 30, 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }

        char *error_message = "<Invalid command>\nCorrect Usage: LOGIN <username> <password>\n\n";
        // Check if the command has the correct number of arguments
        char* username = strtok(NULL, " ");
        char* password = strtok(NULL, " ");
        if(username == NULL || password == NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }
        // Check if there are no more arguments
        if(strtok(NULL, " ") != NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }

        admin_logged_in = admin_login(username, password, client_socket, client_address, client_address_len);
        return;
    }

    if(admin_logged_in == 0){
        sendto(client_socket, "<Unauthorized command>\nYou need to be logged in as an admin to use this console\n", 81, 0, (struct sockaddr*) &client_address, client_address_len);
        return;
    }

    if(strcmp("ADD_USER", token) == 0){
        char *error_message = "<Invalid command>\nCorrect Usage: ADD_USER <username> <password> <role>\n\n";
        // Check if the command has the correct number of arguments
        char* username = strtok(NULL, " ");
        char* password = strtok(NULL, " ");
        char* role = strtok(NULL, " ");
        if(username == NULL || password == NULL || role == NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }
        // Check if there are no more arguments
        if(strtok(NULL, " ") != NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }
        add_user(username, password, role, client_socket, client_address, client_address_len);
        return;
    }

    if(strcmp("DEL", token) == 0){
        char *error_message = "<Invalid command>\nCorrect Usage: DEL <username>\n\n";
        // Check if the command has the correct number of arguments
        char* username = strtok(NULL, " ");
        if(username == NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }
        // Check if there are no more arguments
        if(strtok(NULL, " ") != NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }
        remove_user(username, client_socket, client_address, client_address_len);
        return;
    }

    if(strcmp("LIST", token) == 0){
        char *error_message = "<Invalid command>\nCorrect Usage: LIST\n\n";
        // Check if the command has the correct number of arguments
        if(strtok(NULL, " ") != NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }
        list_users(client_socket, client_address, client_address_len);
        return;
    }

    if(strcmp("QUIT_SERVER", token) == 0){
        char *error_message = "<Invalid command>\nCorrect Usage: QUIT_SERVER\n\n";
        // Check if the command has the correct number of arguments
        if(strtok(NULL, " ") != NULL){
            sendto(client_socket, error_message, strlen(error_message), 0, (struct sockaddr*) &client_address, client_address_len);
            return;
        }
        shutdown_server(client_socket, client_address, client_address_len);
        return;
    }

    sendto(client_socket, "<Invalid command>\n", 19, 0, (struct sockaddr*) &client_address, client_address_len);
}

int read_configuration_file(char* file_name){
    FILE *file = fopen(file_name, "r");
    if(file == NULL){
        perror("Error opening configuration file");
        exit(1);
    }

    char buffer[BUFLEN];
    int current_user = 0;

    // Read the file line by line
    while(fgets(buffer, BUFLEN, file) != NULL){
        char* token = strtok(buffer, ";"); // Split the line by the ';' character
        if(token == NULL){ // Skip empty lines
            printf("Empty line in configuration file\n");
            continue;
        }

        User* new_user = (User*) malloc(sizeof(User));
        strcpy(new_user->username, token);

        token = strtok(NULL, ";");
        if(token == NULL){
            printf("Missing password in configuration file line %s\n", buffer);
            free(new_user); // Skip lines with missing password
            continue;
        }
        strcpy(new_user->password, token);

        token = strtok(NULL, ";");
        if(token == NULL){ // Skip lines with missing role
            printf("Missing role in configuration file line %s\n", buffer);
            free(new_user);
            continue;
        }

        token[strcspn(token, "\n")] = 0; // Remove the newline character from the end of the string
        // Check if the role is valid
        if(strcmp(token, "administrador") != 0 && strcmp(token, "aluno") != 0 && strcmp(token, "professor") != 0){
            printf("Invalid role in configuration file line %s\n", buffer);
            free(new_user);
            continue;
        }
        strcpy(new_user->role, token);


        // Check if there are no more arguments
        if(strtok(NULL, ";") != NULL){
            printf("Invalid line in configuration file\n");
            exit(1);
        }

        // Check if the maximum number of registered users has been reached
        if(current_user >= MAX_REGISTERED_USERS){
            printf("Maximum number of registered users reached\n");
            break;
        }

        // Check for duplicate usernames
        for(int i = 0; i < current_user; i++){
            if(strcmp(registered_users[i].username, new_user->username) == 0){
                printf("Duplicate username in configuration file\n");
                free(new_user);
                continue;
            }
        }

        // Add the user to the list of registered users
        registered_users[current_user] = *new_user;
        current_user++;
    }
    fclose(file);
    return current_user;
}