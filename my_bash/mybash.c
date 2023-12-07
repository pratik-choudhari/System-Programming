#define _XOPEN_SOURCE 500
#define MAX_CMD_LEN 512
#define MAX_BUF_LEN 5120
#define ANSI_COLOR "\x1b[33m"
#define ANSI_COLOR_RESET "\x1b[0m"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>

// structure to pass multiple values from exec_cmd function
struct ExecRetValue
{
    int exit_status;
    char output_buffer[MAX_BUF_LEN];
    int pipe_read_end;
};

// create special characters array
const char SPECIAL_CHARACTERS[5] = {'|', '<', '>', '&', ';'};

/* returns operation type by checking the last two characters e.g conditional, piping, redirection
Returns:
0: no op
1: && op
2: || op
3: ; op
4: > op
5: >> op
6: < op
7: | op
8: & op
*/
int get_operation_type(char *input_string, int op_type)
{
    // printf("Input for operation type: %s\n", input_string);
    if (op_type == 4 || op_type == 5)
    {
        return op_type;
    }

    if (input_string[strlen(input_string) - 1] == '&')
    {
        if (input_string[strlen(input_string) - 2] == '&')
            return 1;
        else
            return 8;
    }
    else if (input_string[strlen(input_string) - 1] == '|')
    {
        if (input_string[strlen(input_string) - 2] == '|')
            return 2;
        else
            return 7;
    }
    else if (input_string[strlen(input_string) - 1] == ';')
    {
        return 3;
    }
    else if (input_string[strlen(input_string) - 1] == '>')
    {
        if (input_string[strlen(input_string) - 2] == '>')
            return 5;
        else
            return 4;
    }
    else if (input_string[strlen(input_string) - 1] == '>')
    {
        return 4;
    }
    else if (input_string[strlen(input_string) - 1] == '<')
    {
        return 6;
    }
    return 0;
}

/* remove trailing and leading spaces*/
char *remove_spaces(char *data)
{
    while (isspace((char)*data))
    {
        data++;
    }

    char *last = data + strlen(data) - 1;
    while (last > data && isspace((char)*last))
    {
        last--;
    }

    last[1] = '\0';
    return data;
}

/* executes given string after cleaning of leading and trailing special characters; returns type of exit performed by child process
Returns:
1: normal exit of child
2: abnormal exit*/
struct ExecRetValue exec_cmd(char *input_str, int pipe_input, int op_type)
{
    struct ExecRetValue ret_val;
    memset(&ret_val.output_buffer[0], 0, sizeof(ret_val.output_buffer));
    ret_val.exit_status = -1;

    char tmp_data[MAX_BUF_LEN] = "";
    strncpy(tmp_data, input_str, strlen(input_str));

    // remove special characters from start of string
    for (int idx2 = 0; idx2 < 5; idx2++)
    {
        if (tmp_data[strlen(tmp_data) - 1] == SPECIAL_CHARACTERS[idx2])
        {
            if (SPECIAL_CHARACTERS[idx2] != '&')
                tmp_data[strlen(tmp_data) - 1] = '\0';

            if (tmp_data[strlen(tmp_data) - 1] == SPECIAL_CHARACTERS[idx2])
            {
                tmp_data[strlen(tmp_data) - 1] = '\0';

                // because & also means run as bg process, special condition for &
                if (SPECIAL_CHARACTERS[idx2] == '&')
                    tmp_data[strlen(tmp_data) - 1] = '\0';
            }
            break;
        }
    }
    // printf("%s\n", tmp_data);
    char *data = remove_spaces(tmp_data);

    // copy executable name from data
    char executable[MAX_CMD_LEN] = "";
    int tmp;
    for (tmp = 0; tmp < strlen(data); tmp++)
    {
        if (data[tmp] == ' ')
            break;
    }
    strncpy(executable, data, tmp);

    // break string command to array of string to pass to execvp
    char *arguments[10];
    tmp = 0;
    char *token = strtok(data, " ");

    while (token != NULL)
    {
        arguments[tmp] = token;
        tmp++;
        token = strtok(NULL, " ");
    }
    arguments[tmp] = NULL;

    int pipe1[2];

    if (pipe(pipe1) == -1)
    {
        perror("Pipe failed\n");
        ret_val.exit_status = -1;
        return ret_val;
    }

    int pid = fork();

    if (pid == 0)
    {
        close(pipe1[0]);
        dup2(pipe1[1], STDOUT_FILENO);
        if (pipe_input > 0)
        {
            dup2(pipe_input, STDIN_FILENO);
        }

        if (execvp(executable, arguments) == -1)
            exit(EXIT_FAILURE);
        exit(EXIT_SUCCESS);
    }
    else if (pid > 0 && op_type != 8)
    {
        int status;
        wait(&status);

        if (WIFEXITED(status))
        {
            if (WEXITSTATUS(status) == 1)
                ret_val.exit_status = 2;
            else
                ret_val.exit_status = 1;
        }
        else
        {
            ret_val.exit_status = 2;
        }

        close(pipe1[1]);
        ret_val.pipe_read_end = pipe1[0];
    }
    return ret_val;
}

/* return t/f based on commands, checks the argc for decision*/
int is_invalid_cmd(char *input_str)
{
    char data[MAX_CMD_LEN] = "";
    strncpy(data, input_str, strlen(input_str));
    for (int idx2 = 0; idx2 < 5; idx2++)
    {
        if (data[strlen(data) - 2] == SPECIAL_CHARACTERS[idx2] && data[strlen(data) - 1] == SPECIAL_CHARACTERS[idx2])
        {
            data[strlen(data) - 2] = '\0';
            break;
        }
        else if (data[strlen(data) - 1] == SPECIAL_CHARACTERS[idx2])
        {
            data[strlen(data) - 1] = '\0';
            break;
        }
    }

    int argc = 1;
    for (int tmp = 0; tmp < strlen(data); tmp++)
    {
        if (tmp != strlen(data) - 1 && data[tmp] == ' ' && (isalnum(data[tmp + 1]) || data[tmp + 1] == '-'))
            argc++;
    }

    if (1 <= argc && argc <= 3)
        return 0;
    return 1;
}

/* This program works by dividing the user input into sub commands and executing them, after execution the pipe read end which contains the output of command is sent back */
int main(int argc, char *argv[])
{
    char user_input[MAX_CMD_LEN] = "", tmp_buf[MAX_BUF_LEN] = "";
    int is_invalid = 0, op_type = -1, prev_op_type = -1, fd = -1;
    FILE *fptr;
    char *sub_cmd = NULL;
    struct ExecRetValue ret_val, prev_ret_value;

    while (1)
    {
        printf(ANSI_COLOR "\nmybash$ " ANSI_COLOR_RESET);

        if (fgets(user_input, sizeof(user_input), stdin) == NULL)
        {
            continue;
        }

        user_input[strlen(user_input) - 1] = '\0';

        if (strcmp(user_input, "exit") == 0)
        {
            printf("Exiting...\n");
            exit(EXIT_SUCCESS);
        }

        int begin = -1, end = -1, idx = 0;
        while (idx < strlen(user_input))
        {
            if (idx == 0)
                begin = 0;
            else if (idx == strlen(user_input) - 1)
                end = strlen(user_input);

            for (int idx2 = 0; idx2 < 5; idx2++)
            {
                if (user_input[idx] == SPECIAL_CHARACTERS[idx2])
                {
                    if (begin != -1)
                    {
                        if ((user_input[idx] == '&' && user_input[idx + 1] == '&') || (user_input[idx] == '|' && user_input[idx + 1] == '|') || (user_input[idx] == '>' && user_input[idx + 1] == '>'))
                            end = ++idx + 1;
                        else
                            end = ++idx;
                    }
                    break;
                }
            }

            if (begin != -1 && end != -1)
            {
                // get sub command as a substring of user input
                memset(&tmp_buf[0], 0, sizeof(tmp_buf));
                strncpy(tmp_buf, user_input + begin, end - begin);

                // remove spaces and validate the command
                sub_cmd = remove_spaces(tmp_buf);
                is_invalid = is_invalid_cmd(sub_cmd);
                if (is_invalid)
                {
                    printf("Invalid argc count: %s\n", sub_cmd);
                    break;
                }

                op_type = get_operation_type(sub_cmd, prev_op_type);
                // printf("Op type: %d, Prev op type: %d\n", op_type, prev_op_type);

                // execute the command
                ret_val = exec_cmd(sub_cmd, prev_op_type == 7 ? prev_ret_value.pipe_read_end : -1, op_type);

                // print output to STDOUT
                if (op_type == 0 || op_type == 1 || op_type == 2)
                {
                    read(ret_val.pipe_read_end, ret_val.output_buffer, sizeof(ret_val.output_buffer));
                    printf("%s", ret_val.output_buffer);
                    close(ret_val.pipe_read_end);
                }

                // process failed and && operator is used
                if (op_type == 1 && ret_val.exit_status == 2)
                {
                    printf("Command execution failed: %s\n", sub_cmd);
                    break;
                }
                // process success || operator is used
                else if (op_type == 2 && ret_val.exit_status == 1)
                    break;
                // current command is file name and last command had > or >> operator
                else if ((op_type == 4 || op_type == 5) && sub_cmd[strlen(sub_cmd) - 1] != '>')
                {
                    if (op_type == 4)
                        fptr = fopen(sub_cmd, "w");
                    else
                    {
                        fptr = fopen(sub_cmd, "a");
                    }

                    if (fptr == NULL)
                    {
                        printf("Redirection to file failed\n");
                        close(fd);
                        break;
                    }

                    while (read(prev_ret_value.pipe_read_end, prev_ret_value.output_buffer, sizeof(prev_ret_value.output_buffer)) > 0){
                        fprintf(fptr, "%s", prev_ret_value.output_buffer);
                        printf("%s", prev_ret_value.output_buffer);
                    }

                    fclose(fptr);
                    op_type = -1;
                }

                begin = end;
                end = -1;

                prev_ret_value = ret_val;
                prev_op_type = op_type;
            }
            idx++;
        }

        // cleanup the variables
        memset(&tmp_buf[0], 0, sizeof(tmp_buf));
        memset(&user_input[0], 0, sizeof(user_input));
        sub_cmd = NULL;
        is_invalid = 0;
    }

    return 0;
}
