#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

// Function declarations
void validate_argument_number(int argc);
void parse_arguments(int argc, char *argv[], char **ip, char **port, char **filename, char **keyword);
void validate_arguments(char **ip, char **port, char **filename, char **keyword);
int is_valid_ip(const char *ip);
int is_valid_port(const char *port);
int is_valid_file(const char *filename);
int is_valid_keyword (const char *keyword);
long get_file_size(FILE* file);
char* read_file_content(FILE* file, long file_size);
int create_client_fd();
void connect_server(char *port, char *ip, int client_fd);
void send_message_to_server(int client_socket, const char* message, long size);
void receive_server_response(int client_socket);
void close_socket(int client_socket);

int main(int argc, char *argv[])
{
    char *ip = NULL, *port = NULL, *filename = NULL, *keyword = NULL;

    // Validate the number of arguments
    validate_argument_number(argc);

    // Parse command-line arguments
    parse_arguments(argc, argv, &ip, &port, &filename, &keyword);

    // Validate the parsed arguments
    validate_arguments(&ip, &port, &filename, &keyword);

    // Open file for reading
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Get file size
    long file_size = get_file_size(file);

    // Read file content into memory
    char* file_content = read_file_content(file, file_size);

    printf("Creating socket...\n");

    // Create client socket
    int client_fd = create_client_fd();
    printf("Socket Created\n");

    // Connect to the server
    connect_server(port, ip, client_fd);

    // Send keyword and file content to server
    send_message_to_server(client_fd, keyword, strlen(keyword));
    send_message_to_server(client_fd, "\n", 1);
    send_message_to_server(client_fd, file_content, file_size);
    printf("Message sent to the server.\n\n");

    // Shutdown write side to indicate no more data
    shutdown(client_fd, SHUT_WR);

    // Wait for server response
    receive_server_response(client_fd);

    // Close client socket
    close_socket(client_fd);

    // Free allocated memory and close file
    free(file_content);
    fclose(file);

    return 0;
}

// Validate the number of command-line arguments
void validate_argument_number(int argc)
{
    if (argc != 9)
    {
        fprintf(stderr, "Usage: -ip <IP Address> -p <Port> -f <Filename> -key <Keyword>\n");
        exit(EXIT_FAILURE);
    }
}

// Parse command-line arguments
void parse_arguments(int argc, char *argv[], char **ip, char **port, char **filename, char **keyword)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc)
        {
            *ip = argv[i + 1];
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
        {
            *port = argv[i + 1];
        }
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc)
        {
            *filename = argv[i + 1];
        }
        else if (strcmp(argv[i], "-key") == 0 && i + 1 < argc)
        {
            *keyword = argv[i + 1];
        }
    }

    if (*ip == NULL || *port == NULL || *filename == NULL || *keyword == NULL)
    {
        fprintf(stderr, "Error: Missing or incorrect arguments.\n");
        fprintf(stderr, "Usage: -ip <IP Address> -p <Port> -f <Filename> -key <Keyword>\n");
        exit(EXIT_FAILURE);
    }
}

// Additional validation functions...

// Function to create a client socket
int create_client_fd()
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("ERR: Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return client_fd;
}

// Function to connect to the server
void connect_server(char *port, char *ip, int client_fd)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(strtol(port, NULL, 10));

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1 ) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to the server...\n");
}

// Function to send message to server
void send_message_to_server(int client_socket, const char* message, long size) {
    long total_sent = 0;
    while (total_sent < size) {
        ssize_t sent = send(client_socket, message + total_sent, size - total_sent, 0);
        if (sent == -1) {
            perror("ERR: Failed to send message");
            close_socket(client_socket);
            exit(EXIT_FAILURE);
        }
        total_sent += sent;
    }
}

// Function to receive server response
void receive_server_response(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    bool done_receiving = false;

    printf("Encrypted message received from the server:\n");

    while (!done_receiving) {
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
            if (bytes_received < BUFFER_SIZE - 1) {
                done_receiving = true;
            }
        } else if (bytes_received == 0) {
            done_receiving = true;
            printf("\nServer closed the connection.\n");
        } else {
            perror("ERR: Receiving error");
            done_receiving = true;
        }
    }
    printf("\nDisconnected from the server.\n");
}
