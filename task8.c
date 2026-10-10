#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/stat.h>

int digit_value(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 10;
    }

    if (ch >= 'a' && ch <= 'z') {
        return ch - 'a' + 10;
    }

    return -1;
}


char *canonical_path(const char *path) {
    char *resolved;
    char *directory;
    char *resolved_directory;
    char *result;
    const char *base;
    const char *slash;
    size_t len;

    if (path == NULL || path[0] == '\0') {
        return NULL;
    }

    resolved = realpath(path, NULL);
    if (resolved != NULL) {
        return resolved;
    }

    slash = strrchr(path, '/');

    if (slash == NULL) {
        directory = (char *)malloc(2);
        if (directory == NULL) {
            return NULL;
        }

        strcpy(directory, ".");
        base = path;
    } else {
        base = slash + 1;

        if (*base == '\0') {
            return NULL;
        }

        if (slash == path) {
            directory = (char *)malloc(2);
            if (directory == NULL) {
                return NULL;
            }

            strcpy(directory, "/");
        } else {
            len = (size_t)(slash - path);
            directory = (char *)malloc(len + 1);

            if (directory == NULL) {
                return NULL;
            }

            memcpy(directory, path, len);
            directory[len] = '\0';
        }
    }

    resolved_directory = realpath(directory, NULL);
    free(directory);

    if (resolved_directory == NULL) {
        return NULL;
    }

    len = strlen(resolved_directory) + strlen(base) + 2;
    result = (char *)malloc(len);

    if (result == NULL) {
        free(resolved_directory);
        return NULL;
    }

    if (resolved_directory[strlen(resolved_directory) - 1] == '/') {
        snprintf(result, len, "%s%s", resolved_directory, base);
    } else {
        snprintf(result, len, "%s/%s", resolved_directory, base);
    }

    free(resolved_directory);
    return result;
}

int same_file(const char *path1, const char *path2)
{
    struct stat st1, st2;
    char *p1;
    char *p2;
    int same;

    if (path1 == NULL || path2 == NULL) {
        return 1;
    }

    if (stat(path1, &st1) == 0 && stat(path2, &st2) == 0) {
        return st1.st_dev == st2.st_dev &&
               st1.st_ino == st2.st_ino;
    }

    p1 = canonical_path(path1);
    p2 = canonical_path(path2);

    if (p1 == NULL || p2 == NULL) {
        free(p1);
        free(p2);
        return 1;
    }

    same = strcmp(p1, p2) == 0;

    free(p1);
    free(p2);

    return same;
}


int read_token(FILE *file, char **token, int *has_token) {
    char *buffer;
    size_t length = 0;
    size_t capacity = 16;
    int ch;

    if (file == NULL || token == NULL || has_token == NULL) {
        return 1;
    }

    *token = NULL;
    *has_token = 0;

    buffer = (char *)malloc(capacity);

    if (buffer == NULL) {
        return 3;
    }

    while ((ch = fgetc(file)) != EOF) {
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            if (length > 0) {
                break;
            }

            continue;
        }

        if (length + 1 >= capacity) {
            size_t new_capacity;
            char *temp;

            if (capacity > (size_t)-1 / 2) {
                free(buffer);
                return 3;
            }

            new_capacity = capacity * 2;
            temp = (char *)realloc(buffer, new_capacity);

            if (temp == NULL) {
                free(buffer);
                return 3;
            }

            buffer = temp;
            capacity = new_capacity;
        }

        buffer[length] = (char)ch;
        length++;
    }

    if (ch == EOF && !feof(file)) {
        free(buffer);
        return 2;
    }

    if (length == 0) {
        free(buffer);
        return 0;
    }

    buffer[length] = '\0';

    *token = buffer;
    *has_token = 1;

    return 0;
}

int normalize_token(char *token, int *base) {
    int i;
    int max_digit = 0;
    int length;

    if (token == NULL || base == NULL) {
        return 1;
    }

    for (i = 0; token[i] != '\0'; i++) {
        int value = digit_value(token[i]);

        if (value < 0) {
            return 4;
        }

        if (value > max_digit) {
            max_digit = value;
        }
    }

    *base = max_digit + 1;

    if (*base < 2) {
        *base = 2;
    }

    i = 0;

    while (token[i] == '0' && token[i + 1] != '\0') {
        i++;
    }

    if (i > 0) {
        length = (int)strlen(token + i);
        memmove(token, token + i, (size_t)length + 1);
    }

    return 0;
}

int convert_to_decimal(const char *token, int base, unsigned long long *result) {
    int i;
    unsigned long long value = 0;

    if (token == NULL || result == NULL ||
        base < 2 || base > 36) {
        return 1;
    }

    for (i = 0; token[i] != '\0'; i++) {
        int digit = digit_value(token[i]);

        if (digit < 0 || digit >= base) {
            return 4;
        }

        if (value > (ULLONG_MAX - (unsigned long long)digit) / (unsigned long long)base) {
            return 5;
        }

        value = value * (unsigned long long)base + (unsigned long long)digit;
    }

    *result = value;

    return 0;
}

int process_file(const char *input_path, const char *output_path) {
    FILE *input;
    FILE *output;
    char *token = NULL;
    int has_token = 0;
    int base;
    int status = 0;
    unsigned long long decimal_value;

    input = fopen(input_path, "r");

    if (input == NULL) {
        return 2;
    }

    output = fopen(output_path, "w");

    if (output == NULL) {
        fclose(input);
        return 2;
    }

    while (status == 0) {
        status = read_token(input, &token, &has_token);

        if (status != 0 || has_token == 0) {
            break;
        }

        status = normalize_token(token, &base);

        if (status == 0) {
            status = convert_to_decimal(token, base, &decimal_value);
        }

        if (status == 0) {
            if (fprintf(output, "%s %d %llu\n", token, base, decimal_value) < 0) {
                status = 2;
            }
        }

        free(token);
        token = NULL;
    }

    free(token);

    if (fclose(input) == EOF && status == 0) {
        status = 2;
    }

    if (fclose(output) == EOF && status == 0) {
        status = 2;
    }

    return status;
}

int main(int argc, char *argv[]) {
    int status;

    if (argc != 3) {
        printf("use program input.txt output.txt\n");
        return 1;
    }

    if (same_file(argv[1], argv[2])) {
        printf("input and output files must be different.\n");
        return 1;
    }

    status = process_file(argv[1], argv[2]);

    if (status == 2) {
        printf("error read or write files.\n");
        return 1;
    }

    if (status == 3) {
        printf("not enough memory.\n");
        return 1;
    }

    if (status == 4) {
        printf("invalid number.\n");
        return 1;
    }

    if (status == 5) {
        printf("number is too large.\n");
        return 1;
    }

    if (status != 0) {
        printf("operation failed.\n");
        return 1;
    }

    return 0;
}
