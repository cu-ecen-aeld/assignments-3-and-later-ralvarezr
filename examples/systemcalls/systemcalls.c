#include "systemcalls.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    bool retval = false;

    int status = system(cmd);
    if (-1 == status) {
        // system failed to execute the command
        perror("Error executing command with system()");
    }
    else if (0 == status)
    {
        // The command was executed successfully
        printf("Command '%s' executed successfully\n", cmd);
        retval = true;
    }
    else
    {
        // The command returned a non-zero exit status
        fprintf(stderr, "Command '%s' failed with status %d\n", cmd, status);
    }

    return retval;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    bool retval = false;

    if (count < 1 || command[0] == NULL)
    {
        perror("Invalid arguments");
        va_end(args);
        return retval;
    }

    // Flush the stdout
    fflush(stdout);

    pid_t pid = fork();
    if (0 == pid)
    {
        // Child process
        execv(command[0], command);

        // If execv fails this line will be executed
        perror("execv failed");
        _exit(EXIT_FAILURE);
    }
    else if (-1 == pid)
    {
        // fork failed
        perror("fork failed\n");
    }
    else
    {
        // Parent process
        int status = 0;
        pid_t wait_pid = waitpid(pid, &status, 0);
        if (wait_pid == -1)
        {
            // waitpid failed
            perror("Error waiting for child process\n");
        }
        else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            retval = true; // Command executed successfully
            printf("Command '%s' executed successfully\n", command[0]);
        }
    }

    va_end(args);

    return retval;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    bool retval = false;

    if (count < 1 || command[0] == NULL || outputfile == NULL)
    {
        perror("Invalid arguments");
        va_end(args);
        return retval;
    }

    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (-1 == fd)
    {
        perror("Error opening output file for redirection");
        return retval;
    }

    // Flush the stdout
    fflush(stdout);

    pid_t pid = fork();
    if (0 == pid)
    {
        int status = dup2(fd, STDOUT_FILENO);
        if (status == -1)
        {
            perror("Error redirecting stdout to file");
            close(fd);
            _exit(EXIT_FAILURE);
        }

        close(fd); // Close the file descriptor after duplicating it

        // Child process
        execv(command[0], command);

        // If execv fails this line will be executed
        perror("execv failed");
        _exit(EXIT_FAILURE);
    }
    else if (-1 == pid)
    {
        // fork failed
        perror("fork failed\n");
        close(fd);
    }
    else
    {
        close(fd); // Close the file descriptor in the parent process

        // Parent process
        int status = 0;
        pid_t wait_pid = waitpid(pid, &status, 0);
        if (wait_pid == -1)
        {
            // waitpid failed
            perror("Error waiting for child process\n");
        }
        else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            retval = true; // Command executed successfully
            printf("Command '%s' executed successfully\n", command[0]);
        }
    }

    va_end(args);

    return true;
}
