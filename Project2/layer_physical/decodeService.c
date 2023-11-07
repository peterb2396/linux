#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

#define FRAME_LEN 64
char * generator = "100000100110000001001110110110111";
int debug = 0;


// Helper power function
// Doesn't work with neg numbers / doubles simply because
// It is written just to decode the binary.
int power(int base, int exp) {
    int result = 1;

    for (int i = 0; i < exp; i++) 
            result *= base;

    return result;
}



int checkCRC(char * data) {
    // Make strings for remainder and dividend
    char * remainder = calloc(strlen(generator) - 1, sizeof(char));
    char * currentDividendChunk = calloc(strlen(generator), sizeof(char));

    // Initialize dividend
    strncpy(currentDividendChunk, data, strlen(generator));
    int count = 1;

    // Division Loop
    for (int e = strlen(generator) - 1; e < strlen(data); e++) {
        // XOR
        for (int i = 0; i < strlen(generator); i++) {
            if (generator[i] == '1') {
                if (currentDividendChunk[i] == '1')
                    remainder[i] = (remainder[i] == '1') ? '0' : '1';
                else
                    remainder[i] = (remainder[i] == '1') ? '1' : '0';
            } else {
                remainder[i] = (remainder[i] == '1') ? '1' : '0';
            }
        }

        // Set next dividend (drop number down)
        memmove(currentDividendChunk, currentDividendChunk + 1, strlen(generator) - 1);
        currentDividendChunk[strlen(generator) - 1] = data[e + 1];
    }
    remainder[strlen(remainder) - 1] = '\0';

    // Check if the remainder is all zeros
    for (int j = 0; j < count; j++)
    {
        if (remainder[j] != '0') {
            // If any non-zero remainder is found, an error is detected
            if(debug)
                printf("CRC DETECTED ERROR IN THIS FRAME: \n");
            free(remainder);
            free(currentDividendChunk);
            
            return 1; // Error detected
        }
    }

    // No non-zero remainder found, no error detected
    free(remainder);
    free(currentDividendChunk);
    return 0; // No error detected
}


// Takes an encoded binary frame through the pipe and
// returns the characters for each byte.
int decodeFrame(int decode_pipe[2])
{
    // Add space for the data and 3 control bytes * 8 bits for each
    // Now, also add space for 32 CRC bits and 1 crc flag 
    char buffer[(FRAME_LEN + 3) * 8 + strlen(generator) + 1 + 50];
    bzero(buffer, sizeof(buffer));
    

    // Read the chunk from the consumer through the decode pipe
    __ssize_t num_read = read(decode_pipe[0], buffer, sizeof(buffer));
    buffer[sizeof(buffer)] = '\0';
    //printf("\nDecoding: %s\n", buffer);
    
    close(decode_pipe[0]);

    char res[68];
    bzero(res, 68);
    
    int crc_flag = (int)buffer[0] - 48;

    // Check bits for error before converting
    if(crc_flag)
    {
        checkCRC(buffer);
    }
    else{
        //hamming TBD
    }

    // For each byte... but NOT the 32/8 CRC bytes
    // AND NOT THE FIRST CRC BYTE!
    for (int i = 1; i < (strlen(buffer) - (crc_flag? 32: 0)); i+=8) {
        int num = 0; //the ascii value of this byte
        //printf("\n");
        // For each bit in the byte
        for (int j = 0; j < 8; j++)
            num += ((j == 0)? 0: (((int)buffer[i+j] - 48) * power(2, (7-j))));
            
            
        // The ASCII letter
        char ch = (char)num;
        char str[2];
        sprintf(str, "%c", ch);
        strcat(res, str);
    }
    write(decode_pipe[1], res, strlen(res));
    

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
    debug = atoi(argv[3]);
    
    return decodeFrame(decode_pipe);
}
