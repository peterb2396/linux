#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

// Helper node for the server
// Handles multithreaded capitalization of vowels.

// Input: Encoded frame from client
// Procedure: Capitalize vowels
// Output: Encoded frame from client, capitalized

// Process:
// 1. Decode the frame (decodeService)
// 2. Capitalize the frame (capitalizeService)
// 3. Encode the frame back (encodeService)
// 4. Send back to server through helper node socket

char * serverEncoder(char* raw);

// Perform the service
int serverDecoder(int helper_pipe[2])
{
    // The input
    char buffer[750];
    bzero(buffer, sizeof(buffer));
    

    // Read the encoded chunk from the consumer through the decode pipe
    __ssize_t num_read = read(helper_pipe[0], buffer, sizeof(buffer));
    buffer[sizeof(buffer)] = '\0';
    
    // The helper is done reading
    close(helper_pipe[0]);

    // STEP 1::: DECODE & DEFRAME TO OBTAIN RAW DATA
    // Will decode and deframe
    char parsed_frame[65];

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
        execl("../layer_physical/decodeService", "decodeService", decode_read, decode_write, "0", NULL);
        perror("execl");  // If execl fails
        exit(EXIT_FAILURE);
    }
    else // Parent
    {
        //printf("\nTO DECODER: %s\n", buffer);
        // Write data to be decoded through decode pipe
        write(decode_pipe[1], buffer, strlen(buffer));
        close(decode_pipe[1]);  // Done writing frame to be decoded

        // When child is done, read result
        waitpid(decode_pid, NULL, 0);;

        // Parent reads result from the child process (the decoded frame)
        char decoded_frame[64 + 4 + 1 + 1]; // The decoded frame is 1/8 the size
        bzero(decoded_frame, sizeof(decoded_frame));
            // NOTE 67 (frame len) * 9 with spaces, *8 without, is perfect amount
            // Does not contain 32 CRC bits. DOES contain 4 control chars + 64 of data

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
            bzero(parsed_frame, sizeof(parsed_frame));

            int chunk_len = read(deframe_pipe[0], parsed_frame, sizeof(parsed_frame));
            close(deframe_pipe[0]); 
            
        } // Last fork parent.
    } // Inner decode pipeline over.


    // HERE WE HAVE PARSED_FRAME: THE ORIGINAL DATA SECTION
    

    // STEP 2: CAPITALISE IT BY PASSING THROUGH CAPITALIZATION SERVICE
    char capped_frame[65];

    // Pipe before forking to share a pipe for 
    // transmission of data
    int capitalize_pipe[2];
    if (pipe(capitalize_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Attempt to fork so child can exec the subroutine
    fflush(stdout);
    pid_t capitalize_pid = fork();
    if (capitalize_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    // Decode this single frame
    if (capitalize_pid == 0)
    {
        // Make string versions of the pipe id's to pass to argv
        char capitalize_read[10]; // Buffer for converting arg1 to a string
        char capitalize_write[10]; // Buffer for converting arg2 to a string

        // Convert integers to strings
        snprintf(capitalize_read, sizeof(capitalize_read), "%d", capitalize_pipe[0]);
        snprintf(capitalize_write, sizeof(capitalize_write), "%d", capitalize_pipe[1]);
        
        // Child process: Call encode then die
        execl("../layer_physical/capitalizeService", "capitalizeService", capitalize_read, capitalize_write, "0", NULL);
        perror("execl");  // If execl fails
        exit(EXIT_FAILURE);
    }
    else // Parent
    {
        // Write data to be capitalizeed through capitalize pipe
        write(capitalize_pipe[1], parsed_frame, strlen(parsed_frame));
        close(capitalize_pipe[1]);  // Done writing frame to be capitalized

        // When child is done, read result
        waitpid(capitalize_pid, NULL, 0);;

        bzero(capped_frame, sizeof(capped_frame));

        // Listen for & store capitalized frame: same length as input
        int capitalized_len = read(capitalize_pipe[0], capped_frame, strlen(parsed_frame));
        close(capitalize_pipe[0]);  // Done reading capitalizee data
    }
    
    // HERE, CAPPED_FRAME IS THE RESULT!

    // STEP 3: ENCODE THE NEW DATA WITH SERVER ENCODER
    char * encodedCappedFrame = serverEncoder(capped_frame);
    


    // Write the result to the server
    // NOTE: THIS MUST BE DONE BY THREAD 6
    write(helper_pipe[1], encodedCappedFrame, strlen(encodedCappedFrame));

    // Free memory for result
    free(encodedCappedFrame);
    
    // Done writing
    close(helper_pipe[1]); 
    return EXIT_SUCCESS;

}

char * serverEncoder(char* raw)
{
    
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
        
        execl("../layer_data-link/frameService", "frameService", frame_read, frame_write, "0", NULL);  // Execute frame.c
        perror("execl");  // If execl fails
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        
        // Write data to be framed to frame.c through the frame pipe
        write(frame_pipe[1], raw, strlen(raw));
        //works
        
        
        close(frame_pipe[1]);  // Close the write end of the frame pipe

        // When child is done, read
        waitpid(frame_pid, NULL, 0);
        
        
        // Parent reads result from the child process (the new frame)
        char frame[69]; // The frame to be recieved will be stored here
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
            execl("../layer_physical/encodeService", "encodeService", encode_read, encode_write, "0", NULL);
            perror("execl");  // If execl fails
            exit(EXIT_FAILURE);
        }
        else
        {

            // Write data to be encoded through encode pipe
            write(encode_pipe[1], frame, frame_len);
            
            close(encode_pipe[1]);  // Done writing frame to be encoded

            
            waitpid(encode_pid, NULL, 0);;
            
            
            char * encoded_frame = malloc(750); // The encoded frame
            bzero(encoded_frame, 750);
            // Malloc so we dont return a local address, instead a heap address.
            // Note sizeof(encoded_Frame) will be 8, sizeof(pointer) = sizeof(int)
                
            // Listen for & store encoded frame
            int encoded_len = read(encode_pipe[0], encoded_frame, 750);
            close(encode_pipe[0]);  // Done reading encode data
            

            encoded_frame[encoded_len] = '\0';
            
            

            // Here, we have the encoded_frame!
            // Return it to the main funciton
            return encoded_frame;
            
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
    int helper_pipe[2];
    helper_pipe[0] = atoi(argv[1]); // Assign the first integer
    helper_pipe[1] = atoi(argv[2]); // Assign the second integer
    
    return serverDecoder(helper_pipe);
}
