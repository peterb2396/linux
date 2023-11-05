#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "../encDec.h" // The project header file defines all processes required.

#define FRAME_LEN 64


// Writes back the provided data frame
// with all characters in uppercase form

int toUpperFrame(int uppercase_pipe[2])
{
    // Add space for \0 character
    char buffer[FRAME_LEN + 1];

    // Read the chunk from the consumer through the uppercase pipe
    __ssize_t num_read = read(uppercase_pipe[0], buffer, sizeof(buffer));
    close(uppercase_pipe[0]); 

    for (int i = 0; i < num_read; i++) { // Exclude the null terminator
        char cap_char = toupper(buffer[i]);
        write(uppercase_pipe[1], &cap_char, sizeof(char));
    }
        
    

    // Finished capitalizing. Close pipe and return
    close(uppercase_pipe[1]); 
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    
    int uppercase_pipe[2];

    // Set the pipe fd from execl arguments
    uppercase_pipe[0] = atoi(argv[1]); // Assign the first integer
    uppercase_pipe[1] = atoi(argv[2]); // Assign the second integer

    return toUpperFrame(uppercase_pipe);
}
