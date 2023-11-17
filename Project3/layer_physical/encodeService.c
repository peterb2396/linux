#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

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

// Return the new length of the hamming code
// Given the length of the encoded data section
int newSize(int len)
{
    int i = 0;
    while (i < len)
    {
        i++;
        if (isPowerOfTwo(i))
            len++;
    }
    return len;
}

// Calculate the hamming code and return
char * make_hamming(char* data_in)
{
    

    // Length of the new string, which will include the parity bits
    int new_len = newSize(strlen(data_in));

    
    // The new data section, with parity bits
    char* res = malloc(new_len);
    
    int j = 0; // Index of original data
    for (int i = 0; i < new_len; i++)

        // Initialize parity bit locations to 0
        if (isPowerOfTwo(i + 1))
            res[i] = 'p';
            
        else
            res[i] = data_in[j++]; // Load the original data into the new bitstring
        
    
    // String is initialized. Set the parity bits
    int i = 1; // The parity bit at index i - 1
    while (i <= new_len)
    {
        int sum = 0;
        int j = i; // The step size (distance for parity bit i) ('Skip i')
        while (j < new_len)
        {
            // ('pick i') sum up i bits
            for (int k = 0; k < i; k++)
            {
                // Make sure we dont look past the end of the buffer
                if (j+k > new_len)
                    break;
                
                // Do not include the parity itself
                if (j+k == i)
                    continue;

                // Count the parity
                sum += res[j+k - 1] - '0'; // convert the char to int
            }
            j+=2*i; // Skip bits
        }
        res[i - 1] = (sum % 2) + '0';
        i*=2; // The next parity bit
    }

    // Print hamming code
    //printf("%s\n\n%s\n", res,data_in);
    return res;


}


int encodeFrame(int encode_pipe[2], int crc_flag)
{
    

    // Add space for 4 control chars
    char buffer[FRAME_LEN + 4 + 1];
    bzero(buffer, sizeof(buffer));
    

    // Read the frame from the producer through the encode pipe
    __ssize_t num_read = read(encode_pipe[0], buffer, sizeof(buffer));
    close(encode_pipe[0]);

    

    // The data, without crc or hamming bits
    char data[(FRAME_LEN + 4) * (crc_flag? 8: 7) + 1 + 1]; // +1 for CRC flag. Don't add parity bits for hamming yet
    memset(data, 0, strlen(data));

    // Append CRC flag
    char crc_flag_string[2];
    sprintf(crc_flag_string, "%d", crc_flag);
    strcat(data, crc_flag_string);

    for (int i = 0; i < num_read; i++) {
        char ch = buffer[i];
        // Add the parity but, if CRC. If hamming, we will compute these later
        if (crc_flag)
            strcat(data, __builtin_parity((int)ch)? "1" : "0"); // add the parity bit to the encoded data string

        // For the next 7 bits...
        for (int j = 6; j >= 0; j--) {
            // Determine whether the bit at position i should be one
            int bit = ((int)ch >> j) & 1;
            char bit_str[2];
            sprintf(bit_str, "%d", bit);
            
            strcat(data, bit_str);
        }
    }

    if (crc_flag)
    {
        // Send the data with CRC bits, T = D+R
        char * crc_res = CRC(data);
        int res = write(encode_pipe[1], crc_res, strlen(crc_res));
        //printf("\nEncoded: %s\n", crc_res);
        free(crc_res);
        
       
    }
    else{ // Hamming TBD
    // for now just write the data with no CRC bits
        //write(encode_pipe[1], data, strlen(data));
        //printf("\nEncoded: %s\n", data);
        
        // Seperate the data portion from the control chars
        char* data_portion = &data[1 + 4*7];
        
        char* ham_res = make_hamming(data_portion);
        // Can now isolate control sequence
        data[1 + 4*7] = '\0';

        char res[strlen(data) + strlen(ham_res)];
        sprintf(res, "%s%s", data, ham_res); // append hamming code to control flags
        free(ham_res);

        
        

        // The final result is control + hamming code
        write(encode_pipe[1], res, strlen(res));
        
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
    
    
    fflush(stdout);

    return encodeFrame(encode_pipe, crc_flag);
}
