#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    int pid = fork();
    if (pid == 0)
    {
        printf("Child\n");
        execl("toUpperService", "toUpperService", "z", NULL);
    }
    else if (pid > 0)
    {
        wait(NULL);
        printf("Parent\n");
    }
    else
    {
        printf("Forking Error\n");
    }
}