#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_USERS 100
#define MAX_NAME_LENGTH 50
#define MAX_PASS_LENGTH 50
#define PORT 12346

typedef struct {
    int socket;
    char name[MAX_NAME_LENGTH];
} Client;

Client active_users[MAX_USERS];
int num_users = 0;

void* handle_client(void* arg);
int authenticate_user(int client_socket, char* username, char* password);
int register_user(char* username, char* password);

int main() {
    int server_socket, client_socket, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(1);
    }

    // Bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        exit(1);
    }

    // Listen
    if (listen(server_socket, 5) < 0) {
        perror("Error listening");
        exit(1);
    }

    printf("Server listening on port %d", PORT);
    fflush(stdout);

    while (1) {
        // Accept a new connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Error accepting client connection");
            continue;
        }

        // Create a new thread to handle the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, &client_socket) != 0) {
            perror("Error creating thread for client");
            continue;
        }
    }

    close(server_socket);
    return 0;
}

void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    char username[MAX_NAME_LENGTH];
    char password[MAX_PASS_LENGTH];
    char buffer[1024];


    int authed; // Is this client logged in

    // Prompt the client for login or register until they're authenticated
    do
    {
        // Boolean: Is the user logged in? Repeat this loop UNTIL so!
        authed = 0;
        
        send(client_socket, "1. Login\n2. Register\nEnter your choice: ", 41, 0);
        int choice;
        recv(client_socket, &choice, sizeof(int), 0);
        choice = choice - '0'; // Convert ascii code to actual int choice

        if (choice == 1) {
            // Handle login
            send(client_socket, "Enter username: ", 17, 0);
            recv(client_socket, username, MAX_NAME_LENGTH, 0);
            send(client_socket, "Enter password: ", 17, 0);
            recv(client_socket, password, MAX_PASS_LENGTH, 0);

            if (authenticate_user(client_socket, username, password)) {
                authed = 1;
                send(client_socket, "Login successful. You can now start chatting.\n", 46, 0);
            } else {
                // Login failed
                send(client_socket, "Login failed. Please try again.\n", 32, 0);
               
            }
        } else if (choice == 2) {
            // Handle registration
            send(client_socket, "Enter username: ", 17, 0);
            recv(client_socket, username, MAX_NAME_LENGTH, 0);
            send(client_socket, "Enter password: ", 17, 0);
            recv(client_socket, password, MAX_PASS_LENGTH, 0);

            // Try to register the user, and then log them in. Return auth status
            if (register_user(username, password)) {
                // Registration successful
                authed = 1;
                send(client_socket, "Registration successful. You can now start chatting.\n", 54, 0);
            } else {
                // Registration failed
                send(client_socket, "Registration failed. Please try again.\n", 38, 0);
                
            }
        } else {
            // Invalid choice
            send(client_socket, "Invalid choice.\n", 16, 0);
        }

    }
    while (!authed);
    

    
    // Handle chat functionality
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recv(client_socket, buffer, sizeof(buffer), 0);

        // Process the received message and send it to the appropriate recipient(s)
        
    }

    // Close the client socket
    close(client_socket);
    return NULL;
}


int authenticate_user(int client_socket, char* username, char* password) {
    // Read users.txt and verify username and password
    FILE* file = fopen("users.txt", "r");
    if (file == NULL) {
        perror("Error opening users.txt");
        return 0;
    }

    char line[MAX_NAME_LENGTH + MAX_PASS_LENGTH + 2];
    int auth = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        char file_username[MAX_NAME_LENGTH];
        char file_password[MAX_PASS_LENGTH];
        if (sscanf(line, "%[^:]:%s", file_username, file_password) == 2) {
            if (strcmp(username, file_username) == 0 && strcmp(password, file_password) == 0) {
                auth = 1;
                break;
            }
        }
    }

    fclose(file);
    return auth;
}

int register_user(char* username, char* password) {
    // Add a new user to users.txt
    FILE* file = fopen("users.txt", "a");
    if (file == NULL) {
        perror("Error opening users.txt");
        return 0;
    }

    if (fprintf(file, "%s:%s\n", username, password) < 0) {
        perror("Error writing to users.txt");
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}
