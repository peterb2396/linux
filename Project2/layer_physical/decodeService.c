#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "../encDec.h"

#define FRAME_LEN 64
char * generator = "100000100110000001001110110110111";
int debug = 0;
int frame = 0;


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
    for (int j = 0; j < strlen(remainder); j++)
    {
        if (remainder[j] != '0') {
            // If any non-zero remainder is found, an error is detected in this frame
            if(debug)
                printf("\nCRC DETECTED ERROR (FRAME %d):\n", frame);
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


// Check Hamming
void checkHamming(char * data)
{
    
    // Takes data section only as a param to ignore control sequence
    int hamming_len = strlen(data);
    
    int wrong_indexes[hamming_len];
    int wrong_count = 0;
    int i = 1; // The parity bit at index i - 1
    while (i <= hamming_len)
    {
        int sum = 0;
        int j = i; // The step size (distance for parity bit i) ('Skip i')
        while (j < hamming_len)
        {
            // ('pick i') sum up i bits
            for (int k = 0; k < i; k++)
            {
                // Make sure we dont look past the end of the buffer
                if (j+k > hamming_len)
                    break;
                
                // Do not include the parity itself
                if (j+k == i)
                    continue;

                // Count the parity
                sum += data[j+k - 1] - '0'; // convert the char to int
            }
            j+=2*i; // Skip bits
        }
        // Bad byte if calculated parity does not equal parity bit
        if ((data[i - 1] - '0') != (sum % 2))
            wrong_indexes[wrong_count++] = i;

        i*=2; // The next parity bit
    }

    if (wrong_count)
    {
        int bad_index = 0;          // Detect
        for (int b = 0; b < wrong_count; bad_index += wrong_indexes[b++]);
                                    // Correct
        data[bad_index - 1] = (data[bad_index - 1] == '0')? '1': '0';
                                    // Print
        if(debug)
            printf("\nHAMMING CORRECTED BIT %d OF FRAME %d:\n", bad_index, frame);
    }

    
}
// Helper functions for hamming
double Log2(int x)
{
    return (log10(x) / log10(2));
}

// Return if an integer is a power of two
// (if it will be a parity bit)
int isPowerOfTwo(int n)
{
    return (ceil(Log2(n)) == floor(Log2(n)));
}

// Remove the hamming bits from the data sequence.
// This allows us to print the original data
void removeHammingBits(char* data)
{
    int len = strlen(data);
    char original[len + 1];
    int j = 0; // Index of original data

    for (int i = 0; i < len; i++)
    {
        if (isPowerOfTwo(i+1)) // Bitwise and: Is it a power of two?S
            continue; // Ignore it if so
            

        original[j++] = data[i]; // It was not a parity bit, add it back to the OG data
    }

    original[j] = '\0'; // Null term
    strcpy(data, original); // Overwrite hamming code with original data     
                       
}


// Takes an encoded binary frame through the pipe and
// returns the characters for each byte.
int decodeFrame(int decode_pipe[2])
{
    // Add space for the data and 4 control bytes * 8 bits for each
    // Now, also add space for 32 CRC bits and 1 crc flag 
    char buffer[750];
    bzero(buffer, sizeof(buffer));
    

    // Read the encoded chunk from the consumer through the decode pipe
    __ssize_t num_read = read(decode_pipe[0], buffer, sizeof(buffer));
    buffer[sizeof(buffer)] = '\0';
    //printf("\n Into decode %s\n", buffer);
    
    
    close(decode_pipe[0]);

    char res[69];
    bzero(res, 69);
    
    // Find CRC flag
    int crc_flag = (int)buffer[0] - 48;

    // Find frame flag
    int num = 0; //the ascii value of this byte
        // For each bit in the byte
        for (int j = (crc_flag? 1: 0); j < (crc_flag? 8: 7); j++)
            num += ((int)buffer[1+ 3*(crc_flag? 8:7)+j] - 48) * power(2, ((crc_flag? 7: 6)-j));
    frame = num - 1;

    // Check bits for error before converting

    if(crc_flag && frame) // Valid chat frame & CRC
    {
        checkCRC(buffer);
    }
    else if (!crc_flag){ // Hamming code
    
        // check + correct hamming
        checkHamming(&buffer[1 + 7*4]);

        // remove hamming bits
        removeHammingBits(&buffer[1 + 7*4]);
        
    }

    // For each byte... but NOT the 32/8 CRC bytes
    // AND NOT THE FIRST CRC BYTE!                         
    for (int i = 1; i < (strlen(buffer) - (crc_flag? 32: 0)); i+=(crc_flag? 8: 7)) {
        num = 0; //the ascii value of this byte
        
        // For each bit in the byte
        for (int j = (crc_flag? 1: 0); j < (crc_flag? 8: 7); j++)
            num += ((int)buffer[i+j] - 48) * power(2, ((crc_flag? 7: 6)-j));
            
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
