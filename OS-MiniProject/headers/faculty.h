#ifndef FACULTY_H
#define FACULTY_H

#include "common.h"

// Function declarations for faculty operations
// Displays courses offered by the specified faculty
void view_offering_courses(int client_sock, const char *faculty_id);
// Adds a new course to the system, assigned to the faculty
void add_new_course(int client_sock, const char *faculty_id);
// Removes a course from the catalog, if offered by the faculty
void remove_course(int client_sock, const char *faculty_id);
// Updates details of a course offered by the faculty
void update_course_details(int client_sock, const char *faculty_id);
// Changes the faculty member's password
void change_faculty_password(int client_sock, const char *faculty_id);

#endif