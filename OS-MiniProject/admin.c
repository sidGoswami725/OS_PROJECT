#include <stdio.h>              // Standard I/O functions for formatting and output
#include <stdlib.h>             // Standard library functions (e.g., memory allocation)
#include <string.h>             // String manipulation functions (e.g., strcpy, strcmp)
#include <unistd.h>             // POSIX system calls (e.g., write, close)
#include <fcntl.h>              // File control options (e.g., open, file locking)
#include <sys/file.h>           // File locking mechanisms (fcntl)
#include <sys/socket.h>         // Socket functions for client communication
#include "headers/admin.h"              // Header file with admin function declarations and shared structures

// Adds a new student to the system
void add_student(int client_sock) {
    // Declare student structure to store input data
    struct Student student;
    // Buffer for temporary input storage
    char buffer[BUFFER_SIZE];

    // Prompt client for student ID and store it
    prompt_and_receive(client_sock, "Enter Student ID: ", student.id, sizeof(student.id));
    // Check if the student ID already exists in students.txt
    if (id_exists("data/students.txt", student.id)) {
        // Notify client of duplicate ID error
        char *error = "Error: Student ID already exists.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Prompt for and receive remaining student details
    prompt_and_receive(client_sock, "Enter Student Name: ", student.name, sizeof(student.name));
    prompt_and_receive(client_sock, "Enter Student Email: ", student.email, sizeof(student.email));
    prompt_and_receive(client_sock, "Enter Student Phone: ", student.phone, sizeof(student.phone));
    prompt_and_receive(client_sock, "Enter Student Password: ", student.password, sizeof(student.password));
    // Set student status to active (1) by default
    student.status = 1;

    // Open students.txt in write-append mode, create if it doesn't exist
    int fd = open("data/students.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        // Notify client if file cannot be opened
        char *error = "Failed to open students file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_WRLCK;      // Write lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Format student data into a single string entry
    char entry[BUFFER_SIZE];
    snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s %d\n", student.id, student.name, student.email, student.phone, student.password, student.status);
    // Append the entry to the file
    write(fd, entry, strlen(entry));

    // Release the write lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close the file
    close(fd);

    // Notify client of successful addition
    char *msg = "Student added successfully.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Displays details of all students in a formatted table
void view_student_details(int client_sock) {
    // Open students.txt in read-only mode
    int fd = open("data/students.txt", O_RDONLY);
    if (fd == -1) {
        // Notify client if no students exist
        char *error = "No students found.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up read lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_RDLCK;      // Read lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];           // Buffer for reading chunks
    char file_content[BUFFER_SIZE * 10] = {0}; // Large buffer for entire file
    int bytes_read, content_pos = 0;   // Track bytes read and position

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

    // Check if file is empty
    if (content_pos == 0) {
        // Notify client if no students are found
        char *msg = "No students found.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Send table header
    send_table_separator(client_sock);
    send_table_row(client_sock, "ID", "Name", "Email", "Phone", "Status");
    send_table_separator(client_sock);

    // Parse file content line by line
    char *line = strtok(file_content, "\n");
    while (line) {
        // Parse student details from the line
        struct Student student;
        int status;
        sscanf(line, "%s %s %s %s %s %d", student.id, student.name, student.email, student.phone, student.password, &status);
        // Convert status to string for display
        char status_str[10];
        snprintf(status_str, sizeof(status_str), "%s", status ? "Active" : "Blocked");
        // Send student details as a table row
        send_table_row(client_sock, student.id, student.name, student.email, student.phone, status_str);
        // Move to next line
        line = strtok(NULL, "\n");
    }

    // Send table footer
    send_table_separator(client_sock);
    // Send newline for formatting
    char *msg = "\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Adds a new faculty member to the system
void add_faculty(int client_sock) {
    // Declare faculty structure to store input data
    struct Faculty faculty;
    // Buffer for temporary input storage
    char buffer[BUFFER_SIZE];

    // Prompt client for faculty ID and store it
    prompt_and_receive(client_sock, "Enter Faculty ID: ", faculty.id, sizeof(faculty.id));
    // Check if the faculty ID already exists in faculties.txt
    if (id_exists("data/faculties.txt", faculty.id)) {
        // Notify client of duplicate ID error
        char *error = "Error: Faculty ID already exists.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Prompt for and receive remaining faculty details
    prompt_and_receive(client_sock, "Enter Faculty Name: ", faculty.name, sizeof(faculty.name));
    prompt_and_receive(client_sock, "Enter Faculty Email: ", faculty.email, sizeof(faculty.email));
    prompt_and_receive(client_sock, "Enter Faculty Phone: ", faculty.phone, sizeof(faculty.phone));
    prompt_and_receive(client_sock, "Enter Faculty Password: ", faculty.password, sizeof(faculty.password));

    // Open faculties.txt in write-append mode, create if it doesn't exist
    int fd = open("data/faculties.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        // Notify client if file cannot be opened
        char *error = "Failed to open faculties file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_WRLCK;      // Write lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Format faculty data into a single string entry
    char entry[BUFFER_SIZE];
    snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s\n", faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
    // Append the entry to the file
    write(fd, entry, strlen(entry));

    // Release the write lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close the file
    close(fd);

    // Notify client of successful addition
    char *msg = "Faculty added successfully.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Displays details of all faculty members in a formatted table
void view_faculty_details(int client_sock) {
    // Open faculties.txt in read-only mode
    int fd = open("data/faculties.txt", O_RDONLY);
    if (fd == -1) {
        // Notify client if no faculty members exist
        char *error = "No faculty found.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up read lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_RDLCK;      // Read lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];           // Buffer for reading chunks
    char file_content[BUFFER_SIZE * 10] = {0}; // Large buffer for entire file
    int bytes_read, content_pos = 0;   // Track bytes read and position

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

    // Check if file is empty
    if (content_pos == 0) {
        // Notify client if no faculty are found
        char *msg = "No faculty found.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Send table header
    send_table_separator(client_sock);
    send_table_row(client_sock, "ID", "Name", "Email", "Phone", "Password");
    send_table_separator(client_sock);

    // Parse file content line by line
    char *line = strtok(file_content, "\n");
    while (line) {
        // Parse faculty details from the line
        struct Faculty faculty;
        sscanf(line, "%s %s %s %s %s", faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
        // Send faculty details as a table row
        send_table_row(client_sock, faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
        // Move to next line
        line = strtok(NULL, "\n");
    }

    // Send table footer
    send_table_separator(client_sock);
    // Send newline for formatting
    char *msg = "\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Activates a student's account by setting their status to active
void activate_student(int client_sock) {
    // Buffer for student ID
    char student_id[10];
    // Prompt client for student ID to activate
    prompt_and_receive(client_sock, "Enter Student ID to Activate: ", student_id, sizeof(student_id));

    // Check if the student ID exists
    if (!id_exists("data/students.txt", student_id)) {
        // Notify client if student does not exist
        char *error = "Error: Student does not exist.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Open students.txt in read-write mode
    int fd = open("data/students.txt", O_RDWR);
    if (fd == -1) {
        // Notify client if file cannot be opened
        char *error = "Failed to open students file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_WRLCK;      // Write lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];           // Buffer for reading chunks
    char file_content[BUFFER_SIZE * 10] = {0}; // Large buffer for entire file
    int bytes_read, content_pos = 0;   // Track bytes read and position

    // Read entire file content
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate chunk
        // Append chunk to file_content
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Open temporary file for updated content
    int fd_temp = open("data/students_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // Parse file content line by line
    char *line = strtok(file_content, "\n");
    int found = 0; // Flag to track if student is found
    while (line) {
        // Parse student details
        struct Student student;
        int status;
        sscanf(line, "%s %s %s %s %s %d", student.id, student.name, student.email, student.phone, student.password, &status);
        if (strcmp(student.id, student_id) == 0) {
            // Check if student is already active
            if (status == 1) {
                // Notify client and clean up
                char *error = "Error: Student is already active.\n";
                send(client_sock, error, strlen(error), 0);
                lock.l_type = F_UNLCK;
                fcntl(fd, F_SETLK, &lock);
                close(fd);
                close(fd_temp);
                remove("data/students_temp.txt");
                return;
            }
            // Activate student by setting status to 1
            status = 1;
            found = 1;
        }
        // Write student record to temporary file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s %d\n", student.id, student.name, student.email, student.phone, student.password, status);
        write(fd_temp, entry, strlen(entry));
        // Move to next line
        line = strtok(NULL, "\n");
    }

    // Release the write lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close both files
    close(fd);
    close(fd_temp);

    // Replace original file with updated content
    remove("data/students.txt");
    rename("data/students_temp.txt", "data/students.txt");

    // Notify client of result
    char *msg = found ? "Student activated successfully.\n" : "Student not found.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Blocks a student's account by setting their status to blocked
void block_student(int client_sock) {
    // Buffer for student ID
    char student_id[10];
    // Prompt client for student ID to block
    prompt_and_receive(client_sock, "Enter Student ID to Block: ", student_id, sizeof(student_id));

    // Check if the student ID exists
    if (!id_exists("data/students.txt", student_id)) {
        // Notify client if student does not exist
        char *error = "Error: Student does not exist.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Open students.txt in read-write mode
    int fd = open("data/students.txt", O_RDWR);
    if (fd == -1) {
        // Notify client if file cannot be opened
        char *error = "Failed to open students file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_WRLCK;      // Write lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];           // Buffer for reading chunks
    char file_content[BUFFER_SIZE * 10] = {0}; // Large buffer for entire file
    int bytes_read, content_pos = 0;   // Track bytes read and position

    // Read entire file content
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate chunk
        // Append chunk to file_content
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Open temporary file for updated content
    int fd_temp = open("data/students_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // Parse file content line by line
    char *line = strtok(file_content, "\n");
    int found = 0; // Flag to track if student is found
    while (line) {
        // Parse student details
        struct Student student;
        int status;
        sscanf(line, "%s %s %s %s %s %d", student.id, student.name, student.email, student.phone, student.password, &status);
        if (strcmp(student.id, student_id) == 0) {
            // Check if student is already blocked
            if (status == 0) {
                // Notify client and clean up
                char *error = "Error: Student is already blocked.\n";
                send(client_sock, error, strlen(error), 0);
                lock.l_type = F_UNLCK;
                fcntl(fd, F_SETLK, &lock);
                close(fd);
                close(fd_temp);
                remove("data/students_temp.txt");
                return;
            }
            // Block student by setting status to 0
            status = 0;
            found = 1;
        }
        // Write student record to temporary file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s %d\n", student.id, student.name, student.email, student.phone, student.password, status);
        write(fd_temp, entry, strlen(entry));
        // Move to next line
        line = strtok(NULL, "\n");
    }

    // Release the write lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close both files
    close(fd);
    close(fd_temp);

    // Replace original file with updated content
    remove("data/students.txt");
    rename("data/students_temp.txt", "data/students.txt");

    // Notify client of result
    char *msg = found ? "Student blocked successfully.\n" : "Student not found.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Modifies specific details of a student's record
void modify_student_details(int client_sock) {
    // Buffer for student ID
    char student_id[10];
    // Prompt client for student ID to modify
    prompt_and_receive(client_sock, "Enter Student ID to Modify: ", student_id, sizeof(student_id));

    // Check if the student ID exists
    if (!id_exists("data/students.txt", student_id)) {
        // Notify client if student does not exist
        char *error = "Error: Student does not exist.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Open students.txt in read-write mode
    int fd = open("data/students.txt", O_RDWR);
    if (fd == -1) {
        // Notify client if file cannot be opened
        char *error = "Failed to open students file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_WRLCK;      // Write lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];           // Buffer for reading chunks
    char file_content[BUFFER_SIZE * 10] = {0}; // Large buffer for entire file
    int bytes_read, content_pos = 0;   // Track bytes read and position

    // Read entire file content
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate chunk
        // Append chunk to file_content
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Open temporary file for updated content
    int fd_temp = open("data/students_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // Parse file content line by line
    char *line = strtok(file_content, "\n");
    int found = 0; // Flag to track if student is found
    while (line) {
        // Parse student details
        struct Student student;
        int status;
        sscanf(line, "%s %s %s %s %s %d", student.id, student.name, student.email, student.phone, student.password, &status);
        if (strcmp(student.id, student_id) == 0) {
            found = 1;
            // Interactive menu for selecting fields to update
            int choice;
            char choice_str[10];
            char new_id[10];
            do {
                // Construct and send menu for field selection
                char menu[BUFFER_SIZE];
                snprintf(menu, BUFFER_SIZE, "Select field to update for Student %s:\n1. ID\n2. Name\n3. Email\n4. Phone\n5. Done\nEnter choice: ", student.id);
                prompt_and_receive(client_sock, menu, choice_str, sizeof(choice_str));
                choice = atoi(choice_str);

                switch (choice) {
                    case 1: // Update ID
                        // Prompt for new ID
                        prompt_and_receive(client_sock, "Enter New ID: ", new_id, sizeof(new_id));
                        // Check if new ID already exists
                        if (id_exists("data/students.txt", new_id)) {
                            char *error = "Error: New Student ID already exists.\n";
                            send(client_sock, error, strlen(error), 0);
                        } else {
                            // Update student ID
                            strcpy(student.id, new_id);
                            char *msg = "Student ID updated successfully.\n";
                            send(client_sock, msg, strlen(msg), 0);
                        }
                        break;
                    case 2: // Update Name
                        // Prompt for new name
                        prompt_and_receive(client_sock, "Enter New Name: ", student.name, sizeof(student.name));
                        // Notify client of success
                        char *msg_name = "Student Name updated successfully.\n";
                        send(client_sock, msg_name, strlen(msg_name), 0);
                        break;
                    case 3: // Update Email
                        // Prompt for new email
                        prompt_and_receive(client_sock, "Enter New Email: ", student.email, sizeof(student.email));
                        // Notify client of success
                        char *msg_email = "Student Email updated successfully.\n";
                        send(client_sock, msg_email, strlen(msg_email), 0);
                        break;
                    case 4: // Update Phone
                        // Prompt for new phone
                        prompt_and_receive(client_sock, "Enter New Phone: ", student.phone, sizeof(student.phone));
                        // Notify client of success
                        char *msg_phone = "Student Phone updated successfully.\n";
                        send(client_sock, msg_phone, strlen(msg_phone), 0);
                        break;
                    case 5: // Done
                        // Exit the update loop
                        break;
                    default:
                        // Handle invalid menu choice
                        char *error = "Invalid choice. Please try again.\n";
                        send(client_sock, error, strlen(error), 0);
                }
            } while (choice != 5);
        }
        // Write student record to temporary file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s %d\n", student.id, student.name, student.email, student.phone, student.password, status);
        write(fd_temp, entry, strlen(entry));
        // Move to next line
        line = strtok(NULL, "\n");
    }

    // Release the write lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close both files
    close(fd);
    close(fd_temp);

    // Replace original file with updated content
    remove("data/students.txt");
    rename("data/students_temp.txt", "data/students.txt");

    // Notify client of result
    char *msg = found ? "Student details modification completed.\n" : "Student not found.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Modifies specific details of a faculty member's record
void modify_faculty_details(int client_sock) {
    // Buffer for faculty ID
    char faculty_id[10];
    // Prompt client for faculty ID to modify
    prompt_and_receive(client_sock, "Enter Faculty ID to Modify: ", faculty_id, sizeof(faculty_id));

    // Check if the faculty ID exists
    if (!id_exists("data/faculties.txt", faculty_id)) {
        // Notify client if faculty does not exist
        char *error = "Error: Faculty does not exist.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Open faculties.txt in read-write mode
    int fd = open("data/faculties.txt", O_RDWR);
    if (fd == -1) {
        // Notify client if file cannot be opened
        char *error = "Failed to open faculties file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock to prevent concurrent modifications
    struct flock lock;
    lock.l_type = F_WRLCK;      // Write lock
    lock.l_whence = SEEK_SET;   // Start from beginning
    lock.l_start = 0;           // Offset 0
    lock.l_len = 0;             // Lock entire file
    fcntl(fd, F_SETLKW, &lock); // Apply lock, wait if necessary

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];           // Buffer for reading chunks
    char file_content[BUFFER_SIZE * 10] = {0}; // Large buffer for entire file
    int bytes_read, content_pos = 0;   // Track bytes read and position

    // Read entire file content
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate chunk
        // Append chunk to file_content
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Open temporary file for updated content
    int fd_temp = open("data/faculties_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    // Parse file content line by line
    char *line = strtok(file_content, "\n");
    int found = 0; // Flag to track if faculty is found
    while (line) {
        // Parse faculty details
        struct Faculty faculty;
        sscanf(line, "%s %s %s %s %s", faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
        if (strcmp(faculty.id, faculty_id) == 0) {
            found = 1;
            // Interactive menu for selecting fields to update
            int choice;
            char choice_str[10];
            char new_id[10];
            do {
                // Construct and send menu for field selection
                char menu[BUFFER_SIZE];
                snprintf(menu, BUFFER_SIZE, "Select field to update for Faculty %s:\n1. ID\n2. Name\n3. Email\n4. Phone\n5. Done\nEnter choice: ", faculty.id);
                prompt_and_receive(client_sock, menu, choice_str, sizeof(choice_str));
                choice = atoi(choice_str);

                switch (choice) {
                    case 1: // Update ID
                        // Prompt for new ID
                        prompt_and_receive(client_sock, "Enter New ID: ", new_id, sizeof(new_id));
                        // Check if new ID already exists
                        if (id_exists("data/faculties.txt", new_id)) {
                            char *error = "Error: New Faculty ID already exists.\n";
                            send(client_sock, error, strlen(error), 0);
                        } else {
                            // Update faculty_id in courses.txt if ID changes
                            int fd_courses = open("data/courses.txt", O_RDWR);
                            if (fd_courses != -1) {
                                // Set up write lock for courses file
                                struct flock lock_courses;
                                lock_courses.l_type = F_WRLCK;
                                lock_courses.l_whence = SEEK_SET;
                                lock_courses.l_start = 0;
                                lock_courses.l_len = 0;
                                fcntl(fd_courses, F_SETLKW, &lock_courses);

                                // Read courses file content
                                char courses_content[BUFFER_SIZE * 10] = {0};
                                int courses_pos = 0;
                                while ((bytes_read = read(fd_courses, buffer, BUFFER_SIZE - 1)) > 0) {
                                    buffer[bytes_read] = '\0';
                                    strncpy(courses_content + courses_pos, buffer, bytes_read);
                                    courses_pos += bytes_read;
                                }

                                // Open temporary file for updated courses
                                int fd_courses_temp = open("data/courses_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                // Parse courses content
                                char *course_line = strtok(courses_content, "\n");
                                while (course_line) {
                                    // Parse course details
                                    struct Course course;
                                    sscanf(course_line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
                                    // Update faculty ID if it matches
                                    if (strcmp(course.faculty_id, faculty_id) == 0) {
                                        strcpy(course.faculty_id, new_id);
                                    }
                                    // Write course record to temporary file
                                    char course_entry[BUFFER_SIZE];
                                    snprintf(course_entry, BUFFER_SIZE, "%s %s %d %s\n", course.id, course.name, course.seats, course.faculty_id);
                                    write(fd_courses_temp, course_entry, strlen(course_entry));
                                    // Move to next line
                                    course_line = strtok(NULL, "\n");
                                }

                                // Release the write lock
                                lock_courses.l_type = F_UNLCK;
                                fcntl(fd_courses, F_SETLK, &lock_courses);
                                // Close both files
                                close(fd_courses);
                                close(fd_courses_temp);

                                // Replace original courses file
                                remove("data/courses.txt");
                                rename("data/courses_temp.txt", "data/courses.txt");
                            }
                            // Update faculty ID
                            strcpy(faculty.id, new_id);
                            // Notify client of success
                            char *msg = "Faculty ID updated successfully.\n";
                            send(client_sock, msg, strlen(msg), 0);
                        }
                        break;
                    case 2: // Update Name
                        // Prompt for new name
                        prompt_and_receive(client_sock, "Enter New Name: ", faculty.name, sizeof(faculty.name));
                        // Notify client of success
                        char *msg_name = "Faculty Name updated successfully.\n";
                        send(client_sock, msg_name, strlen(msg_name), 0);
                        break;
                    case 3: // Update Email
                        // Prompt for new email
                        prompt_and_receive(client_sock, "Enter New Email: ", faculty.email, sizeof(faculty.email));
                        // Notify client of success
                        char *msg_email = "Faculty Email updated successfully.\n";
                        send(client_sock, msg_email, strlen(msg_email), 0);
                        break;
                    case 4: // Update Phone
                        // Prompt for new phone
                        prompt_and_receive(client_sock, "Enter New Phone: ", faculty.phone, sizeof(faculty.phone));
                        // Notify client of success
                        char *msg_phone = "Faculty Phone updated successfully.\n";
                        send(client_sock, msg_phone, strlen(msg_phone), 0);
                        break;
                    case 5: // Done
                        // Exit the update loop
                        break;
                    default:
                        // Handle invalid menu choice
                        char *error = "Invalid choice. Please try again.\n";
                        send(client_sock, error, strlen(error), 0);
                }
            } while (choice != 5);
        }
        // Write faculty record to temporary file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s\n", faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
        write(fd_temp, entry, strlen(entry));
        // Move to next line
        line = strtok(NULL, "\n");
    }

    // Release the write lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close both files
    close(fd);
    close(fd_temp);

    // Replace original file with updated content
    remove("data/faculties.txt");
    rename("data/faculties_temp.txt", "data/faculties.txt");

    // Notify client of result
    char *msg = found ? "Faculty details modification completed.\n" : "Faculty not found.\n";
    send(client_sock, msg, strlen(msg), 0);
}
