#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_INVALID_EXPRESSION,
    STATUS_MEMORY_ERROR,
    STATUS_OVERFLOW_ERROR,
    STATUS_READ_ERROR,
    STATUS_WRITE_ERROR
} Status;

typedef struct Node {
    char *name;
    struct Node *parent;
    struct Node *first_child;
    struct Node *last_child;
    struct Node *next_sibling;
} Node;

Status create_node(const char *start, size_t length, Node *parent, Node **result) {
    if (start == NULL || result == NULL || length == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;

    if (length == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }

    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    node->name = (char *)malloc(length + 1);
    if (node->name == NULL) {
        free(node);
        return STATUS_MEMORY_ERROR;
    }

    memcpy(node->name, start, length);
    node->name[length] = '\0';
    node->parent = parent;
    node->first_child = NULL;
    node->last_child = NULL;
    node->next_sibling = NULL;

    *result = node;
    return STATUS_OK;
}

void free_tree(Node *root) {
    Node *current = root;

    while (current != NULL) {
        if (current->first_child != NULL) {
            Node *child = current->first_child;
            current->first_child = child->next_sibling;
            current = child;
        } else {
            Node *parent = current->parent;
            free(current->name);
            free(current);
            current = parent;
        }
    }
}

Status read_line(FILE *input, char **result, int *end_of_file) {
    if (input == NULL || result == NULL || end_of_file == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;
    *end_of_file = 0;

    int ch = fgetc(input);
    if (ch == EOF) {
        if (ferror(input)) {
            return STATUS_READ_ERROR;
        }
        *end_of_file = 1;
        return STATUS_OK;
    }

    size_t capacity = 64;
    size_t length = 0;
    char *buffer = (char *)malloc(capacity);
    if (buffer == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    while (ch != EOF && ch != '\n') {
        if (ch == '\0') {
            free(buffer);
            return STATUS_INVALID_EXPRESSION;
        }

        if (length == capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                free(buffer);
                return STATUS_OVERFLOW_ERROR;
            }

            size_t new_capacity = capacity * 2;
            char *new_buffer = (char *)realloc(buffer, new_capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return STATUS_MEMORY_ERROR;
            }

            buffer = new_buffer;
            capacity = new_capacity;
        }

        buffer[length] = (char)ch;
        length++;
        ch = fgetc(input);
    }

    if (ch == EOF && ferror(input)) {
        free(buffer);
        return STATUS_READ_ERROR;
    }

    buffer[length] = '\0';
    *result = buffer;
    return STATUS_OK;
}

Status parse_expression(const char *text, Node **result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;

    Node *root = NULL;
    Node *parent = NULL;
    Node *last_node = NULL;
    int expect_name = 1;
    int just_opened = 0;
    int just_closed = 0;
    size_t i = 0;
    Status status = STATUS_OK;

    while (text[i] != '\0') {
        unsigned char ch = (unsigned char)text[i];

        if (isspace(ch)) {
            i++;
            continue;
        }

        if (ch == '(') {
            if (expect_name || last_node == NULL || just_closed) {
                status = STATUS_INVALID_EXPRESSION;
                break;
            }

            parent = last_node;
            last_node = NULL;
            expect_name = 1;
            just_opened = 1;
            just_closed = 0;
            i++;
            continue;
        }

        if (ch == ',') {
            if (expect_name || parent == NULL) {
                status = STATUS_INVALID_EXPRESSION;
                break;
            }

            last_node = NULL;
            expect_name = 1;
            just_opened = 0;
            just_closed = 0;
            i++;
            continue;
        }

        if (ch == ')') {
            if (parent == NULL || (expect_name && !just_opened)) {
                status = STATUS_INVALID_EXPRESSION;
                break;
            }

            last_node = parent;
            parent = parent->parent;
            expect_name = 0;
            just_opened = 0;
            just_closed = 1;
            i++;
            continue;
        }

        if (!expect_name) {
            status = STATUS_INVALID_EXPRESSION;
            break;
        }

        size_t start = i;
        while (text[i] != '\0' &&
               !isspace((unsigned char)text[i]) &&
               text[i] != '(' && text[i] != ')' && text[i] != ',') {
            i++;
        }

        Node *node = NULL;
        status = create_node(text + start, i - start, parent, &node);
        if (status != STATUS_OK) {
            break;
        }

        if (parent != NULL) {
            if (parent->last_child == NULL) {
                parent->first_child = node;
            } else {
                parent->last_child->next_sibling = node;
            }
            parent->last_child = node;
        } else if (root == NULL) {
            root = node;
        } else {
            free(node->name);
            free(node);
            status = STATUS_INVALID_EXPRESSION;
            break;
        }

        last_node = node;
        expect_name = 0;
        just_opened = 0;
        just_closed = 0;
    }

    if (status == STATUS_OK && (root == NULL || parent != NULL || expect_name)) {
        status = STATUS_INVALID_EXPRESSION;
    }

    if (status != STATUS_OK) {
        free_tree(root);
        return status;
    }

    *result = root;
    return STATUS_OK;
}

Status write_tree(FILE *output, const Node *root) {
    if (output == NULL || root == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    const Node *current = root;
    size_t depth = 0;

    while (current != NULL) {
        for (size_t i = 0; i < depth; i++) {
            if (fputs("  ", output) == EOF) {
                return STATUS_WRITE_ERROR;
            }
        }

        if (fputs(current->name, output) == EOF || fputc('\n', output) == EOF) {
            return STATUS_WRITE_ERROR;
        }

        if (current->first_child != NULL) {
            current = current->first_child;
            depth++;
        } else {
            while (current != NULL && current->next_sibling == NULL) {
                current = current->parent;
                if (current != NULL) {
                    depth--;
                }
            }

            if (current != NULL) {
                current = current->next_sibling;
            }
        }
    }

    return STATUS_OK;
}

Status process_file(FILE *input, FILE *output, size_t *error_line) {
    if (input == NULL || output == NULL || error_line == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *error_line = 0;
    size_t line_number = 0;

    while (1) {
        if (line_number == SIZE_MAX) {
            return STATUS_OVERFLOW_ERROR;
        }

        *error_line = line_number + 1;
        char *line = NULL;
        int end_of_file = 0;
        Status status = read_line(input, &line, &end_of_file);
        if (status != STATUS_OK) {
            return status;
        }

        if (end_of_file) {
            return STATUS_OK;
        }

        line_number++;
        Node *root = NULL;
        status = parse_expression(line, &root);

        if (status == STATUS_OK && fprintf(output, "Tree %zu:\n", line_number) < 0) {
            status = STATUS_WRITE_ERROR;
        }

        if (status == STATUS_OK) {
            status = write_tree(output, root);
        }

        if (status == STATUS_OK && fputc('\n', output) == EOF) {
            status = STATUS_WRITE_ERROR;
        }

        free_tree(root);
        free(line);

        if (status != STATUS_OK) {
            return status;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.txt output.txt\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], argv[2]) == 0) {
        fprintf(stderr, "Error: input and output files must be different.\n");
        return 1;
    }

    struct stat input_info;
    struct stat output_info;
    if (stat(argv[1], &input_info) == 0 &&
        stat(argv[2], &output_info) == 0 &&
        input_info.st_dev == output_info.st_dev &&
        input_info.st_ino == output_info.st_ino) {
        fprintf(stderr, "Error: input and output refer to the same file.\n");
        return 1;
    }

    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        fprintf(stderr, "Error: cannot open input file.\n");
        return 1;
    }

    FILE *output = fopen(argv[2], "wb");
    if (output == NULL) {
        fprintf(stderr, "Error: cannot open output file.\n");
        fclose(input);
        return 1;
    }

    size_t error_line = 0;
    Status status = process_file(input, output, &error_line);

    if (fclose(output) != 0 && status == STATUS_OK) {
        status = STATUS_WRITE_ERROR;
    }
    if (fclose(input) != 0 && status == STATUS_OK) {
        status = STATUS_READ_ERROR;
    }

    if (status == STATUS_OK) {
        return 0;
    }

    if (status == STATUS_INVALID_EXPRESSION) {
        fprintf(stderr, "Error: invalid expression on line %zu.\n", error_line);
    } else if (status == STATUS_MEMORY_ERROR) {
        fprintf(stderr, "Error: cannot allocate memory.\n");
    } else if (status == STATUS_OVERFLOW_ERROR) {
        fprintf(stderr, "Error: data size overflow.\n");
    } else if (status == STATUS_READ_ERROR) {
        fprintf(stderr, "Error: cannot read input file.\n");
    } else if (status == STATUS_WRITE_ERROR) {
        fprintf(stderr, "Error: cannot write output file.\n");
    } else {
        fprintf(stderr, "Error: invalid function argument.\n");
    }

    return 1;
}
