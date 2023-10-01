// frame.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 68

int main(int argc, char *argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    char buffer[MAX_BUFFER_SIZE];
    char* chunk = &buffer[3]; // Will point to actual content ignoring control chars
    int deframe_pipe[2];

    deframe_pipe[0] = atoi(argv[1]); // Assign the first integer
    deframe_pipe[1] = atoi(argv[2]); // Assign the second integer

    // Read the framed chunk from the producer through the deframe pipe
    // ex of incoming stream: SS@Hello, World!
    __ssize_t num_read = read(deframe_pipe[0], buffer, sizeof(buffer));
    close(deframe_pipe[0]);
    // Send the deframed chunk through the pipe. It will be 3 chars shorter than input.
    write(deframe_pipe[1], chunk, num_read - 3);

    // close this writing pipe, we are done
    close(deframe_pipe[1]);

    return EXIT_SUCCESS;
}
