#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../encDec.h"

#define FRAME_LEN 64
char * generator = "100000100110000001001110110110111";

// Takes a data frame and converts to
// Binary by sending back one char at a time as binary
// Also adds a parity bit for error detection



char * CRC(char * data_in) {
    // Make strings for remainder and dividend
    char * remainder = calloc(strlen(generator) - 1, sizeof(char));
    char * currentDividendChunk = calloc(strlen(generator), sizeof(char));

    // Pad data with 0s
    char * data = calloc(strlen(data_in) + strlen(generator) - 1, sizeof(char));
    sprintf(data, "%s%0*d", data_in, (int)(strlen(generator) - 1), 0);

    // Initialize dividend
    strncpy(currentDividendChunk, data, strlen(generator));

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
        memmove(currentDividendChunk, currentDividendChunk + 1, strlen(generator));
        currentDividendChunk[strlen(generator)] = data[e + 1];
    }

    // Create a new data frame with CRC bits appended
    char * data_with_crc = calloc(strlen(data_in) + strlen(generator), sizeof(char));
    sprintf(data_with_crc, "%s%s", data_in, remainder);
    remainder[strlen(remainder) - 1] = '\0';

    free(data);
    free(remainder);
    free(currentDividendChunk);

    return data_with_crc;
}


int encodeFrame(int encode_pipe[2], int crc_flag)
{
    

    // Add space for 3 control chars
    char buffer[FRAME_LEN + 3 + 1];
    bzero(buffer, sizeof(buffer));

    // Read the frame from the producer through the encode pipe
    __ssize_t num_read = read(encode_pipe[0], buffer, sizeof(buffer));
    close(encode_pipe[0]);

    // The data 
    char data[(FRAME_LEN + 3) * 8 + 1 + 1]; // +1 for CRC flag
    memset(data, 0, strlen(data));

    // Append CRC flag
    char crc_flag_string[2];
    sprintf(crc_flag_string, "%d", crc_flag);
    strcat(data, crc_flag_string);

    for (int i = 0; i < num_read; i++) {
        char ch = buffer[i];
        // Send the parity bit through the pipe, first
        strcat(data, __builtin_parity((int)ch)? "0" : "1"); // add the parity bit to the encoded data string

        // For the next 7 bits...
        for (int i = 6; i >= 0; i--) {
            // Determine whether the bit at position i should be one
            int bit = ((int)ch >> i) & 1;
            char bit_str[2];
            sprintf(bit_str, "%d", bit);
            
            strcat(data, bit_str);
        }
    }

    if (crc_flag)
    {
        // Send the data with CRC bits, T = D+R
        char * crc_res = CRC(data);
        write(encode_pipe[1], crc_res, strlen(crc_res));
        //printf("\nEncoded: %s\n", crc_res);
        //free(crc_res);
        
       
    }
    else{ // Hamming TBD
    // for now just write the data with no CRC bits
        write(encode_pipe[1], data, strlen(data));
        //printf("\nEncoded: %s\n", data);
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
