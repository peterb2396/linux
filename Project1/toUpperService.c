#include <stdio.h>
#include <ctype.h>
#include "encDec.h"

int main(int argc, char *argv[])
{
    printf("%c\n", toUpper(argv[1]));
}

int toUpper(char *inData)
{
    if((int)*inData >='a' && (int)*inData <= 'z')
        return (toupper(*inData));
}