#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


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
            size_t new_capacity = capacity * 2;
            char *temp = (char *)realloc(buffer, new_capacity);

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

    if (ferror(file)) {
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


void to_lower_latin(char *token) {
    int i = 0;

    if (token == NULL) {
        return;
    }

    while (token[i] != '\0') {
        if (token[i] >= 'A' && token[i] <= 'Z') {
            token[i] = token[i] - 'A' + 'a';
        }

        i++;
    }
}


int write_number_base(int number, int base, FILE *output) {
    char digits[32];
    int length = 0;
    int remainder;
    int i;

    if (output == NULL || number < 0 ||
        (base != 4 && base != 8)) {
        return 1;
    }

    if (number == 0) {
        digits[length] = '0';
        length++;
    }

    while (number > 0) {
        remainder = number % base;
        digits[length] = (char)('0' + remainder);
        length++;
        number = number / base;
    }

    for (i = length - 1; i >= 0; i--) {
        if (fputc(digits[i], output) == EOF) {
            return 2;
        }
    }

    return 0;
}


int write_processed_token(char *token, int token_number, FILE *output) {
    int i;
    int status;

    if (token == NULL || output == NULL || token_number < 1) {
        return 1;
    }

    if (token_number % 10 == 0) {
        to_lower_latin(token);

        for (i = 0; token[i] != '\0'; i++) {
            status = write_number_base((unsigned char)token[i], 4, output);

            if (status != 0) {
                return status;
            }

            if (token[i + 1] != '\0') {
                if (fputc(' ', output) == EOF) {
                    return 2;
                }
            }
        }
    } else if (token_number % 2 == 0) {
        to_lower_latin(token);

        if (fputs(token, output) == EOF) {
            return 2;
        }
    } else if (token_number % 5 == 0) {
        for (i = 0; token[i] != '\0'; i++) {
            status = write_number_base((unsigned char)token[i], 8, output);

            if (status != 0) {
                return status;
            }

            if (token[i + 1] != '\0') {
                if (fputc(' ', output) == EOF) {
                    return 2;
                }
            }
        }
    } else {
        if (fputs(token, output) == EOF) {
            return 2;
        }
    }

    return 0;
}


int process_a(const char *input_path, const char *output_path) {
    FILE *input;
    FILE *output;
    char *token = NULL;
    int has_token = 0;
    int token_number = 0;
    int status = 0;

    if (input_path == NULL || output_path == NULL) {
        return 1;
    }

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

        token_number++;

        if (token_number > 1) {
            if (fputc(' ', output) == EOF) {
                status = 2;
            }
        }

        if (status == 0) {
            status = write_processed_token(token, token_number, output);
        }

        free(token);
        token = NULL;
    }

    free(token);

    if (ferror(output) && status == 0) {
        status = 2;
    }

    if (fclose(input) == EOF && status == 0) {
        status = 2;
    }

    if (fclose(output) == EOF && status == 0) {
        status = 2;
    }

    return status;
}


int process_r(const char *first_path, const char *second_path,
              const char *output_path)
{
    FILE *first;
    FILE *second;
    FILE *output;
    char *token_first = NULL;
    char *token_second = NULL;
    int has_first = 0;
    int has_second = 0;
    int first_done = 0;
    int second_done = 0;
    int written = 0;
    int status = 0;

    if (first_path == NULL || second_path == NULL ||
        output_path == NULL) {
        return 1;
    }

    first = fopen(first_path, "r");

    if (first == NULL) {
        return 2;
    }

    second = fopen(second_path, "r");

    if (second == NULL) {
        fclose(first);
        return 2;
    }

    output = fopen(output_path, "w");

    if (output == NULL) {
        fclose(first);
        fclose(second);
        return 2;
    }

    while (status == 0 && (!first_done || !second_done)) {
        if (!first_done) {
            status = read_token(first, &token_first, &has_first);

            if (status == 0 && has_first == 0) {
                first_done = 1;
            }
        }

        if (status == 0 && !second_done) {
            status = read_token(second, &token_second, &has_second);

            if (status == 0 && has_second == 0) {
                second_done = 1;
            }
        }

        if (status == 0 && has_first == 1) {
            if (written > 0 && fputc(' ', output) == EOF) {
                status = 2;
            }

            if (status == 0 && fputs(token_first, output) == EOF) {
                status = 2;
            }

            if (status == 0) {
                written++;
            }
        }

        free(token_first);
        token_first = NULL;
        has_first = 0;

        if (status == 0 && has_second == 1) {
            if (written > 0 && fputc(' ', output) == EOF) {
                status = 2;
            }

            if (status == 0 && fputs(token_second, output) == EOF) {
                status = 2;
            }

            if (status == 0) {
                written++;
            }
        }

        free(token_second);
        token_second = NULL;
        has_second = 0;
    }

    free(token_first);
    free(token_second);

    if (ferror(output) && status == 0) {
        status = 2;
    }

    if (fclose(first) == EOF && status == 0) {
        status = 2;
    }

    if (fclose(second) == EOF && status == 0) {
        status = 2;
    }

    if (fclose(output) == EOF && status == 0) {
        status = 2;
    }

    return status;
}


int main(int argc, char *argv[])
{
    int status;

    if (argc < 2) {
        printf("Args read error.\n");
        return 1;
    }

    if (strcmp(argv[1], "-a") == 0 ||
        strcmp(argv[1], "/a") == 0) {

        if (argc != 4) {
            printf("use -a input output\n");
            return 1;
        }

        if (same_file(argv[2], argv[3])) {
            printf("Input and output refer to the same file.\n");
            return 1;
        }

        status = process_a(argv[2], argv[3]);

    } else if (strcmp(argv[1], "-r") == 0 ||
               strcmp(argv[1], "/r") == 0) {

        if (argc != 5) {
            printf("use -r input1 input2 output\n");
            return 1;
        }

        if (same_file(argv[2], argv[4]) ||
            same_file(argv[3], argv[4])) {
            printf("Output and input refer to the same file.\n");
            return 1;
        }

        status = process_r(argv[2], argv[3], argv[4]);

    } else {
        printf("unknown flag.\n");
        return 1;
    }

    if (status != 0) {
        printf("operation failed %d\n", status);
        return 1;
    }

    return 0;
}
