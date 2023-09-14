#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Helper power function
// Doesn't work with neg numbers / doubles simply because
// It is written just to decode the binary.
int power(int base, int exp) {
    int result = 1;

    for (int i = 0; i < exp; i++) 
            result *= base;

    return result;
}


int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    char buffer[67 * 8];
    int decode_pipe[2];

    // Set the pipe fd from execl arguments
    decode_pipe[0] = atoi(argv[1]); // Assign the first integer
    decode_pipe[1] = atoi(argv[2]); // Assign the second integer

    // Read the chunk from the consumer through the decode pipe
    __ssize_t num_read = read(decode_pipe[0], buffer, sizeof(buffer));
    
    close(decode_pipe[0]); 

    // For each byte...
    for (int i = 0; i < num_read; i+=8) {
        int num = 0; //the ascii value of this byte
        printf("\n");
        // For each bit in the byte
        for (int j = 0; j < 8; j++)
        {
            num += (j == 0)? 0: (int)buffer[i+j] * power(2, (8-j));
            printf("%d", (int)buffer[i+j]);
        }
            
            
        // The ASCII letter
        char ch = (char)num;
        //printf("%c\n", ch);
        
        // Send the char through the pipe
        write(decode_pipe[1], &ch, 1);
    }

    // Finished encoding, close pipe & return
    close(decode_pipe[0]); 
    return EXIT_SUCCESS;
}
