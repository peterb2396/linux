#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

#define FRAME_LEN 64
char * generator = "100000100110000001001110110110111";


// Helper power function
// Doesn't work with neg numbers / doubles simply because
// It is written just to decode the binary.
int power(int base, int exp) {
    int result = 1;

    for (int i = 0; i < exp; i++) 
            result *= base;

    return result;
}

char * checkCRC(char * data) {
    printf("CHECKING: %s\n", data);
	// Make strings for remainder and dividend
	char * remainder = calloc(strlen(generator) - 1, sizeof(char));
	char * currentDividendChunk = calloc(strlen(generator), sizeof(char));

    // Initialize dividend
    strncpy(currentDividendChunk, data, strlen(generator));

    
	// Division Loop
	for(int e = strlen(generator)-1; e < strlen(data); e++) {
        // XOR
		for(int i = 1; i < strlen(generator); i++) {
			
			if((int)generator[i] == (int)currentDividendChunk[i]) 
				remainder[i-1] = '0';
			else 
				remainder[i-1] = '1';
			
		}
            // Set next dividend (drop number down)
			bzero(currentDividendChunk, (int)strlen(generator));
			strcat(currentDividendChunk, remainder);
			currentDividendChunk[strlen(generator)-1] = data[e+1];

	}
	return remainder;
}

// Takes an encoded binary frame through the pipe and
// returns the characters for each byte.
int decodeFrame(int decode_pipe[2], int crc_flag)
{
    // Add space for the data and 3 control bytes * 8 bits for each
    // Now, also add space for 32 CRC bits
    char buffer[(FRAME_LEN + 3) * 8 + strlen(generator)-1];

    // Read the chunk from the consumer through the decode pipe
    __ssize_t num_read = read(decode_pipe[0], buffer, sizeof(buffer));
    
    close(decode_pipe[0]); 

    // Check bits for error before converting
    if(crc_flag)
    {
        printf("CRC Check: %s\n", checkCRC(buffer));
    }
    else{
        //hamming TBD
    }

    // For each byte... but NOT the 32/8 CRC bytes
    for (int i = 0; i < (num_read - 32); i+=8) {
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
