#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

#define FRAME_LEN 64
char * generator = "100000100110000001001110110110111";
char * padding =   "00000000000000000000000000000000";

// Takes a data frame and converts to
// Binary by sending back one char at a time as binary
// Also adds a parity bit for error detection

char * CRC(char * data_in) {

	// Pad data with 0s
	char * data = calloc(strlen(data_in) + strlen(padding), sizeof(char));
    sprintf(data, "%-*s%0*d", (int)strlen(padding), data_in, (int)strlen(padding), 0);
	
	// Make strings for remainder and dividend
	char * remainder = calloc(strlen(padding), sizeof(char));
	char * currentDividendChunk = calloc(strlen(generator), sizeof(char));

    // Initialize dividend
    strncpy(currentDividendChunk, data, strlen(generator));

    // Initialize first dividend
	for(int i = 0; i < strlen(generator); i++) 
		currentDividendChunk[i] = data[i];
    
    
	
    
	// Division Loop
	for(int i = strlen(padding); i < strlen(data); i++) {
		// XOR
		for(int i = 1; i < strlen(generator); i++) {
			
			if((int)generator[i] == (int)currentDividendChunk[i]) 
				remainder[i-1] = '0';
			else 
				remainder[i-1] = '1';
			
		}
		bzero(currentDividendChunk, strlen(generator));
		strcat(currentDividendChunk, remainder);
		// Drop new value into dividend
		currentDividendChunk[strlen(generator)-1] = data[i+1];
	}

	// T = D + R (Append remainder to data)
    bzero(data, sizeof(data));
    sprintf(data, "%s%s", data_in, remainder);
	return data;
}

int encodeFrame(int encode_pipe[2], int crc_flag)
{
    

    // Add space for 3 control chars
    char buffer[FRAME_LEN + 3];

    // Read the frame from the producer through the encode pipe
    __ssize_t num_read = read(encode_pipe[0], buffer, sizeof(buffer));
    close(encode_pipe[0]); 

    // The data 
    char data[(FRAME_LEN + 3) * 8];
    memset(data, 0, strlen(data));

    for (int i = 0; i < num_read; i++) {
        char ch = buffer[i];
        // Send the parity bit through the pipe, first
        write(encode_pipe[1], __builtin_parity((int)ch)? "0" : "1", 1);
        strcat(data, __builtin_parity((int)ch)? "0" : "1"); // add the parity bit to the encoded data string

        // For the next 7 bits...
        for (int i = 6; i >= 0; i--) {
            // Determine whether the bit at position i should be one
            int bit = ((int)ch >> i) & 1;
            char bit_str[2];
            sprintf(bit_str, "%d", bit);
            
            strcat(data, bit_str);
            write(encode_pipe[1], bit_str, 1);
        }
    }

    if (crc_flag)
    {
        // Send the data with CRC bits, T = D+R
        //write(encode_pipe[1], CRC(data), strlen(data) + strlen(padding));
        printf("CRC: %s\n", CRC(data));

    }
    else{ // Hamming TBD
        
    }

    // Finished encoding, close pipe & return
    close(encode_pipe[1]); 
    return EXIT_SUCCESS;

}

int main(int argc, char* argv[]) {

    if (argc < 3)
    {
        // We are probably just compiling the file, don't run without args
        return EXIT_FAILURE;
    }
    
    
    int encode_pipe[2];

    encode_pipe[0] = atoi(argv[1]); // Assign the first integer
    encode_pipe[1] = atoi(argv[2]); // Assign the second integer
    int crc_flag = atoi(argv[3]);

    return encodeFrame(encode_pipe, crc_flag);
}
