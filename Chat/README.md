# This program is a collection of services which simulate the transmission of data.
### Add any input text file and observe stages of framing, encoding and so on.


## How to run this program
1. Navigate to /layer_application
2. Run ```./app```

If you want to edit source code and recompile,
1. Main app: ```gcc -o app app.c -lm```
2. Service files: ```gcc -o <serviceName> <serviceName>.c```



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

Navigate from terminal to where file is located and compile:

```
gcc -o my_program my_program.c
```

Run the program
```
./my_program
```

## PIPE NOTES
Client writes to service pipe then closes write end.
Client waits for service to terminate
Client reads from pipe
Client closes read end

Service reads input data and then closes read end.
Service writes result(s) and closes write end
Service terminates

Deadlock will occur if two processes are waiting on each other to read/write,
often caused by failure to close pipe ends or misplaced locations.

## EXAMPLE SERVICE:

```
int service_pipe[2];
if (pipe(service_pipe) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
}

// Attempt to fork so child can exec the subroutine
fflush(stdout);
pid_t service_pid = fork();
if (service_pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
}
// service
if (service_pid == 0)
{
    // Make string versions of the pipe id's to pass to argv
    char service_read[10]; // Buffer for converting arg1 to a string
    char service_write[10]; // Buffer for converting arg2 to a string

    // Convert integers to strings
    snprintf(service_read, sizeof(service_read), "%d", service_pipe[0]);
    snprintf(service_write, sizeof(service_write), "%d", service_pipe[1]);
    
    // Child process: Call service then die
    execl("serviceService", "serviceService", service_read, service_write, NULL);
    perror("execl");  // If execl fails
    exit(EXIT_FAILURE);
}
else //parent
{
    // Write data to be serviced through service pipe
    write(service_pipe[1], data, data_len);
    close(service_pipe[1]);  // Done writing frame to be serviced

    // When child is done, read result
    wait(NULL);

    // Parent reads result from the child process (the serviced data)
    char serviced_data[RESPONSE_LEN]; // The serviced data

    // Listen for & store serviced frame
    int serviced_len = read(service_pipe[0], serviced_data, sizeof(serviced_data));
    close(service_pipe[0]);  // Done reading service data

    // Here, we have the serviced_data! Use it.

    fwrite(serviced_data, sizeof(char), serviced_data, file);

}
```


TRANSMISSION ERRORS
One bit at random is flipped to simulate a transmission error.
A frame is selected at random, and then a random bit of the data section is flipped.

EXAMPLE: (explained below)

JOKE:
MCAFEE-QUESTION: IS WINDOWS A VIRUS?
NO, WINDOWS IS NOT A VIRUS. HERE'S WHAT VIRUSES DO:
1. THEY REPLICATE QUICKLY-OKAY, WINDOWS DOES THAT.
2. VIRUSES USE UP VALUABLE SYSTEM RESOURCES, SLOWING DOWN THE SYSTEM AS THEY DO SO-OKAY,
WINDOWS DOES THAT.
3. VIRUSES WILL, FROM TIME TO TIME, TRASH YOUR HARD DISK-OKAY, WINDOWS DOES THAT TOO.
4. VIRUSES ARE USUALLY CARRIED, UNKNOWN TO THE USER, ALONG WITH VALUABLE PROGRAMS AND
SYSTEMS. SIGN... WINDOWS DOES THAT, TOO.
5. VIRUSES WILL OCCASIONALLY MAKE THE USER {USPECT THEIR SYSTEM IS TOO SLOW (SEE 2.) AND THE
USER WILL BUY NEW HARDWARE. YUP, THATS WITH WINDOWS, TOO.
UNTIL NOW IT SEEMS WINDOWS IS A VIRUS BUT THERE ARE FUNDAMENTAL DIFFERENCES:
VIRUSES ARE WELL SUPPORTED BY THEIR AUTHORS, ARE RUNNING ON MOST SYSTEMS, THEIR PROGRAM
CODE IS FAST, COMPACT AND EFFICIENT AND THEY TEND TO BECOME MORE SOPHISTICATED AS THEY
MATURE.
SO, WINDOWS IS NOT A VIRUS. IT'S A BUG.

Note after point 5., 'SUSPECT' was changed to '{USPECT' because bit # 524 of frame #8 / 15 was flipped