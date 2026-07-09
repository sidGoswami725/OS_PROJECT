#ifndef ADMIN_H
#define ADMIN_H

#include "common.h"

// Function declarations for administrative operations
// Adds a new student to the system
void add_student(int client_sock);
// Displays details of all students in a formatted table
void view_student_details(int client_sock);
// Adds a new faculty member to the system
void add_faculty(int client_sock);
// Displays details of all faculty members in a formatted table
void view_faculty_details(int client_sock);
// Activates a student's account by setting their status to active
void activate_student(int client_sock);
// Blocks a student's account by setting their status to blocked
void block_student(int client_sock);
// Modifies specific details of a student's record
void modify_student_details(int client_sock);
// Modifies specific details of a faculty member's record
void modify_faculty_details(int client_sock);

#endif