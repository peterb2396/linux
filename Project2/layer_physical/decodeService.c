#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

#define FRAME_LEN 64

// Helper power function
// Doesn't work with neg numbers / doubles simply because
// It is written just to decode the binary.
int power(int base, int exp) {
    int result = 1;

    for (int i = 0; i < exp; i++) 
            result *= base;

    return result;
}

// Takes an encoded binary frame through the pipe and
// returns the characters for each byte.
int decodeFrame(int decode_pipe[2], int crc_flag)
{
    // Add space for the data and 3 control bytes * 8 bits for each
    char buffer[(FRAME_LEN + 3) * 8];

    // Read the chunk from the consumer through the decode pipe
    __ssize_t num_read = read(decode_pipe[0], buffer, sizeof(buffer));
    
    close(decode_pipe[0]); 

    // For each byte...
    for (int i = 0; i < num_read; i+=8) {
        int num = 0; //the ascii value of this byte
        //printf("\n");
        // For each bit in the byte
        for (int j = 0; j < 8; j++)
            num += ((j == 0)? 0: (((int)buffer[i+j] - 48) * power(2, (7-j))));
            
            
        // The ASCII letter
        char ch = (char)num;
        
        // Send the char through the pipe
        write(decode_pipe[1], &ch, 1);
    }

    // Finished encoding, close pipe & return
    close(decode_pipe[1]); 
    return EXIT_SUCCESS;

}


int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }

    // Set the pipe fd from execl arguments
    int decode_pipe[2];
    decode_pipe[0] = atoi(argv[1]); // Assign the first integer
    decode_pipe[1] = atoi(argv[2]); // Assign the second integer
    int crc_flag = atoi(argv[3]);
    
    return decodeFrame(decode_pipe, crc_flag);
}
