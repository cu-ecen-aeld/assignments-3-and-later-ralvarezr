#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <sys/queue.h>

// Define constants for the server configuration
#define PORT "9000"
#define BUFFER_SIZE 1024
#define FILE_PATH "/var/tmp/aesdsocketdata"

// Global variables for socket file descriptors and a flag for graceful shutdown
int server_socket = -1;
int client_socket = -1; // This is a temporary variable within the main loop. It's better to manage it per thread.
volatile sig_atomic_t exit_requested = 0;

// Mutex for synchronizing file access between threads
pthread_mutex_t file_mutex;

// Structure to hold information about each thread
struct thread_info {
    pthread_t thread_id;        // The ID of the thread
    int client_socket;          // The socket file descriptor for the client
    SLIST_ENTRY(thread_info) entries; // Macro for the singly linked list
};

// Head of the singly linked list to track active threads
SLIST_HEAD(thread_list, thread_info);
struct thread_list thread_head = SLIST_HEAD_INITIALIZER(thread_head);

/**
 * @brief Signal handler for SIGINT and SIGTERM.
 *
 * This function handles signals to perform a clean exit. It closes the sockets,
 * removes the data file, and exits the program.
 * @param sig The signal number.
 */
static void handle_signal(int sig) {
    (void)sig;
    printf("Caught signal, exiting\n");
    if (server_socket != -1) close(server_socket);
    // The client_socket global variable is not used correctly in a multithreaded context.
    // It's better to close the socket within the thread handler.
    if (client_socket != -1) close(client_socket);
    remove(FILE_PATH);
    exit(0);
}

/**
 * @brief Converts the process into a daemon.
 *
 * This function forks the process, exits the parent, creates a new session,
 * changes the working directory to root, and closes standard file descriptors
 * (stdin, stdout, stderr).
 */
static void daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS); // Exit the parent process
    }
    umask(0); // Set file mode creation mask to 0
    if (setsid() < 0) {
        perror("Failed to create new session");
        exit(EXIT_FAILURE);
    }
    if (chdir("/") < 0) {
        perror("Failed to change directory to root");
        exit(EXIT_FAILURE);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

/**
 * @brief Thread function to write a timestamp to the log file every 10 seconds.
 *
 * It acquires a mutex before writing to ensure thread-safe file access.
 * The loop continues until the `exit_requested` flag is set.
 * @param arg Not used.
 * @return void* Always returns NULL.
 */
void* timestamp_writer(void* arg) {
    (void)arg;
    while(!exit_requested) {
        sleep(10); // Wait for 10 seconds
        pthread_mutex_lock(&file_mutex);
        int file_fd = open(FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (file_fd >= 0) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            char timebuf[100];
            strftime(timebuf, sizeof(timebuf), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", tm_info);
            if (-1 == write(file_fd, timebuf, strlen(timebuf)))
	    {
		perror("write");
	    }
            close(file_fd);
        }
        pthread_mutex_unlock(&file_mutex);
    }

    printf("Timestamp writer thread exiting\n");
    return NULL;
}

/**
 * @brief Thread function to handle a single client connection.
 *
 * This function receives data from a client, writes it to the data file,
 * and then reads the entire file content to send it back to the client.
 * All file operations are protected by a mutex.
 * @param arg A pointer to the thread_info structure containing client socket data.
 * @return void* Always returns NULL.
 */
void* connection_handler(void* arg) {

    struct thread_info *tinfo = (struct thread_info*) arg;
    int client_socket = tinfo->client_socket;
    ssize_t bytes_received;
    char buffer[BUFFER_SIZE];

    printf("Accepted connection\n");

    // Receive data from the client until a newline character is found
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        // Lock the mutex for thread-safe file writing
        pthread_mutex_lock(&file_mutex);
        int file_fd = open(FILE_PATH, O_CREAT | O_WRONLY | O_APPEND, 0644);
        if (file_fd >= 0) {
            if (-1 == write(file_fd, buffer, bytes_received))
	    {
		perror("write");
	    }
            close(file_fd);
        }
        pthread_mutex_unlock(&file_mutex);

        if (strchr(buffer, '\n')) break;
    }

    // Lock the mutex for thread-safe file reading
    pthread_mutex_lock(&file_mutex);
    int file_fd = open(FILE_PATH, O_RDONLY);
    if (file_fd >= 0) {
        // Read the entire file content and send it back to the client
        while ((bytes_received = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
            send(client_socket, buffer, bytes_received, 0);
        }
        close(file_fd);
    }
    pthread_mutex_unlock(&file_mutex);

    close(client_socket);
    printf("Closed connection");
    return NULL;
}

/**
 * @brief Main function of the aesdsocket server.
 *
 * It initializes the server, handles command-line arguments for daemon mode,
 * sets up signal handlers, creates the main listening socket, and enters
 * a loop to accept client connections and spawn threads to handle them.
 * It also manages the lifecycle of these threads by joining them at the end.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @return int The exit status of the program.
 */
int main(int argc, char *argv[]) {
    struct addrinfo hints, *res;
    int daemon_mode = 0;
    int optval = 1;

    // Initialize the mutex
    pthread_mutex_init(&file_mutex, NULL);

    // Check for the daemon mode command-line argument
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            daemon_mode = 1;
        }
    }

    // Set up signal handlers for graceful exit
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Configure the address information for socket creation
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        return -1;
    }

    // Create the server socket
    if ((server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        perror("Failed to create socket");
        freeaddrinfo(res);
        return -1;
    }

    // Set socket options to reuse the address
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        perror("Failed to set socket options");
        close(server_socket);
        freeaddrinfo(res);
        return -1;
    }

    // Bind the socket to the specified port
    if (bind(server_socket, res->ai_addr, res->ai_addrlen) == -1) {
        perror("Failed to bind socket");
        close(server_socket);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);

    // If daemon mode is requested, daemonize the process
    if (daemon_mode) {
        daemonize();
    }

    // Start listening for incoming connections
    if (listen(server_socket, 10) == -1) {
        perror("Failed to listen on socket");
        return -1;
    }

    // Create the timestamp writer thread
    pthread_t timestamp_thread;
    pthread_create(&timestamp_thread, NULL, timestamp_writer, NULL);

    // Main loop to accept new connections
    while (!exit_requested) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            if (exit_requested) break; // Exit loop if a signal was received
            perror("Failed to accept connection");
            continue;
        }

        // Allocate memory for thread info and add it to the list
        struct thread_info *tinfo = malloc(sizeof(struct thread_info));
        tinfo->client_socket = client_socket;
        SLIST_INSERT_HEAD(&thread_head, tinfo, entries);

        // Create a new thread to handle the connection
        pthread_create(&tinfo->thread_id, NULL, connection_handler, tinfo);
    }

    // Join with all remaining threads before exiting
    struct thread_info *tinfo;
    while (!SLIST_EMPTY(&thread_head)) {
        tinfo = SLIST_FIRST(&thread_head);
        pthread_join(tinfo->thread_id, NULL);
        SLIST_REMOVE_HEAD(&thread_head, entries);
        free(tinfo);
    }

    // Clean up remaining resources
    close(server_socket);
    remove(FILE_PATH);

    // Note: The timestamp thread is not explicitly joined here, but the program exits
    // due to the signal handler's `exit(0)`. This is a slight weakness in the code.
    // A better approach would be to set a flag and join the thread gracefully.
    return 0;
}
