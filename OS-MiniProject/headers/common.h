#ifndef COMMON_H
#define COMMON_H

// Constants for network configuration and buffer size
#define PORT 8080           // Port number for server communication
#define MAX_CLIENTS 10      // Maximum number of simultaneous client connections
#define BUFFER_SIZE 1024    // Size of the communication buffer

// Struct definitions for data entities
// Represents a student record
struct Student {
    char id[10];        // Student ID
    char name[50];      // Student name
    char email[50];     // Student email address
    char phone[15];     // Student phone number
    char password[20];  // Student password
    int status;         // Status: 1 for active, 0 for blocked
};

// Represents a faculty record
struct Faculty {
    char id[10];        // Faculty ID
    char name[50];      // Faculty name
    char email[50];     // Faculty email address
    char phone[15];     // Faculty phone number
    char password[20];  // Faculty password
};

// Represents an admin record
struct Admin {
    char id[10];        // Admin ID
    char password[20];  // Admin password
};

// Represents a course record
struct Course {
    char id[10];        // Course ID
    char name[50];      // Course name
    int seats;          // Number of available seats
    char faculty_id[10]; // ID of the faculty offering the course
};

// Represents an enrollment record
struct Enrollment {
    char student_id[10]; // ID of the enrolled student
    char course_id[10];  // ID of the course
};

// Function declarations for shared utility functions
// Sends a prompt to the client and receives input
void prompt_and_receive(int client_sock, char *prompt, char *buffer, int buffer_size);
// Checks if an ID exists in a specified file
int id_exists(const char *filename, const char *id);
// Checks if a student is enrolled in a specific course
int is_enrolled(const char *student_id, const char *course_id);
// Sends a formatted table row to the client
void send_table_row(int client_sock, const char *col1, const char *col2, const char *col3, const char *col4, const char *col5);
// Sends a table separator line to the client
void send_table_separator(int client_sock);

#endif
