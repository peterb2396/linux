#include <stdio.h>
#include <ctype.h>
#include "encDec.h" // The project header file defines all processes required.

// Main service gets ran here when calling exec. 
// this simply prints the character returned by the required ctype function wrapper.
int main(int argc, char *argv[])
{
    printf("%c\n", toUpper(argv[1]));
}

// Wraps existing function into my own
// Capitalizes and returns the passed character.
int toUpper(char *inData)
{
    if((int)*inData >='a' && (int)*inData <= 'z')
        return (toupper(*inData));
}