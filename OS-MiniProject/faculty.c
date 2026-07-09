#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include "headers/faculty.h"

// Displays courses offered by a specific faculty
void view_offering_courses(int client_sock, const char *faculty_id) {
    // Open courses file in read-only mode
    int fd = open("data/courses.txt", O_RDONLY);
    if (fd == -1) {
        // Send error if no courses exist
        char *error = "No courses found.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

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

    // Check if file is empty
    if (content_pos == 0) {
        char *msg = "No courses found.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Create a copy of file content for initial scanning
    char file_content_copy[BUFFER_SIZE * 10];
    strncpy(file_content_copy, file_content, sizeof(file_content_copy));

    // Check if faculty offers any courses
    int has_courses = 0;
    char *line = strtok(file_content_copy, "\n");
    while (line) {
        struct Course course;
        // Parse course details from line
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
        if (strcmp(course.faculty_id, faculty_id) == 0) {
            has_courses = 1;
            break;
        }
        line = strtok(NULL, "\n");
    }

    // If no courses found for this faculty, notify client
    if (!has_courses) {
        char *msg = "You are not offering any courses.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Send table header
    send_table_separator(client_sock);
    send_table_row(client_sock, "Course ID", "Name", "Seats", "Faculty ID", "");
    send_table_separator(client_sock);

    // Parse original file content to display courses
    line = strtok(file_content, "\n");
    while (line) {
        struct Course course;
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
        if (strcmp(course.faculty_id, faculty_id) == 0) {
            // Convert seats to string for display
            char seats_str[10];
            snprintf(seats_str, sizeof(seats_str), "%d", course.seats);
            // Send course details as table row
            send_table_row(client_sock, course.id, course.name, seats_str, course.faculty_id, "");
        }
        line = strtok(NULL, "\n");
    }

    // Send table footer
    send_table_separator(client_sock);
    char *msg = "\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Adds a new course to the system
void add_new_course(int client_sock, const char *faculty_id) {
    // Declare course structure
    struct Course course;
    char buffer[BUFFER_SIZE];

    // Prompt for and receive course ID
    prompt_and_receive(client_sock, "Enter Course ID: ", course.id, sizeof(course.id));
    // Check if course ID already exists
    if (id_exists("data/courses.txt", course.id)) {
        char *error = "Error: Course ID already exists.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Prompt for and receive course details
    prompt_and_receive(client_sock, "Enter Course Name: ", course.name, sizeof(course.name));
    prompt_and_receive(client_sock, "Enter Number of Seats: ", buffer, BUFFER_SIZE);
    course.seats = atoi(buffer);
    // Assign faculty ID to the course
    strcpy(course.faculty_id, faculty_id);

    // Open courses file in append mode
    int fd = open("data/courses.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        char *error = "Failed to open courses file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);

    // Format course data into a string
    char entry[BUFFER_SIZE];
    snprintf(entry, BUFFER_SIZE, "%s %s %d %s\n", course.id, course.name, course.seats, course.faculty_id);
    // Write course data to file
    write(fd, entry, strlen(entry));

    // Release the write lock
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    // Close the file
    close(fd);

    // Notify client of success
    char *msg = "Course added successfully.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Removes a course from the catalog
void remove_course(int client_sock, const char *faculty_id) {
    // Buffer for course ID
    char course_id[10];
    // Prompt for course ID to remove
    prompt_and_receive(client_sock, "Enter Course ID to Remove: ", course_id, sizeof(course_id));

    // Check if course exists
    if (!id_exists("data/courses.txt", course_id)) {
        char *error = "Error: Course does not exist.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Open courses file in read-write mode
    int fd = open("data/courses.txt", O_RDWR);
    if (fd == -1) {
        char *error = "Failed to open courses file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];
    char file_content[BUFFER_SIZE * 10] = {0};
    int bytes_read, content_pos = 0;

    // Read entire file content
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Open temporary file for updated content
    int fd_temp = open("data/courses_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char *line = strtok(file_content, "\n");
    int found = 0;
    // Parse and filter out the course to remove
    while (line) {
        struct Course course;
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
        if (strcmp(course.id, course_id) == 0 && strcmp(course.faculty_id, faculty_id) == 0) {
            found = 1; // Mark course as found
            line = strtok(NULL, "\n");
            continue; // Skip writing this course
        }
        // Write unchanged course to temp file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %d %s\n", course.id, course.name, course.seats, course.faculty_id);
        write(fd_temp, entry, strlen(entry));
        line = strtok(NULL, "\n");
    }

    // Release lock and close files
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    close(fd_temp);

    // Replace original file with updated content
    remove("data/courses.txt");
    rename("data/courses_temp.txt", "data/courses.txt");

    // If course wasn't found or not offered by this faculty, notify client
    if (!found) {
        char *msg = "Course not found or not offered by you.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Remove related enrollments
    fd = open("data/enrollments.txt", O_RDWR);
    if (fd != -1) {
        // Apply write lock
        lock.l_type = F_WRLCK;
        fcntl(fd, F_SETLKW, &lock);

        // Read enrollment content
        char enroll_content[BUFFER_SIZE * 10] = {0};
        content_pos = 0;
        while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
            buffer[bytes_read] = '\0';
            strncpy(enroll_content + content_pos, buffer, bytes_read);
            content_pos += bytes_read;
        }

        // Write filtered enrollments to temp file
        fd_temp = open("data/enrollments_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        line = strtok(enroll_content, "\n");
        while (line) {
            struct Enrollment enrollment;
            sscanf(line, "%s %s", enrollment.student_id, enrollment.course_id);
            // Skip enrollments for the removed course
            if (strcmp(enrollment.course_id, course_id) != 0) {
                char entry[BUFFER_SIZE];
                snprintf(entry, BUFFER_SIZE, "%s %s\n", enrollment.student_id, enrollment.course_id);
                write(fd_temp, entry, strlen(entry));
            }
            line = strtok(NULL, "\n");
        }

        // Release lock and close files
        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);
        close(fd);
        close(fd_temp);

        // Replace original enrollments file
        remove("data/enrollments.txt");
        rename("data/enrollments_temp.txt", "data/enrollments.txt");
    }

    // Notify client of success
    char *msg = "Course removed successfully.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Updates details of a course offered by the faculty
void update_course_details(int client_sock, const char *faculty_id) {
    // Buffer for course ID
    char course_id[10];
    // Prompt for course ID to update
    prompt_and_receive(client_sock, "Enter Course ID to Update: ", course_id, sizeof(course_id));

    // Check if course exists
    if (!id_exists("data/courses.txt", course_id)) {
        char *error = "Error: Course does not exist.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Open courses file in read-write mode
    int fd = open("data/courses.txt", O_RDWR);
    if (fd == -1) {
        char *error = "Failed to open courses file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];
    char file_content[BUFFER_SIZE * 10] = {0};
    int bytes_read, content_pos = 0;

    // Read entire file content
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Open temporary file for updated content
    int fd_temp = open("data/courses_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char *line = strtok(file_content, "\n");
    int found = 0;
    // Parse and update course details
    while (line) {
        struct Course course;
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
        if (strcmp(course.id, course_id) == 0 && strcmp(course.faculty_id, faculty_id) == 0) {
            found = 1;
            int choice;
            char choice_str[10];
            char update_str[10];
            // Menu loop for updating fields
            do {
                // Create and send menu for field selection
                char menu[BUFFER_SIZE];
                snprintf(menu, BUFFER_SIZE, "Select field to update for Course %s:\n1. ID\n2. Name\n3. Seats\n4. Done\nEnter choice: ", course.id);
                prompt_and_receive(client_sock, menu, choice_str, sizeof(choice_str));
                choice = atoi(choice_str);

                switch (choice) {
                    case 1: // Update Course ID
                        // Prompt for new course ID
                        prompt_and_receive(client_sock, "Enter New Course ID: ", update_str, sizeof(update_str));
                        // Check if new ID already exists
                        if (id_exists("data/courses.txt", update_str)) {
                            char *error = "Error: New Course ID already exists.\n";
                            send(client_sock, error, strlen(error), 0);
                        } else {
                            // Update course ID in enrollments file
                            int fd_enroll = open("data/enrollments.txt", O_RDWR);
                            if (fd_enroll != -1) {
                                struct flock lock_enroll;
                                lock_enroll.l_type = F_WRLCK;
                                lock_enroll.l_whence = SEEK_SET;
                                lock_enroll.l_start = 0;
                                lock_enroll.l_len = 0;
                                fcntl(fd_enroll, F_SETLKW, &lock_enroll);

                                // Read enrollment content
                                char enroll_content[BUFFER_SIZE * 10] = {0};
                                int enroll_pos = 0;
                                while ((bytes_read = read(fd_enroll, buffer, BUFFER_SIZE - 1)) > 0) {
                                    buffer[bytes_read] = '\0';
                                    strncpy(enroll_content + enroll_pos, buffer, bytes_read);
                                    enroll_pos += bytes_read;
                                }

                                // Write updated enrollments to temp file
                                int fd_enroll_temp = open("data/enrollments_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                char *enroll_line = strtok(enroll_content, "\n");
                                while (enroll_line) {
                                    struct Enrollment enrollment;
                                    sscanf(enroll_line, "%s %s", enrollment.student_id, enrollment.course_id);
                                    if (strcmp(enrollment.course_id, course_id) == 0) {
                                        strcpy(enrollment.course_id, update_str);
                                    }
                                    char enroll_entry[BUFFER_SIZE];
                                    snprintf(enroll_entry, BUFFER_SIZE, "%s %s\n", enrollment.student_id, enrollment.course_id);
                                    write(fd_enroll_temp, enroll_entry, strlen(enroll_entry));
                                    enroll_line = strtok(NULL, "\n");
                                }

                                // Release lock and close files
                                lock_enroll.l_type = F_UNLCK;
                                fcntl(fd_enroll, F_SETLK, &lock_enroll);
                                close(fd_enroll);
                                close(fd_enroll_temp);

                                // Replace original enrollments file
                                remove("data/enrollments.txt");
                                rename("data/enrollments_temp.txt", "data/enrollments.txt");
                            }
                            // Update course ID
                            strcpy(course.id, update_str);
                            char *msg = "Course ID updated successfully.\n";
                            send(client_sock, msg, strlen(msg), 0);
                        }
                        break;
                    case 2: // Update Course Name
                        // Prompt for new course name
                        prompt_and_receive(client_sock, "Enter New Course Name: ", course.name, sizeof(course.name));
                        char *msg_name = "Course Name updated successfully.\n";
                        send(client_sock, msg_name, strlen(msg_name), 0);
                        break;
                    case 3: // Update Seats
                        // Prompt for new number of seats
                        prompt_and_receive(client_sock, "Enter New Number of Seats: ", update_str, sizeof(update_str));
                        course.seats = atoi(update_str);
                        char *msg_seats = "Course Seats updated successfully.\n";
                        send(client_sock, msg_seats, strlen(msg_seats), 0);
                        break;
                    case 4: // Done
                        break;
                    default:
                        char *error = "Invalid choice. Please try again.\n";
                        send(client_sock, error, strlen(error), 0);
                }
            } while (choice != 4);
        }
        // Write course (updated or unchanged) to temp file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %d %s\n", course.id, course.name, course.seats, course.faculty_id);
        write(fd_temp, entry, strlen(entry));
        line = strtok(NULL, "\n");
    }

    // Release lock and close files
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    close(fd_temp);

    // Replace original file
    remove("data/courses.txt");
    rename("data/courses_temp.txt", "data/courses.txt");

    // Notify client of result
    char *msg = found ? "Course details update completed.\n" : "Course not found or not offered by you.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Changes the faculty member's password
void change_faculty_password(int client_sock, const char *faculty_id) {
    // Buffer for new password
    char new_password[20];
    // Prompt for new password
    prompt_and_receive(client_sock, "Enter New Password: ", new_password, sizeof(new_password));

    // Open faculties file in read-write mode
    int fd = open("data/faculties.txt", O_RDWR);
    if (fd == -1) {
        char *error = "Failed to open faculties file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up write lock
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);

    // Buffers for reading file content
    char buffer[BUFFER_SIZE];
    char file_content[BUFFER_SIZE * 10] = {0};
    int bytes_read, content_pos = 0;

    // Read entire file content
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncpy(file_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Open temporary file for updated content
    int fd_temp = open("data/faculties_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char *line = strtok(file_content, "\n");
    int found = 0;
    // Parse and update password
    while (line) {
        struct Faculty faculty;
        sscanf(line, "%s %s %s %s %s", faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
        if (strcmp(faculty.id, faculty_id) == 0) {
            // Update password
            strcpy(faculty.password, new_password);
            found = 1;
        }
        // Write faculty record to temp file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s\n", faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
        write(fd_temp, entry, strlen(entry));
        line = strtok(NULL, "\n");
    }

    // Release lock and close files
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    close(fd_temp);

    // Replace original file
    remove("data/faculties.txt");
    rename("data/faculties_temp.txt", "data/faculties.txt");

    // Notify client of result
    char *msg = found ? "Password changed successfully.\n" : "Faculty not found.\n";
    send(client_sock, msg, strlen(msg), 0);
}