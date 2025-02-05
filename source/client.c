#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>


void validate_argument_number(int argc);
void parse_arguments(int argc, char *argv[], char **ip, char **port, char **filename, char **keyword);
int is_valid_ip(const char *ip);
int is_valid_port(const char *port);
void validate_arguments(char **ip, char **port, char **filename, char **keyword);

int main(int argc, char *argv[])
{
    char *ip = NULL, *port = NULL, *filename = NULL, *keyword = NULL;

    validate_argument_number(argc);
    parse_arguments(argc, argv, &ip, &port, &filename, &keyword);
    validate_arguments(&ip, &port, &filename, &keyword);

    printf("IP Address: %s\n", ip);
    printf("Port: %s\n", port);
    printf("Filename: %s\n", filename);
    printf("Keyword: %s\n", keyword);

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

    // Validate Keyword
    if (*keyword == NULL || strlen(*keyword) == 0)
    {
        fprintf(stderr, "Error: Keyword cannot be empty.\n");
        exit(EXIT_FAILURE);
    }
}

int is_valid_ip(const char *ip)
{
    if (ip == NULL) return 0;

    size_t len = strlen(ip);

    // IPv4 min length is 7, e.g., "0.0.0.0", IPv4 max length is 15, e.g., "255.255.255.255"
    if (len < 7 || len > 15)
    {
        fprintf(stderr, "Error: Invalid IP length.\n");
        return 0;
    }

    // Check for leading or trailing dots
    if (ip[0] == '.' || ip[len - 1] == '.')
    {
        fprintf(stderr, "Error: IP cannot start or end with a dot.\n");
        return 0;
    }

    // Check for consecutive dots
    for (int i = 0; i < len - 1; i++)
    {
        if (ip[i] == '.' && ip[i + 1] == '.')
        {
            fprintf(stderr, "Error: Consecutive dots in IP address.\n");
            return 0;
        }
    }

    // Count total dots (must be exactly 3)
    int dot_count = 0;
    for (int i = 0; ip[i]; i++)
    {
        if (ip[i] == '.') dot_count++;
    }
    if (dot_count != 3)
    {
        fprintf(stderr, "Error: IP must contain exactly 3 dots.\n");
        return 0;
    }

    // Copy to temporary buffer for tokenization
    char temp[16];
    strncpy(temp, ip, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    // Split into segments and validate each
    int segments = 0;
    char *token = strtok(temp, ".");
    while (token != NULL)
    {
        // Check if segment is empty (strtok skips empty, but our checks above prevent this) (consecutive dots check)
        if (*token == '\0')
        {
            fprintf(stderr, "Error: Empty IP segment.\n");
            return 0;
        }

        // Check for non-digit characters
        for (int i = 0; token[i]; i++)
        {
            if (!isdigit((unsigned char)token[i]))
            {
                fprintf(stderr, "Error: Invalid character '%c' in IP segment.\n", token[i]);
                return 0;
            }
        }

        // Avoid leading zeros (e.g., "01" is invalid unless the segment is "0")
        if (strlen(token) > 1 && token[0] == '0')
        {
            fprintf(stderr, "Error: Segment '%s' has leading zeros.\n", token);
            return 0;
        }

        // Convert to number and check range
        long num = strtol(token, NULL, 10);
        if (num < 0 || num > 255)
        {
            fprintf(stderr, "Error: IP segment '%s' is out of range (0-255).\n", token);
            return 0;
        }

        segments++;
        token = strtok(NULL, ".");
    }

    // Ensure exactly 4 segments
    if (segments != 4)
    {
        fprintf(stderr, "Error: IP must have exactly 4 segments.\n");
        return 0;
    }

    return 1;  // Valid IPv4
}


int is_valid_port(const char *port)
{
    char *endptr;
    long port_num = strtol(port, &endptr, 10);

    // Check if conversion was successful and if there are extra non-numeric characters
    if (*endptr != '\0' || port == endptr)
    {
        return 0;  // Invalid input: non-numeric characters or empty string
    }

    // Check if the number is within the valid port range (1-65535)
    if (port_num < 1 || port_num > 65535 || (port[0] == '0' && strlen(port) > 1))
    {
        return 0;
    }

    return 1;  // Valid port
}