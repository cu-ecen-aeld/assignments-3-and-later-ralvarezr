#include <stdio.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
    // Open the syslog for logging
    openlog(NULL, 0, LOG_USER);

    // Check if the correct number of arguments is provided
    if (argc != 3) {
        syslog(LOG_ERR, "Usage: %s <writefile> <writestr>\n", argv[0]);
        return 1;
    }

    // Open the file for writing
    FILE *file = fopen(argv[1], "w");
    if (NULL == file) {
        syslog(LOG_ERR, "Error opening the file");
        return 1;
    }

    // Write the string to the file
    fprintf(file, "%s\n", argv[2]);
    syslog(LOG_DEBUG, "Writing %s to file %s", argv[2], argv[1]);

    // Close the file
    fclose(file);

    // Close the syslog
    closelog();

    return 0;
}