#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_USERS 100
#define MAX_NAME_LENGTH 10
#define MAX_PASS_LENGTH 20
#define PORT 12344

typedef struct {
    int socket;
    char name[MAX_NAME_LENGTH];
} Client;

Client active_users[MAX_USERS];
int num_users = 0;

void* handle_client(void* arg);
int authenticate_user(int client_socket, char* username, char* password);
int register_user(int client_socket, char* username, char* password);
void sendUserNamesToClient(int clientSocket);
void clientConnected(int client_socket, char* username);
void clientDisconnected(int client_socket);

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

    printf("Server listening on port %d\n", PORT);
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

        memset(buffer, 0, sizeof(buffer));

        // Receive the index from the client
        ssize_t bytesRead = recv(client_socket, buffer, sizeof(buffer) - 1, 0);


        if (strcmp(buffer, "exit") == 0) 
        {
            clientDisconnected(client_socket);
            return NULL;
        }

        choice = atoi(buffer); // Convert ascii code to actual int choice

        if (choice == 1) {
            // Handle login
            send(client_socket, "Enter username: ", 17, 0);
            recv(client_socket, username, MAX_NAME_LENGTH, 0);
            send(client_socket, "Enter password: ", 17, 0);
            recv(client_socket, password, MAX_PASS_LENGTH, 0);

            if (authenticate_user(client_socket, username, password)) {
                authed = 1;
                send(client_socket, ("Welcome back, %s! Choose a recipient:\n", username), 46, 0);
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
            if (register_user(client_socket, username, password)) {
                // Registration successful
                authed = 1;
                send(client_socket, ("Welcome, %s! Choose a recipient:\n", username), 54, 0);
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

    // User logged in!
    // Present list of users to chat to
    sendUserNamesToClient(client_socket);

    // Handle chat functionality
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesRead < 0)
        {
            exit(EXIT_FAILURE);
        } 
        else if (bytesRead == 0) {
            
            clientDisconnected(client_socket);
            return NULL;
        } 
        else if (strcmp(buffer, "exit") == 0) 
        {
            clientDisconnected(client_socket);
            return NULL;
        }
        // Process the received message and send it to the appropriate recipient(s)
        
    }

    // Close the client socket
    clientDisconnected(client_socket);
    return NULL;
}

// Login
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
                // Add to the active users list, notify server of login
                clientConnected(client_socket, username);
                break;
            }
        }
    }

    fclose(file);
    return auth;
}

int register_user(int client_socket, char* username, char* password) {
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

    // Add to the active users list, notify server of login
    clientConnected(client_socket, username);

    fclose(file);
    return 1;
}



void sendUserNamesToClient(int clientSocket) {
    FILE *file = fopen("users.txt", "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char *usernames[MAX_USERS];  // Array to store usernames
    int numUsers = 0;

    // Read, display, and store usernames from the file
    char line[MAX_NAME_LENGTH + MAX_PASS_LENGTH + 1];
    while (numUsers < MAX_USERS && fgets(line, sizeof(line), file)) {
        char *user = strtok(line, ":");
        if (user != NULL) {
            usernames[numUsers] = strdup(user);  // Store the username

            // Send username as an option
            char message[MAX_NAME_LENGTH + 32];
            memset(message, 0, strlen(message));
            
            snprintf(message, sizeof(message), "<LOGIN_LIST>%d. %s</LOGIN_LIST>", ++numUsers, user);
            //printf("%s\n", message);
            // send(clientSocket, message, strlen(message), 0);
            // printf("%d, %s\n", res, message);
        }   
    }

    fclose(file);

    if (numUsers == 0)
    {
        // Send message that no users exist.
        // We will notify you when they do.

        send(clientSocket, "No users exist yet!\nWill notify you when they do...\n", 53, 0);

        // Note call this function each time a new client registers, for all clients
        // who are not in an active chat
    }

    // Listen for client response
    int valid_res = 0;
    do
    {
        char buffer[2];
        int index;

        memset(buffer, 0, sizeof(buffer));

        // Receive the index from the client
        ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead == -1) {
            perror("Error receiving data from the client");
        } else if (bytesRead == 0) {
            
            clientDisconnected(clientSocket);
            return;
        } 
        else if (strcmp(buffer, "exit") == 0) 
        {
            clientDisconnected(clientSocket);
            return;
        }
        else {
            // Convert the received index to an integer
            index = atoi(buffer);

            if (index >= 1 && index <= numUsers) {
                // Send the corresponding username to the client
                valid_res = 1;
                char message[29 + strlen(usernames[index - 1])];
                snprintf(message, sizeof(message), "You are now chatting with %s", usernames[index - 1]);
                send(clientSocket, message, strlen(message), 0);
            } else {
                // Handle an invalid index
                send(clientSocket, "Invalid index", strlen("Invalid index"), 0);
            }
        }
        
    } while (!valid_res);
    

    // Free the allocated memory for usernames
    for (int i = 0; i < numUsers; i++) {
        free(usernames[i]);
    }
}

void clientDisconnected(int client_socket)
{
    printf("Client has disconnected.\n");
    close(client_socket);
    // TODO
    // remove client from active user list
    // notify anyone chatting with this client (sockfd) that the user logged off
    
}

void clientConnected(int client_socket, char* username)
{
    // add client from active user list
    // notify anyone chatting with this client (sockfd) that the user logged on
    printf("Client %s has connected!\n", username);
}