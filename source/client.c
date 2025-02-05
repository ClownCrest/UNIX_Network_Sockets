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
    int dots = 0;
    char temp[16];

    if (ip == NULL || strlen(ip) > 15) return 0;

    strncpy(temp, ip, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *token = strtok(temp, ".");
    while (token)
    {
        char *endptr;
        long num = strtol(token, &endptr, 10);
        if (*endptr != '\0')
        {
            fprintf(stderr, "Error: Invalid IP format. Only numbers and dots are allowed.\n");
            return 0;
        }

        if (num < 0 || num > 255)
        {
            fprintf(stderr, "Error: IP segment '%s' is out of range (0-255).\n", token);
            return 0;
        }

        dots++;  // Count segments (not dots)
        token = strtok(NULL, ".");
    }

    return dots == 4 ? 1 : 0;  // IPv4 must have exactly 4 parts
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