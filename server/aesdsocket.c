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

#define PORT "9000"
#define BACKLOG 10
#define FILEPATH "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

// Global variables
static int server_fd = -1;
static int client_fd = -1;

// Function to handle cleanup on exit
static void cleanup(void)
{
    syslog(LOG_INFO, "Cleaning up resources");

    if (-1 != client_fd) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
    }

    if (-1 != server_fd) {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
    }

    remove(FILEPATH);

    syslog(LOG_INFO, "Cleanup done");
    closelog();
}

// Function to handle signals
static void signal_handler(int signal_number)
{
    syslog(LOG_INFO, "Caught signal %d, exiting", signal_number);

    if (SIGINT == signal_number || SIGTERM == signal_number)
    {
        // cleanup();
        exit(0);
    }
}

// Function to get the address from a sockaddr structure
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    else
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    // Create variables
    struct addrinfo hints;                              // Hints for getaddrinfo
    struct addrinfo *res = NULL;                        // Result from getaddrinfo
    struct sockaddr_storage client_addr;                // Client address structure
    socklen_t client_addr_len = sizeof(client_addr);    // Length of client address structure
    char buffer[BUFFER_SIZE];                           // Buffer for data transfer
    char client_ip[INET_ADDRSTRLEN];                    // Buffer for client IP address
    ssize_t bytes_received;                             // Number of bytes received from client
    struct sigaction new_action;                        // Structure for signal handling
    FILE *fd;                                           // File descriptor pointer for writing to the file

    // Open the syslog for logging
    openlog("aesdsocket", LOG_PID | LOG_PERROR, LOG_USER);  //*********************** REMOVE LOG_PERROR */

    // Initialize signal handling
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_handler = signal_handler;

    // Set up the signal handlers for SIGINT and SIGTERM
    if (0 != sigaction(SIGINT, &new_action, NULL))
    {
        syslog(LOG_ERR, "Failed to set SIGINT handler: %s", strerror(errno));
        cleanup();
        return -1;
    }

    if (0 != sigaction(SIGTERM, &new_action, NULL))
    {
        syslog(LOG_ERR, "Failed to set SIGTERM handler: %s", strerror(errno));
        cleanup();
        return -1;
    }

    // Initialize the hints structure
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;        // Use my IP

    // Get address information
    if (0 != getaddrinfo(NULL, PORT, &hints, &res))
    {
        syslog(LOG_ERR, "getaddrinfo failed: %s", gai_strerror(errno));
        cleanup();
        return -1;
    }

    // Create the socket
    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (-1 == server_fd)
    {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        cleanup();
        freeaddrinfo(res);
        return -1;
    }
    syslog(LOG_INFO, "Socket created successfully");

    // Set the socket to be reusable
    int optval = 1;
    if (-1 == setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    {
        syslog(LOG_ERR, "setsockopt failed: %s", strerror(errno));
        cleanup();
        freeaddrinfo(res);
        return -1;
    }
    syslog(LOG_INFO, "Socket options set successfully");

    // Bind the socket
    if (-1 == bind(server_fd, res->ai_addr, res->ai_addrlen))
    {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        cleanup();
        freeaddrinfo(res);
        return -1;
    }
    syslog(LOG_INFO, "Socket bound successfully");

    // Free the address information
    freeaddrinfo(res);

    // Listen for incoming connections
    if (-1 == listen(server_fd, BACKLOG))
    {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        cleanup();
        return -1;
    }
    syslog(LOG_INFO, "Listening for incoming connections on port %s", PORT);

    // Main loop to accept and handle client connections
    while (1)
    {
        syslog(LOG_INFO, "Server is running, waiting for connections...");

        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (-1 == client_fd)
        {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr),
                  client_ip, sizeof(client_ip));

        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        fd = fopen(FILEPATH, "a+");
        if (!fd)
        {
            syslog(LOG_ERR, "Failed to open file: %s", strerror(errno));
            close(client_fd);
            continue;
        }

        char *packet = NULL;
        size_t total_received = 0;

        while (0 < (bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0)))
        {

            char *temp = realloc(packet, total_received + bytes_received + 1);
            if (!temp)
            {
                syslog(LOG_ERR, "Memory allocation failed");
                free(packet);
                fclose(fd);
                close(client_fd);
                client_fd = -1;
                continue;
            }

            // Update packet pointer to the newly allocated memory
            packet = temp;

            // Copy the received data into the packet
            memcpy(packet + total_received, buffer, bytes_received);
            total_received += bytes_received;
            packet[total_received] = '\0';

            // Check if the packet contains a newline character
            if (memchr(buffer, '\n', bytes_received))
                break;
        }

        syslog(LOG_INFO, "Total bytes received: %zu", total_received);
        // If data was received, write it to the file
        if (packet)
        {
            syslog(LOG_INFO, "Received data: %s\n", packet);  // DELETE********************

            fwrite(packet, 1, total_received, fd);
            fflush(fd);
            free(packet);
        }

        // Reset the file pointer to the beginning of the file
        fseek(fd, 0, SEEK_SET);

        // Send the contents of the file back to the client
        syslog(LOG_INFO, "Sending file contents to client");
        while (0 < (bytes_received = fread(buffer, 1, BUFFER_SIZE, fd)))
        {
            syslog(LOG_INFO, "Buffer content: %.*s\n", (int)bytes_received, buffer);  // DELETE********************
            send(client_fd, buffer, bytes_received, 0);
        }

        fclose(fd);

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        client_fd = -1;
    }

    // Cleanup resources on exit
    cleanup();

    return 0;
}