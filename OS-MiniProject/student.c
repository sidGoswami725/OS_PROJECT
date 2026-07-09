#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include "headers/student.h"

// Displays all available courses
void view_all_courses(int client_sock) {
    // Open courses file in read-only mode
    int fd = open("data/courses.txt", O_RDONLY);
    if (fd == -1) {
        // Send error if no courses exist
        char *error = "No courses available.\n";
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
        char *msg = "No courses available.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Send table header
    send_table_separator(client_sock);
    send_table_row(client_sock, "Course ID", "Name", "Seats", "Faculty ID", "");
    send_table_separator(client_sock);

    // Parse and display each course
    char *line = strtok(file_content, "\n");
    while (line) {
        struct Course course;
        // Parse course details
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
        // Convert seats to string
        char seats_str[10];
        snprintf(seats_str, sizeof(seats_str), "%d", course.seats);
        // Send course details as table row
        send_table_row(client_sock, course.id, course.name, seats_str, course.faculty_id, "");
        line = strtok(NULL, "\n");
    }

    // Send table footer
    send_table_separator(client_sock);
    char *msg = "\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Enrolls a student in a new course
void enroll_course(int client_sock, const char *student_id) {
    // Buffer for course ID
    char course_id[10];
    // Prompt for course ID to enroll
    prompt_and_receive(client_sock, "Enter Course ID to Enroll: ", course_id, sizeof(course_id));

    // Check if course exists
    if (!id_exists("data/courses.txt", course_id)) {
        char *error = "Error: Course does not exist.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Check if student is already enrolled
    if (is_enrolled(student_id, course_id)) {
        char *error = "Error: You are already enrolled in this course.\n";
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
    int found = 0, seats = 0;
    // Parse and update course seats
    while (line) {
        struct Course course;
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
        if (strcmp(course.id, course_id) == 0) {
            if (course.seats > 0) {
                // Decrease seats by 1
                course.seats--;
                seats = course.seats;
                found = 1;
            } else {
                found = -1; // No seats available
            }
        }
        // Write course to temp file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %d %s\n", course.id, course.name, course.seats, course.faculty_id);
        write(fd_temp, entry, strlen(entry));
        line = strtok(NULL, "\n");
    }

    // Ensure writes are flushed to disk
    fsync(fd_temp);
    // Release lock and close files
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    close(fd_temp);

    // Replace original file
    remove("data/courses.txt");
    rename("data/courses_temp.txt", "data/courses.txt");

    // Handle no seats or course not found
    if (found == -1) {
        char *msg = "No seats available in this course.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }
    if (!found) {
        char *msg = "Course not found.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Add enrollment record
    fd = open("data/enrollments.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);

    // Write enrollment entry
    char entry[BUFFER_SIZE];
    snprintf(entry, BUFFER_SIZE, "%s %s\n", student_id, course_id);
    write(fd, entry, strlen(entry));
    // Ensure writes are flushed
    fsync(fd);

    // Release lock and close file
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);

    // Notify client of success
    char msg[BUFFER_SIZE];
    snprintf(msg, BUFFER_SIZE, "Enrolled successfully. Remaining seats: %d\n", seats);
    send(client_sock, msg, strlen(msg), 0);
}

// Drops a course for a student
void drop_course(int client_sock, const char *student_id) {
    // Buffer for course ID
    char course_id[10];
    // Prompt for course ID to drop
    prompt_and_receive(client_sock, "Enter Course ID to Drop: ", course_id, sizeof(course_id));

    // Check if student is enrolled
    if (!is_enrolled(student_id, course_id)) {
        char *error = "Error: You are not enrolled in this course.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Open enrollments file in read-write mode
    int fd = open("data/enrollments.txt", O_RDWR);
    if (fd == -1) {
        char *error = "Failed to open enrollments file\n";
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
    int fd_temp = open("data/enrollments_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char *line = strtok(file_content, "\n");
    int found = 0;
    // Parse and remove enrollment
    while (line) {
        struct Enrollment enrollment;
        sscanf(line, "%s %s", enrollment.student_id, enrollment.course_id);
        if (strcmp(enrollment.student_id, student_id) == 0 && strcmp(enrollment.course_id, course_id) == 0) {
            found = 1; // Mark enrollment as found
            line = strtok(NULL, "\n");
            continue; // Skip this enrollment
        }
        // Write unchanged enrollment to temp file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s\n", enrollment.student_id, enrollment.course_id);
        write(fd_temp, entry, strlen(entry));
        line = strtok(NULL, "\n");
    }

    // Ensure writes are flushed
    fsync(fd_temp);
    // Release lock and close files
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    close(fd_temp);

    // Replace original file
    remove("data/enrollments.txt");
    rename("data/enrollments_temp.txt", "data/enrollments.txt");

    // If enrollment wasn't found, notify client
    if (!found) {
        char *msg = "You are not enrolled in this course.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Increase seats in course
    fd = open("data/courses.txt", O_RDWR);
    if (fd == -1) {
        char *error = "Failed to open courses file\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Apply write lock
    lock.l_type = F_WRLCK;
    fcntl(fd, F_SETLKW, &lock);

    // Read course content
    char course_content[BUFFER_SIZE * 10] = {0};
    content_pos = 0;
    while ((bytes_read = read(fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncpy(course_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Write updated courses to temp file
    fd_temp = open("data/courses_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    line = strtok(course_content, "\n");
    while (line) {
        struct Course course;
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);
        if (strcmp(course.id, course_id) == 0) {
            // Increase seats by 1
            course.seats++;
        }
        // Write course to temp file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %d %s\n", course.id, course.name, course.seats, course.faculty_id);
        write(fd_temp, entry, strlen(entry));
        line = strtok(NULL, "\n");
    }

    // Ensure writes are flushed
    fsync(fd_temp);
    // Release lock and close files
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    close(fd_temp);

    // Replace original file
    remove("data/courses.txt");
    rename("data/courses_temp.txt", "data/courses.txt");

    // Notify client of success
    char *msg = "Course dropped successfully.\n";
    send(client_sock, msg, strlen(msg), 0);
}

// Displays courses a student is enrolled in
void view_enrolled_courses(int client_sock, const char *student_id) {
    // Open enrollments file in read-only mode
    int fd_enroll = open("data/enrollments.txt", O_RDONLY);
    if (fd_enroll == -1) {
        // Send error if no enrollments exist
        char *error = "You are not enrolled in any courses.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Set up read lock
    struct flock lock;
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd_enroll, F_SETLKW, &lock);

    // Buffers for reading enrollment content
    char buffer[BUFFER_SIZE];
    char enroll_content[BUFFER_SIZE * 10] = {0};
    int bytes_read, content_pos = 0;

    // Read entire file content
    while ((bytes_read = read(fd_enroll, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncpy(enroll_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Release lock and close file
    lock.l_type = F_UNLCK;
    fcntl(fd_enroll, F_SETLK, &lock);
    close(fd_enroll);

    // Check if file is empty
    if (content_pos == 0) {
        char *msg = "You are not enrolled in any courses.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Extract enrolled course IDs for the student
    char *enroll_line = strtok(enroll_content, "\n");
    char enrolled_ids[100][10]; // Max 100 courses
    int enrolled_count = 0;

    while (enroll_line) {
        char sid[10], cid[10];
        sscanf(enroll_line, "%s %s", sid, cid);
        if (strcmp(sid, student_id) == 0) {
            // Store course ID
            strcpy(enrolled_ids[enrolled_count++], cid);
        }
        enroll_line = strtok(NULL, "\n");
    }

    // If no enrollments found, notify client
    if (enrolled_count == 0) {
        char *msg = "You are not enrolled in any courses.\n";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    // Open courses file to get course details
    int fd_course = open("data/courses.txt", O_RDONLY);
    if (fd_course == -1) {
        char *error = "Courses file not found.\n";
        send(client_sock, error, strlen(error), 0);
        return;
    }

    // Apply read lock
    lock.l_type = F_RDLCK;
    fcntl(fd_course, F_SETLKW, &lock);

    // Read course content
    char course_content[BUFFER_SIZE * 10] = {0};
    content_pos = 0;

    while ((bytes_read = read(fd_course, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        strncpy(course_content + content_pos, buffer, bytes_read);
        content_pos += bytes_read;
    }

    // Release lock and close file
    lock.l_type = F_UNLCK;
    fcntl(fd_course, F_SETLK, &lock);
    close(fd_course);

    // Send table header
    send_table_separator(client_sock);
    send_table_row(client_sock, "Course ID", "Name", "Seats", "Faculty ID", "");
    send_table_separator(client_sock);

    // Parse and display enrolled courses
    char *line = strtok(course_content, "\n");
    while (line) {
        struct Course course;
        sscanf(line, "%s %s %d %s", course.id, course.name, &course.seats, course.faculty_id);

        // Check if course is in enrolled list
        for (int i = 0; i < enrolled_count; ++i) {
            if (strcmp(course.id, enrolled_ids[i]) == 0) {
                // Convert seats to string
                char seats_str[10];
                snprintf(seats_str, sizeof(seats_str), "%d", course.seats);
                // Send course details as table row
                send_table_row(client_sock, course.id, course.name, seats_str, course.faculty_id, "");
                break;
            }
        }

        line = strtok(NULL, "\n");
    }

    // Send table footer
    send_table_separator(client_sock);
    char *end = "\n";
    send(client_sock, end, strlen(end), 0);
}

// Changes a student's password
void change_student_password(int client_sock, const char *student_id) {
    // Buffer for new password
    char new_password[20];
    // Prompt for new password
    prompt_and_receive(client_sock, "Enter New Password: ", new_password, sizeof(new_password));

    // Open students file in read-write mode
    int fd = open("data/students.txt", O_RDWR);
    if (fd == -1) {
        char *error = "Failed to open students file\n";
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
    int fd_temp = open("data/students_temp.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char *line = strtok(file_content, "\n");
    int found = 0;
    // Parse and update password
    while (line) {
        struct Student student;
        int status;
        sscanf(line, "%s %s %s %s %s %d", student.id, student.name, student.email, student.phone, student.password, &status);
        if (strcmp(student.id, student_id) == 0) {
            // Update password
            strcpy(student.password, new_password);
            found = 1;
        }
        // Write student record to temp file
        char entry[BUFFER_SIZE];
        snprintf(entry, BUFFER_SIZE, "%s %s %s %s %s %d\n", student.id, student.name, student.email, student.phone, student.password, status);
        write(fd_temp, entry, strlen(entry));
        line = strtok(NULL, "\n");
    }

    // Ensure writes are flushed
    fsync(fd_temp);
    // Release lock and close files
    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    close(fd);
    close(fd_temp);

    // Replace original file
    remove("data/students.txt");
    rename("data/students_temp.txt", "data/students.txt");

    // Notify client of result
    char *msg = found ? "Password changed successfully.\n" : "Student not found.\n";
    send(client_sock, msg, strlen(msg), 0);
}