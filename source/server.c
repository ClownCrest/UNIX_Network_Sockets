#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>

#define BUFFER_SIZE 1024  // Buffer size for data transfer
#define BACKLOG 10        // Max number of pending connections in the server's queue

// Function declarations
void validate_argument_number(int argc);
void parse_arguments(int argc, char *argv[], char **ip, char **port);
void validate_arguments(char **ip, char **port);
int is_valid_ip(const char *ip);
int is_valid_port(const char *port);
int create_server_fd();
void config_server(const char *ip, const char *port, int server_fd);
void accept_client_connections(int server_socket);
void process_client_message(int client_socket);
void vigenere_cipher(char *text, const char *key);

int main(int argc, char *argv[])
{
    char *ip = NULL, *port = NULL;
    validate_argument_number(argc);  // Validate the number of arguments passed
    parse_arguments(argc, argv, &ip, &port);  // Parse the arguments for IP and Port
    validate_arguments(&ip, &port);  // Validate the IP and Port

    printf("IP Address: %s\n", ip);
    printf("Port: %s\n", port);

    // Create the server socket
    printf("Creating socket...\n");
    int server_fd = create_server_fd();
    printf("Socket Created\n");

    // Configure the server with the provided IP and port
    config_server(ip, port, server_fd);

    // Accept client connections in a loop
    accept_client_connections(server_fd);

    return 0;
}

// Function to validate the number of arguments passed to the program
void validate_argument_number(int argc)
{
    if (argc != 5)  // Expecting 5 arguments
    {
        fprintf(stderr, "Usage: -ip <IP Address> -p <Port>\n");
        exit(EXIT_FAILURE);
    }
}

// Function to parse the arguments to extract the IP address and Port
void parse_arguments(int argc, char *argv[], char **ip, char **port)
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc)
        {
            *ip = argv[i + 1];  // Set the IP address
        }
        else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc)
        {
            *port = argv[i + 1];  // Set the Port
        }
    }

    // If either IP or Port is missing or incorrect, print an error message and exit
    if (*ip == NULL || *port == NULL)
    {
        fprintf(stderr, "Error: Missing or incorrect arguments.\n");
        fprintf(stderr, "Usage: -ip <IP Address> -p <Port>\n");
        exit(EXIT_FAILURE);
    }
}

// Function to validate the IP address and Port
void validate_arguments(char **ip, char **port)
{
    if (*ip == NULL || !is_valid_ip(*ip))  // Check if IP address is valid
    {
        fprintf(stderr, "Error: Invalid IP Address format. Expected format: xxx.xxx.xxx.xxx\n");
        exit(EXIT_FAILURE);
    }

    if (*port == NULL || !is_valid_port(*port))  // Check if Port is valid
    {
        fprintf(stderr, "Error: Invalid Port. Must be a number between 1 and 65535.\n");
        exit(EXIT_FAILURE);
    }
}

// Function to check if the provided IP address is valid
int is_valid_ip(const char *ip)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;  // Return 1 if IP is valid, otherwise 0
}

// Function to check if the provided Port is valid
int is_valid_port(const char *port)
{
    char *endptr;
    long port_num = strtol(port, &endptr, 10);

    // Check if the port is a valid number between 1 and 65535
    if (*endptr != '\0' || port_num < 1 || port_num > 65535)
        return 0;

    if (port[0] == '0' && strlen(port) > 1)
        return 0;  // Invalid if port starts with 0 and is not a single character

    return 1;
}

// Function to create the server socket
int create_server_fd()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);  // Create a socket using IPv4 and TCP
    if (server_fd == -1)
    {
        perror("ERR: Socket creation failed");  // Handle socket creation failure
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

// Function to configure the server with IP, Port, and bind the socket
void config_server(const char *ip, const char *port, int server_fd)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(strtol(port, NULL, 10));  // Convert port to network byte order
    serv_addr.sin_addr.s_addr = inet_addr(ip);  // Convert IP address to binary format

    int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)  // Set socket options
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the provided IP and port
    printf("Binding to: %s:%s\n", ip, port);
    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming client connections
    printf("Listening...\n");
    if (listen(server_fd, BACKLOG) == -1)
    {
        perror("Listen Error");
        exit(EXIT_FAILURE);
    }
}

// Function to accept client connections and process them
void accept_client_connections(int server_socket)
{
    while (1)
    {
        printf("Waiting for a client...\n");

        int client_socket = accept(server_socket, NULL, NULL);  // Accept a client connection
        if (client_socket == -1)
        {
            perror("ERR: Accept failed");
            continue;
        }

        printf("Client connected.\n");
        process_client_message(client_socket);  // Process the client's message

        close(client_socket);  // Close the client socket after processing
        printf("Client disconnected.\n\n");
    }
}

// Function to process the message received from a client
void process_client_message(int client_socket)
{
    char buffer[BUFFER_SIZE];  // Buffer for receiving data from the client
    char keyword[BUFFER_SIZE] = {0};  // Buffer for storing the keyword (Vigenère cipher key)
    ssize_t bytes_read;
    int keyword_received = 0;  // Flag to track if the keyword is received
    size_t buffer_offset = 0;  // Offset for buffer position

    // Read data from the client until the keyword is found
    while (!keyword_received && (bytes_read = recv(client_socket, buffer + buffer_offset, BUFFER_SIZE - buffer_offset - 1, 0)) > 0)
    {
        buffer_offset += bytes_read;
        buffer[buffer_offset] = '\0';  // Null-terminate the buffer

        char *newline_pos = strchr(buffer, '\n');
        if (newline_pos)
        {
            *newline_pos = '\0';  // Terminate the keyword at the newline
            strncpy(keyword, buffer, sizeof(keyword) - 1);  // Copy the keyword to the buffer

            size_t message_start = newline_pos - buffer + 1;
            size_t message_len = buffer_offset - message_start;

            // Allocate memory for the message
            char *message = malloc(message_len + 1);
            if (!message)
            {
                perror("malloc failed");
                return;
            }
            memcpy(message, buffer + message_start, message_len);
            message[message_len] = '\0';  // Null-terminate the message

            // Read the remaining message data
            while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0)
            {
                buffer[bytes_read] = '\0';  // Null-terminate the buffer
                // Resize the message buffer and append the new data
                char *temp = realloc(message, message_len + bytes_read + 1);
                if (!temp)
                {
                    perror("realloc failed");
                    free(message);
                    return;
                }
                message = temp;
                memcpy(message + message_len, buffer, bytes_read);
                message_len += bytes_read;
                message[message_len] = '\0';  // Null-terminate the message
            }

            // Encrypt the message using the Vigenère cipher
            vigenere_cipher(message, keyword);

            // Send the encrypted message back to the client
            send(client_socket, message, message_len, 0);
            printf("Encrypted message sent back to client.\n");

            free(message);  // Free the dynamically allocated message buffer
            keyword_received = 1;  // Set the flag indicating the keyword has been processed
            break;
        }
    }

    // Handle errors if recv fails or if the keyword is not received
    if (bytes_read == -1)
    {
        perror("recv error");
    }

    if (!keyword_received)
    {
        printf("Error receiving keyword or message.\n");
    }
}

// Function to encrypt the text using the Vigenère cipher
void vigenere_cipher(char *text, const char *key)
{
    int text_len = strlen(text);  // Get the length of the text
    int key_len = strlen(key);    // Get the length of the key

    // Normalize the key to uppercase (ignore non-alphabetic characters)
    char *key_upper = malloc(key_len + 1);
    int valid_key_len = 0;
    for (int i = 0; i < key_len; i++)
    {
        if (isalpha(key[i]))  // Ignore non-alphabetic characters in the key
        {
            key_upper[valid_key_len++] = toupper(key[i]);  // Convert key characters to uppercase
        }
    }
    key_upper[valid_key_len] = '\0';  // Null-terminate the key

    if (valid_key_len == 0)  // If the key is invalid (empty), free the key buffer and return
    {
        free(key_upper);
        return;
    }

    // Encrypt the text using the Vigenère cipher
    for (int i = 0, j = 0; i < text_len; i++)
    {
        if (isupper(text[i]))  // Encrypt uppercase letters
        {
            text[i] = ((text[i] - 'A' + (key_upper[j % valid_key_len] - 'A')) % 26) + 'A';
            j++;  // Increment the key index
        }
        else if (islower(text[i]))  // Encrypt lowercase letters
        {
            text[i] = ((text[i] - 'a' + (key_upper[j % valid_key_len] - 'A')) % 26) + 'a';
            j++;  // Increment the key index
        }
    }

    free(key_upper);  // Free the dynamically allocated memory for the key
}
