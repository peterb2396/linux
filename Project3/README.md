# This program allows for chat between any number of clients.
### It includes features such as the following
1. User database for login/registration
2. Username/password validation
3. Status updates such as user has come online
4. Option to eject all data from the database
5. Option to leave the chat and come back later
6. Chat history restored each session
7. If a user is offline, messages will forward on their next login
8. Automatic user list refresh when a new user registers
9. Framing, encoding, malforming, decoding, deframing of data.
10. Error detection by CRC and hamming.
11. File simulator that runs automatically to show error insertion / detection / framing
12. Sophisticated folder structure for input, output and debug files to be viewed
13. Multithreaded capitalization service.

### NOTE ABOUT THIS PROJECT
1. This project was created as a fork of the previous CRC/HAMMING-Chat-Transmitter.
2. Input files display CRC / HAMMING working perfectly
3. Chat error insertion was disabled (but detection was not)
4. The above was to instead display the new feature: Multithreaded vowel capitalization!
5. The new feature added files ```helper.c```, ```capitalizeService.c```.

### THE CAPITALIZATION SPECIFICATION
The new feature is performed by a new helper node. Data is decoded and sent to this node by a serverDecoder.
The new feature was added to display 6 threads working on the same data, waiting for each other to finish so that sensitive data is protected and not accessed in bad times with race conditions. 5 threads for vowels exist each with a queue of length 5.

Thread 1 is responsible for the letter a and is the only thread unlocked to begin with. it processes 5 characters of the input file, capitilizing any a's and passing the queue to the next thread, thread 2, which is then unlocked to operate on all e's, and so on. The next 5 letters enter thread 1's queue in a pipelined nature to process the next 5 characters.

The result is again encoded by serverEncoder and sent back to the original server to transmit to clients and store in their history.

## How to run this program
1. Navigate to this containing folder
2. Run ```./server```
3. Run ```./client``` for each client terminal

Note if running between machines, you can use ```./client <IP> <PORT>```
to specify a custom IP and PORT.

Similarly, to run the server on a custom port, you can use ```./server <PORT>```
However, it is not necessary to specify a custom port.

## Program Notes
1. A client can type ```/logout``` at anytime to DELETE their account and all associated data
2. A client can type ```/exit``` at anytime to simply leave the chat, persisting their data
3. The server terminal will show status notes such as client connections and requests.
4. Every other client will process errors with CRC. The others, Hamming.
5. The server will by default execute files.c for all input files. This program will generate files that show framing a file, encoding to binary, and malforming one bit as well as detecting the error with CRC or hamming.

## If you want to edit source code and recompile,
1. Server: ```gcc -o server server.c -lpthread```
2. Client: ```gcc -o client client.c -lm```
3. Service files: ```gcc -o <serviceName> <serviceName>.c -lm```
4. File Malformer: ```gcc -o files files.c -lm```



## Getting started with c on linux
1. ```sudo apt update```
2. ```sudo apt install gcc```
3. ```nano my_program.c``` OR use IDE

Hello World example:
```
#include <stdio.h>

int main()
{
    printf("Hello, World!\n");
    return 0;
}
```

Finding and killing locking port
```
lsof -i tcp:PORT
kill -3 PID
```

Navigate from terminal to where file is located and compile:

```
gcc -o my_program my_program.c
```

Run the program
```
./my_program
```

