# Academia Portal - Course Registration System

A multi-user **Course Registration Portal** developed in **C** using **TCP Socket Programming**, **POSIX Threads**, and **File Locking** as part of the Operating Systems Lab Mini Project.

The system follows a **client-server architecture** and supports concurrent access by multiple users while maintaining data consistency through synchronization mechanisms.

---

## Features

### Admin
- Login authentication
- Add new students
- Add new faculty
- View student details
- View faculty details
- Modify student information
- Modify faculty information
- Activate blocked students
- Block students

### Faculty
- Login authentication
- View offered courses
- Add new courses
- Remove courses
- Update course details
- Change password

### Student
- Login authentication
- View available courses
- Enroll in courses
- Drop enrolled courses
- View enrolled courses
- Change password

---

## System Architecture

```
                +----------------+
                |    Client      |
                +--------+-------+
                         |
                    TCP Socket
                         |
                +--------v-------+
                |    Server      |
                | (Multi-thread) |
                +--------+-------+
                         |
         +---------------+----------------+
         |               |                |
      Admin         Faculty          Student
       APIs            APIs             APIs
                         |
                 File Locking Layer
                         |
                  Persistent Storage
```

---

## Technologies Used

- C Programming
- Linux System Calls
- TCP Socket Programming
- POSIX Threads (pthread)
- File Locking (`fcntl`)
- File-based Database

---

## Project Structure

```
.
├── server.c          # Multi-threaded server
├── client.c          # TCP client
├── admin.c           # Admin functionalities
├── faculty.c         # Faculty functionalities
├── student.c         # Student functionalities
├── utils.c           # Shared utility functions
├── data/
│   ├── students.txt
│   ├── faculty.txt
│   ├── courses.txt
│   └── ...
└── README.md
```

---

## Compilation

Compile the server:

```bash
gcc server.c admin.c faculty.c student.c utils.c -o server -pthread
```

Compile the client:

```bash
gcc client.c -o client
```

---

## Running the Project

Start the server in one terminal:

```bash
./server
```

Start the client in another terminal:

```bash
./client
```

---

## Default Login Credentials

### Admin

| Username | Password |
|----------|----------|
| admin1 | pass1 |
| admin2 | pass2 |

### Faculty

| Username | Password |
|----------|----------|
| prof1 | pass1 |
| prof2 | pass2 |

### Student

| Username | Password |
|----------|----------|
| stud1 | pass1 |
| stud2 | pass2 |
| ... | ... |

Additional user records can be found inside the `data/` directory.

---

## Synchronization

The project uses **POSIX File Locking (`fcntl`)** to ensure safe concurrent access to shared files.

This prevents:
- Race conditions
- Data corruption
- Simultaneous conflicting updates

---

## Major Components

### `server.c`
- Creates TCP server
- Accepts client connections
- Creates a new thread for every client
- Authenticates users
- Routes requests based on user role

### `client.c`
- Connects to the server
- Handles user login
- Displays role-specific menus
- Sends user requests
- Displays server responses

### `admin.c`
Implements administrator operations including:
- Student management
- Faculty management
- Account activation/blocking
- Record modification

### `faculty.c`
Provides faculty operations:
- Course creation
- Course updates
- Course deletion
- Password management

### `student.c`
Handles student operations:
- Course enrollment
- Course withdrawal
- Viewing enrolled courses
- Password updates

### `utils.c`
Contains shared helper functions including:
- Input validation
- ID verification
- Enrollment checking
- Output formatting

---

## Key Operating Systems Concepts Used

- Client-Server Architecture
- TCP Socket Programming
- Concurrent Server Design
- POSIX Threads
- File Locking
- Synchronization
- Authentication
- Persistent File Storage

---

## Sample Workflow

1. Start the server.
2. Launch one or more clients.
3. Log in as Admin, Faculty, or Student.
4. Perform role-specific operations.
5. Server processes requests concurrently.
6. Shared data is protected using file locks.

---

## Future Improvements

- Replace text files with an SQL database.
- Encrypt stored passwords.
- Add secure communication using SSL/TLS.
- Develop a graphical user interface.
- Support multiple departments and semesters.
- Implement role-based access control with finer permissions.

---

## Author

**Siddharth Goswami**

Operating Systems Lab Mini Project
