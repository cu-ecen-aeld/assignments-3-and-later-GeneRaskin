#ifndef AESDSOCKET_H
#define AESDSOCKET_H

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <getopt.h>

#define PORT 9000
#define BACKLOG 5
#define DATA_FILE "/var/tmp/aesdsocketdata"

extern volatile sig_atomic_t shutdown_requested;

#endif