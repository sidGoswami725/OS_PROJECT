#ifndef STUDENT_H
#define STUDENT_H

#include "common.h"

// Function declarations for student operations
// Displays all available courses in the system
void view_all_courses(int client_sock);
// Enrolls a student in a new course
void enroll_course(int client_sock, const char *student_id);
// Drops a course for a student
void drop_course(int client_sock, const char *student_id);
// Displays details of courses a student is enrolled in
void view_enrolled_courses(int client_sock, const char *student_id);
// Changes a student's password
void change_student_password(int client_sock, const char *student_id);

#endif