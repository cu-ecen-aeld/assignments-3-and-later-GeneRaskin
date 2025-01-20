#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

bool do_system(const char *command);

bool do_exec(int count, ...);

bool do_exec_redirect(const char *outputfile, int count, ...);

#define PERROR_ARGS(FMT,...) \
	fprintf(stderr, FMT ": %s\n", ##__VA_ARGS__, strerror(errno))
