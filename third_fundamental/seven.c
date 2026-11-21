#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>

typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef struct {
    char* surname;
    char* name;
    char* patronymic;
    Date birth;
    char gender;
    double income;
} Liver;

typedef struct LiverNode {
    Liver data;
    struct LiverNode* next;
} LiverNode;

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_MEMORY_ERROR,
    STATUS_FILE_ERROR,
    STATUS_INPUT_ERROR,
    STATUS_NOT_FOUND,
    STATUS_OVERFLOW_ERROR,
    STATUS_END_OF_FILE
} Status;

Status validate_name(const char* name) {
    if (name == NULL || name[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t i = 0;
    while (name[i] != '\0') {
        if (!((name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= 'a' && name[i] <= 'z'))) {
            return STATUS_INVALID_ARGUMENT;
        }
        i++;
    }

    return STATUS_OK;
}

Status validate_patronymic(const char* patronymic) {
    if (patronymic == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t i = 0;
    while (patronymic[i] != '\0') {
        if (!((patronymic[i] >= 'A' && patronymic[i] <= 'Z') || (patronymic[i] >= 'a' && patronymic[i] <= 'z'))) {
            return STATUS_INVALID_ARGUMENT;
        }
        i++;
    }

    return STATUS_OK;
}

Status validate_gender(char gender) {
    if (gender == 'M' || gender == 'W') {
        return STATUS_OK;
    } else {
        return STATUS_INVALID_ARGUMENT;
    }
}

Status validate_income(double income) {
    if (!isfinite(income) || income < 0.0) {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

Status is_leap_year(int year, int *result) {
    if (result == NULL || year <= 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) {
        *result = 1;
    } else {
        *result = 0;
    }

    return STATUS_OK;
}

Status days_in_month(int month, int year, int *result) {
    if (result == NULL || year <= 0 || month < 1 || month > 12) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (month == 2) {
        int leap;
        Status status = is_leap_year(year, &leap);

        if (status != STATUS_OK) {
            return status;
        }

        if (leap == 1) {
            *result = 29;
        } else {
            *result = 28;
        }
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        *result = 30;
    } else {
        *result = 31;
    }

    return STATUS_OK;
}

Status validate_date(const Date *date) {
    if (date == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    int days;
    Status status = days_in_month(date->month, date->year, &days);

    if (status != STATUS_OK) {
        return status;
    }

    if (date->day < 1 || date->day > days) {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

Status copy_liver(const Liver *source, Liver *destination) {
    if (source == NULL || destination == NULL || source == destination) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (source->surname == NULL || source->name == NULL || source->patronymic == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (destination->surname != NULL || destination->name != NULL || destination->patronymic != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_name(source->surname) != STATUS_OK || validate_name(source->name) != STATUS_OK || validate_patronymic(source->patronymic) != STATUS_OK || validate_date(&source->birth) != STATUS_OK || validate_gender(source->gender) != STATUS_OK || validate_income(source->income) != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t surname_len = strlen(source->surname);
    size_t name_len = strlen(source->name);
    size_t patronymic_len = strlen(source->patronymic);

    if (surname_len == SIZE_MAX || name_len == SIZE_MAX || patronymic_len == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }

    char *surname_copy = (char *)malloc(surname_len + 1);
    if (surname_copy == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    memcpy(surname_copy, source->surname, surname_len + 1);

    char *name_copy = (char *)malloc(name_len + 1);
    if (name_copy == NULL) {
        free(surname_copy);
        return STATUS_MEMORY_ERROR;
    }

    memcpy(name_copy, source->name, name_len + 1);

    char *patronymic_copy = (char *)malloc(patronymic_len + 1);
    if (patronymic_copy == NULL) {
        free(surname_copy);
        free(name_copy);
        return STATUS_MEMORY_ERROR;
    }

    memcpy(patronymic_copy, source->patronymic, patronymic_len + 1);

    destination->surname = surname_copy;
    destination->name = name_copy;
    destination->patronymic = patronymic_copy;

    destination->birth = source->birth;
    destination->gender = source->gender;
    destination->income = source->income;

    return STATUS_OK;
}

Status free_liver(Liver* liver) {
    if (liver == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    free(liver->surname);
    free(liver->name);
    free(liver->patronymic);
    liver->surname = NULL;
    liver->name = NULL;
    liver->patronymic = NULL;
    liver->gender = '\0';
    liver->income = 0.0;
    liver->birth.day = 0;
    liver->birth.month = 0;
    liver->birth.year = 0;

    return STATUS_OK;
}

Status compare_dates(const Date* first, const Date* second, int* result) {
    if (first == NULL || second == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_date(first) != STATUS_OK || validate_date(second) != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first->year < second->year) {
        *result = -1;
        return STATUS_OK;
    }

    if (first->year > second->year) {
        *result = 1;
        return STATUS_OK;
    }

    if (first->month < second->month) {
        *result = -1;
        return STATUS_OK;
    }

    if (first->month > second->month) {
        *result = 1;
        return STATUS_OK;
    }

    if (first->day < second->day) {
        *result = -1;
        return STATUS_OK;
    }

    if (first->day > second->day) {
        *result = 1;
        return STATUS_OK;
    }

    *result = 0;
    return STATUS_OK;
}

Status create_liver_node(const Liver* source, LiverNode** result) {
    if (source == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status;
    LiverNode* node = (LiverNode* )malloc(sizeof(LiverNode));
    if (node == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    *node = (LiverNode){0};

    status = copy_liver(source, &node->data);
    if (status != STATUS_OK) {
        free(node);
        return status;
    }

    *result = node;

    return STATUS_OK;
}

Status insert_sorted(LiverNode **head, const Liver *source) {
    if (head == NULL || source == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    LiverNode *new_node = NULL;
    Status status = create_liver_node(source, &new_node);

    if (status != STATUS_OK) {
        return status;
    }

    if (*head == NULL) {
        *head = new_node;
        return STATUS_OK;
    }

    int comparison;

    status = compare_dates(&new_node->data.birth, &(*head)->data.birth, &comparison);

    if (status != STATUS_OK) {
        free_liver(&new_node->data);
        free(new_node);
        return status;
    }

    if (comparison > 0) {
        new_node->next = *head;
        *head = new_node;
        return STATUS_OK;
    }

    LiverNode *current = *head;

    while (current->next != NULL) {
        status = compare_dates(&new_node->data.birth, &current->next->data.birth, &comparison);

        if (status != STATUS_OK) {
            free_liver(&new_node->data);
            free(new_node);
            return status;
        }

        if (comparison > 0) {
            break;
        }

        current = current->next;
    }

    new_node->next = current->next;
    current->next = new_node;

    return STATUS_OK;
}

typedef enum {
    CHANGE_ADD,
    CHANGE_DELETE,
    CHANGE_EDIT
} ChangeType;

typedef struct Change {
    ChangeType type;
    LiverNode *node;
    Liver previous;
    struct Change *next;
} Change;

typedef struct {
    Change *top;
    size_t count;
} History;

typedef struct {
    char *surname;
    char *name;
    char *patronymic;
    int use_patronymic;
    Date birth;
    int use_birth;
    char gender;
    int use_gender;
    double income;
    double epsilon;
    int use_income;
} SearchCriteria;

typedef struct {
    size_t index;
    const LiverNode *node;
} SearchMatch;

typedef struct {
    SearchMatch *items;
    size_t count;
} SearchResult;

Status validate_liver(const Liver *liver) {
    if (liver == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (validate_name(liver->surname) != STATUS_OK ||
        validate_name(liver->name) != STATUS_OK ||
        validate_patronymic(liver->patronymic) != STATUS_OK ||
        validate_date(&liver->birth) != STATUS_OK ||
        validate_gender(liver->gender) != STATUS_OK ||
        validate_income(liver->income) != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }
    return STATUS_OK;
}

Status free_list(LiverNode **head) {
    if (head == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    LiverNode *current = *head;
    while (current != NULL) {
        LiverNode *next = current->next;
        free_liver(&current->data);
        free(current);
        current = next;
    }
    *head = NULL;
    return STATUS_OK;
}

Status attach_node_sorted(LiverNode **head, LiverNode *node) {
    if (head == NULL || node == NULL || node->next != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (validate_date(&node->data.birth) != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }

    LiverNode **link = head;
    while (*link != NULL) {
        int comparison;
        Status status = compare_dates(&node->data.birth, &(*link)->data.birth, &comparison);
        if (status != STATUS_OK) {
            return status;
        }
        if (comparison > 0) {
            break;
        }
        link = &(*link)->next;
    }
    node->next = *link;
    *link = node;
    return STATUS_OK;
}

Status detach_node(LiverNode **head, LiverNode *target) {
    if (head == NULL || target == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    LiverNode **link = head;
    while (*link != NULL && *link != target) {
        link = &(*link)->next;
    }
    if (*link == NULL) {
        return STATUS_NOT_FOUND;
    }

    *link = target->next;
    target->next = NULL;
    return STATUS_OK;
}

Status get_node_by_index(LiverNode *head, size_t index, LiverNode **result) {
    if (result == NULL || index == 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    size_t current_index = 1;
    while (head != NULL && current_index < index) {
        head = head->next;
        current_index++;
    }
    if (head == NULL) {
        return STATUS_NOT_FOUND;
    }
    *result = head;
    return STATUS_OK;
}

Status read_line(FILE *stream, char **result) {
    if (stream == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t capacity = 64;
    size_t length = 0;
    char *buffer = (char *)malloc(capacity);
    if (buffer == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    int ch;
    while ((ch = fgetc(stream)) != EOF) {
        if (ch == '\n') {
            break;
        }
        if (ch == '\0') {
            free(buffer);
            return STATUS_INPUT_ERROR;
        }
        if (length >= capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                free(buffer);
                return STATUS_OVERFLOW_ERROR;
            }
            size_t new_capacity = capacity * 2;
            char *grown = (char *)realloc(buffer, new_capacity);
            if (grown == NULL) {
                free(buffer);
                return STATUS_MEMORY_ERROR;
            }
            buffer = grown;
            capacity = new_capacity;
        }
        buffer[length] = (char)ch;
        length++;
    }

    if (ferror(stream)) {
        free(buffer);
        return STATUS_FILE_ERROR;
    }
    if (ch == EOF && length == 0) {
        free(buffer);
        return STATUS_END_OF_FILE;
    }
    if (length > 0 && buffer[length - 1] == '\r') {
        length--;
    }
    buffer[length] = '\0';
    *result = buffer;
    return STATUS_OK;
}

Status parse_date(const char *text, Date *result) {
    if (text == NULL || result == NULL || strlen(text) != 10 ||
        text[2] != '.' || text[5] != '.') {
        return STATUS_INPUT_ERROR;
    }

    const int positions[] = {0, 1, 3, 4, 6, 7, 8, 9};
    size_t i;
    for (i = 0; i < sizeof(positions) / sizeof(positions[0]); i++) {
        char ch = text[positions[i]];
        if (ch < '0' || ch > '9') {
            return STATUS_INPUT_ERROR;
        }
    }

    Date parsed;
    parsed.day = (text[0] - '0') * 10 + (text[1] - '0');
    parsed.month = (text[3] - '0') * 10 + (text[4] - '0');
    parsed.year = (text[6] - '0') * 1000 + (text[7] - '0') * 100 + (text[8] - '0') * 10 + (text[9] - '0');
    if (validate_date(&parsed) != STATUS_OK) {
        return STATUS_INPUT_ERROR;
    }
    *result = parsed;
    return STATUS_OK;
}

Status parse_nonnegative_real(const char *text, double *result) {
    if (text == NULL || result == NULL || text[0] == '\0') {
        return STATUS_INPUT_ERROR;
    }

    size_t i = 0;
    int has_digits = 0;
    int nonzero_mantissa = 0;
    if (text[i] == '+') {
        i++;
    }
    while (text[i] >= '0' && text[i] <= '9') {
        has_digits = 1;
        if (text[i] != '0') {
            nonzero_mantissa = 1;
        }
        i++;
    }
    if (text[i] == '.') {
        i++;
        while (text[i] >= '0' && text[i] <= '9') {
            has_digits = 1;
            if (text[i] != '0') {
                nonzero_mantissa = 1;
            }
            i++;
        }
    }
    if (!has_digits) {
        return STATUS_INPUT_ERROR;
    }
    if (text[i] == 'e' || text[i] == 'E') {
        i++;
        if (text[i] == '+' || text[i] == '-') {
            i++;
        }
        if (text[i] < '0' || text[i] > '9') {
            return STATUS_INPUT_ERROR;
        }
        while (text[i] >= '0' && text[i] <= '9') {
            i++;
        }
    }
    if (text[i] != '\0') {
        return STATUS_INPUT_ERROR;
    }

    char *end = NULL;
    double value = strtod(text, &end);
    if (end == text || *end != '\0' || validate_income(value) != STATUS_OK ||
        (value == 0.0 && nonzero_mantissa)) {
        return STATUS_INPUT_ERROR;
    }
    *result = value;
    return STATUS_OK;
}

Status parse_liver_line(const char *line, Liver *result) {
    if (line == NULL || result == NULL || result->surname != NULL ||
        result->name != NULL || result->patronymic != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t length = strlen(line);
    if (length == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }
    char *copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    memcpy(copy, line, length + 1);

    char *fields[6];
    size_t count = 1;
    fields[0] = copy;
    size_t i;
    for (i = 0; i < length; i++) {
        if (copy[i] == ';') {
            if (count >= 6) {
                free(copy);
                return STATUS_INPUT_ERROR;
            }
            copy[i] = '\0';
            fields[count] = &copy[i + 1];
            count++;
        }
    }
    if (count != 6) {
        free(copy);
        return STATUS_INPUT_ERROR;
    }

    Liver temporary = {0};
    temporary.surname = fields[0];
    temporary.name = fields[1];
    temporary.patronymic = fields[2];
    if (parse_date(fields[3], &temporary.birth) != STATUS_OK ||
        strlen(fields[4]) != 1 || validate_gender(fields[4][0]) != STATUS_OK ||
        parse_nonnegative_real(fields[5], &temporary.income) != STATUS_OK) {
        free(copy);
        return STATUS_INPUT_ERROR;
    }
    temporary.gender = fields[4][0];
    if (validate_liver(&temporary) != STATUS_OK) {
        free(copy);
        return STATUS_INPUT_ERROR;
    }

    Status status = copy_liver(&temporary, result);
    free(copy);
    return status;
}

Status load_livers(const char *path, LiverNode **result) {
    if (path == NULL || path[0] == '\0' || result == NULL || *result != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    LiverNode *loaded = NULL;
    Status status = STATUS_OK;
    size_t line_number = 0;
    while (status == STATUS_OK) {
        char *line = NULL;
        Status read_status = read_line(file, &line);
        if (read_status == STATUS_END_OF_FILE) {
            break;
        }
        if (read_status != STATUS_OK) {
            status = read_status;
            break;
        }
        if (line_number == SIZE_MAX) {
            free(line);
            status = STATUS_OVERFLOW_ERROR;
            break;
        }
        line_number++;

        Liver resident = {0};
        status = parse_liver_line(line, &resident);
        free(line);
        if (status == STATUS_OK) {
            status = insert_sorted(&loaded, &resident);
            free_liver(&resident);
        } else {
            fprintf(stderr, "Некорректная строка файла: %zu\n", line_number);
        }
    }

    if (fclose(file) != 0 && status == STATUS_OK) {
        status = STATUS_FILE_ERROR;
    }
    if (status != STATUS_OK) {
        free_list(&loaded);
        return status;
    }
    *result = loaded;
    return STATUS_OK;
}

Status save_livers(const char *path, const LiverNode *head) {
    if (path == NULL || path[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    Status status = STATUS_OK;
    const LiverNode *current = head;
    while (current != NULL) {
        const Liver *person = &current->data;
        if (validate_liver(person) != STATUS_OK) {
            status = STATUS_INVALID_ARGUMENT;
            break;
        }
        if (fprintf(file, "%s;%s;%s;%02d.%02d.%04d;%c;%.17g\n",
                    person->surname, person->name, person->patronymic,
                    person->birth.day, person->birth.month, person->birth.year,
                    person->gender, person->income) < 0) {
            status = STATUS_FILE_ERROR;
            break;
        }
        current = current->next;
    }
    if (fclose(file) != 0) {
        status = STATUS_FILE_ERROR;
    }
    return status;
}

Status apply_add(LiverNode **head, History *history, const Liver *person) {
    if (head == NULL || history == NULL || person == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (history->count == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }

    LiverNode *node = NULL;
    Status status = create_liver_node(person, &node);
    if (status != STATUS_OK) {
        return status;
    }
    Change *change = (Change *)calloc(1, sizeof(Change));
    if (change == NULL) {
        free_liver(&node->data);
        free(node);
        return STATUS_MEMORY_ERROR;
    }
    status = attach_node_sorted(head, node);
    if (status != STATUS_OK) {
        free(change);
        free_liver(&node->data);
        free(node);
        return status;
    }

    change->type = CHANGE_ADD;
    change->node = node;
    change->next = history->top;
    history->top = change;
    history->count++;
    return STATUS_OK;
}

Status apply_delete(LiverNode **head, History *history, size_t index) {
    if (head == NULL || history == NULL || index == 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (history->count == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }

    LiverNode *node = NULL;
    Status status = get_node_by_index(*head, index, &node);
    if (status != STATUS_OK) {
        return status;
    }
    Change *change = (Change *)calloc(1, sizeof(Change));
    if (change == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    status = detach_node(head, node);
    if (status != STATUS_OK) {
        free(change);
        return status;
    }
    change->type = CHANGE_DELETE;
    change->node = node;
    change->next = history->top;
    history->top = change;
    history->count++;
    return STATUS_OK;
}

Status apply_edit(LiverNode **head, History *history, size_t index, const Liver *replacement_source) {
    if (head == NULL || history == NULL || replacement_source == NULL || index == 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (history->count == SIZE_MAX) {
        return STATUS_OVERFLOW_ERROR;
    }

    LiverNode *node = NULL;
    Status status = get_node_by_index(*head, index, &node);
    if (status != STATUS_OK) {
        return status;
    }
    Liver replacement = {0};
    status = copy_liver(replacement_source, &replacement);
    if (status != STATUS_OK) {
        return status;
    }
    Change *change = (Change *)calloc(1, sizeof(Change));
    if (change == NULL) {
        free_liver(&replacement);
        return STATUS_MEMORY_ERROR;
    }

    status = detach_node(head, node);
    if (status != STATUS_OK) {
        free(change);
        free_liver(&replacement);
        return status;
    }

    Liver old = node->data;
    node->data = replacement;
    status = attach_node_sorted(head, node);
    if (status != STATUS_OK) {
        node->data = old;
        free_liver(&replacement);
        attach_node_sorted(head, node);
        free(change);
        return status;
    }

    change->type = CHANGE_EDIT;
    change->node = node;
    change->previous = old;
    change->next = history->top;
    history->top = change;
    history->count++;
    return STATUS_OK;
}

Status clear_history(History *history) {
    if (history == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    Change *change = history->top;
    while (change != NULL) {
        Change *next = change->next;
        if (change->type == CHANGE_DELETE) {
            free_liver(&change->node->data);
            free(change->node);
        } else if (change->type == CHANGE_EDIT) {
            free_liver(&change->previous);
        }
        free(change);
        change = next;
    }
    history->top = NULL;
    history->count = 0;
    return STATUS_OK;
}

Status undo_last_half(LiverNode **head, History *history, size_t *undone) {
    if (head == NULL || history == NULL || undone == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *undone = 0;
    size_t limit = history->count / 2;
    if (limit == 0) {
        return STATUS_OK;
    }

    size_t i;
    for (i = 0; i < limit; i++) {
        Change *change = history->top;
        if (change == NULL) {
            return STATUS_INVALID_ARGUMENT;
        }
        Status status = STATUS_OK;
        if (change->type == CHANGE_ADD) {
            status = detach_node(head, change->node);
            if (status == STATUS_OK) {
                free_liver(&change->node->data);
                free(change->node);
            }
        } else if (change->type == CHANGE_DELETE) {
            status = attach_node_sorted(head, change->node);
        } else if (change->type == CHANGE_EDIT) {
            status = detach_node(head, change->node);
            if (status == STATUS_OK) {
                Liver newer = change->node->data;
                change->node->data = change->previous;
                status = attach_node_sorted(head, change->node);
                if (status == STATUS_OK) {
                    change->previous = (Liver){0};
                    free_liver(&newer);
                } else {
                    change->node->data = newer;
                    attach_node_sorted(head, change->node);
                }
            }
        } else {
            return STATUS_INVALID_ARGUMENT;
        }
        if (status != STATUS_OK) {
            return status;
        }
        history->top = change->next;
        history->count--;
        free(change);
        (*undone)++;
    }

    return clear_history(history);
}

Status print_liver(const Liver *person, size_t index) {
    if (person == NULL || index == 0 || validate_liver(person) != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }
    printf("%zu) %s %s %s | %02d.%02d.%04d | %c | %.17g\n",
           index, person->surname, person->name, person->patronymic,
           person->birth.day, person->birth.month, person->birth.year,
           person->gender, person->income);
    return STATUS_OK;
}

Status print_livers(const LiverNode *head) {
    size_t index = 1;
    if (head == NULL) {
        puts("Список жителей пуст.");
        return STATUS_OK;
    }
    while (head != NULL) {
        Status status = print_liver(&head->data, index);
        if (status != STATUS_OK) {
            return status;
        }
        index++;
        head = head->next;
    }
    return STATUS_OK;
}

Status parse_size(const char *text, size_t *result) {
    if (text == NULL || result == NULL || text[0] == '\0') {
        return STATUS_INPUT_ERROR;
    }
    size_t value = 0;
    size_t i;
    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return STATUS_INPUT_ERROR;
        }
        size_t digit = (size_t)(text[i] - '0');
        if (value > (SIZE_MAX - digit) / 10) {
            return STATUS_OVERFLOW_ERROR;
        }
        value = value * 10 + digit;
    }
    *result = value;
    return STATUS_OK;
}

Status prompt_line(const char *prompt, char **result) {
    if (prompt == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    puts(prompt);
    return read_line(stdin, result);
}

void print_error(Status status) {
    if (status == STATUS_INVALID_ARGUMENT) {
        puts("Ошибка: некорректный аргумент или данные.");
    } else if (status == STATUS_MEMORY_ERROR) {
        puts("Ошибка: недостаточно памяти.");
    } else if (status == STATUS_FILE_ERROR) {
        puts("Ошибка работы с файлом.");
    } else if (status == STATUS_INPUT_ERROR) {
        puts("Ошибка: неверный формат ввода.");
    } else if (status == STATUS_NOT_FOUND) {
        puts("Житель с указанным номером не найден.");
    } else if (status == STATUS_OVERFLOW_ERROR) {
        puts("Ошибка: переполнение размера или числа.");
    } else if (status == STATUS_END_OF_FILE) {
        puts("Завершён ввод.");
    }
}

Status prompt_index(size_t *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    char *line = NULL;
    Status status = prompt_line("Введите номер жителя:", &line);
    if (status != STATUS_OK) {
        return status;
    }
    status = parse_size(line, result);
    free(line);
    if (status == STATUS_OK && *result == 0) {
        return STATUS_INPUT_ERROR;
    }
    return status;
}

Status prompt_person(Liver *result) {
    if (result == NULL || result->surname != NULL || result->name != NULL ||
        result->patronymic != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    char *line = NULL;
    Status status = prompt_line(
        "Введите жителя: Фамилия;Имя;Отчество;ДД.ММ.ГГГГ;M/W;Доход", &line);
    if (status != STATUS_OK) {
        return status;
    }
    status = parse_liver_line(line, result);
    free(line);
    return status;
}

void free_search_criteria(SearchCriteria *criteria) {
    if (criteria != NULL) {
        free(criteria->surname);
        free(criteria->name);
        free(criteria->patronymic);
        *criteria = (SearchCriteria){0};
    }
}

Status prompt_search_criteria(SearchCriteria *criteria) {
    if (criteria == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *criteria = (SearchCriteria){0};
    Status status;
    char *text = NULL;

    status = prompt_line("Фамилия (Enter = любая):", &text);
    if (status != STATUS_OK) {
        return status;
    }
    if (text[0] != '\0') {
        if (validate_name(text) != STATUS_OK) {
            free(text);
            return STATUS_INPUT_ERROR;
        }
        criteria->surname = text;
    } else {
        free(text);
    }

    text = NULL;
    status = prompt_line("Имя (Enter = любое):", &text);
    if (status != STATUS_OK) {
        free_search_criteria(criteria);
        return status;
    }
    if (text[0] != '\0') {
        if (validate_name(text) != STATUS_OK) {
            free(text);
            free_search_criteria(criteria);
            return STATUS_INPUT_ERROR;
        }
        criteria->name = text;
    } else {
        free(text);
    }

    text = NULL;
    status = prompt_line("Отчество (Enter = любое, - = пустое):", &text);
    if (status != STATUS_OK) {
        free_search_criteria(criteria);
        return status;
    }
    if (text[0] != '\0') {
        if (strcmp(text, "-") == 0) {
            text[0] = '\0';
        }
        if (validate_patronymic(text) != STATUS_OK) {
            free(text);
            free_search_criteria(criteria);
            return STATUS_INPUT_ERROR;
        }
        criteria->patronymic = text;
        criteria->use_patronymic = 1;
    } else {
        free(text);
    }

    text = NULL;
    status = prompt_line("Дата рождения ДД.ММ.ГГГГ (Enter = любая):", &text);
    if (status != STATUS_OK) {
        free_search_criteria(criteria);
        return status;
    }
    if (text[0] != '\0') {
        status = parse_date(text, &criteria->birth);
        if (status == STATUS_OK) {
            criteria->use_birth = 1;
        }
    }
    free(text);
    if (status != STATUS_OK) {
        free_search_criteria(criteria);
        return status;
    }

    text = NULL;
    status = prompt_line("Пол M/W (Enter = любой):", &text);
    if (status != STATUS_OK) {
        free_search_criteria(criteria);
        return status;
    }
    if (text[0] != '\0') {
        if (strlen(text) != 1 || validate_gender(text[0]) != STATUS_OK) {
            free(text);
            free_search_criteria(criteria);
            return STATUS_INPUT_ERROR;
        }
        criteria->gender = text[0];
        criteria->use_gender = 1;
    }
    free(text);

    text = NULL;
    status = prompt_line("Доход (Enter = любой):", &text);
    if (status != STATUS_OK) {
        free_search_criteria(criteria);
        return status;
    }
    if (text[0] != '\0') {
        status = parse_nonnegative_real(text, &criteria->income);
        if (status == STATUS_OK) {
            criteria->use_income = 1;
        }
    }
    free(text);
    if (status != STATUS_OK) {
        free_search_criteria(criteria);
        return status;
    }

    if (criteria->use_income) {
        text = NULL;
        status = prompt_line("Допустимая погрешность дохода epsilon (например, 0.01):", &text);
        if (status != STATUS_OK) {
            free_search_criteria(criteria);
            return status;
        }
        status = parse_nonnegative_real(text, &criteria->epsilon);
        free(text);
        if (status != STATUS_OK) {
            free_search_criteria(criteria);
            return status;
        }
    }
    return STATUS_OK;
}

Status matches_criteria(const Liver *person, const SearchCriteria *criteria, double epsilon, int *result) {
    if (person == NULL || criteria == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (validate_liver(person) != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (criteria->use_income &&
        (validate_income(criteria->income) != STATUS_OK ||
         validate_income(epsilon) != STATUS_OK)) {
        return STATUS_INVALID_ARGUMENT;
    }

    int matches = 1;
    if (criteria->surname != NULL && strcmp(person->surname, criteria->surname) != 0) {
        matches = 0;
    }
    if (criteria->name != NULL && strcmp(person->name, criteria->name) != 0) {
        matches = 0;
    }
    if (criteria->use_patronymic && strcmp(person->patronymic, criteria->patronymic) != 0) {
        matches = 0;
    }
    if (criteria->use_birth) {
        int comparison;
        Status status = compare_dates(&person->birth, &criteria->birth, &comparison);
        if (status != STATUS_OK) {
            return status;
        }
        if (comparison != 0) {
            matches = 0;
        }
    }
    if (criteria->use_gender && person->gender != criteria->gender) {
        matches = 0;
    }
    if (criteria->use_income) {
        double difference = person->income - criteria->income;
        if (difference < -epsilon || difference > epsilon) {
            matches = 0;
        }
    }
    *result = matches;
    return STATUS_OK;
}

Status find_livers(const LiverNode *head, const SearchCriteria *criteria, SearchResult *result) {
    if (criteria == NULL || result == NULL || result->items != NULL ||
        result->count != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    SearchMatch *items = NULL;
    size_t capacity = 0;
    size_t count = 0;
    size_t index = 1;
    while (head != NULL) {
        int matches;
        Status status = matches_criteria(&head->data, criteria, criteria->epsilon, &matches);
        if (status != STATUS_OK) {
            free(items);
            return status;
        }
        if (matches) {
            if (count == capacity) {
                if (capacity > SIZE_MAX / 2) {
                    free(items);
                    return STATUS_OVERFLOW_ERROR;
                }
                size_t new_capacity;

                if (capacity == 0) {
                    new_capacity = 8;
                } else {
                    new_capacity = capacity * 2;
                }
                if (new_capacity > SIZE_MAX / sizeof(SearchMatch)) {
                    free(items);
                    return STATUS_OVERFLOW_ERROR;
                }
                SearchMatch *grown = (SearchMatch *)realloc(items, new_capacity * sizeof(SearchMatch));
                if (grown == NULL) {
                    free(items);
                    return STATUS_MEMORY_ERROR;
                }
                items = grown;
                capacity = new_capacity;
            }
            items[count].index = index;
            items[count].node = head;
            count++;
        }
        if (index == SIZE_MAX && head->next != NULL) {
            free(items);
            return STATUS_OVERFLOW_ERROR;
        }
        index++;
        head = head->next;
    }
    result->items = items;
    result->count = count;
    return STATUS_OK;
}

Status print_search_results(const SearchResult *result) {
    if (result == NULL || (result->count > 0 && result->items == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }
    size_t i;
    for (i = 0; i < result->count; i++) {
        Status status = print_liver(&result->items[i].node->data, result->items[i].index);
        if (status != STATUS_OK) {
            return status;
        }
    }
    printf("Найдено жителей: %zu\n", result->count);
    return STATUS_OK;
}

Status free_search_result(SearchResult *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    free(result->items);
    *result = (SearchResult){0};
    return STATUS_OK;
}

void print_menu(void) {
    puts("1. Показать список жителей");
    puts("2. Найти жителей по параметрам");
    puts("3. Добавить жителя");
    puts("4. Изменить жителя по номеру");
    puts("5. Удалить жителя по номеру");
    puts("6. Отменить последние floor(N/2) изменений");
    puts("7. Сохранить список в файл");
    puts("0. Выход");
}

Status handle_command(size_t command, LiverNode **head, History *history) {
    if (head == NULL || history == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (command == 1) {
        return print_livers(*head);
    }
    if (command == 2) {
        SearchCriteria criteria = {0};
        SearchResult result = {0};
        Status status = prompt_search_criteria(&criteria);
        if (status == STATUS_OK) {
            status = find_livers(*head, &criteria, &result);
        }
        if (status == STATUS_OK) {
            status = print_search_results(&result);
        }
        free_search_result(&result);
        free_search_criteria(&criteria);
        return status;
    }
    if (command == 3) {
        Liver person = {0};
        Status status = prompt_person(&person);
        if (status == STATUS_OK) {
            status = apply_add(head, history, &person);
        }
        free_liver(&person);
        if (status == STATUS_OK) {
            puts("Житель добавлен.");
        }
        return status;
    }
    if (command == 4 || command == 5) {
        Status status = print_livers(*head);
        if (status != STATUS_OK) {
            return status;
        }
        size_t index;
        status = prompt_index(&index);
        if (status != STATUS_OK) {
            return status;
        }
        if (command == 5) {
            status = apply_delete(head, history, index);
            if (status == STATUS_OK) {
                puts("Житель удалён.");
            }
            return status;
        }
        LiverNode *node = NULL;
        status = get_node_by_index(*head, index, &node);
        if (status != STATUS_OK) {
            return status;
        }
        Liver person = {0};
        status = prompt_person(&person);
        if (status == STATUS_OK) {
            status = apply_edit(head, history, index, &person);
        }
        free_liver(&person);
        if (status == STATUS_OK) {
            puts("Житель изменён.");
        }
        return status;
    }
    if (command == 6) {
        size_t modifications = history->count;
        size_t undone;
        Status status = undo_last_half(head, history, &undone);
        if (status == STATUS_OK) {
            printf("Из %zu изменений отменено: %zu.\n", modifications, undone);
            if (undone > 0) {
                puts("История изменений сброшена после Undo.");
            }
        }
        return status;
    }
    if (command == 7) {
        char *path = NULL;
        Status status = prompt_line("Путь к файлу для сохранения:", &path);
        if (status != STATUS_OK) {
            return status;
        }
        status = save_livers(path, *head);
        free(path);
        if (status == STATUS_OK) {
            puts("Список сохранён.");
        }
        return status;
    }
    return STATUS_INPUT_ERROR;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Использование: %s [входной_файл]\n", argv[0]);
        return 1;
    }

    char *entered_path = NULL;
    const char *path = NULL;
    if (argc == 2) {
        path = argv[1];
    } else {
        Status status = prompt_line("Путь к исходному файлу:", &entered_path);
        if (status != STATUS_OK) {
            print_error(status);
            return 1;
        }
        path = entered_path;
    }

    LiverNode *head = NULL;
    History history = {0};
    Status status = load_livers(path, &head);
    free(entered_path);
    if (status != STATUS_OK) {
        print_error(status);
        return 1;
    }
    puts("Данные успешно загружены.");

    int exit_code = 0;
    for (;;) {
        print_menu();
        char *command_line = NULL;
        status = prompt_line("Выберите команду:", &command_line);
        if (status == STATUS_END_OF_FILE) {
            break;
        }
        if (status != STATUS_OK) {
            print_error(status);
            exit_code = 1;
            break;
        }

        size_t command;
        status = parse_size(command_line, &command);
        free(command_line);
        if (status != STATUS_OK || command > 7) {
            print_error(status == STATUS_OK ? STATUS_INPUT_ERROR : status);
            continue;
        }
        if (command == 0) {
            break;
        }
        status = handle_command(command, &head, &history);
        if (status == STATUS_END_OF_FILE) {
            break;
        }
        if (status != STATUS_OK) {
            print_error(status);
            if (status == STATUS_MEMORY_ERROR || status == STATUS_FILE_ERROR || status == STATUS_OVERFLOW_ERROR) {
                exit_code = 1;
            }
        }
    }

    clear_history(&history);
    free_list(&head);
    return exit_code;
}
