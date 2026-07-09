#include <stdio.h>              // Standard I/O functions for printing and formatting
#include <stdlib.h>             // Standard library functions (e.g., malloc, exit)
#include <string.h>             // String manipulation functions (e.g., strcpy, strcmp)
#include <unistd.h>             // POSIX system calls (e.g., close, read)
#include <sys/socket.h>         // Socket programming functions and structures
#include <netinet/in.h>         // Internet address structures (e.g., sockaddr_in)
#include <pthread.h>            // Thread creation and management
#include <fcntl.h>              // File control options (e.g., open, fcntl)
#include <sys/file.h>           // File locking mechanisms
#include "headers/common.h"     // Shared constants (e.g., BUFFER_SIZE, PORT) and structures
#include "headers/admin.h"      // Admin function declarations
#include "headers/faculty.h"    // Faculty function declarations
#include "headers/student.h"    // Student function declarations

// Authenticates a user based on ID, password, and role using file-based storage
int authenticate_user(char *id, char *password, int selected_role) {
    // Determine the appropriate file based on role (1: Admin, 2: Faculty, 3: Student)
    char *filename;
    if (selected_role == 1) filename = "data/admins.txt";
    else if (selected_role == 2) filename = "data/faculties.txt";
    else filename = "data/students.txt";

    // Open the file in read-only mode
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        // Log error if file cannot be opened
        perror("Failed to open file");
        return 0; // Authentication fails if file is inaccessible
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

    // Read entire file content in chunks
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

    // Parse file content line by line to check credentials
    char *line = strtok(file_content, "\n");
    while (line) {
        if (selected_role == 1) {
            // Admin authentication: check ID and password
            char temp_id[10], temp_password[20];
            sscanf(line, "%s %s", temp_id, temp_password);
            if (strcmp(temp_id, id) == 0 && strcmp(temp_password, password) == 0) {
                return 1; // Successful authentication
            }
        } else if (selected_role == 2) {
            // Faculty authentication: parse faculty record
            struct Faculty faculty;
            sscanf(line, "%s %s %s %s %s", faculty.id, faculty.name, faculty.email, faculty.phone, faculty.password);
            if (strcmp(faculty.id, id) == 0 && strcmp(faculty.password, password) == 0) {
                return 1; // Successful authentication
            }
        } else {
            // Student authentication: parse student record with status
            struct Student student;
            int status;
            sscanf(line, "%s %s %s %s %s %d", student.id, student.name, student.email, student.phone, student.password, &status);
            if (strcmp(student.id, id) == 0 && strcmp(student.password, password) == 0) {
                if (status == 0) return -1; // Student is blocked
                return 1; // Successful authentication
            }
        }
        // Move to next line
        line = strtok(NULL, "\n");
    }
    // Authentication failed if no match found
    return 0;
}

// Sends a role-specific menu to the client
void send_menu(int client_sock, int role) {
    // Buffer to store menu text
    char menu[BUFFER_SIZE];
    if (role == 1) { // Admin menu
        // Construct admin menu with available options
        snprintf(menu, BUFFER_SIZE,
            "************ Welcome to ADMIN MENU ************\n"
            "1. ADD Student\n"
            "2. VIEW Student Details\n"
            "3. ADD Faculty\n"
            "4. VIEW Faculty Details\n"
            "5. ACTIVATE Student\n"
            "6. BLOCK Student\n"
            "7. MODIFY Student Details\n"
            "8. MODIFY Faculty Details\n"
            "9. LOGOUT and EXIT\n"
            "ENTER YOUR CHOICE: ");
    } else if (role == 2) { // Faculty menu
        // Construct faculty menu with available options
        snprintf(menu, BUFFER_SIZE,
            "************ Welcome to FACULTY MENU ************\n"
            "1. VIEW OFFERING Courses\n"
            "2. ADD NEW Course\n"
            "3. REMOVE Course from Catalog\n"
            "4. UPDATE Course Details\n"
            "5. CHANGE PASSWORD\n"
            "6. LOGOUT and EXIT\n"
            "ENTER YOUR CHOICE: ");
    } else { // Student menu
        // Construct student menu with available options
        snprintf(menu, BUFFER_SIZE,
            "************ Welcome to STUDENT MENU ************\n"
            "1. VIEW ALL Courses\n"
            "2. ENROLL (pick) NEW Course\n"
            "3. DROP Course\n"
            "4. VIEW ENROLLED Course Details\n"
            "5. CHANGE PASSWORD\n"
            "6. LOGOUT and EXIT\n"
            "ENTER YOUR CHOICE: ");
    }
    // Send the menu text to the client
    send(client_sock, menu, strlen(menu), 0);
}

// Handles a single client connection in a separate thread
void *handle_client(void *arg) {
    // Extract client socket from thread argument
    int client_sock = *(int *)arg;
    // Buffers for receiving user input
    char buffer[BUFFER_SIZE];
    char id[10], password[20];
    int role; // Stores user role (1: Admin, 2: Faculty, 3: Student)

    // Log new client connection
    printf("New client connected\n");

    // Send welcome message and role selection prompt
    char *prompt = "************ Welcome Back to Academia :: Course Registration ************\n"
                   "Login Type\n"
                   "Enter Your Choice { 1. Admin, 2. Professor, 3. Student } : ";
    send(client_sock, prompt, strlen(prompt), 0);

    // Receive role choice from client
    int valread = read(client_sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        // Handle client disconnection during role selection
        printf("Client disconnected during role selection\n");
        close(client_sock);
        return NULL;
    }
    buffer[valread] = '\0';
    // Remove trailing newline from input
    buffer[strcspn(buffer, "\n")] = 0;
    role = atoi(buffer);
    // Validate role input
    if (role < 1 || role > 3) {
        // Notify client of invalid role and close connection
        char *error = "Invalid role selected. Connection closed.\n";
        send(client_sock, error, strlen(error), 0);
        close(client_sock);
        printf("Client selected invalid role\n");
        return NULL;
    }

    // Prompt for user ID
    prompt = "Enter Your ID: ";
    send(client_sock, prompt, strlen(prompt), 0);

    // Receive user ID
    valread = read(client_sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        // Handle client disconnection during ID input
        printf("Client disconnected during ID input\n");
        close(client_sock);
        return NULL;
    }
    buffer[valread] = '\0';
    buffer[strcspn(buffer, "\n")] = 0;
    // Store ID safely with buffer overflow protection
    strncpy(id, buffer, sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';

    // Prompt for password
    prompt = "Enter Your Password: ";
    send(client_sock, prompt, strlen(prompt), 0);

    // Receive password
    valread = read(client_sock, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        // Handle client disconnection during password input
        printf("Client disconnected during password input\n");
        close(client_sock);
        return NULL;
    }
    buffer[valread] = '\0';
    buffer[strcspn(buffer, "\n")] = 0;
    // Store password safely with buffer overflow protection
    strncpy(password, buffer, sizeof(password) - 1);
    password[sizeof(password) - 1] = '\0';

    // Authenticate user with provided credentials
    int auth_result = authenticate_user(id, password, role);
    if (auth_result == 0) {
        // Handle invalid credentials
        char *error = "Invalid credentials. Connection closed.\n";
        send(client_sock, error, strlen(error), 0);
        close(client_sock);
        printf("Client authentication failed: %s\n", id);
        return NULL;
    } else if (auth_result == -1) {
        // Handle blocked student account
        char *error = "Account is blocked. Contact admin to activate. Connection closed.\n";
        send(client_sock, error, strlen(error), 0);
        close(client_sock);
        printf("Blocked student attempted login: %s\n", id);
        return NULL;
    }

    // Log successful authentication
    printf("Client authenticated: %s (Role: %d)\n", id, role);

    // Main menu loop for authenticated client
    while (1) {
        // Send role-specific menu
        send_menu(client_sock, role);

        // Receive menu choice
        valread = read(client_sock, buffer, BUFFER_SIZE);
        if (valread <= 0) {
            // Handle client disconnection
            printf("Client disconnected: %s\n", id);
            break;
        }
        buffer[valread] = '\0';
        buffer[strcspn(buffer, "\n")] = 0;
        int choice = atoi(buffer);

        // Handle logout option
        if ((role == 1 && choice == 9) || (role != 1 && choice == 6)) { // Logout and Exit
            // Notify client and log logout
            char *msg = "Logging out...\n";
            send(client_sock, msg, strlen(msg), 0);
            printf("Client logged out: %s\n", id);
            break;
        }

        // Dispatch based on role and menu choice
        if (role == 1) { // Admin
            switch (choice) {
                case 1: add_student(client_sock); break; // Add new student
                case 2: view_student_details(client_sock); break; // View all students
                case 3: add_faculty(client_sock); break; // Add new faculty
                case 4: view_faculty_details(client_sock); break; // View all faculty
                case 5: activate_student(client_sock); break; // Activate a student
                case 6: block_student(client_sock); break; // Block a student
                case 7: modify_student_details(client_sock); break; // Modify student details
                case 8: modify_faculty_details(client_sock); break; // Modify faculty details
                default: {
                    // Handle invalid menu choice
                    char *msg = "Invalid choice.\n";
                    send(client_sock, msg, strlen(msg), 0);
                }
            }
        } else if (role == 2) { // Faculty
            switch (choice) {
                case 1: view_offering_courses(client_sock, id); break; // View courses offered by faculty
                case 2: add_new_course(client_sock, id); break; // Add a new course
                case 3: remove_course(client_sock, id); break; // Remove a course
                case 4: update_course_details(client_sock, id); break; // Update course details
                case 5: change_faculty_password(client_sock, id); break; // Change faculty password
                default: {
                    // Handle invalid menu choice
                    char *msg = "Invalid choice.\n";
                    send(client_sock, msg, strlen(msg), 0);
                }
            }
        } else { // Student
            switch (choice) {
                case 1: view_all_courses(client_sock); break; // View all available courses
                case 2: enroll_course(client_sock, id); break; // Enroll in a course
                case 3: drop_course(client_sock, id); break; // Drop a course
                case 4: view_enrolled_courses(client_sock, id); break; // View enrolled courses
                case 5: change_student_password(client_sock, id); break; // Change student password
                default: {
                    // Handle invalid menu choice
                    char *msg = "Invalid choice.\n";
                    send(client_sock, msg, strlen(msg), 0);
                }
            }
        }
    }

    // Close client socket
    close(client_sock);
    return NULL;
}

// Main function to set up and run the server
int main() {
    // Variables for server and client sockets
    int server_fd, client_sock;
    // Server address structure
    struct sockaddr_in address;
    // Socket option to reuse address
    int opt = 1;
    // Address length for accept call
    int addrlen = sizeof(address);

    // Create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        // Exit if socket creation fails
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket option to reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        // Exit if setting socket options fails
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    address.sin_family = AF_INET;       // IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface
    address.sin_port = htons(PORT);     // Convert port to network byte order

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        // Exit if binding fails
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        // Exit if listen fails
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Log server startup
    printf("Server listening on port %d...\n", PORT);

    // Main loop to accept client connections
    while (1) {
        // Accept a new client connection
        if ((client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            // Log error and continue on accept failure
            perror("Accept failed");
            continue;
        }

        // Create a new thread to handle the client
        pthread_t thread;
        // Allocate memory for client socket
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        // Create thread and pass client socket
        if (pthread_create(&thread, NULL, handle_client, (void *)new_sock) != 0) {
            // Log error and free memory on thread creation failure
            perror("Thread creation failed");
            free(new_sock);
        }
        // Detach thread to clean up automatically
        pthread_detach(thread);
    }

    // Close server socket (unreachable due to infinite loop)
    close(server_fd);
    return 0;
}