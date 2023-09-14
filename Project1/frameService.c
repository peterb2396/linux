// frame.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 65

int main(int argc, char *argv[]) {
    char buffer[MAX_BUFFER_SIZE];
    int num_read;

    // Open the output file for writing the framed data
    char outputFilename[256]; 
    snprintf(outputFilename, sizeof(outputFilename), "./output/%s/%s.framed", argv[1], argv[1]);
    FILE* output_file = fopen(outputFilename, "ab");
    if (output_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while ((num_read = fread(buffer, 1, 64, stdin)) > 0) {
        char framed_message[68];
        framed_message[0] = (char)22;  // First SYN character
        framed_message[1] = (char)22;  // Second SYN character
        framed_message[2] = (char)num_read;  // Length
        fwrite(framed_message, 1, 3, output_file);  // Write the first 3 bytes
        fwrite(buffer, 1, num_read, output_file);  // Write the data block
    }

    fclose(output_file);

    return 0;
}
