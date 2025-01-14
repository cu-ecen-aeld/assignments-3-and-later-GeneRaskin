#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>

int main(int argc, char **argv) {
	openlog(NULL, LOG_PID, LOG_USER);
	if (argc != 3) {
		syslog(LOG_ERR, "Error: two arguments are required:"
			       	"<writefile> <writestring>");
		exit(EXIT_FAILURE);
	}

	char *writestring = argv[2];
	char *writefilePath = argv[1];
	FILE *writefile = fopen(writefilePath, "w");
	if (writefile == NULL) {
		syslog(LOG_ERR, "Failed to open the file %s: %s",
			       	writefilePath, strerror(errno));
		closelog();
		exit(EXIT_FAILURE);
	}
	size_t numChars = strlen(writestring);
	syslog(LOG_DEBUG, "Writing %s to %s", writestring, writefilePath);
	size_t numWritten = fwrite(writestring, sizeof(char), numChars, writefile);
	if (numWritten < numChars) {
		syslog(LOG_ERR, "Failed to write to the file %s: %s",
			       	writefilePath, strerror(errno));
		fclose(writefile);
		closelog();
		exit(EXIT_FAILURE);
	}

	fclose(writefile);
	closelog();
	exit(EXIT_SUCCESS);
}
