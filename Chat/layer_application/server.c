#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_USERS 10
#define MAX_NAME_LENGTH 10
#define MAX_PASS_LENGTH 20
#define PORT 12345

// TODO:
// Dont allow user to chat with themselves
// Investigate seg fault in double digit clients?

// TODO: Implement this Client.
// Implement display users when new user registers to all not in a chat
// How to send message? <MSG><FROM>c1</FROM><TO>c2></TO><BODY>hi</BODY></MSG>
// Above is not necessary. We know the client easily, so just need to set his
// recip name and socket, when they choose a recipient name look here for its socket.
// NOTE recip socket can be undefined in which client is offline, msg will not fwd

// Then when chat, we can easily find the source and destination
// Simply update the file for chats/source/dest.txt
// AND, update the file for chats/dest/source.txt
// THEN fwd the msg by printing it on the dest client IFF they are online (sockfd valid)

// Seperately, when a client connects to a chat, load the chat file to stdout.
typedef struct {
    int socket;
    char name[MAX_NAME_LENGTH + 1]; // Allow null terminator
    int recip_socket; // Who am i chatting with? When a display is called, ignore if this is valid (im already in a chat!)
    char recip_name[MAX_NAME_LENGTH + 1]; // What is their name?
} Client;

Client active_users[MAX_USERS];
int num_users = 0;

void* handle_client(void* arg);
int authenticate_user(int client_socket, char* username, char* password);
int register_user(int client_socket, char* username, char* password);
void sendUserNamesToClient(int clientSocket);
void clientConnected(int client_socket, char* username);
void clientDisconnected(int client_socket);
Client findClientBySocket(int socket);
Client findClientByName(char* name);
void modifyClient(Client newClient);

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
        server_addr.sin_port = htons(PORT + 1);
        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Error binding server socket");
            exit(1);
        }
        
    }

    // Listen
    if (listen(server_socket, 5) < 0) {
        perror("Error listening");
        exit(1);
    }

    printf("Server listening on port %d\n", server_addr.sin_port);
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

    Client client;

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
            int res = send(client_socket, "Enter username: ", 17, 0);
            recv(client_socket, username, MAX_NAME_LENGTH, 0);
            send(client_socket, "Enter password: ", 17, 0);
            recv(client_socket, password, MAX_PASS_LENGTH, 0);

            if (authenticate_user(client_socket, username, password)) {
                authed = 1;
                
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

        // Get the latest client details
        client = findClientBySocket(client_socket);
        
        // Prepare the message in format Name: message
        int msg_len = strlen(buffer) + strlen(client.name) + 3;
        char* message = (char*)malloc(msg_len); // Add 3 for : , \0
        snprintf(message, msg_len, "%s: %s", client.name, buffer);

        // Send to recipient
        send(client.recip_socket, message, msg_len, 0);

        // Free memory
        free(message);
        
        
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

    //Notify all users who are not in a chat that a new user option is available
    for(int i = 0; i < num_users; i++)
    {
        Client client = active_users[i];
        if (strlen(client.recip_name) == 0)
        {
            send(client.socket, "0", 2, 0); // Trigger refresh list
            
        }
    }

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

    // Find the client
    Client client = findClientBySocket(clientSocket);

    // Send and wait for ACK below
    char message[strlen(client.name) + 46];
    snprintf(message, sizeof(message), "<INFO>Welcome, %s! Choose a recipient:</INFO>", client.name);
    send(client.socket, message, sizeof(message), 0);

    // Read, display, and store usernames from the file
    char line[MAX_NAME_LENGTH + MAX_PASS_LENGTH + 1];
    while (numUsers < MAX_USERS && fgets(line, sizeof(line), file)) {
        char *user = strtok(line, ":");
        if (user != NULL) {
            usernames[numUsers] = strdup(user);  // Store the username

            // Send username as an option
            char message[MAX_NAME_LENGTH + 32];
            memset(message, 0, sizeof(message));
            
            snprintf(message, sizeof(message), "<LOGIN_LIST>%d. %s</LOGIN_LIST>", ++numUsers, user);
       
            // Here we are waiting for ACK
            char res[2];
            recv(clientSocket, res, 2, 0); // Block until we get ACK

            // These send()s all fired before recv() finishes.
            // So, the subsequent send()s were ignored.
            // To fix, I added the above wait for ACK.
            send(clientSocket, message, sizeof(message), 0);

            

        }   
    }

    fclose(file);

    if (numUsers == 1)
    {
        // Send message that no users exist.
        // We will notify you when they do.

        send(clientSocket, "No users exist yet!\nWill notify you when they do...\n", 53, 0);
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
        else if (strcmp(buffer, "|") == 0) 
        {
            // Ignore this, it is just ACK
            continue;
        }

        // Client requested refresh
        else if (strcmp(buffer, "0") == 0) 
        {
            // Get the client
            Client client = findClientBySocket(clientSocket);

            // Refresh the list for this client
            // Clear the terminal
            for (int j = 0; j < 10; j++)
            {
                char message[15];
                snprintf(message, sizeof(message), "<INFO>\n</INFO>");

                char res[2];
                recv(client.socket, res, 1, 0); // Block until we get ACK    
                send(client.socket, message, sizeof(message), 0);
            }

            
            // Display the names
            sendUserNamesToClient(client.socket);
            

            continue; // Loop again once printed
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
                valid_res = 1;

                // Get this client by socket, and its recipient by name
                Client client = findClientBySocket(clientSocket);
                Client recip_client = findClientByName(usernames[index - 1]);

                

                // Set the client's recip_name to this
                strncpy(client.recip_name, usernames[index - 1], sizeof(usernames[index - 1]));
                client.recip_name[sizeof(client.recip_name) - 1] = '\0';

                modifyClient(client);

                // Send with <INFO> the chat history to client.socket
                // Note ACK is necessary

                

                // Link users together, or notify that the other user is not online.
                // Messages can always be sent even if other user is offline,
                // But the message just goes straight to the chat history until user comes online
          
                if (strcmp(client.name, recip_client.recip_name) == 0)
                { // We are trying to chat with each other

                    // Send you are now chatting message
                    char message[38 + strlen(usernames[index - 1])];
                    snprintf(message, sizeof(message), "* You are now chatting with %s (ONLINE)", usernames[index - 1]);
                    send(client.socket, message, strlen(message), 0);

                    // We are both trying to chat with each other. Link the sockets
                    client.recip_socket = recip_client.socket;
                    recip_client.recip_socket = client.socket;


                    // Tell the other client that I just came online.
                    char message2[24 + strlen(client.name)];
                    snprintf(message2, sizeof(message2), "* %s is now ONLINE", client.name);
                    send(client.recip_socket, message2, strlen(message2), 0);

                }
                else
                {
                    // This user was not waiting to chat for me
                    // Send you are now chatting message
                    char message[39 + strlen(usernames[index - 1])];
                    snprintf(message, sizeof(message), "* You are now chatting with %s (OFFLINE)", usernames[index - 1]);
                    send(client.socket, message, strlen(message), 0);

                    // I am in the chat room alone, I can not set my recip socket.
                    // When they come online and join the chat, the above chain will execute
                    // and set each parties recip_socket to allow msgs to be forwarded.
                }

                // Update the copies of the clients back into the array
                modifyClient(client);
                modifyClient(recip_client);

            } else {
                // Handle an invalid index
                if (index == 0)
                {
                    // Disconnect
                    clientDisconnected(clientSocket);

                }
                else{
                    snprintf(message, sizeof(message), "Invalid choice: %d\n", index);
                    send(client.socket, message, strlen(message), 0);
                }
                char message[50];
                
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
    
    int indexToRemove = -1;
    
    // Find users who were chatting with this user
    for (int i = 0; i < num_users; i++) {
        if (active_users[i].recip_socket == client_socket) {
            // Invalidate recip socket, msg can not be fwd
            active_users[i].recip_socket = -1;

            // notify anyone chatting with this client (sockfd) that the user logged off
            char message[256];
            snprintf(message, sizeof(message), "* %s is now OFFLINE.", active_users[i].recip_name);
            send(active_users[i].socket, message, sizeof(message), 0);

            // Note that this user's recipient name does not become invalidated, because 
            // They are still viewing the chat window, and we must keep it to reconnect
        }
    }

    // Find the user who just left (to remove them from active array)
    for (int i = 0; i < num_users; i++) {
        if (active_users[i].socket == client_socket) {
            indexToRemove = i;
            printf("Client %s has disconnected.\n", active_users[i].name);
            break;
        }
    }

    // Shift array to remove this user
    for (int i = indexToRemove; i < num_users - 1; i++) {
        active_users[i] = active_users[i + 1];
    }

    // Reduce the array size
    num_users--;

    close(client_socket);
}

void clientConnected(int client_socket, char* username)
{
    // Initialize the client
    Client newClient;
    newClient.socket = client_socket;
    strncpy(newClient.name, username, sizeof(newClient.name) - 1);
    newClient.name[sizeof(newClient.name) - 1] = '\0'; // Null-terminate the string
    newClient.recip_socket = -1;  // Set recip_socket to -1
    strncpy(newClient.recip_name, "", MAX_NAME_LENGTH);  // Set recip_name

    // add client to active user list
    active_users[num_users++] = newClient;

    printf("Client %s has connected!\n", username);
}

Client findClientBySocket(int socket) {
    for (int i = 0; i < num_users; i++) {
        if (active_users[i].socket == socket) {
            // Return the Client structure when a matching socket is found
            return active_users[i];
        }
    }

    // If no matching socket is found, return a default/empty Client structure
    Client emptyClient;
    emptyClient.socket = -1; 
    
    return emptyClient;
}

// Find a client given their name
Client findClientByName(char* name) {
    for (int i = 0; i < num_users; i++) {
        
        if (strcmp(name, active_users[i].name) == 0) {
            // Return the Client structure when a matching name found
            return active_users[i];
        }
    }

    // If no matching socket is found, return a default/empty Client structure
    Client emptyClient;
    emptyClient.socket = -1; 
    
    return emptyClient;
}

// Update a provided client
void modifyClient(Client newClient) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(active_users[i].name, newClient.name) == 0) {
            active_users[i] = newClient;
            return;
        }
    }
}