#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "headers/common.h"

// Main function for the client application
int main() {
    // Variables for socket and server address
    int sock = 0;
    struct sockaddr_in serv_addr;
    // Buffer for receiving server messages
    char buffer[BUFFER_SIZE] = {0};

    // Create a TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    // Configure server address structure
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_port = htons(PORT); // Convert port to network byte order

    // Convert server IP address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // Main loop to receive and process server messages
    while (1) {
        // Read message from server
        int valread = read(sock, buffer, BUFFER_SIZE - 1);
        // Check for disconnection or error
        if (valread <= 0) {
            printf("Disconnected from server\n");
            break;
        }
        // Null-terminate the received message
        buffer[valread] = '\0';
        // Display the message to the user
        printf("%s", buffer);
        fflush(stdout);

        // Check if the message expects user input (e.g., prompts)
        if (strstr(buffer, "Enter Your Choice") || strstr(buffer, "Enter Your ID") ||
            strstr(buffer, "Enter Your Password") || strstr(buffer, "ENTER YOUR CHOICE") ||
            strstr(buffer, "Enter ") || strstr(buffer, "Enter New ")) {
            // Buffer for user input
            char input[BUFFER_SIZE];
            // Read user input from stdin
            fgets(input, BUFFER_SIZE, stdin);
            // Remove trailing newline
            input[strcspn(input, "\n")] = 0;
            // Send user input to the server
            send(sock, input, strlen(input), 0);
        }
    }

    // Close the socket
    close(sock);
    return 0;
}