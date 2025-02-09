#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define BACKLOG 10

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
    validate_argument_number(argc);
    parse_arguments(argc, argv, &ip, &port);
    validate_arguments(&ip, &port);

    printf("IP Address: %s\n", ip);
    printf("Port: %s\n", port);

    printf("Creating socket...\n");
    int server_fd = create_server_fd();
    printf("Socket Created\n");

    config_server(ip, port, server_fd);
    accept_client_connections(server_fd);

    return 0;
}

void validate_argument_number(int argc)
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: -ip <IP Address> -p <Port>\n");
        exit(EXIT_FAILURE);
    }
}

void parse_arguments(int argc, char *argv[], char **ip, char **port)
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
    }

    if (*ip == NULL || *port == NULL)
    {
        fprintf(stderr, "Error: Missing or incorrect arguments.\n");
        fprintf(stderr, "Usage: -ip <IP Address> -p <Port>\n");
        exit(EXIT_FAILURE);
    }
}

void validate_arguments(char **ip, char **port)
{
    if (*ip == NULL || !is_valid_ip(*ip))
    {
        fprintf(stderr, "Error: Invalid IP Address format. Expected format: xxx.xxx.xxx.xxx\n");
        exit(EXIT_FAILURE);
    }

    if (*port == NULL || !is_valid_port(*port))
    {
        fprintf(stderr, "Error: Invalid Port. Must be a number between 1 and 65535.\n");
        exit(EXIT_FAILURE);
    }
}

int is_valid_ip(const char *ip)
{
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0;
}

int is_valid_port(const char *port)
{
    char *endptr;
    long port_num = strtol(port, &endptr, 10);

    if (*endptr != '\0' || port_num < 1 || port_num > 65535)
        return 0;

    if (port[0] == '0' && strlen(port) > 1)
        return 0;

    return 1;
}

int create_server_fd()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("ERR: Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

void config_server(const char *ip, const char *port, int server_fd)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(strtol(port, NULL, 10));
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    int enable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
    {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    printf("Binding to: %s:%s\n", ip, port);

    if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    printf("Listening...\n");
    if (listen(server_fd, BACKLOG) == -1)
    {
        perror("Listen Error");
        exit(EXIT_FAILURE);
    }
}

void accept_client_connections(int server_socket)
{
    while (1)
    {
        printf("Waiting for a client...\n");

        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket == -1)
        {
            perror("ERR: Accept failed");
            continue;
        }

        printf("Client connected.\n");
        process_client_message(client_socket);

        close(client_socket);
        printf("Client disconnected.\n\n");
    }
}

void process_client_message(int client_socket) {
    char buffer[BUFFER_SIZE];
    char keyword[BUFFER_SIZE] = {0};
    ssize_t bytes_read;
    int keyword_received = 0;
    size_t buffer_offset = 0;

    // Read until keyword is found
    while (!keyword_received && (bytes_read = recv(client_socket, buffer + buffer_offset, BUFFER_SIZE - buffer_offset - 1, 0)) > 0) {
        buffer_offset += bytes_read;
        buffer[buffer_offset] = '\0';

        char *newline_pos = strchr(buffer, '\n');
        if (newline_pos) {
            *newline_pos = '\0';
            strncpy(keyword, buffer, sizeof(keyword) - 1);
            size_t message_start = newline_pos - buffer + 1;
            size_t message_len = buffer_offset - message_start;

            // Initialize message buffer with remaining data
            char *message = malloc(message_len + 1);
            if (!message) {
                perror("malloc failed");
                return;
            }
            memcpy(message, buffer + message_start, message_len);
            message[message_len] = '\0';

            // Read remaining data for the message
            while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
                buffer[bytes_read] = '\0';
                // Resize message buffer and append new data
                char *temp = realloc(message, message_len + bytes_read + 1);
                if (!temp) {
                    perror("realloc failed");
                    free(message);
                    return;
                }
                message = temp;
                memcpy(message + message_len, buffer, bytes_read);
                message_len += bytes_read;
                message[message_len] = '\0';
            }

            //printf("Received keyword: %s\n", keyword);
           // printf("Received message: %s\n", message);

            vigenere_cipher(message, keyword);

            // Send encrypted message back
            send(client_socket, message, message_len, 0);
            printf("Encrypted message sent back to client.\n");

            free(message);
            keyword_received = 1;
            break;
        }
    }

    if (bytes_read == -1) {
        perror("recv error");
    }

    if (!keyword_received) {
        printf("Error receiving keyword or message.\n");
    }
}


void vigenere_cipher(char *text, const char *key) {
    int text_len = strlen(text);
    int key_len = strlen(key);

    // Normalize the key to uppercase (ignore non-alphabetic characters)
    char *key_upper = malloc(key_len + 1);
    int valid_key_len = 0;
    for (int i = 0; i < key_len; i++) {
        if (isalpha(key[i])) {
            key_upper[valid_key_len++] = toupper(key[i]);
        }
    }
    key_upper[valid_key_len] = '\0';

    if (valid_key_len == 0) {
        free(key_upper);
        return;
    }

    for (int i = 0, j = 0; i < text_len; i++) {
        if (isupper(text[i])) {
            text[i] = ((text[i] - 'A' + (key_upper[j % valid_key_len] - 'A')) % 26) + 'A';
            j++;
        } else if (islower(text[i])) {
            text[i] = ((text[i] - 'a' + (key_upper[j % valid_key_len] - 'A')) % 26) + 'a';
            j++;
        }
    }

    free(key_upper);
}