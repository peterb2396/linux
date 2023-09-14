#include <stdio.h>

int main() {
    FILE *inputFile = fopen("framed_data.binf", "rb");

    if (inputFile == NULL) {
        perror("Error opening input file");
        return 1;
    }

    FILE *outputFile = fopen("binary_output.binf", "w");

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

    printf("Conversion complete. Output written to binary_output.txt\n");

    return 0;
}
