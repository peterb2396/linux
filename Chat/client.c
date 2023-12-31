#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 12345

void receiveMessages(int server_socket);
void sendMessages(int server_socket);
int SERVER_PORT;
char SERVER_IP[16];

int main(int argc, char* argv[]) {
    if (argc == 1) 
    {
        // No command line arguments provided
        strncpy(SERVER_IP, DEFAULT_SERVER_IP, strlen(DEFAULT_SERVER_IP));
        SERVER_IP[strlen(DEFAULT_SERVER_IP)] = '\0';
        SERVER_PORT = DEFAULT_SERVER_PORT;
    } 
    else if (argc == 2) 
    {
        // One command line argument provided (server IP)
        strncpy(SERVER_IP, argv[1], strlen(argv[1]));
        SERVER_IP[strlen(argv[1])] = '\0';
        SERVER_PORT = DEFAULT_SERVER_PORT;
    } 
    else if (argc == 3) 
    {
        // Two command line arguments provided (IP and port)
        strncpy(SERVER_IP, argv[1], strlen(argv[1]));
        SERVER_IP[strlen(argv[1])] = '\0';
        SERVER_PORT = atoi(argv[2]);
    } 
    else 
    {
        // More than two arguments provided
        printf("Too many command line arguments provided.\n");
        exit(EXIT_FAILURE);
    }

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
        server_addr.sin_port = htons(SERVER_PORT + 1);
        // First port failed, try the next one
        if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("Error connecting to the server");
            exit(1);
        }
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

    close(server_socket);
    return 0;
}
// Repeatedly listen and send messages to the server on this process.
// Server knows whether it is a chat message because we are in that stage
// of the logic loop and it asked to recieve a chat message on that thread.
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

        // Check if the user wants to exit to print exiting
        if (strcmp(message, "/exit") == 0) {
            printf("Exiting...\n");
            
            send(server_socket, message, strlen(message), 0);
            exit(EXIT_SUCCESS);

        }

        // Check if the user wants to logout to print logout confirm msg
        if (strcmp(message, "/logout") == 0) {
            printf("Deleting Account..\n");

            send(server_socket, message, strlen(message), 0);
            exit(EXIT_SUCCESS);

        }
        // Send the message to the server
        ssize_t bytesSent = send(server_socket, message, strlen(message), 0);
        if (bytesSent == -1) {
            perror("Error sending message to the server");
            break;
        }

        
    }

}

void receiveMessages(int server_socket) {
    while (1) {
        

        char buffer2[5000];
        char *tag = NULL;
        char *message = NULL;

        memset(buffer2, 0, sizeof(buffer2));

        // Receive a message from the server
        ssize_t bytesRead = recv(server_socket, buffer2, sizeof(buffer2), 0);

        

        if (bytesRead == -1) {
                perror("Error receiving data from the server");
                exit(EXIT_FAILURE);
            
        } else if (bytesRead == 0) {
            // Disconnected by server
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(buffer2, "0") == 0) 
        {
            // Server is asking this client to refresh their list.
            // Client must now send request to refresh the list.
            send(server_socket, "0", 2, 0);

        }
            else {
            char *start = strchr(buffer2, '<');
            char *end = strchr(buffer2, '>');

            if (start && end && end > start) {
                *end = '\0';
                tag = start + 1;
                start = end + 1;
                end = strstr(start, "</");
                if (end) {
                    *end = '\0';
                    message = start;
                } else {
                    // Handle case where there's no closing tag
                    message = start;
                }
            } else {
                // If no tag found, set tag to NULL
                tag = NULL;
                message = buffer2;
            }


            // Show the client the message from the server, without the tags
            printf("%s\n", message);
            fflush(stdout);

            

            // I processed the message, i am ready for the next
            // Only send on certain tags, like <LOGIN_LIST> where multi data is processed.
            // Otherwise, if the server is in a feedback loop, this will just cause infinite loop

            if (tag && (strcmp(tag, "LOGIN_LIST") == 0 || strcmp(tag, "INFO") == 0))
            {
                fflush(stdout);
                // I acknowledge the first item was recieved, ready for the next
                //char* ACK = "6";  // ASCII code ACK
                send(server_socket, "|", 1, 0);
            }
            
        }

        
        
    }
}
