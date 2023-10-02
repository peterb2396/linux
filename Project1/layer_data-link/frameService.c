
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

#define FRAME_LEN 64

int frameChunk(int frame_pipe[2])
{
    // Include space for \0
    char buffer[FRAME_LEN + 1];
   

    // Read the block of data from the producer through the frame pipe
    __ssize_t num_read = read(frame_pipe[0], buffer, sizeof(buffer));
    
    close(frame_pipe[0]);

    char control[3];
    control[0] = (char)22;  // First SYN character
    control[1] = (char)22;  // Second SYN character
    control[2] = (char)num_read;  // Length

    // Send ctrl characters through the pipe. Then, send the data block
    write(frame_pipe[1], control, sizeof(control));
    write(frame_pipe[1], buffer, num_read);

    
    

    // close this writing pipe, we are done
    close(frame_pipe[1]);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {


    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    
    int frame_pipe[2];

    frame_pipe[0] = atoi(argv[1]); // Assign the first integer
    frame_pipe[1] = atoi(argv[2]); // Assign the second integer

    return frameChunk(frame_pipe);
}
