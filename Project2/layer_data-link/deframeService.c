
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

#define FRAME_LEN 64

// Service to read a frame and write back the original
// data without two SYN chars and length character
int deframeFrame(int deframe_pipe[2])
{
    // Add space for 3 control characters and \0
    // Note this will not include any crc flag or bits because they are removed by
    // the decode service.
    char buffer[FRAME_LEN + 4];
    bzero(buffer, sizeof(buffer));
    char* chunk = &buffer[3]; // Will point to actual content ignoring control chars
    
    // Read the framed chunk from the producer through the deframe pipe
    // ex of incoming stream: (SYN)(SYN)(CR)Hello, World!
    __ssize_t num_read = read(deframe_pipe[0], buffer, sizeof(buffer));
    close(deframe_pipe[0]);
    // Send the deframed chunk through the pipe. It will be 3 chars shorter than input.
    write(deframe_pipe[1], chunk, num_read - 3);

    // close this writing pipe, we are done
    close(deframe_pipe[1]);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    
    int deframe_pipe[2];

    deframe_pipe[0] = atoi(argv[1]); // Assign the first integer
    deframe_pipe[1] = atoi(argv[2]); // Assign the second integer

    return deframeFrame(deframe_pipe);
}
