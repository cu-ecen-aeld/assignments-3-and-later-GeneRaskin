#include "aesdsocket.h"

volatile sig_atomic_t shutdown_requested = 0;

void signal_handler(int signum) {
    (void) signum;
    shutdown_requested = 1;
}

void send_file_contents(FILE *fp, int client_fd) {
    rewind(fp);
    char buffer[1024];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, sizeof(char), sizeof(buffer), fp)) > 0) {
        size_t sent_total = 0;
        while (sent_total < bytes_read) {
            ssize_t sent = send(client_fd, buffer + sent_total, bytes_read - 
                sent_total, 0);
            if (sent == -1) {
                perror("send");
                return;
            }
            sent_total += sent;
        }
    }
}

void handle_client(int client_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_fd, (struct sockaddr *)&client_addr, &addr_len);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    syslog(LOG_INFO, "Accepted connection from %s", ip_str);

    FILE *fp = fopen(DATA_FILE, "a+");
    if (!fp) {
        perror("fopen");
        close(client_fd);
        return;
    }

    char recv_buffer[1024];
    char *data = NULL;
    size_t data_size = 0;

    while (1) {
        ssize_t bytes_read = recv(client_fd, recv_buffer, sizeof(recv_buffer),
            0);
        if (bytes_read <= 0) {
            if (bytes_read == -1 && errno == EINTR && shutdown_requested)
                break;
            else if (bytes_read == 0)
                break;
            perror("recv");
            break;
        }

        data = realloc(data, data_size + bytes_read);
        memcpy(data + data_size, recv_buffer, bytes_read);
        data_size += bytes_read;

        char *newline;
        while ((newline = memchr(data, '\n', data_size)) != NULL) {
            size_t packet_len = newline - data + 1;
            fwrite(data, sizeof(char), packet_len, fp);
            fflush(fp);
            send_file_contents(fp, client_fd);

            memmove(data, newline + 1, data_size - packet_len);
            data_size -= packet_len;
        }
    }

    free(data);
    fclose(fp);
    close(client_fd);
    syslog(LOG_INFO, "Closed connection from %s", ip_str);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    switch((fork())) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            break;
        default: exit(EXIT_SUCCESS);
    }

    if (chdir("/") < 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int devnull = open("/dev/null", O_RDWR);
    if (devnull == -1) {
        perror("open /dev/null");
        exit(EXIT_FAILURE);
    }

    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDERR_FILENO);
    dup2(devnull, STDOUT_FILENO);
    close(devnull);
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;
    int option;
    int error = 0;

    opterr = 0;

    while ((option = getopt(argc, argv, "d")) != -1) {
        switch (option) {
            case 'd':
                daemon_mode = 1;
                break;
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c'\n", optopt);
                error = 1;
                break;
            default:
                error = 1;
                break;
        }
    }

    // Check for extra arguments
    if (optind < argc) {
        fprintf(stderr, "Error: Unexpected argument '%s'\n", argv[optind]);
        error = 1;
    }

    if (error) {
        fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        closelog();
        return EXIT_FAILURE;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        closelog();
        return EXIT_FAILURE;
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        closelog();
        return EXIT_FAILURE;
    }

    if (daemon_mode) {
        syslog(LOG_INFO, "Starting in daemon mode");
        daemonize();
    }

    while (!shutdown_requested) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd == -1) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        handle_client(client_fd);
    }

    close(server_fd);
    remove(DATA_FILE);
    syslog(LOG_INFO, "Caught signal, exiting");
    closelog();
    return EXIT_SUCCESS;
}