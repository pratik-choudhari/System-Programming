// Creates dummy processes for testing
// get all user processes: ps -u
// get zombie processes: ps -u | grep 'Z'

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main()
{
    int pid1 = fork();

    // if fork failes
    if (pid1 == -1)
    {
        perror("Fork failed!");
        exit(0);
    }

    int pid2 = fork();

    // if fork failes
    if (pid2 == -1)
    {
        perror("Fork failed!");
        exit(0);
    }
    // first child
    if (pid2 == 0 & pid1 > 0)
    {
        int pid3 = fork();
        int pid8 = fork();
        if (pid3 > 0 & pid8 >0)
        {
            for (;;)
            {
            }
        }
        else
        {
            int pid4 = fork();
            if (pid4 > 0)
            {
                int pid6 = fork();
                if (pid6 > 0)
                {
                    for (;;)
                    {
                    }
                }
                for (;;)
                {
                }
            }
            else
            {
                int pid5 = fork();
                if (pid5 > 0)
                {
                    for (;;)
                    {
                    }
                }
                else
                {
                    int pid7 = fork();
                    if (pid7 > 0)
                    {
                        for (;;)
                        {
                        }
                    }
                    else{
                        int pid8 = fork();
                        if(pid8 > 0){
                        for (;;)
                        {
                        }
                        }
                    }
                }
            }
        }
    }
    else if (pid1 > 0 & pid2 > 0)
    {
        printf("%d\n", getpid());
        for (;;)
        {
        }
    }
    return 0;
}