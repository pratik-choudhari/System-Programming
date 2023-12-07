/*

Commands summary:
Fetch parent process id: ps -o ppid= -p <process-id> | grep -Eo '[0-9]+'
Fetch immediate children of a process: pgrep -P <process-id>
Get status of a process: ps --no-headers -o stat <process-id>
Get all children of a process: pstree -p <process-id> | grep -o '([0-9]\\+)' | grep -o '[0-9]\\+'

Options:
- dn: additionally lists the PIDs of all the non-direct descendants of process_id1 (only)
- id: additionally lists the PIDs of all the immediate descendants of process_id1
- lp: additionally lists the PIDs of all the sibling processes of process_id1
- sz: additionally Lists the PIDs of all sibling processes of process_id1 that are defunct
- gp: additionally lists the PIDs of all the grandchildren of process_id1
- zz: additionally prints the status of process_id1 (Defunct/ Not Defunct)
- zc: additionally lists the PIDs of all the direct descendants of process_id1 that are currently in the defunct state
- zx: additionally lists the PIDs of the direct descendants of process_id1..process_id[n] that are currently in the defunct state
*/

#define _XOPEN_SOURCE 500
#define MAX_CMD_LEN 512
#define MAX_BUF_LEN 1024
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>

// returns parent process id for a given process
int fetch_ppid(int pid){
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, MAX_CMD_LEN, "ps -o ppid= -p %d | grep -Eo '[0-9]+'", pid);
    FILE *output_stream = popen(cmd, "r");
    if (output_stream == NULL)
    {
        printf("[ERROR] Failed to execute command %s\n", cmd);
        return -1;
    }
    char buf[MAX_BUF_LEN];
    char *output[MAX_BUF_LEN];
    int output_size = 0;

    if(fgets(buf, sizeof(buf), output_stream) == NULL){
        printf("[ERROR] Fetching output from stream\n");
        return -1;
    }

    pclose(output_stream);
    return atoi(buf);
}

// fetch array of immediate children of a process, store result in a buffer passed as input
void fetch_immediate_child(int pid, int* buf){
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, MAX_CMD_LEN, "pgrep -P %d", pid);
    FILE *output_stream = popen(cmd, "r");
    if (output_stream == NULL)
    {
        printf("[ERROR] Failed to execute command %s\n", cmd);
        return;
    }
    char tmp_buf[MAX_BUF_LEN];
    int idx=0;
    buf[0] = 0;

    while (fgets(tmp_buf, sizeof(tmp_buf), output_stream) != NULL)
    {
        buf[++idx] = atoi(tmp_buf);
    }

    // first element in array is the array length
    buf[0] = idx;
    pclose(output_stream);
}

// get status of a process e.g. Z, Z+, Ss, Ss+, store result in a buffer passed as input
void fetch_process_status(int pid, char* buf){
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, MAX_CMD_LEN, "ps --no-headers -o stat %d", pid);
    FILE *output_stream = popen(cmd, "r");
    if (output_stream == NULL)
    {
        printf("[ERROR] Failed to execute command %s\n", cmd);
        return;
    }

    if(fgets(buf, sizeof(buf), output_stream) == NULL){
        printf("[ERROR] Fetching output from stream\n");
        return;
    }

    pclose(output_stream);
    return;
}

// fetch entire process process tree for a particular process, store result in a buffer passed as input
void fetch_all_child(int pid, int *buf){
    char cmd[MAX_CMD_LEN];
    snprintf(cmd, MAX_CMD_LEN, "pstree -p %d | grep -o '([0-9]\\+)' | grep -o '[0-9]\\+'", pid);
    FILE *output_stream = popen(cmd, "r");
    if (output_stream == NULL)
    {
        printf("[ERROR] Failed to execute command %s\n", cmd);
        return;
    }
    char tmp_buf[MAX_BUF_LEN];
    int idx=0;
    buf[0] = 0;

    while (fgets(tmp_buf, sizeof(tmp_buf), output_stream) != NULL)
    {
        if(atoi(tmp_buf) == pid){
            continue;
        }
        buf[++idx] = atoi(tmp_buf);
    }
    // first element in array is the array length
    buf[0] = idx;
    pclose(output_stream);
}

// controller function
void prcsearch(int root_pid, int *process_ids, int num_processes, char* option)
{
    int all_children[MAX_BUF_LEN];
    fetch_all_child(root_pid, all_children);

    int ppid = 0;
    // check if process ids passed in command line parameters are in the process tree of root process, if so print pid and ppid
    for(int i = 1; i <= all_children[0]; i++){
        for (int j = 0; j < num_processes; j++)
        {
            if (process_ids[j] == all_children[i])
            {
                ppid = fetch_ppid(process_ids[j]);
                printf("%d %d\n", process_ids[j], ppid);
            }
        }
    }

    // evaluate options
    if(option == NULL){
        return;
    }
    else if(strcmp(option, "-id") == 0 | strcmp(option, "-dn") == 0 | strcmp(option, "-zc") == 0){
        int immediate_children[MAX_BUF_LEN];
        fetch_immediate_child(process_ids[0], immediate_children);

        // Option: -id, print immediate children
        if(strcmp(option, "-id") == 0){
            for(int i = 1; i <= immediate_children[0]; i++){
                printf("%d\n", immediate_children[i]);
            }
        }
        // Option: -zc, for every immediate children check the status, if status is Z or Z+ then print pid
        else if(strcmp(option, "-zc") == 0){
            char status[5];
            for(int i = 1; i <= immediate_children[0]; i++){
                fetch_process_status(immediate_children[i], status);
                if(status[0] == 'Z'){
                    printf("%d\n", immediate_children[i]);
                }
            }
        }
        // Option: -dn, Iterate over all children and immediate descendants, pids not in immediate descendants as non immediate descendants
        else{
            fetch_all_child(process_ids[0], all_children);
            bool is_immediate = false;
            for(int i = 1; i <= all_children[0]; i++){
                is_immediate = false;
                for (int j = 1; j <= immediate_children[0]; j++)
                {
                    if(immediate_children[j] == all_children[i]){
                        is_immediate = true;
                        break;
                    }
                }
                if(!is_immediate){
                    printf("%d\n", all_children[i]);
                }
            }
        }
    }
    // Option: -zx, fetch immediate children for all process ids passed as command line parameter and check status, if status is Z or Z+ then print pid
    else if(strcmp(option, "-zx") == 0){
        int immediate_children[MAX_BUF_LEN];
        char status[5];

        for(int idx = 0; idx < num_processes; idx++){
            fetch_immediate_child(process_ids[idx], immediate_children);

            for(int i = 1; i <= immediate_children[0]; i++){
                fetch_process_status(immediate_children[i], status);
                if(status[0] == 'Z'){
                    printf("%d\n", immediate_children[i]);
                }
            }
        }
    }
    // Option: -gp, fetch immediate children of process_id1 from command line paramter and find their immediate children to give grand children of process_id1
    else if(strcmp(option, "-gp") == 0){
        int immediate_children[MAX_BUF_LEN];
        int inner_immediate_children[MAX_BUF_LEN];
        char status[5];

        fetch_immediate_child(process_ids[0], immediate_children);

        for(int i = 1; i <= immediate_children[0]; i++){
            fetch_immediate_child(immediate_children[i], inner_immediate_children);
            for(int j = 1; j <= inner_immediate_children[0]; j++){
                printf("%d\n", inner_immediate_children[j]);
            }
        }
    }
    // Option: -lp, fetch parent process id for process_id1 and print all immediate children of parent process excluding process_id1
    else if(strcmp(option, "-lp") == 0 | strcmp(option, "-sz") == 0){
        ppid = fetch_ppid(process_ids[0]);
        int immediate_children[MAX_BUF_LEN];
        fetch_immediate_child(ppid, immediate_children);

        char status[5];
        for(int i = 1; i <= immediate_children[0]; i++){
            if(immediate_children[i] != process_ids[0]){
                if(strcmp(option, "-lp") == 0){
                    printf("%d\n", immediate_children[i]);
                }
                else{
                    fetch_process_status(immediate_children[i], status);
                    if(status[0] == 'Z'){
                        printf("%d\n", immediate_children[i]);
                    }
                }
            }
        }
    }
    // Option: -zz, fetch status of process_id1, print Defunct/Not Defunct based on status
    else if(strcmp(option, "-zz") == 0){
        char status[5];
        fetch_process_status(process_ids[0], status);
        if(status[0] == 'Z'){
            printf("Defunct\n");
        }
        else{
            printf("Not Defunct\n");
        }
    }

}

// main function
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s root_process process_id1 [process_id(n)] [option]\n", argv[0]);
        return 0;
    }

    int root_pid = atoi(argv[1]);
    int process_ids[6], num_processes = 0;
    char *option = NULL;

    for (int i = 2; i <= argc - 1; i++)
    {
        if (isdigit(argv[i][0]))
        {
            process_ids[num_processes++] = atoi(argv[i]);
        }
        else
        {
            option = argv[i];
        }
    }

    // print input on console to verify is input is received
    printf("Option received: %s\n", option);

    printf("Root process: %d\n", root_pid);
    printf("Following processes will be evaluated:\n");
    for (int i = 0; i < num_processes; i++)
    {
        printf("%d\n", process_ids[i]);
    }

    printf("\n========================================================\n\n");
    prcsearch(root_pid, process_ids, num_processes, option);
    return 0;
}
