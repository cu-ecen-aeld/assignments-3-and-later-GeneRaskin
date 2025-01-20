#include "systemcalls.h"

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
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int res = system(cmd);
    return (res == -1) ? false : true;
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
    int i, status;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    va_end(args);

    int pipefd[2]; // pipe to communicate execv failure in child
    if (pipe(pipefd) == -1) {
      perror("pipe failed");
      return false;
    }

/*
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    pid_t pid = fork();
    if (pid == 0) {
    	close(pipefd[0]); // close read end of the pipe
        if (execv(command[0], command) == -1) {
        	int err = errno;
            write(pipefd[1], &err, sizeof(err)); // send errno to the parent
            close(pipefd[1]);
	    	perror("execv failed");
	    	_exit(EXIT_FAILURE);
        }
    } else if (pid == -1) {
		perror("fork failed");
        close(pipefd[0]);
        close(pipefd[1]);
    	return false;
    }

	close(pipefd[1]); // close write end of the pipe

    if (wait(&status) == -1) {
		perror("wait failed");
    	return false;
    }

	int child_errno = 0;
	ssize_t bytes_read = read(pipefd[0], &child_errno, sizeof(child_errno));
	close(pipefd[0]); // close read end of the pipe

	if (bytes_read > 0) {
		//child execv failed
		fprintf(stderr, "Child execv failed with error: %s\n", strerror(child_errno));
		return false;
	}

    if (WIFEXITED(status)) {
		int exit_status = WEXITSTATUS(status);
		if (exit_status == 0)
    		return true;
		fprintf(stderr, "Command exited with status %d\n", exit_status);
    }

    fprintf(stderr, "Command terminated abnormally\n");
    return false;
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
    int i, status;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    va_end(args);

    int pipefd[2]; // pipe to communicate execv/dup2 failure in child
    if (pipe(pipefd) == -1) {
    	perror("pipe failed");
        return false;
    }
/*
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (fd == -1) {
    	PERROR_ARGS("Failed to open the outputfile %s", outputfile);
		return false;
    }

    pid_t pid = fork();

    if (pid == -1) {
		close(fd);
        close(pipefd[0]);
        close(pipefd[1]);
    	perror("fork failed");
		return false;
    } else if (pid == 0) {
    	close(pipefd[0]); // close read end of the pipe
    	if (dup2(fd, STDOUT_FILENO) == -1) {
	    	perror("dup2 failed");
	    	close(fd);
            int err = errno;
            write(pipefd[1], &err, sizeof(err));
            close(pipefd[1]); // close write end of the pipe
	    	_exit(EXIT_FAILURE);
		}
		close(fd);
		if (execv(command[0], command) == -1) {
	    	perror("execv failed");
            int err = errno;
            write(pipefd[1], &err, sizeof(err));
            close(pipefd[1]); // close write end of the pipe
    	    _exit(EXIT_FAILURE);	    
		}
    }

    close(fd);
    close(pipefd[1]); // close write end of the pipe

    if (wait(&status) == -1) {
    	perror("wait failed");
		return false;
    }

    int child_errno = 0;
    ssize_t bytes_read = read(pipefd[0], &child_errno, sizeof(child_errno));
    if (bytes_read > 0) {
    	// child dup2 or execv failed
        fprintf(stderr, "Child execv or dup2 failed with error: %s\n", strerror(child_errno));
        return false;
    }

    if (WIFEXITED(status)) {
    	int exit_status = WEXITSTATUS(status);
		if (exit_status == 0) {
	    	return true;
		}
		fprintf(stderr, "Command exited with status %d\n", exit_status);
		return false;
    }

    fprintf(stderr, "Command terminated abnormally\n");
    return false;
}
