#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include "headers/common.h"

// Sends a prompt to the client and receives input into a buffer
void prompt_and_receive(int client_sock, char *prompt, char *buffer, int buffer_size) {
    // Send the prompt message to the client
    send(client_sock, prompt, strlen(prompt), 0);
    // Read client input into the buffer
    int valread = read(client_sock, buffer, buffer_size);
    // Null-terminate the buffer to ensure it's a valid string
    buffer[valread] = '\0';
    // Remove trailing newline character, if present
    buffer[strcspn(buffer, "\n")] = 0;
}

// Checks if a given ID exists in the specified file
int id_exists(const char *filename, const char *id) {
    // Open the file in read-only mode
    int fd = open(filename, O_RDONLY);
    // If file doesn't exist, ID cannot exist, so return 0
    if (fd == -1) return 0;

    // Set up read lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_RDLCK;      // Read lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];
    char file_content[BUFFER_SIZE * 10] = {0}; // Large buffer for file content
    int bytes_read, content_pos = 0;

    // Read file content in chunks
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate chunk
        // Append chunk to file_content
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Release the read lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close the file
    close(fd);

    // Parse file content line by line to check for ID
    char temp_id[10];
    char *line = strtok(file_content, "\n");
    while (line) {
        // Extract the first field (ID) from the line
        sscanf(line, "%s", temp_id);
        // If ID matches, return 1 (found)
        if (strcmp(temp_id, id) == 0) return 1;
        // Move to next line
        line = strtok(NULL, "\n");
    }
    // ID not found, return 0
    return 0;
}

// Checks if a student is enrolled in a specific course
int is_enrolled(const char *student_id, const char *course_id) {
    // Open enrollments file in read-only mode
    int fd = open("data/enrollments.txt", O_RDONLY);
    // If file doesn't exist, no enrollments, return 0
    if (fd == -1) return 0;

    // Set up read lock
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];
    char file_content[BUFFER_SIZE * 10] = {0};
    int bytes_read, content_pos = 0;

    // Read file content in chunks
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Release the read lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close the file
    close(fd);

    // Parse enrollments line by line
    char s_id[10], c_id[10];
    char *line = strtok(file_content, "\n");
    while (line) {
        // Extract student ID and course ID from the line
        sscanf(line, "%s %s", s_id, c_id);
        // If both IDs match, return 1 (enrolled)
        if (strcmp(s_id, student_id) == 0 && strcmp(c_id, course_id) == 0) return 1;
        // Move to next line
        line = strtok(NULL, "\n");
    }
    // Not enrolled, return 0
    return 0;
}

// Sends a formatted table row to the client
void send_table_row(int client_sock, const char *col1, const char *col2, const char *col3, const char *col4, const char *col5) {
    // Buffer for the formatted row
    char row[BUFFER_SIZE];
    // Format the row with fixed-width columns
    snprintf(row, BUFFER_SIZE, "| %-8s | %-15s | %-18s | %-11s | %-7s |\n", col1, col2, col3, col4, col5);
    // Send the row to the client
    send(client_sock, row, strlen(row), 0);
}

// Sends a table separator line to the client
void send_table_separator(int client_sock) {
    // Define the separator line for the table
    char separator[] = "+----------+-----------------+--------------------+-------------+---------+\n";
    // Send the separator to the client
    send(client_sock, separator, strlen(separator), 0);
}