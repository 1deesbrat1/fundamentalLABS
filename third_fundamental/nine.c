#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_INPUT_ERROR,
    STATUS_FILE_ERROR,
    STATUS_MEMORY_ERROR,
    STATUS_OVERFLOW_ERROR,
    STATUS_FORMAT_ERROR
} Status;

typedef struct TreeNode {
    char *word;
    size_t count;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

void free_tree(TreeNode *root) {
    if (root == NULL) {
        return;
    }

    free_tree(root->left);
    free_tree(root->right);
    free(root->word);
    free(root);
}

Status read_line(const char *prompt, char **result) {
    if (prompt == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;
    puts(prompt);

    size_t capacity = 32;
    size_t length = 0;
    char *line = (char *)malloc(capacity);
    if (line == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (ch == '\0') {
            free(line);
            return STATUS_INPUT_ERROR;
        }

        if (length >= capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                free(line);
                return STATUS_OVERFLOW_ERROR;
            }
            size_t new_capacity = capacity * 2;
            char *new_line = (char *)realloc(line, new_capacity);
            if (new_line == NULL) {
                free(line);
                return STATUS_MEMORY_ERROR;
            }
            line = new_line;
            capacity = new_capacity;
        }
        line[length++] = (char)ch;
    }

    if (ferror(stdin)) {
        free(line);
        return STATUS_INPUT_ERROR;
    }
    if (ch == EOF && length == 0) {
        free(line);
        return STATUS_INPUT_ERROR;
    }

    line[length] = '\0';
    *result = line;
    return STATUS_OK;
}

Status parse_positive_number(const char *text, size_t *result) {
    if (text == NULL || result == NULL || text[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t value = 0;
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return STATUS_INPUT_ERROR;
        }
        size_t digit = (size_t)(text[i] - '0');
        if (value > (SIZE_MAX - digit) / 10) {
            return STATUS_OVERFLOW_ERROR;
        }
        value = value * 10 + digit;
    }

    if (value == 0) {
        return STATUS_INPUT_ERROR;
    }
    *result = value;
    return STATUS_OK;
}

Status insert_word(TreeNode **root, const char *word, size_t *unique_count) {
    if (root == NULL || word == NULL || word[0] == '\0' || unique_count == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    TreeNode **current = root;
    while (*current != NULL) {
        int cmp = strcmp(word, (*current)->word);
        if (cmp == 0) {
            if ((*current)->count == SIZE_MAX) {
                return STATUS_OVERFLOW_ERROR;
            }
            (*current)->count++;
            return STATUS_OK;
        }
        if (cmp < 0) {
            current = &(*current)->left;
        } else {
            current = &(*current)->right;
        }
    }

    if (*unique_count == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }

    size_t length = strlen(word);
    if (length == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    if (node == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    node->word = (char *)malloc(length + 1);
    if (node->word == NULL) {
        free(node);
        return STATUS_MEMORY_ERROR;
    }
    memcpy(node->word, word, length + 1);
    node->count = 1;
    node->left = NULL;
    node->right = NULL;
    *current = node;
    (*unique_count)++;
    return STATUS_OK;
}

Status read_text(const char *path, const unsigned char separators[256], TreeNode **root, size_t *unique_count) {
    if (path == NULL || separators == NULL || root == NULL || unique_count == NULL || path[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    size_t capacity = 32;
    size_t length = 0;
    char *word = (char *)malloc(capacity);
    if (word == NULL) {
        fclose(file);
        return STATUS_MEMORY_ERROR;
    }

    Status status = STATUS_OK;
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (separators[(unsigned char)ch] != 0) {
            if (length != 0) {
                word[length] = '\0';
                status = insert_word(root, word, unique_count);
                length = 0;
            }
        } else if (ch == '\0') {
            status = STATUS_INPUT_ERROR;
        } else {
            if (length >= capacity - 1) {
                if (capacity > SIZE_MAX / 2) {
                    status = STATUS_OVERFLOW_ERROR;
                } else {
                    size_t new_capacity = capacity * 2;
                    char *new_word = (char *)realloc(word, new_capacity);
                    if (new_word == NULL) {
                        status = STATUS_MEMORY_ERROR;
                    } else {
                        word = new_word;
                        capacity = new_capacity;
                    }
                }
            }
            if (status == STATUS_OK) {
                word[length++] = (char)ch;
            }
        }
        if (status != STATUS_OK) {
            break;
        }
    }

    if (status == STATUS_OK && ferror(file)) {
        status = STATUS_FILE_ERROR;
    }
    if (status == STATUS_OK && length != 0) {
        word[length] = '\0';
        status = insert_word(root, word, unique_count);
    }
    free(word);
    if (fclose(file) != 0 && status == STATUS_OK) {
        status = STATUS_FILE_ERROR;
    }
    return status;
}

const TreeNode *find_word(const TreeNode *root, const char *word) {
    if (word == NULL || word[0] == '\0') {
        return NULL;
    }

    while (root != NULL) {
        int cmp = strcmp(word, root->word);
        if (cmp == 0) {
            return root;
        }
        if (cmp < 0) {
            root = root->left;
        } else {
            root = root->right;
        }
    }
    return NULL;
}

Status print_words(const TreeNode *root) {
    if (root == NULL) {
        return STATUS_OK;
    }

    Status status = print_words(root->left);
    if (status != STATUS_OK) {
        return status;
    }
    if (printf("%s: %zu\n", root->word, root->count) < 0) {
        return STATUS_FILE_ERROR;
    }
    return print_words(root->right);
}

Status find_extremes(const TreeNode *root, const TreeNode **shortest, const TreeNode **longest) {
    if (shortest == NULL || longest == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (root == NULL) {
        return STATUS_OK;
    }

    if (*shortest == NULL || strlen(root->word) < strlen((*shortest)->word)) {
        *shortest = root;
    }
    if (*longest == NULL || strlen(root->word) > strlen((*longest)->word)) {
        *longest = root;
    }

    Status status = find_extremes(root->left, shortest, longest);
    if (status != STATUS_OK) {
        return status;
    }
    return find_extremes(root->right, shortest, longest);
}

void collect_nodes(const TreeNode *root, const TreeNode **array, size_t *index) {
    if (root == NULL || array == NULL || index == NULL) {
        return;
    }
    collect_nodes(root->left, array, index);
    array[(*index)++] = root;
    collect_nodes(root->right, array, index);
}

int compare_frequency(const void *first, const void *second) {
    if (first == NULL || second == NULL) {
        return 0;
    }

    const TreeNode *a = *(const TreeNode *const *)first;
    const TreeNode *b = *(const TreeNode *const *)second;

    if (a->count > b->count) {
        return -1;
    }
    if (a->count < b->count) {
        return 1;
    }
    return strcmp(a->word, b->word);
}

Status get_top_words(const TreeNode *root, size_t unique_count, const TreeNode ***result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *result = NULL;
    if (unique_count == 0) {
        return STATUS_OK;
    }
    if (root == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (unique_count > SIZE_MAX / sizeof(const TreeNode *)) {
        return STATUS_OVERFLOW_ERROR;
    }

    const TreeNode **array = (const TreeNode **)malloc(unique_count * sizeof(const TreeNode *));
    if (array == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    size_t index = 0;
    collect_nodes(root, array, &index);
    qsort(array, unique_count, sizeof(const TreeNode *), compare_frequency);
    *result = array;
    return STATUS_OK;
}

Status tree_depth(const TreeNode *root, size_t *depth) {
    if (depth == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (root == NULL) {
        *depth = 0;
        return STATUS_OK;
    }

    size_t left_depth = 0;
    size_t right_depth = 0;
    Status status = tree_depth(root->left, &left_depth);
    if (status != STATUS_OK) {
        return status;
    }
    status = tree_depth(root->right, &right_depth);
    if (status != STATUS_OK) {
        return status;
    }
    size_t maximum = left_depth;
    if (right_depth > maximum) {
        maximum = right_depth;
    }
    if (maximum == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }
    *depth = maximum + 1;
    return STATUS_OK;
}

Status save_node(FILE *file, const TreeNode *root) {
    if (file == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    unsigned char marker = 0;
    if (root != NULL) {
        marker = 1;
    }
    if (fwrite(&marker, 1, 1, file) != 1) {
        return STATUS_FILE_ERROR;
    }
    if (root == NULL) {
        return STATUS_OK;
    }

    size_t length = strlen(root->word);
    if (fwrite(&length, sizeof(length), 1, file) != 1 ||
        fwrite(&root->count, sizeof(root->count), 1, file) != 1 ||
        fwrite(root->word, 1, length, file) != length) {
        return STATUS_FILE_ERROR;
    }

    Status status = save_node(file, root->left);
    if (status != STATUS_OK) {
        return status;
    }
    return save_node(file, root->right);
}

Status save_tree(const char *path, const TreeNode *root) {
    if (path == NULL || path[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    const unsigned char header[4] = {'B', 'S', 'T', '1'};
    Status status = STATUS_OK;
    if (fwrite(header, 1, sizeof(header), file) != sizeof(header)) {
        status = STATUS_FILE_ERROR;
    } else {
        status = save_node(file, root);
    }
    if (fclose(file) != 0 && status == STATUS_OK) {
        status = STATUS_FILE_ERROR;
    }
    return status;
}

Status load_node(FILE *file, TreeNode **root, const char *lower, const char *upper, size_t *unique_count) {
    if (file == NULL || root == NULL || unique_count == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *root = NULL;

    unsigned char marker;
    if (fread(&marker, 1, 1, file) != 1) {
        return STATUS_FORMAT_ERROR;
    }
    if (marker == 0) {
        return STATUS_OK;
    }
    if (marker != 1) {
        return STATUS_FORMAT_ERROR;
    }

    size_t length;
    size_t count;
    if (fread(&length, sizeof(length), 1, file) != 1 ||
        fread(&count, sizeof(count), 1, file) != 1 ||
        length == 0 || length == SIZE_MAX || count == 0) {
        return STATUS_FORMAT_ERROR;
    }

    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    if (node == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    node->word = (char *)malloc(length + 1);
    if (node->word == NULL) {
        free(node);
        return STATUS_MEMORY_ERROR;
    }
    node->left = NULL;
    node->right = NULL;
    node->count = count;
    if (fread(node->word, 1, length, file) != length ||
        memchr(node->word, '\0', length) != NULL) {
        free(node->word);
        free(node);
        return STATUS_FORMAT_ERROR;
    }
    node->word[length] = '\0';
    if ((lower != NULL && strcmp(node->word, lower) <= 0) ||
        (upper != NULL && strcmp(node->word, upper) >= 0)) {
        free(node->word);
        free(node);
        return STATUS_FORMAT_ERROR;
    }

    if (*unique_count == SIZE_MAX) {
        free(node->word);
        free(node);
        return STATUS_OVERFLOW_ERROR;
    }
    (*unique_count)++;
    *root = node;
    Status status = load_node(file, &node->left, lower, node->word, unique_count);
    if (status != STATUS_OK) {
        return status;
    }
    return load_node(file, &node->right, node->word, upper, unique_count);
}

Status load_tree(const char *path, TreeNode **root, size_t *unique_count) {
    if (path == NULL || path[0] == '\0' || root == NULL || unique_count == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    unsigned char header[4];
    const unsigned char expected[4] = {'B', 'S', 'T', '1'};
    TreeNode *new_root = NULL;
    size_t new_count = 0;
    Status status = STATUS_OK;

    if (fread(header, 1, sizeof(header), file) != sizeof(header) ||
        memcmp(header, expected, sizeof(header)) != 0) {
        status = STATUS_FORMAT_ERROR;
    } else {
        status = load_node(file, &new_root, NULL, NULL, &new_count);
    }
    if (status == STATUS_OK && fgetc(file) != EOF) {
        status = STATUS_FORMAT_ERROR;
    }
    if (ferror(file)) {
        status = STATUS_FILE_ERROR;
    }
    if (fclose(file) != 0 && status == STATUS_OK) {
        status = STATUS_FILE_ERROR;
    }

    if (status != STATUS_OK) {
        free_tree(new_root);
        return status;
    }
    free_tree(*root);
    *root = new_root;
    *unique_count = new_count;
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argv[1][0] == '\0') {
        fprintf(stderr, "Использование: %s файл разделитель [разделитель ...]\n", argv[0]);
        return 1;
    }

    unsigned char separators[256] = {0};
    for (int i = 2; i < argc; i++) {
        if (strlen(argv[i]) != 1 || argv[i][0] == '\0') {
            fprintf(stderr, "Каждый разделитель должен быть одним однобайтовым символом.\n");
            return 1;
        }
        separators[(unsigned char)argv[i][0]] = 1;
    }

    TreeNode *root = NULL;
    size_t unique_count = 0;
    Status status = read_text(argv[1], separators, &root, &unique_count);
    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка чтения текста (код %d).\n", (int)status);
        free_tree(root);
        return 1;
    }
    printf("Прочитан файл. Различных слов: %zu\n", unique_count);

    while (1) {
        char *choice = NULL;
        status = read_line("\n1 - Все слова\n2 - Частота слова\n3 - Топ N\n4 - Самое длинное и короткое\n5 - Глубина дерева\n6 - Сохранить дерево\n7 - Восстановить дерево\n0 - Выход\nВыберите пункт:", &choice);
        if (status != STATUS_OK) {
            free(choice);
            break;
        }

        if (strcmp(choice, "0") == 0) {
            free(choice);
            break;
        } else if (strcmp(choice, "1") == 0) {
            if (root == NULL) {
                puts("Слов нет.");
            } else {
                status = print_words(root);
            }
        } else if (strcmp(choice, "2") == 0) {
            char *word = NULL;
            status = read_line("Введите слово:", &word);
            if (status == STATUS_OK) {
                int valid = word[0] != '\0';
                for (size_t i = 0; word[i] != '\0'; i++) {
                    if (separators[(unsigned char)word[i]] != 0) {
                        valid = 0;
                    }
                }
                if (!valid) {
                    puts("Некорректное слово.");
                } else {
                    const TreeNode *found = find_word(root, word);
                    if (found == NULL) {
                        puts("Количество: 0");
                    } else {
                        printf("Количество: %zu\n", found->count);
                    }
                }
            }
            free(word);
        } else if (strcmp(choice, "3") == 0) {
            char *line = NULL;
            status = read_line("Введите N:", &line);
            if (status == STATUS_OK) {
                size_t n = 0;
                status = parse_positive_number(line, &n);
                if (status == STATUS_OK) {
                    const TreeNode **array = NULL;
                    status = get_top_words(root, unique_count, &array);
                    if (status == STATUS_OK) {
                        size_t limit = n;
                        if (limit > unique_count) {
                            limit = unique_count;
                        }
                        for (size_t i = 0; i < limit; i++) {
                            printf("%s: %zu\n", array[i]->word, array[i]->count);
                        }
                    }
                    free(array);
                }
            }
            free(line);
        } else if (strcmp(choice, "4") == 0) {
            const TreeNode *shortest = NULL;
            const TreeNode *longest = NULL;
            status = find_extremes(root, &shortest, &longest);
            if (status == STATUS_OK) {
                if (shortest == NULL) {
                    puts("Слов нет.");
                } else {
                    printf("Самое короткое: %s\nСамое длинное: %s\n", shortest->word, longest->word);
                }
            }
        } else if (strcmp(choice, "5") == 0) {
            size_t levels = 0;
            status = tree_depth(root, &levels);
            if (status == STATUS_OK) {
                if (levels == 0) {
                    puts("Глубина дерева: 0 (пустое дерево)");
                } else {
                    printf("Глубина дерева: %zu\n", levels - 1);
                }
            }
        } else if (strcmp(choice, "6") == 0) {
            char *path = NULL;
            status = read_line("Путь для сохранения:", &path);
            if (status == STATUS_OK) {
                status = save_tree(path, root);
                if (status == STATUS_OK) {
                    puts("Дерево сохранено.");
                }
            }
            free(path);
        } else if (strcmp(choice, "7") == 0) {
            char *path = NULL;
            status = read_line("Путь к сохранённому дереву:", &path);
            if (status == STATUS_OK) {
                status = load_tree(path, &root, &unique_count);
                if (status == STATUS_OK) {
                    puts("Дерево восстановлено.");
                }
            }
            free(path);
        } else {
            puts("Неизвестная команда.");
        }

        free(choice);
        if (status != STATUS_OK) {
            printf("Ошибка операции (код %d).\n", (int)status);
            if (status == STATUS_MEMORY_ERROR) {
                break;
            }
        }
    }

    free_tree(root);
    return 0;
}
