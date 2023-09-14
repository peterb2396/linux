#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    char buffer[67];
    int encode_pipe[2];

    encode_pipe[0] = atoi(argv[1]); // Assign the first integer
    encode_pipe[1] = atoi(argv[2]); // Assign the second integer

    // Read the frame from the producer through the encode pipe
    __ssize_t num_read = read(encode_pipe[0], buffer, sizeof(buffer));
    close(encode_pipe[0]); 

    for (int i = 0; i < num_read; i++) {
        char ch = buffer[i];
        // Send the parity bit through the pipe, first
        write(encode_pipe[1], __builtin_parity((int)ch)? "0" : "1", 1);
        
        // For the next 7 bits...
        for (int i = 6; i >= 0; i--) {
            // Determine whether the bit at position i should be one
            int bit = ((int)ch >> i) & 1;
            char bit_str[1];
            sprintf(bit_str, "%d", bit);
            
            write(encode_pipe[1], bit_str, 1);
        }
        //write(encode_pipe[1], " ", 1); // write space for debugging
        // if commenting out be sure to change encode buffer size to 67 * 8 in producer
    }

    // Finished encoding, close pipe & return
    close(encode_pipe[1]); 
    return EXIT_SUCCESS;
}
