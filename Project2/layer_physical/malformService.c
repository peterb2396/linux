#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

// Start after control characters (3x 8) + 1 crc bit
#define L_BOUND 25

// Malforms a provided data frame by choosing
// a random bit within it and flipping it.

int malformFrame(int malform_pipe[2], int malform_padding)
{
    char buffer[67 * 8 + 1 + 32]; // SPace for encoded frame
    // Do not allow malform to affect these first n bits:
    int padding = L_BOUND + malform_padding;

    // Read the frame from the producer through the malform pipe
    __ssize_t num_read = read(malform_pipe[0], buffer, sizeof(buffer));
 
    close(malform_pipe[0]); 

    // Choose a bit to flip
    // Do not flip first 3 * 8 = 24 bits, they are control characters
    // So choose a random number from 24 to len-1
    // Get the current process ID as the seed
    unsigned int seed = (unsigned int)getpid();
    srand(seed);

    // Generate a random bit in the range [24, len-1]
    int random_bit = (rand() % (strlen(buffer) - 32 - padding)) + padding;

    // Make sure the bit is not a parity bit
    if (random_bit % 8 == 0)
    {
        // If it was a parity bit, chose a random bit in this byte.
        random_bit+= (rand() % 7) + 1;
    }

    //Flip the bit
    if (buffer[random_bit] == '0') {
        buffer[random_bit] = '1';
    } else if (buffer[random_bit] == '1') {
        buffer[random_bit] = '0';
    }

    printf("Flipped bit: %d\n", random_bit);
    fflush(stdout);

    // Write the new buffer back
    write(malform_pipe[1], buffer, strlen(buffer));

    // Finished malforming, close pipe & return
    close(malform_pipe[1]); 
    return EXIT_SUCCESS;
}


// Service to malform the data by flipping one bit of the encoded frame.
int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    
    int malform_pipe[2];

    malform_pipe[0] = atoi(argv[1]); // Assign the first integer
    malform_pipe[1] = atoi(argv[2]); // Assign the second integer
    int malform_padding = atoi(argv[3]); // Assign the padding bits

    return malformFrame(malform_pipe, malform_padding);
}
