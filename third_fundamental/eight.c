#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_MEMORY_ERROR,
    STATUS_FILE_ERROR,
    STATUS_INPUT_ERROR,
    STATUS_OVERFLOW,
    STATUS_DIVISION_BY_ZERO
} Status;

typedef struct Term {
    long long coefficient;
    unsigned int degree;
    struct Term *next;
} Term;

typedef struct {
    Term *head;
} Polynomial;

void free_polynomial(Polynomial *polynomial) {
    if (polynomial == NULL) {
        return;
    }

    Term *current = polynomial->head;
    while (current != NULL) {
        Term *next = current->next;
        free(current);
        current = next;
    }
    polynomial->head = NULL;
}

Status checked_add(long long a, long long b, long long *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b)) {
        return STATUS_OVERFLOW;
    }
    *result = a + b;
    return STATUS_OK;
}

Status checked_sub(long long a, long long b, long long *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if ((b < 0 && a > LLONG_MAX + b) || (b > 0 && a < LLONG_MIN + b)) {
        return STATUS_OVERFLOW;
    }
    *result = a - b;
    return STATUS_OK;
}

Status checked_mul(long long a, long long b, long long *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (a > 0) {
        if ((b > 0 && a > LLONG_MAX / b) || (b < 0 && b < LLONG_MIN / a)) {
            return STATUS_OVERFLOW;
        }
    } else if (a < 0) {
        if ((b > 0 && a < LLONG_MIN / b) || (b < 0 && a < LLONG_MAX / b)) {
            return STATUS_OVERFLOW;
        }
    }
    *result = a * b;
    return STATUS_OK;
}

Status insert_term(Polynomial *polynomial, long long coefficient, unsigned int degree, int subtract) {
    if (polynomial == NULL || (subtract != 0 && subtract != 1)) {
        return STATUS_INVALID_ARGUMENT;
    }
    Term **place = &polynomial->head;
    while (*place != NULL && (*place)->degree > degree) {
        place = &(*place)->next;
    }

    long long updated = coefficient;
    Status status = STATUS_OK;
    if (*place != NULL && (*place)->degree == degree) {
        if (subtract) {
            status = checked_sub((*place)->coefficient, coefficient, &updated);
        } else {
            status = checked_add((*place)->coefficient, coefficient, &updated);
        }
        if (status != STATUS_OK) {
            return status;
        }
        if (updated == 0) {
            Term *removed = *place;
            *place = removed->next;
            free(removed);
        } else {
            (*place)->coefficient = updated;
        }
        return STATUS_OK;
    }

    if (subtract) {
        status = checked_sub(0, coefficient, &updated);
        if (status != STATUS_OK) {
            return status;
        }
    }
    if (updated == 0) {
        return STATUS_OK;
    }
    Term *node = (Term *)malloc(sizeof(Term));
    if (node == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    node->coefficient = updated;
    node->degree = degree;
    node->next = *place;
    *place = node;
    return STATUS_OK;
}

Status copy_polynomial(const Polynomial *source, Polynomial *result) {
    if (source == NULL || result == NULL || source == result) {
        return STATUS_INVALID_ARGUMENT;
    }
    result->head = NULL;
    Term **tail = &result->head;
    for (const Term *term = source->head; term != NULL; term = term->next) {
        Term *node = (Term *)malloc(sizeof(Term));
        if (node == NULL) {
            free_polynomial(result);
            return STATUS_MEMORY_ERROR;
        }
        node->coefficient = term->coefficient;
        node->degree = term->degree;
        node->next = NULL;
        *tail = node;
        tail = &node->next;
    }
    return STATUS_OK;
}

Status polynomial_sum(const Polynomial *first, const Polynomial *second, int subtract, Polynomial *result) {
    if (first == NULL || second == NULL || result == NULL || result == first || result == second || (subtract != 0 && subtract != 1)) {
        return STATUS_INVALID_ARGUMENT;
    }
    result->head = NULL;
    const Term *a = first->head;
    const Term *b = second->head;
    Term **tail = &result->head;
    while (a != NULL || b != NULL) {
        long long coefficient = 0;
        unsigned int degree = 0;
        Status status = STATUS_OK;
        if (b == NULL || (a != NULL && a->degree > b->degree)) {
            coefficient = a->coefficient;
            degree = a->degree;
            a = a->next;
        } else if (a == NULL || b->degree > a->degree) {
            degree = b->degree;
            if (subtract) {
                status = checked_sub(0, b->coefficient, &coefficient);
            } else {
                coefficient = b->coefficient;
            }
            b = b->next;
        } else {
            degree = a->degree;
            if (subtract) {
                status = checked_sub(a->coefficient, b->coefficient, &coefficient);
            } else {
                status = checked_add(a->coefficient, b->coefficient, &coefficient);
            }
            a = a->next;
            b = b->next;
        }
        if (status != STATUS_OK) {
            free_polynomial(result);
            return status;
        }
        if (coefficient != 0) {
            Term *node = (Term *)malloc(sizeof(Term));
            if (node == NULL) {
                free_polynomial(result);
                return STATUS_MEMORY_ERROR;
            }
            node->coefficient = coefficient;
            node->degree = degree;
            node->next = NULL;
            *tail = node;
            tail = &node->next;
        }
    }
    return STATUS_OK;
}

Status polynomial_mult(const Polynomial *first, const Polynomial *second, Polynomial *result) {
    if (first == NULL || second == NULL || result == NULL || result == first || result == second) {
        return STATUS_INVALID_ARGUMENT;
    }
    result->head = NULL;
    for (const Term *a = first->head; a != NULL; a = a->next) {
        for (const Term *b = second->head; b != NULL; b = b->next) {
            if (a->degree > UINT_MAX - b->degree) {
                free_polynomial(result);
                return STATUS_OVERFLOW;
            }
            long long coefficient = 0;
            Status status = checked_mul(a->coefficient, b->coefficient, &coefficient);
            if (status == STATUS_OK) {
                status = insert_term(result, coefficient, a->degree + b->degree, 0);
            }
            if (status != STATUS_OK) {
                free_polynomial(result);
                return status;
            }
        }
    }
    return STATUS_OK;
}

Status polynomial_divmod(const Polynomial *first, const Polynomial *second, Polynomial *quotient, Polynomial *remainder) {
    if (first == NULL || second == NULL || quotient == NULL || remainder == NULL || quotient == remainder || quotient == first || quotient == second || remainder == first || remainder == second) {
        return STATUS_INVALID_ARGUMENT;
    }
    quotient->head = NULL;
    remainder->head = NULL;
    if (second->head == NULL) {
        return STATUS_DIVISION_BY_ZERO;
    }

    Polynomial working = {NULL};
    Status status = copy_polynomial(first, &working);
    if (status != STATUS_OK) {
        return status;
    }

    while (working.head != NULL && status == STATUS_OK) {
        Term *leading = working.head;
        long long q = 0;
        unsigned int difference = 0;
        if (leading->degree >= second->head->degree) {
            if (leading->coefficient == LLONG_MIN &&
                second->head->coefficient == -1) {
                status = STATUS_OVERFLOW;
                break;
            }
            q = leading->coefficient / second->head->coefficient;
            difference = leading->degree - second->head->degree;
        }

        if (q != 0) {
            status = insert_term(quotient, q, difference, 0);
            for (const Term *term = second->head;
                 term != NULL && status == STATUS_OK; term = term->next) {
                long long product = 0;
                status = checked_mul(q, term->coefficient, &product);
                if (status == STATUS_OK) {
                    status = insert_term(&working, product, term->degree + difference, 1);
                }
            }
        } else {
            working.head = leading->next;
            status = insert_term(remainder, leading->coefficient, leading->degree, 0);
            free(leading);
        }
    }
    free_polynomial(&working);
    if (status != STATUS_OK) {
        free_polynomial(quotient);
        free_polynomial(remainder);
    }
    return status;
}

Status checked_power(long long base, unsigned int exponent, long long *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    long long value = 1;
    while (exponent != 0) {
        if (exponent % 2 != 0) {
            Status status = checked_mul(value, base, &value);
            if (status != STATUS_OK) {
                return status;
            }
        }
        exponent /= 2;
        if (exponent != 0) {
            Status status = checked_mul(base, base, &base);
            if (status != STATUS_OK) {
                return status;
            }
        }
    }
    *result = value;
    return STATUS_OK;
}

Status polynomial_eval(const Polynomial *polynomial, long long x, long long *result) {
    if (polynomial == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    long long sum = 0;
    for (const Term *term = polynomial->head; term != NULL; term = term->next) {
        long long power = 0;
        long long product = 0;
        Status status = checked_power(x, term->degree, &power);
        if (status == STATUS_OK) {
            status = checked_mul(term->coefficient, power, &product);
        }
        if (status == STATUS_OK) {
            status = checked_add(sum, product, &sum);
        }
        if (status != STATUS_OK) {
            return status;
        }
    }
    *result = sum;
    return STATUS_OK;
}

Status polynomial_diff(const Polynomial *polynomial, Polynomial *result) {
    if (polynomial == NULL || result == NULL || result == polynomial) {
        return STATUS_INVALID_ARGUMENT;
    }
    result->head = NULL;
    for (const Term *term = polynomial->head; term != NULL; term = term->next) {
        if (term->degree != 0) {
            long long coefficient = 0;
            Status status = checked_mul(term->coefficient, (long long)term->degree, &coefficient);
            if (status == STATUS_OK) {
                status = insert_term(result, coefficient, term->degree - 1, 0);
            }
            if (status != STATUS_OK) {
                free_polynomial(result);
                return status;
            }
        }
    }
    return STATUS_OK;
}

Status polynomial_power(const Polynomial *base, unsigned int exponent, Polynomial *result) {
    if (base == NULL || result == NULL || base == result) {
        return STATUS_INVALID_ARGUMENT;
    }
    result->head = NULL;
    Status status = insert_term(result, 1, 0, 0);
    Polynomial factor = {NULL};
    if (status == STATUS_OK) {
        status = copy_polynomial(base, &factor);
    }
    while (exponent != 0 && status == STATUS_OK) {
        if (exponent % 2 != 0) {
            Polynomial next = {NULL};
            status = polynomial_mult(result, &factor, &next);
            if (status == STATUS_OK) {
                free_polynomial(result);
                *result = next;
            }
        }
        exponent /= 2;
        if (exponent != 0 && status == STATUS_OK) {
            Polynomial next = {NULL};
            status = polynomial_mult(&factor, &factor, &next);
            if (status == STATUS_OK) {
                free_polynomial(&factor);
                factor = next;
            }
        }
    }
    free_polynomial(&factor);
    if (status != STATUS_OK) {
        free_polynomial(result);
    }
    return status;
}

Status polynomial_compose(const Polynomial *outer, const Polynomial *inner, Polynomial *result) {
    if (outer == NULL || inner == NULL || result == outer ||
        result == inner) {
        return STATUS_INVALID_ARGUMENT;
    }
    result->head = NULL;
    for (const Term *term = outer->head; term != NULL; term = term->next) {
        Polynomial power = {NULL};
        Status status = polynomial_power(inner, term->degree, &power);
        for (Term *item = power.head; item != NULL && status == STATUS_OK;
             item = item->next) {
            status = checked_mul(item->coefficient, term->coefficient, &item->coefficient);
        }
        if (status == STATUS_OK) {
            Polynomial next = {NULL};
            status = polynomial_sum(result, &power, 0, &next);
            if (status == STATUS_OK) {
                free_polynomial(result);
                *result = next;
            }
        }
        free_polynomial(&power);
        if (status != STATUS_OK) {
            free_polynomial(result);
            return status;
        }
    }
    return STATUS_OK;
}

void skip_spaces(const char **cursor, const char *end) {
    if (cursor == NULL || *cursor == NULL || end == NULL) {
        return;
    }
    while (*cursor < end && isspace((unsigned char)**cursor)) {
        (*cursor)++;
    }
}

Status parse_digits(const char **cursor, const char *end, unsigned long long limit, unsigned long long *value, int *found) {
    if (cursor == NULL || *cursor == NULL || end == NULL ||
        value == NULL || found == NULL || *cursor > end) {
        return STATUS_INVALID_ARGUMENT;
    }
    *value = 0;
    *found = 0;
    while (*cursor < end && **cursor >= '0' && **cursor <= '9') {
        unsigned int digit = (unsigned int)(**cursor - '0');
        if (*value > (limit - digit) / 10) {
            return STATUS_OVERFLOW;
        }
        *value = *value * 10 + digit;
        *found = 1;
        (*cursor)++;
    }
    return STATUS_OK;
}

Status parse_integer(const char *text, size_t length, long long *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    const char *cursor = text;
    const char *end = text + length;
    skip_spaces(&cursor, end);
    int negative = 0;
    if (cursor < end && (*cursor == '-' || *cursor == '+')) {
        negative = (*cursor == '-');
        cursor++;
    }
    skip_spaces(&cursor, end);
    unsigned long long limit = (unsigned long long)LLONG_MAX;
    if (negative) {
        limit++;
    }
    unsigned long long magnitude = 0;
    int found = 0;
    Status status = parse_digits(&cursor, end, limit, &magnitude, &found);
    if (status != STATUS_OK || !found) {
        if (status == STATUS_OK) {
            return STATUS_INPUT_ERROR;
        }
        return status;
    }
    skip_spaces(&cursor, end);
    if (cursor != end) {
        return STATUS_INPUT_ERROR;
    }
    if (negative && magnitude == (unsigned long long)LLONG_MAX + 1ULL) {
        *result = LLONG_MIN;
    } else {
        *result = (long long)magnitude;
        if (negative) {
            *result = -*result;
        }
    }
    return STATUS_OK;
}

Status parse_polynomial(const char *text, size_t length, Polynomial *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    result->head = NULL;
    const char *cursor = text;
    const char *end = text + length;
    int has_term = 0;

    while (1) {
        skip_spaces(&cursor, end);
        if (cursor == end) {
            break;
        }
        int negative = 0;
        if (*cursor == '-' || *cursor == '+') {
            negative = (*cursor == '-');
            cursor++;
        } else if (has_term) {
            free_polynomial(result);
            return STATUS_INPUT_ERROR;
        }
        skip_spaces(&cursor, end);
        unsigned long long limit = (unsigned long long)LLONG_MAX;
        if (negative) {
            limit++;
        }
        unsigned long long magnitude = 0;
        int digits = 0;
        Status status = parse_digits(&cursor, end, limit, &magnitude, &digits);
        if (status != STATUS_OK) {
            free_polynomial(result);
            return status;
        }
        skip_spaces(&cursor, end);
        unsigned int degree = 0;
        if (cursor < end && *cursor == 'x') {
            cursor++;
            degree = 1;
            if (!digits) {
                magnitude = 1;
            }
            skip_spaces(&cursor, end);
            if (cursor < end && *cursor == '^') {
                cursor++;
                skip_spaces(&cursor, end);
                unsigned long long exponent = 0;
                int exponent_digits = 0;
                status = parse_digits(&cursor, end, UINT_MAX, &exponent, &exponent_digits);
                if (status != STATUS_OK || !exponent_digits) {
                    free_polynomial(result);
                    if (status == STATUS_OK) {
                        return STATUS_INPUT_ERROR;
                    }
                    return status;
                }
                degree = (unsigned int)exponent;
            }
        } else if (!digits) {
            free_polynomial(result);
            return STATUS_INPUT_ERROR;
        }

        long long coefficient = 0;
        if (negative && magnitude == (unsigned long long)LLONG_MAX + 1ULL) {
            coefficient = LLONG_MIN;
        } else {
            coefficient = (long long)magnitude;
            if (negative) {
                coefficient = -coefficient;
            }
        }
        status = insert_term(result, coefficient, degree, 0);
        if (status != STATUS_OK) {
            free_polynomial(result);
            return status;
        }
        has_term = 1;
        skip_spaces(&cursor, end);
        if (cursor < end && *cursor != '+' && *cursor != '-') {
            free_polynomial(result);
            return STATUS_INPUT_ERROR;
        }
    }
    if (!has_term) {
        free_polynomial(result);
        return STATUS_INPUT_ERROR;
    }
    return STATUS_OK;
}

void print_polynomial(const Polynomial *polynomial) {
    if (polynomial == NULL || polynomial->head == NULL) {
        printf("0");
        return;
    }
    int first = 1;
    for (const Term *term = polynomial->head; term != NULL; term = term->next) {
        long long coefficient = term->coefficient;
        unsigned long long magnitude = (unsigned long long)coefficient;
        if (coefficient < 0) {
            magnitude = (unsigned long long)(-(coefficient + 1)) + 1ULL;
        }
        if (first) {
            if (coefficient < 0) {
                printf("-");
            }
        } else {
            if (coefficient < 0) {
                printf(" - ");
            } else {
                printf(" + ");
            }
        }
        if (term->degree == 0 || magnitude != 1) {
            printf("%llu", magnitude);
        }
        if (term->degree != 0) {
            printf("x");
            if (term->degree != 1) {
                printf("^%u", term->degree);
            }
        }
        first = 0;
    }
}

Status execute_command(const char *name, const char *arguments[2], const size_t lengths[2], int count, Polynomial *accumulator, long long *number, int *is_number) {
    if (name == NULL || arguments == NULL || lengths == NULL ||
        accumulator == NULL || number == NULL || is_number == NULL ||
        count < 0 || count > 2) {
        return STATUS_INVALID_ARGUMENT;
    }
    *is_number = 0;
    int is_eval = strcmp(name, "Eval") == 0;
    int is_diff = strcmp(name, "Diff") == 0;
    int is_add = strcmp(name, "Add") == 0;
    int is_sub = strcmp(name, "Sub") == 0;
    int is_mult = strcmp(name, "Mult") == 0;
    int is_div = strcmp(name, "Div") == 0;
    int is_mod = strcmp(name, "Mod") == 0;
    int is_cmps = strcmp(name, "Cmps") == 0;
    if (!(is_eval || is_diff || is_add || is_sub || is_mult || is_div || is_mod || is_cmps)) {
        return STATUS_INPUT_ERROR;
    }
    if ((is_diff && count > 1) || (!is_diff && count < 1)) {
        return STATUS_INPUT_ERROR;
    }

    Polynomial left = {NULL};
    Polynomial right = {NULL};
    Polynomial result = {NULL};
    Polynomial quotient = {NULL};
    Polynomial remainder = {NULL};
    const Polynomial *first = accumulator;
    Status status = STATUS_OK;

    if (is_eval) {
        if (count == 2) {
            status = parse_polynomial(arguments[0], lengths[0], &left);
            first = &left;
        }
        if (status == STATUS_OK) {
            status = parse_integer(arguments[count - 1], lengths[count - 1], number);
        }
        if (status == STATUS_OK) {
            status = polynomial_eval(first, *number, number);
        }
        if (status == STATUS_OK) {
            *is_number = 1;
        }
    } else if (is_diff) {
        if (count == 1) {
            status = parse_polynomial(arguments[0], lengths[0], &left);
            first = &left;
        }
        if (status == STATUS_OK) {
            status = polynomial_diff(first, &result);
        }
    } else {
        if (count == 2) {
            status = parse_polynomial(arguments[0], lengths[0], &left);
            first = &left;
        }
        if (status == STATUS_OK) {
            status = parse_polynomial(arguments[count - 1], lengths[count - 1], &right);
        }
        if (status == STATUS_OK) {
            if (is_add || is_sub) {
                status = polynomial_sum(first, &right, is_sub, &result);
            } else if (is_mult) {
                status = polynomial_mult(first, &right, &result);
            } else if (is_div || is_mod) {
                status = polynomial_divmod(first, &right, &quotient, &remainder);
                if (status == STATUS_OK) {
                    if (is_div) {
                        result = quotient;
                        quotient.head = NULL;
                    } else {
                        result = remainder;
                        remainder.head = NULL;
                    }
                }
            } else if (is_cmps) {
                status = polynomial_compose(first, &right, &result);
            }
        }
    }

    if (status == STATUS_OK && !is_eval) {
        free_polynomial(accumulator);
        *accumulator = result;
        result.head = NULL;
    }
    free_polynomial(&left);
    free_polynomial(&right);
    free_polynomial(&result);
    free_polynomial(&quotient);
    free_polynomial(&remainder);
    return status;
}

Status read_file(const char *path, char **content) {
    if (path == NULL || content == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *content = NULL;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }
    size_t capacity = 256;
    size_t length = 0;
    char *buffer = (char *)malloc(capacity);
    Status status = STATUS_OK;
    if (buffer == NULL) {
        status = STATUS_MEMORY_ERROR;
    }
    if (status == STATUS_OK) {
        int current = 0;
        while ((current = fgetc(file)) != EOF) {
            if (current == 0) {
                status = STATUS_INPUT_ERROR;
                break;
            }
            if (length >= capacity - 1) {
                if (capacity > SIZE_MAX / 2) {
                    status = STATUS_OVERFLOW;
                    break;
                }
                size_t new_capacity = capacity * 2;
                char *new_buffer = (char *)realloc(buffer, new_capacity);
                if (new_buffer == NULL) {
                    status = STATUS_MEMORY_ERROR;
                    break;
                }
                buffer = new_buffer;
                capacity = new_capacity;
            }
            buffer[length++] = (char)current;
        }
        if (ferror(file)) {
            status = STATUS_FILE_ERROR;
        }
    }
    if (fclose(file) != 0) {
        status = STATUS_FILE_ERROR;
    }
    if (status != STATUS_OK) {
        free(buffer);
        return status;
    }
    buffer[length] = '\0';
    *content = buffer;
    return STATUS_OK;
}

Status remove_comments(char *content) {
    if (content == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    int mode = 0;
    for (size_t i = 0; content[i] != '\0'; i++) {
        char character = content[i];
        if (mode == 1) {
            if (character == '\n') {
                mode = 0;
            } else {
                content[i] = ' ';
            }
        } else if (mode == 2) {
            if (character == '[') {
                return STATUS_INPUT_ERROR;
            }
            if (character == ']') {
                mode = 0;
            }
            if (character != '\n') {
                content[i] = ' ';
            }
        } else if (character == '%') {
            content[i] = ' ';
            mode = 1;
        } else if (character == '[') {
            content[i] = ' ';
            mode = 2;
        } else if (character == ']') {
            return STATUS_INPUT_ERROR;
        }
    }
    if (mode == 2) {
        return STATUS_INPUT_ERROR;
    }
    return STATUS_OK;
}

Status process_commands(const char *content, Polynomial *accumulator) {
    if (content == NULL || accumulator == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    const char *cursor = content;
    unsigned long long command_number = 0;
    while (*cursor != '\0') {
        while (*cursor != '\0' && isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }
        command_number++;
        char name[8];
        size_t name_length = 0;
        while (isalpha((unsigned char)*cursor)) {
            if (name_length >= sizeof(name) - 1) {
                fprintf(stderr, "Неверная команда %llu\n", command_number);
                return STATUS_INPUT_ERROR;
            }
            name[name_length++] = *cursor;
            cursor++;
        }
        name[name_length] = '\0';
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (name_length == 0 || *cursor != '(') {
            fprintf(stderr, "Ошибка синтаксиса команды %llu\n", command_number);
            return STATUS_INPUT_ERROR;
        }
        cursor++;
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }
        const char *arguments[2] = {NULL, NULL};
        size_t lengths[2] = {0, 0};
        int count = 0;
        if (*cursor != ')') {
            while (1) {
                if (count >= 2) {
                    fprintf(stderr, "Слишком много аргументов в команде %llu\n", command_number);
                    return STATUS_INPUT_ERROR;
                }
                const char *begin = cursor;
                while (*cursor != '\0' && *cursor != ',' && *cursor != ')') {
                    cursor++;
                }
                const char *end = cursor;
                while (begin < end && isspace((unsigned char)*begin)) {
                    begin++;
                }
                while (end > begin && isspace((unsigned char)end[-1])) {
                    end--;
                }
                if (begin == end) {
                    fprintf(stderr, "Пустой аргумент команды %llu\n", command_number);
                    return STATUS_INPUT_ERROR;
                }
                arguments[count] = begin;
                lengths[count] = (size_t)(end - begin);
                count++;
                if (*cursor != ',') {
                    break;
                }
                cursor++;
            }
        }
        if (*cursor != ')') {
            fprintf(stderr, "Нет закрывающей скобки в команде %llu\n", command_number);
            return STATUS_INPUT_ERROR;
        }
        cursor++;
        while (isspace((unsigned char)*cursor)) {
            cursor++;
        }
        if (*cursor != ';') {
            fprintf(stderr, "Нет ';' после команды %llu\n", command_number);
            return STATUS_INPUT_ERROR;
        }
        cursor++;

        long long number = 0;
        int is_number = 0;
        Status status = execute_command(name, arguments, lengths, count, accumulator, &number, &is_number);
        if (status != STATUS_OK) {
            fprintf(stderr, "Ошибка выполнения команды %llu (%s), код %d\n", command_number, name, (int)status);
            return status;
        }
        printf("%s: ", name);
        if (is_number) {
            printf("%lld", number);
        } else {
            print_polynomial(accumulator);
        }
        printf("\n");
        if (ferror(stdout)) {
            return STATUS_FILE_ERROR;
        }
    }
    return STATUS_OK;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл_команд>\n", argv[0]);
        return 1;
    }
    char *content = NULL;
    Status status = read_file(argv[1], &content);
    if (status != STATUS_OK) {
        fprintf(stderr, "Не удалось прочитать файл, код %d\n", (int)status);
        return 1;
    }
    status = remove_comments(content);
    Polynomial accumulator = {NULL};
    if (status == STATUS_OK) {
        status = process_commands(content, &accumulator);
    } else {
        fprintf(stderr, "Некорректный комментарий в файле\n");
    }
    free_polynomial(&accumulator);
    free(content);
    if (status != STATUS_OK) {
        return 1;
    }
    return 0;
}
