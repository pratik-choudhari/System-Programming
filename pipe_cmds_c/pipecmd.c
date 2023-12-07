#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <signal.h>

int main()
{
    // create first pipe between cat and grep processes
    int pipe1[2];
    if(pipe(pipe1) == -1){
        perror("Pipe1 creation failed!");
        exit(0);
    }

    int pid1 = fork();

    // if fork failes
    if (pid1 == -1)
    {
        perror("Fork failed!");
        exit(0);
    }

    // child process
    if (pid1 == 0)
    {
        // close read end and dup2 write end
        close(pipe1[0]);
        dup2(pipe1[1], STDOUT_FILENO);
        execlp("cat", "cat", "f23.txt", NULL); 
    }

    // create second pipe between grep and wc processes
    int pipe2[2];
    if(pipe(pipe2) == -1){
        perror("Pipe2 creation failed!");
        exit(0);
    }
    int pid2 = fork();

    // if fork failes
    if (pid2 == -1)
    {
        perror("Fork failed!");
        exit(0);
    }

    // child process
    if (pid2 == 0)
    {
        // communicate with child 1
        close(pipe1[1]);
        dup2(pipe1[0], STDIN_FILENO);

        // communicate with child 2
        close(pipe2[0]);
        dup2(pipe2[1], STDOUT_FILENO);
        execlp("grep", "grep", "Welcome", NULL); 
    }

    int pid3 = fork();

    // if fork failes
    if (pid3 == -1)
    {
        perror("Fork failed!");
        exit(0);
    }

    // child process
    if (pid3 == 0)
    {
        // close both ends of pipe 1 and write end of pipe2
        close(pipe1[1]);
        close(pipe1[0]);
        close(pipe2[1]);
        dup2(pipe2[0], STDIN_FILENO);
        execlp("wc", "wc", NULL); 
    }
}