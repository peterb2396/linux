#include <stdio.h>

int main(int argc, char* argv[]) {
    char framed_file_name[256]; 
    snprintf(framed_file_name, sizeof(framed_file_name), "./output/%s/%s.framed", argv[1], argv[1]);
    FILE *inputFile = fopen(framed_file_name, "rb");

    if (inputFile == NULL) {
        perror("Error opening input file");
        return 1;
    }

    // Name the output file 
    char output_file_name[256]; 
    snprintf(output_file_name, sizeof(output_file_name), "./output/%s/%s.binf", argv[1], argv[1]);
    FILE *outputFile = fopen(output_file_name, "w");

    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(inputFile);
        return 1;
    }

    int ch;

    while ((ch = fgetc(inputFile)) != EOF) {
        // add parity bit
        fprintf(outputFile, "%d", __builtin_parity(ch)? 0 : 1);
        
        // For the next 7 bits...
        for (int i = 6; i >= 0; i--) {
            // Determine whether the bit at position i should be one
            int bit = (ch >> i) & 1;
            fprintf(outputFile, "%d", bit);
        }
    }

    fclose(inputFile);
    fclose(outputFile);

    return 0;
}
