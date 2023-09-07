Getting started with c on linux
sudo apt update
sudo apt install gcc
nano my_program.c OR use IDE

ex:
#include <stdio.h>

int main()
{
    printf("Hello, World!\n");
    return 0;
}

navigate from terminal to where file is located and compile:
gcc -o my_program my_program.c

Run the program
./my_program