#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <math.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ftw.h>
#include <fcntl.h>

#define DEFAULT_SERVER_IP "127.0.0.1"
#define DEFAULT_SERVER_PORT 12345
#define FRAME_LEN 64
#define MAX_MSG_LEN 5000


void receiveMessages(int server_socket, int pipefd[2]);
void sendMessages(int server_socket, int pipefd[2]);
char * encodeMessage(char* message, char* CRC);
char * prepareFrame(char* chunk, int malform, char* CRC);

char SERVER_IP[16];
int SERVER_PORT;


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

    // Make a pipe so send / recieve can communicate when needed.
    // This is used when recieve gets a flag and needs to set it for send
    int pipefd[2]; // Pipe file descriptors

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed");
        exit(1);
    }

    if (child_pid == 0) {
        // Child process: responsible for receiving messages
        close(pipefd[0]); // child doesnt need to read. We are WRITING flags from server
        receiveMessages(server_socket, pipefd);
        
    } else {
        // Parent process: responsible for sending messages
        close(pipefd[1]); // parent doesnt need to write. We are READING flags into the client
        sendMessages(server_socket, pipefd);
        
    }
    
    // Process sendMessages or recieveMessages quit, usually a failure or exit

    close(server_socket);
    return 0;
}


// Repeatedly listen and send messages to the server on this process.
// Server knows whether it is a chat message because we are in that stage
// of the logic loop and it asked to recieve a chat message on that thread.
void sendMessages(int server_socket, int pipefd[2])
{
    int chatting = 0; // Server makes a request to modify this value when we begin a chat with someone
    char CRC[2] = "0";

    close(pipefd[1]); // We will read only through the pipe to get signals from the server
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK); // Set the pipe to not block, read once
    


    // Loop to listen to client input and send messages
    while (1) {
        char message[MAX_MSG_LEN];
        memset(message, 0, sizeof(message));

        // Check if we got a signal from the server, first
        char buf[10];
        bzero(buf, sizeof(buf));
        int bytes = read(pipefd[0], buf, sizeof(buf));
        if (bytes > 0)
        {  
            // We got the encode flag. Set the encoding method, and notify that we are chatting 
            printf("FLAG RECIEVED: %s\n", buf);

            // Recieved a control flag from the server.
            if (strcmp(buf, "CRC") == 0)
                CRC[0] = '1';

            else if (strcmp(buf, "HAM") == 0)
                CRC[0] = '0';

            chatting = 1;

            // Empty the buffer and listen for the next flag

        }

        

        // Read user input:
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

        // If the user is chatting, encode and send it.
        // Otherwise, it's initialization data, just send it.
        // (we dont want to malform their login info)
        if (chatting && 0)
        {
        
            //char * encoded_message = encodeMessage(message, CRC);
            
            // // Send the encoded message to the server
            // ssize_t bytesSent = send(server_socket, encoded_message, strlen(encoded_message), 0);
            // if (bytesSent == -1) {
            //     perror("Error sending message to the server");
            //     break;
            // }

        }
        else
        {
            // Send the raw message to the server
            ssize_t bytesSent = send(server_socket, message, strlen(message), 0);
            if (bytesSent == -1) {
                perror("Error sending message to the server");
                break;
            }
        }
    }

    

    close(pipefd[0]);
}

// Encode and return all frames of this message.
char * encodeMessage(char* message, char* CRC)
{
    int len = strlen(message);

    // There will be MAX_MSG_LEN / FRAME_LEN frames at most. Each has F data chars and
    // 3 control chars, each being 8 bits. Also each has 1 CRC flag, and may have 32 CRC bits
    char * encoded_message = malloc((MAX_MSG_LEN / FRAME_LEN) * (((FRAME_LEN + 3) * 8) + 1 + (strcmp(CRC, "1") == 0? 32: 0)) + 1);
    memset(encoded_message, 0, sizeof(encoded_message));
        
    char frame[FRAME_LEN + 1];  // 64 characters + 1 for the null terminator

    // frames is chars / frame length
    int frames = ceil(len / FRAME_LEN);

    //  Get a random frame
    unsigned int seed = (unsigned int)getpid();
    srand(seed);

    // between 0 and frames - 1
    int random_frame = rand() % frames;
    printf("Malformed frame: %d\n", random_frame + 1);

    for (int i = 0; i < len; i += FRAME_LEN) {
        int j;
        for (j = 0; j < FRAME_LEN && message[i + j] != '\0'; j++) {
            frame[j] = message[i + j];
        }
        frame[j] = '\0';  // Null-terminate the substring

        // We have a 64 char frame of text. Process it
        char* encoded_frame =  prepareFrame(frame, (random_frame == (i / FRAME_LEN)), CRC);
        strcat(encoded_message, encoded_frame);

        //printf("%s\n", frame);  // Print the chunk of the message
    }

    return encoded_message;
}

// Prepare frame: takes <65 characters of the message at a time, 
// encodes (with CRC), flips a bit, returns encoded & malformed frame with control characters
char * prepareFrame(char* chunk, int malform, char* CRC)
{
    // Make files to show debug info for framing and encoding
    FILE * binfFile = fopen("../output/chat/last_msg.binf", "w");
    FILE * frmeFile = fopen("../output/chat/last_msg.frme", "w");

    // Create a pipe to communicate with frame.c
    int frame_pipe[2];
    if (pipe(frame_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    
    fflush(stdout);
    pid_t frame_pid = fork();

    if (frame_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (frame_pid == 0) {

        char frame_read[10]; // Buffer for converting arg1 to a string
        char frame_write[10]; // Buffer for converting arg2 to a string

        // Convert FD integers to strings
        // so they can be passed as args

        snprintf(frame_read, sizeof(frame_read), "%d", frame_pipe[0]);
        snprintf(frame_write, sizeof(frame_write), "%d", frame_pipe[1]);
        
        // Child process (frame.c)
        
        execl("../layer_data-link/frameService", "frameService", frame_read, frame_write, NULL);  // Execute frame.c
        perror("execl");  // If execl fails
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        
        // Write data to be framed to frame.c through the frame pipe
        write(frame_pipe[1], chunk, strlen(chunk));
        close(frame_pipe[1]);  // Close the write end of the frame pipe

        // When child is done, read
        waitpid(frame_pid, NULL, 0);
        
        
        // Parent reads result from the child process (the new frame)
        char frame[68]; // The frame to be recieved will be stored here
        bzero(frame, sizeof(frame));

        // Read frame result
        int frame_len = read(frame_pipe[0], frame, sizeof(frame));
        close(frame_pipe[0]);  // Close the read end of the frame pipe


        // Null-terminate
        if (frame_len > 0) {
            if (frame_len < sizeof(frame)) {
                frame[frame_len] = '\0';
            } else {
                frame[sizeof(frame) - 1] = '\0';
            }
        }
        
        // At this point, we have recieved the frame and can encode it.

        // Write frame to .frme for debug
        fwrite(frame, sizeof(char), frame_len, frmeFile);

        // Pipe before forking to share a pipe for 
        // transmission of encoding data
        int encode_pipe[2];
        if (pipe(encode_pipe) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        
        // Attempt to fork so child can exec the subroutine
        fflush(stdout);
        pid_t encode_pid = fork();
        if (encode_pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        
        // Encode this single frame
        if (encode_pid == 0)
        {
            
            
            // Make string versions of the pipe id's to pass to argv
            char encode_read[10]; // Buffer for converting arg1 to a string
            char encode_write[10]; // Buffer for converting arg2 to a string

            // Convert integers to strings
            snprintf(encode_read, sizeof(encode_read), "%d", encode_pipe[0]);
            snprintf(encode_write, sizeof(encode_write), "%d", encode_pipe[1]);
            
            // Child process: Call encode then dieencode_pip
            execl("../layer_physical/encodeService", "encodeService", encode_read, encode_write, CRC, NULL);
            perror("execl");  // If execl fails
            exit(EXIT_FAILURE);
        }
        else
        {
            

            // Write data to be encoded through encode pipe
            write(encode_pipe[1], frame, frame_len);
            close(encode_pipe[1]);  // Done writing frame to be encoded

            
            waitpid(encode_pid, NULL, 0);;
            
            

            // Parent reads result from the child process (the encoded frame)
            // Add space for control chars and bit conversion
            char * encoded_frame = malloc((FRAME_LEN + 3) * 8 + 32); // The encoded frame
            bzero(encoded_frame, sizeof(encoded_frame));
            // Otherwise, would have old bytes in it
                
            // Listen for & store encoded frame
            int encoded_len = read(encode_pipe[0], encoded_frame, sizeof(encoded_frame));
            close(encode_pipe[0]);  // Done reading encode data

            // Here, we have the encoded_frame!
            

            // Simulate transmission error here
            // Determine if this frame should be malformed to simulate error
            if (malform)
            {
                int malform_pipe[2];
                if (pipe(malform_pipe) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }

                // Attempt to fork so child can exec the subroutine
                fflush(stdout);
                pid_t malform_pid = fork();
                if (malform_pid == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
                // service
                if (malform_pid == 0)
                {
                    // Make string versions of the pipe id's to pass to argv
                    char malform_read[10]; // Buffer for converting arg1 to a string
                    char malform_write[10]; // Buffer for converting arg2 to a string

                    // Convert integers to strings
                    snprintf(malform_read, sizeof(malform_read), "%d", malform_pipe[0]);
                    snprintf(malform_write, sizeof(malform_write), "%d", malform_pipe[1]);
                    
                    // Child process: Call service then die
                    execl("../layer_physical/malformService", "malformService", malform_read, malform_write, NULL);
                    perror("execl");  // If execl fails
                    exit(EXIT_FAILURE);
                }
                else //parent
                {
                    // Write data to be serviced (the correct encoded frame) through service pipe
        
                    write(malform_pipe[1], encoded_frame, strlen(encoded_frame));
                    close(malform_pipe[1]);  // Done writing frame to be serviced

                    // When child is done, read result
                    waitpid(malform_pid, NULL, 0);

                    // Listen for & store malformed frame in previously alocated encoded_frame buff
                    read(malform_pipe[0], encoded_frame, sizeof(encoded_frame));
                    close(malform_pipe[0]);  // Done reading service data


                }
            }
            
            // Write the encoded frame to the file, AND to the consumer to decode!

            // May contain a flipped bit now.
            fwrite(encoded_frame, sizeof(char), encoded_len, binfFile);
            return encoded_frame;

        }

    }
}

void receiveMessages(int server_socket, int pipefd[2]) {
    while (1) {
        

        char buffer2[MAX_MSG_LEN];
        char *tag = NULL;
        char *message = NULL;

        memset(buffer2, 0, sizeof(buffer2));

        // Receive a message from the server
        ssize_t bytesRead = recv(server_socket, buffer2, sizeof(buffer2), 0);
        // printf("%s\n", buffer2);
        // fflush(stdout);

        

        if (bytesRead == -1) {
                perror("Error receiving data from the server");
                exit(EXIT_FAILURE);
            
        } else if (bytesRead == 0) {
            // Disconnected by server
            exit(EXIT_SUCCESS);
        }
        else { // Process an incoming message
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


            // Check what the tag was, and process accordingly
            if (tag)
            {

                if (strcmp(tag, "LOGIN_LIST") == 0 )
                {
                    // Print this user from the login list
                    printf("%s\n", message);
                    fflush(stdout);
                    // I acknowledge the first item was recieved, ready for the next
                    // Send ACK to recieve the next username in the loop
                    send(server_socket, "|", 1, 0);
                }

                // We are refreshing our list of usernames.
                // Note the difference between here and <INFO>REFRESH</INFO>
                // Is in INFO, the server requests a refresh from us, and this is
                // the server actually processing the refresh request.
                if (strcmp(tag, "REFRESH") == 0 )
                {
                    // Print this user from the login list
                    printf("\n\n\n\n\n");
                    fflush(stdout);
                    // I acknowledged I have cleared my console and im ready to recieve the first username
                    // Send ACK to recieve the first username in the loop
                    send(server_socket, "|", 1, 0);
                }

                // Server is sending info: 
                else if (strcmp(tag, "INFO") == 0 )
                {
                    if (strcmp(message, "refresh") == 0) 
                    {
                        // Server is asking this client to refresh their list.
                        // Client must now send request to refresh the list.
                        send(server_socket, "0", 2, 0);

                    }

                    // This was a generic information message.
                    // Display it to the client
                    else
                    {
                        printf("%s\n", message);
                        fflush(stdout);
                    }
                    
                    
                }

                // Server is telling us to use CRC encoding
                else if (strcmp(tag, "ENCODE") == 0 )
                {
                    if (strcmp(message, "CRC") == 0)
                        // Set the chat flag to 1 in the parent process (sendingMessages)
                        write(pipefd[1], "CRC", 4);
                    else
                        // Set the chat flag to 1 in the parent process (sendingMessages)
                        write(pipefd[1], "HAM", 4);
                    
                    //fflush(stdout);
                    // Acknowledge we recieved CRC flag so we can now get the user list
                    send(server_socket, "|", 1, 0);
                }

                // Client has sent us a message. Display it
                else if (strcmp(tag, "MSG") == 0 )
                {
                    printf("%s\n", message);
                    fflush(stdout);
                    
                }


            }
            else
            {
                // No tag was sent with the message.
                // Print it (typically debug)
                printf("%s\n", message);
                fflush(stdout);
            }
            
        }

        
        
    }
}
