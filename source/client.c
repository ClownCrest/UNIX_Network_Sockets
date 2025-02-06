#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void validate_argument_number(int argc);
void parse_arguments(int argc, char *argv[], char **ip, char **port, char **filename, char **keyword);
void validate_arguments(char **ip, char **port, char **filename, char **keyword);
int is_valid_ip(const char *ip);
int is_valid_port(const char *port);
int is_valid_file(const char *filename);
long get_file_size(FILE* file);
char* read_file_content(FILE* file, long file_size);
int create_client_fd();
void config_server(char *port, char *ip, int client_fd);
void send_message_to_server(int client_socket, const char* message, long size);
void receive_server_response(int client_socket);
void close_socket(int client_socket);

int main(int argc, char *argv[])
{
    char *ip = NULL, *port = NULL, *filename = NULL, *keyword = NULL;

    validate_argument_number(argc);
    parse_arguments(argc, argv, &ip, &port, &filename, &keyword);
    validate_arguments(&ip, &port, &filename, &keyword);

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    long file_size = get_file_size(file);
    char* file_content = read_file_content(file, file_size);

    printf("IP Address: %s\n", ip);
    printf("Port: %s\n", port);
    printf("Filename: %s\n", filename);
    printf("Keyword: %s\n", keyword);
    printf("Filesize: %ld\n", file_size);
    printf("File contents: %s\n", file_content);

    printf("Creating socket...\n");
    int client_fd = create_client_fd();
    printf("Socket Created\n");

    config_server(port, ip, client_fd);
    send_message_to_server(client_fd, file_content, file_size);
    receive_server_response(client_fd);
    close_socket(client_fd);

    free(file_content);
    fclose(file);

    return 0;
}


void validate_argument_number(int argc)
{
    if (argc != 9)
    {
        fprintf(stderr, "Usage: -ip <IP Address> -p <Port> -f <Filename> -key <Keyword>\n");
        exit(EXIT_FAILURE);
    }
}

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

void validate_arguments (char **ip, char **port, char **filename, char **keyword)
{
    // Validate IP Address
    if (*ip == NULL || !is_valid_ip(*ip))
    {
        fprintf(stderr, "Error: Invalid IP Address format. Expected format: xxx.xxx.xxx.xxx\n");
        exit(EXIT_FAILURE);
    }

    // Validate Port
    if (*port == NULL || !is_valid_port(*port))
    {
        fprintf(stderr, "Error: Invalid Port. Must be a number between 1 and 65535.\n");
        exit(EXIT_FAILURE);
    }

    // Validate Filename
    if (*filename == NULL || strlen(*filename) == 0)
    {
        fprintf(stderr, "Error: Filename cannot be empty.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        if (!is_valid_file(*filename)) {
            exit(EXIT_FAILURE);
        }
    }

    // Validate Keyword
    if (*keyword == NULL || strlen(*keyword) == 0)
    {
        fprintf(stderr, "Error: Keyword cannot be empty.\n");
        exit(EXIT_FAILURE);
    }
}

int is_valid_ip(const char *ip)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

int is_valid_port(const char *port) {
    char *endptr;
    long port_num = strtol(port, &endptr, 10);

    if (*endptr != '\0' || port_num < 1 || port_num > 65535)
        return 0;

    // Prevent leading zeros like "0123"
    if (port[0] == '0' && strlen(port) > 1)
        return 0;

    return 1;
}

int is_valid_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: File '%s' does not exist.\n", filename);
        return 0;
    }
    else
    {
        fseek(file, 0, SEEK_END);
        if (ftell(file) == 0) {
            fprintf(stderr, "Error: File '%s' is empty.\n", filename);
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    return 1;
}

// Get file size
long get_file_size(FILE* file) {
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}

// Read file content into memory
char* read_file_content(FILE* file, long file_size) {
    char* buffer = malloc(file_size + 1);
    if (!buffer) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        perror("File read error");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    buffer[file_size] = '\0';
    return buffer;
}

// Create client socket
int create_client_fd() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("ERR: Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return client_fd;
}

//Configure server address
void config_server(char *port, char *ip, int client_fd)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(strtol(port, NULL, 10));

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // CONNECT TO SOCKET
    if (connect(client_fd,(struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1 ) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }
    printf("Connected to the server...\n");
}

// Send message to the server
void send_message_to_server(int client_socket, const char* message, long size) {
    if (send(client_socket, message, size, 0) == -1) {
        perror("ERR: Failed to send message");
        close_socket(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("Message sent to the server.\n\n");
}

// Receive server response
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

// Close socket safely
void close_socket(int client_socket) {
    close(client_socket);
}
