#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>

#define PORT 9000
#define BACKLOG 10
#define FILEPATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

// Global variables
// static int server_fd = -1;
// static int client_fd = -1;

// // Function to handle cleanup on exit
// static void cleanup(void)
// {
//     syslog(LOG_INFO, "Cleaning up resources");

//     if (-1 != client_fd) {
//         shutdown(client_fd, SHUT_RDWR);
//         close(client_fd);
//     }

//     if (-1 != server_fd) {
//         shutdown(server_fd, SHUT_RDWR);
//         close(server_fd);
//     }

//     remove(FILEPATH);

//     syslog(LOG_INFO, "Cleanup done");
//     closelog();
// }

static void signal_handler(int signal_number)
{
    syslog(LOG_INFO, "Caught signal %d, exiting", signal_number);

    if (SIGINT == signal_number || SIGTERM == signal_number)
    {
        // cleanup();
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    // Open the syslog for logging
    openlog("aesdsocket", 0, LOG_USER);

    // Initialize signal handling
    struct sigaction new_action;

    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = signal_handler;

    // Set up the signal handlers for SIGINT and SIGTERM
    if (0 != sigaction(SIGINT, &new_action, NULL))
    {
        syslog(LOG_ERR, "Failed to set SIGINT handler: %s", strerror(errno));
        return 1;
    }

    if (0 != sigaction(SIGTERM, &new_action, NULL))
    {
        syslog(LOG_ERR, "Failed to set SIGTERM handler: %s", strerror(errno));
        return 1;
    }

    pause();


    // Cleanup resources on exit
    // cleanup();

    return 0;
}