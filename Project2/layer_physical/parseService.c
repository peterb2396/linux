#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ftw.h>
#include <math.h>
#include <fcntl.h>








int parseFrame(int parse_pipe[2])
{
    // add space for crc bits, control flags, crc bit, newline
    char encoded_frame[(64 + 3)*8 + 1 + 32 + 1];
    bzero(encoded_frame, sizeof(encoded_frame));
    // Read the encoded data of length (64+3)*8 + 1
    //__ssize_t num_read = read(parse_pipe[0], encoded_frame, sizeof(encoded_frame));
    //printf("\nIN PARSER %s\n", encoded_frame);
    close(parse_pipe[0]);
    
// Pipe before forking to share a pipe for 
        // transmission of data
        int decode_pipe[2];
        if (pipe(decode_pipe) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        
        // Attempt to fork so child can exec the subroutine
        fflush(stdout);
        pid_t decode_pid = fork();
        if (decode_pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        // Decode this single frame
        if (decode_pid == 0)
        {
            // Make string versions of the pipe id's to pass to argv
            char decode_read[10]; // Buffer for converting arg1 to a string
            char decode_write[10]; // Buffer for converting arg2 to a string

            // Convert integers to strings
            snprintf(decode_read, sizeof(decode_read), "%d", decode_pipe[0]);
            snprintf(decode_write, sizeof(decode_write), "%d", decode_pipe[1]);
            
            // Child process: Call encode then die
            execl("../layer_physical/decodeService", "decodeService", decode_read, decode_write, NULL);
            perror("execl");  // If execl fails
            exit(EXIT_FAILURE);
        }
        else // Parent
        {
            printf("\nTO DECODER: %s\n", encoded_frame);
            // Write data to be decoded through decode pipe
            write(decode_pipe[1], encoded_frame, strlen(encoded_frame));
            close(decode_pipe[1]);  // Done writing frame to be decoded

            // When child is done, read result
            waitpid(decode_pid, NULL, 0);;

            // Parent reads result from the child process (the decoded frame)
            char decoded_frame[64 + 3 + 1]; // The decoded frame is 1/8 the size
            bzero(decoded_frame, sizeof(decoded_frame));
                // NOTE 67 (frame len) * 9 with spaces, *8 without, is perfect amount
                // Does not contain 32 CRC bits. DOES contain 3 control chars + 64 of data

            // Listen for & store decoded frame
            int decoded_len = read(decode_pipe[0], decoded_frame, sizeof(decoded_frame));
            close(decode_pipe[0]);  // Done reading encode data

            // Here, we have the decoded frame!

            // Now, it's time to deframe it back to a chunk.

            // Create a pipe to communicate with deframe.c
            int deframe_pipe[2];
            if (pipe(deframe_pipe) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            fflush(stdout);
            pid_t deframe_pid = fork();

            if (deframe_pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (deframe_pid == 0) {
                char deframe_read[10]; // Buffer for converting arg1 to a string
                char deframe_write[10]; // Buffer for converting arg2 to a string

                // Convert integers to strings
                snprintf(deframe_read, sizeof(deframe_read), "%d", deframe_pipe[0]);
                snprintf(deframe_write, sizeof(deframe_write), "%d", deframe_pipe[1]);
                
                // Child process (deframe.c)
                execl("../layer_data-link/deframeService", "deframeService", deframe_read, deframe_write, NULL);  // Execute deframe.c
                perror("execl");  // If execl fails
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                // Write data to be framed to deframe.c through the deframe pipe
                write(deframe_pipe[1], decoded_frame, decoded_len);
                
                close(deframe_pipe[1]);

                // When child is done, read chunk (ctrl chars removed)
                waitpid(deframe_pid, NULL, 0);
                char chunk[65];
                bzero(chunk, sizeof(chunk));

                int chunk_len = read(deframe_pipe[0], chunk, sizeof(chunk));
                close(deframe_pipe[0]); 
                
                chunk[strlen(chunk)] = '\0';
                
                write(parse_pipe[1], chunk, chunk_len);
                close(parse_pipe[1]);

                return EXIT_SUCCESS;


        }

        
    }
}

int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }

    // Set the pipe fd from execl arguments
    int parse_pipe[2];
    parse_pipe[0] = atoi(argv[1]); // Assign the first integer
    parse_pipe[1] = atoi(argv[2]); // Assign the second integer
    
    return parseFrame(parse_pipe);
}