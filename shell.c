#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

char *input_str = NULL;
char **argv;
FILE *file;

int *childred_pids;
int NUM_OF_CHILDREN = 32;
int ARGC_MAX = 64;

pid_t child_pid = -1;

void resize_argv(char **argv)
{
    ARGC_MAX += 32;
    argv = (char**) realloc(argv, ARGC_MAX * sizeof(char *));
}

size_t parse_input()
{
    size_t input_len = 0, index = 0;

    getline(&input_str, &input_len, stdin);

    int is_parsing = 0;
    for (size_t i = 0; input_str[i]; i++)
    {
        if (isspace(input_str[i]))
            input_str[i] = 0, is_parsing = 0;
        else if (!is_parsing)
        {
            is_parsing = 1;
            argv[index++] = input_str + i;
            if(index == ARGC_MAX)
                resize_argv(argv);
        }
    }
    argv[index] = 0;
    return index;
}


void child_termination_callback(int signum)
{
    pid_t terminated_child_pid = waitpid(-1, NULL, WNOHANG);
    if(terminated_child_pid <= 0)
        terminated_child_pid = child_pid;
    // printf("[*] child process %d terminated\n", terminated_child_pid);
    fprintf(file, "child process %d terminated\n", terminated_child_pid);
    // terminated_child_pid = -1;
}


void initialize_shell()
{
    signal(SIGCHLD, child_termination_callback);
    childred_pids = (int *)malloc(32 * sizeof(int));
    memset(childred_pids, 0, sizeof(childred_pids));
    file = fopen("./shell.log", "w");
    setvbuf(file, NULL, _IONBF, 1024);
    argv = (char **)malloc(ARGC_MAX);
}

int main()
{

    initialize_shell();

    while (1)
    {
        free(input_str);
        printf("JARVIS> ");
        int argc = parse_input();
        if (argc == 0)
            continue;

        if (strncmp(argv[0], "exit", 64) == 0)
        {
            free(input_str);
            free(argv);
            fclose(file);
            free(childred_pids);
            exit(EXIT_SUCCESS);
        }

        child_pid = fork();

        int is_non_blocking = strncmp(argv[argc - 1], "&\0", 4);

        if (!is_non_blocking)
            argv[argc - 1] = NULL;

        if (child_pid < 0)
        {
            // printf("error creating child process\n");
            fprintf(file, "error creating child process `%s`\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        else if (child_pid == 0)
        {
            int error_status = 0;
            fprintf(file, "fork successful. creating child process `%s`\n", argv[0]);
            if (execvp(argv[0], argv) == -1)
            {
                // printf("ERROR OCCURED IN CHILD PROCESS\n");
                printf("error occured in child process %s\n", argv[0]);
                fprintf(file, "error occured in child process %s\n", argv[0]);
                error_status = -1;
            }else
            {
                // printf("[*] Child process %s has been created successfully\n", argv[0]);
                fprintf(file, "Child process %s has been created successfully\n", argv[0]);
            }
            
            if (error_status == -1) {
                fprintf(file, "Exiting child process %s with status code %d\n", argv[0], EXIT_FAILURE);
                exit(EXIT_FAILURE);
            }
            else {
                fprintf(file, "Exiting child process %s with status code %d\n", argv[0], EXIT_SUCCESS);
                exit(EXIT_SUCCESS);
            }

            free(input_str);
            free(argv);
            fclose(file);
        }
        else
        {
            if (argc == 1 || is_non_blocking)
                waitpid(child_pid, NULL, 0);
            else
                child_pid = -1;
        }
    }

    return 0;
}