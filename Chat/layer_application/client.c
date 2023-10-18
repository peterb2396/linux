#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_NAME_LENGTH 50
#define MAX_PASS_LENGTH 50
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12346

void receiveMessages(int server_socket);
void sendMessages(int server_socket);

int main() {
    int server_socket;
    struct sockaddr_in server_addr;

    // Create socket and connect to the server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating client socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to the server");
        exit(1);
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed");
        exit(1);
    }

    if (child_pid == 0) {
        // Child process: responsible for receiving messages
        receiveMessages(server_socket);
    } else {
        // Parent process: responsible for sending messages
        sendMessages(server_socket);
    }
    
    // Process sendMessages or recieveMessages quit, usually a failure or exit
    printf("Shutting down connection");
    close(server_socket);
    return 0;
}
// Repeatedly listen and send messages to the server on this process.
// Server knows whether it is a chat message because we are in that stage
// of the logic loop and it asked to recieve a chat message on that thread.
// Client can have a global variable of target recipient, in order to
// Figure out how to format <TO><BODY> etc... if target is null, no <BODY>?
// Or we can break the following function into different such as chat(), login() etc
void sendMessages(int server_socket)
{
    while (1) {
        char message[5000];
        memset(message, 0, sizeof(message));

        // Read user input
        if (fgets(message, sizeof(message), stdin) == NULL) {
            perror("Error reading user input");
            break;
        }

        // Remove the newline character from the input
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        // Check if the user wants to exit
        if (strcmp(message, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        // Send the message to the server
        ssize_t bytesSent = send(server_socket, message, strlen(message), 0);
        if (bytesSent == -1) {
            perror("Error sending message to the server");
            break;
        }
    }

}

void receiveMessages(int server_socket)
{
    while(1)
    {
        // Communication loop: wait for a message from the server, and display it
        char buffer[5000];
        memset(buffer, 0, sizeof(buffer));

        // Receive a message from the server
        ssize_t bytesRead = recv(server_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead == -1) {
            perror("Error receiving data from the server");
            exit(1);
        } else if (bytesRead == 0) {
            printf("Server has closed the connection. Exiting...\n");
            break;
        } else {
            // Print the received message
            printf("%s\n", buffer);
        }
    }
}
