#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
#include <ctype.h>
#include <math.h>


#define SCAN_OK 0
#define SCAN_INVALID_ARGUMENT 1
#define SCAN_INPUT_ERROR 2
#define SCAN_MATCH_FAILURE 3
#define SCAN_END_OF_INPUT 4
#define SCAN_MEMORY_ERROR 5
#define SCAN_OVERFLOW 6

#define INPUT_FILE 1
#define INPUT_STRING 2

#define LENGTH_DEFAULT 0
#define LENGTH_H 1
#define LENGTH_HH 2
#define LENGTH_L 3
#define LENGTH_LL 4
#define LENGTH_J 5
#define LENGTH_CAP_L 6

typedef struct {
    int type;
    FILE *stream;
    const char *source;
    size_t position;
    size_t characters_read;
} Input;

static int input_init_file(Input *input, FILE *stream) {
    if (input == NULL || stream == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    input->type = INPUT_FILE;
    input->stream = stream;
    input->source = NULL;
    input->position = 0;
    input->characters_read = 0;
    return SCAN_OK;
}

static int input_init_string(Input *input, const char *source) {
    if (input == NULL || source == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    input->type = INPUT_STRING;
    input->stream = NULL;
    input->source = source;
    input->position = 0;
    input->characters_read = 0;
    return SCAN_OK;
}

static int input_get(Input *input, int *character) {
    if (input == NULL || character == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }
    if (input->characters_read == SIZE_MAX) {
        return SCAN_OVERFLOW;
    }

    if (input->type == INPUT_FILE) {
        int value;

        if (input->stream == NULL) {
            return SCAN_INVALID_ARGUMENT;
        }

        value = fgetc(input->stream);
        if (value == EOF) {
            return SCAN_END_OF_INPUT;
        }

        *character = value;
        input->characters_read++;
        return SCAN_OK;
    }

    if (input->type == INPUT_STRING) {
        if (input->source == NULL) {
            return SCAN_INVALID_ARGUMENT;
        }
        if (input->source[input->position] == '\0') {
            return SCAN_END_OF_INPUT;
        }

        *character = (unsigned char)input->source[input->position];
        input->position++;
        input->characters_read++;
        return SCAN_OK;
    }

    return SCAN_INVALID_ARGUMENT;
}

static int input_unget(Input *input, int character) {
    if (input == NULL || character == EOF || input->characters_read == 0) {
        return SCAN_INVALID_ARGUMENT;
    }

    if (input->type == INPUT_FILE) {
        if (input->stream == NULL || ungetc(character, input->stream) == EOF) {
            return SCAN_INPUT_ERROR;
        }
        input->characters_read--;
        return SCAN_OK;
    }

    if (input->type == INPUT_STRING) {
        if (input->source == NULL || input->position == 0 ||
            character != (unsigned char)input->source[input->position - 1]) {
            return SCAN_INVALID_ARGUMENT;
        }
        input->position--;
        input->characters_read--;
        return SCAN_OK;
    }

    return SCAN_INVALID_ARGUMENT;
}

static int input_get_limited(Input *input, size_t width, size_t *used, int *character) {
    if (input == NULL || used == NULL || character == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }
    if (width != 0 && *used >= width) {
        return SCAN_END_OF_INPUT;
    }

    int status = input_get(input, character);
    if (status == SCAN_OK) {
        (*used)++;
    }
    return status;
}

static int input_skip_spaces(Input *input) {
    int character;
    int status;

    if (input == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    status = input_get(input, &character);
    while (status == SCAN_OK && isspace((unsigned char)character)) {
        status = input_get(input, &character);
    }

    if (status == SCAN_OK) {
        return input_unget(input, character);
    }
    return status;
}

static int digit_value(int character, int letter_mode, int *value) {
    if (value == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    if (character >= '0' && character <= '9') {
        *value = character - '0';
        return SCAN_OK;
    }
    if (character >= 'a' && character <= 'z' && letter_mode != 2) {
        *value = character - 'a' + 10;
        return SCAN_OK;
    }
    if (character >= 'A' && character <= 'Z' && letter_mode != 1) {
        *value = character - 'A' + 10;
        return SCAN_OK;
    }
    return SCAN_MATCH_FAILURE;
}

static int input_read_integer(Input *input, int base, int letter_mode,
                              size_t width, uintmax_t *magnitude, int *negative) {
    size_t used = 0;
    int character;
    int status;
    int actual_base = base;
    int have_character = 1;
    int digits = 0;
    uintmax_t value = 0;

    if (input == NULL || magnitude == NULL || negative == NULL ||
        (base != 0 && (base < 2 || base > 36))) {
        return SCAN_INVALID_ARGUMENT;
    }

    status = input_skip_spaces(input);
    if (status != SCAN_OK) {
        return status;
    }

    status = input_get_limited(input, width, &used, &character);
    if (status != SCAN_OK) {
        return status;
    }

    *negative = 0;
    if (character == '+' || character == '-') {
        *negative = (character == '-');
        status = input_get_limited(input, width, &used, &character);
        if (status != SCAN_OK) {
            return SCAN_MATCH_FAILURE;
        }
    }

    if (letter_mode == 0 && (base == 0 || base == 16) && character == '0') {
        int next_character;
        int next_status = input_get_limited(input, width, &used, &next_character);

        if (next_status == SCAN_OK &&
            (next_character == 'x' || next_character == 'X')) {
            int first_hex_digit;
            int digit;
            status = input_get_limited(input, width, &used, &first_hex_digit);
            if (status != SCAN_OK ||
                digit_value(first_hex_digit, 0, &digit) != SCAN_OK || digit >= 16) {
                return SCAN_MATCH_FAILURE;
            }
            actual_base = 16;
            value = (uintmax_t)digit;
            digits = 1;
            status = input_get_limited(input, width, &used, &character);
            if (status != SCAN_OK) {
                have_character = 0;
            }
        } else {
            if (base == 0) {
                actual_base = 8;
            } else {
                actual_base = 16;
            }
            value = 0;
            digits = 1;
            if (next_status == SCAN_OK) {
                character = next_character;
            } else {
                have_character = 0;
            }
        }
    } else if (base == 0) {
        actual_base = 10;
    }

    while (have_character) {
        int digit;
        status = digit_value(character, letter_mode, &digit);
        if (status != SCAN_OK || digit >= actual_base) {
            if (digits == 0) {
                input_unget(input, character);
                return SCAN_MATCH_FAILURE;
            }
            input_unget(input, character);
            break;
        }

        if (value > (UINTMAX_MAX - (uintmax_t)digit) / (uintmax_t)actual_base) {
            return SCAN_OVERFLOW;
        }
        value = value * (uintmax_t)actual_base + (uintmax_t)digit;
        digits++;

        status = input_get_limited(input, width, &used, &character);
        if (status != SCAN_OK) {
            have_character = 0;
        }
    }

    if (digits == 0) {
        return SCAN_MATCH_FAILURE;
    }

    *magnitude = value;
    return SCAN_OK;
}

static int signed_limits(int length, uintmax_t *positive_limit, uintmax_t *negative_limit) {
    uintmax_t limit;

    if (positive_limit == NULL || negative_limit == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    if (length == LENGTH_HH) {
        limit = (uintmax_t)SCHAR_MAX;
    } else if (length == LENGTH_H) {
        limit = (uintmax_t)SHRT_MAX;
    } else if (length == LENGTH_L) {
        limit = (uintmax_t)LONG_MAX;
    } else if (length == LENGTH_LL) {
        limit = (uintmax_t)LLONG_MAX;
    } else if (length == LENGTH_J) {
        limit = (uintmax_t)INTMAX_MAX;
    } else {
        limit = (uintmax_t)INT_MAX;
    }

    *positive_limit = limit;
    *negative_limit = limit + 1;
    return SCAN_OK;
}

static int store_signed_integer(void *destination, int length, uintmax_t magnitude, int negative) {
    uintmax_t positive_limit;
    uintmax_t negative_limit;
    intmax_t value;

    if (destination == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }
    signed_limits(length, &positive_limit, &negative_limit);
    if ((!negative && magnitude > positive_limit) ||
        (negative && magnitude > negative_limit)) {
        return SCAN_OVERFLOW;
    }

    if (negative) {
        if (magnitude == negative_limit) {
            value = -(intmax_t)positive_limit - 1;
        } else {
            value = -(intmax_t)magnitude;
        }
    } else {
        value = (intmax_t)magnitude;
    }

    if (length == LENGTH_HH) {
        *(signed char *)destination = (signed char)value;
    } else if (length == LENGTH_H) {
        *(short *)destination = (short)value;
    } else if (length == LENGTH_L) {
        *(long *)destination = (long)value;
    } else if (length == LENGTH_LL) {
        *(long long *)destination = (long long)value;
    } else if (length == LENGTH_J) {
        *(intmax_t *)destination = value;
    } else {
        *(int *)destination = (int)value;
    }
    return SCAN_OK;
}

static int store_unsigned_integer(void *destination, int length, uintmax_t magnitude, int negative) {
    uintmax_t value = magnitude;

    if (negative) {
        value = (uintmax_t)0 - magnitude;
    }
    if (destination == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    if (length == LENGTH_HH) {
        *(unsigned char *)destination = (unsigned char)value;
    } else if (length == LENGTH_H) {
        *(unsigned short *)destination = (unsigned short)value;
    } else if (length == LENGTH_L) {
        *(unsigned long *)destination = (unsigned long)value;
    } else if (length == LENGTH_LL) {
        *(unsigned long long *)destination = (unsigned long long)value;
    } else if (length == LENGTH_J) {
        *(uintmax_t *)destination = value;
    } else {
        *(unsigned int *)destination = (unsigned int)value;
    }
    return SCAN_OK;
}

static int roman_value(int character, int *value) {
    if (value == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    if (character == 'I') {
        *value = 1;
    } else if (character == 'V') {
        *value = 5;
    } else if (character == 'X') {
        *value = 10;
    } else if (character == 'L') {
        *value = 50;
    } else if (character == 'C') {
        *value = 100;
    } else if (character == 'D') {
        *value = 500;
    } else if (character == 'M') {
        *value = 1000;
    } else {
        return SCAN_MATCH_FAILURE;
    }
    return SCAN_OK;
}

static int input_read_roman(Input *input, int *result) {
    int character;
    int status;
    int negative = 0;
    int previous_value;
    int current_value;
    intmax_t total;

    if (input == NULL || result == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    status = input_skip_spaces(input);
    if (status != SCAN_OK) {
        return status;
    }
    status = input_get(input, &character);
    if (status != SCAN_OK) {
        return status;
    }

    if (character == '-') {
        negative = 1;
        status = input_get(input, &character);
        if (status != SCAN_OK) {
            return SCAN_MATCH_FAILURE;
        }
    }

    if (character == 'N') {
        if (negative) {
            return SCAN_MATCH_FAILURE;
        }
        *result = 0;
        return SCAN_OK;
    }

    status = roman_value(character, &previous_value);
    if (status != SCAN_OK) {
        input_unget(input, character);
        return SCAN_MATCH_FAILURE;
    }

    total = previous_value;
    while ((status = input_get(input, &character)) == SCAN_OK) {
        if (roman_value(character, &current_value) != SCAN_OK) {
            input_unget(input, character);
            break;
        }

        if (current_value > previous_value) {
            total += (intmax_t)current_value - 2 * (intmax_t)previous_value;
        } else {
            total += current_value;
        }
        previous_value = current_value;

        intmax_t limit = INT_MAX;

        if (negative) {
            limit++;
        }

        if (total > limit) {
            return SCAN_OVERFLOW;
        }
    }

    if (status != SCAN_OK && status != SCAN_END_OF_INPUT) {
        return status;
    }
    intmax_t limit = INT_MAX;

    if (negative) {
        limit++;
    }

    if (total > limit) {
        return SCAN_OVERFLOW;
    }

    if (negative) {
        if (total == (intmax_t)INT_MAX + 1) {
            *result = INT_MIN;
        } else {
            *result = -(int)total;
        }
    } else {
        *result = (int)total;
    }
    return SCAN_OK;
}

static int zeck_add_bit(int bit, int *previous_bit, uintmax_t *first_weight, uintmax_t *second_weight, uintmax_t *sum, int *finished) {
    uintmax_t next_weight;
    uintmax_t limit = (uintmax_t)UINT_MAX;

    if (bit == 1 && *previous_bit == 1) {
        *finished = 1;
        return SCAN_OK;
    }

    if (bit == 1) {
        if (*first_weight > limit - *sum) {
            return SCAN_OVERFLOW;
        }
        *sum += *first_weight;
    }

    if (*first_weight > limit - *second_weight) {
        next_weight = limit + 1;
    } else {
        next_weight = *first_weight + *second_weight;
    }
    *first_weight = *second_weight;
    *second_weight = next_weight;
    *previous_bit = bit;
    return SCAN_OK;
}

static int input_read_zeckendorf(Input *input, unsigned int *result) {
    int first;
    int second;
    int current;
    int status;
    int previous_bit = 0;
    int finished = 0;
    uintmax_t first_weight = 1;
    uintmax_t second_weight = 2;
    uintmax_t sum = 0;

    if (input == NULL || result == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    status = input_skip_spaces(input);
    if (status != SCAN_OK) {
        return status;
    }
    status = input_get(input, &first);
    if (status != SCAN_OK) {
        return status;
    }
    if (first != '0' && first != '1') {
        input_unget(input, first);
        return SCAN_MATCH_FAILURE;
    }

    status = input_get(input, &second);
    if (status != SCAN_OK) {
        return SCAN_MATCH_FAILURE;
    }
    if (second != '0' && second != '1') {
        input_unget(input, second);
        return SCAN_MATCH_FAILURE;
    }

    if (first == '0' && second == '1') {
        status = input_get(input, &current);
        if (status == SCAN_END_OF_INPUT) {
            *result = 0;
            return SCAN_OK;
        }
        if (status != SCAN_OK) {
            return status;
        }
        if (current != '0' && current != '1') {
            input_unget(input, current);
            *result = 0;
            return SCAN_OK;
        }

        status = zeck_add_bit(0, &previous_bit, &first_weight, &second_weight, &sum, &finished);
        if (status == SCAN_OK) {
            status = zeck_add_bit(1, &previous_bit, &first_weight, &second_weight, &sum, &finished);
        }
        if (status != SCAN_OK) {
            return status;
        }
    } else {
        status = zeck_add_bit(first - '0', &previous_bit, &first_weight, &second_weight, &sum, &finished);
        if (status == SCAN_OK) {
            status = zeck_add_bit(second - '0', &previous_bit, &first_weight, &second_weight, &sum, &finished);
        }
        if (status != SCAN_OK) {
            return status;
        }
        if (finished) {
            *result = (unsigned int)sum;
            return SCAN_OK;
        }
        current = 0;
    }

    if (!(first == '0' && second == '1')) {
        status = input_get(input, &current);
        if (status != SCAN_OK) {
            return SCAN_MATCH_FAILURE;
        }
    }

    while (current == '0' || current == '1') {
        status = zeck_add_bit(current - '0', &previous_bit, &first_weight, &second_weight, &sum, &finished);
        if (status != SCAN_OK) {
            return status;
        }
        if (finished) {
            *result = (unsigned int)sum;
            return SCAN_OK;
        }
        status = input_get(input, &current);
        if (status != SCAN_OK) {
            return SCAN_MATCH_FAILURE;
        }
    }

    input_unget(input, current);
    return SCAN_MATCH_FAILURE;
}

static int input_read_characters(Input *input, char *result, size_t width, int suppress) {
    size_t count;
    int character;
    int status;

    if (input == NULL || (!suppress && result == NULL)) {
        return SCAN_INVALID_ARGUMENT;
    }
    if (width == 0) {
        width = 1;
    }

    for (count = 0; count < width; count++) {
        status = input_get(input, &character);
        if (status != SCAN_OK) {
            return status;
        }
        if (!suppress) {
            result[count] = (char)character;
        }
    }
    return SCAN_OK;
}

static int input_read_text(Input *input, char *result, size_t width, int suppress, const char *set, size_t set_length, int invert, int skip_spaces) {
    size_t count = 0;
    int character;
    int status;

    if (input == NULL || (!suppress && result == NULL)) {
        return SCAN_INVALID_ARGUMENT;
    }
    if (skip_spaces) {
        status = input_skip_spaces(input);
        if (status != SCAN_OK) {
            return status;
        }
    }

    while (width == 0 || count < width) {
        status = input_get(input, &character);
        if (status != SCAN_OK) {
            break;
        }

        int accepted;
        if (set == NULL) {
            accepted = !isspace((unsigned char)character);
        } else {
            size_t i;
            accepted = 0;
            for (i = 0; i < set_length; i++) {
                if (i + 2 < set_length && set[i + 1] == '-') {
                    unsigned char low = (unsigned char)set[i];
                    unsigned char high = (unsigned char)set[i + 2];
                    if (low <= (unsigned char)character && (unsigned char)character <= high) {
                        accepted = 1;
                    }
                    i += 2;
                } else if ((unsigned char)set[i] == (unsigned char)character) {
                    accepted = 1;
                }
            }
            if (invert) {
                accepted = !accepted;
            }
        }

        if (!accepted) {
            input_unget(input, character);
            break;
        }
        if (!suppress) {
            result[count] = (char)character;
        }
        count++;
    }

    if (count == 0) {
        if (status == SCAN_END_OF_INPUT || status == SCAN_INPUT_ERROR) {
            return status;
        }
        return SCAN_MATCH_FAILURE;
    }
    if (!suppress) {
        result[count] = '\0';
    }
    return SCAN_OK;
}

static int append_character(char **buffer, size_t *length, size_t *capacity, int character) {
    char *new_buffer;
    size_t new_capacity;

    if (*length > SIZE_MAX - 2) {
        return SCAN_OVERFLOW;
    }
    if (*length + 1 >= *capacity) {
        if (*capacity == 0) {
            new_capacity = 32;
        } else {
            new_capacity = *capacity * 2;
        }

        if (new_capacity <= *capacity || new_capacity < *length + 2) {
            new_capacity = *length + 2;
        }
        new_buffer = (char *)realloc(*buffer, new_capacity);
        if (new_buffer == NULL) {
            return SCAN_MEMORY_ERROR;
        }
        *buffer = new_buffer;
        *capacity = new_capacity;
    }

    (*buffer)[*length] = (char)character;
    (*length)++;
    (*buffer)[*length] = '\0';
    return SCAN_OK;
}

static int input_read_float(Input *input, void *destination, int length, size_t width) {
    char *buffer = NULL;
    size_t buffer_length = 0;
    size_t capacity = 0;
    size_t used = 0;
    int character;
    int status;
    int digits = 0;
    long double value;
    char *end_pointer;

    if (input == NULL || destination == NULL) {
        return SCAN_INVALID_ARGUMENT;
    }

    status = input_skip_spaces(input);
    if (status != SCAN_OK) {
        return status;
    }
    status = input_get_limited(input, width, &used, &character);
    if (status != SCAN_OK) {
        return status;
    }
    if (character == '+' || character == '-') {
        status = append_character(&buffer, &buffer_length, &capacity, character);
        if (status != SCAN_OK) {
            free(buffer);
            return status;
        }
        status = input_get_limited(input, width, &used, &character);
        if (status != SCAN_OK) {
            free(buffer);
            return SCAN_MATCH_FAILURE;
        }
    }

    while (status == SCAN_OK && isdigit((unsigned char)character)) {
        status = append_character(&buffer, &buffer_length, &capacity, character);
        if (status != SCAN_OK) {
            free(buffer);
            return status;
        }
        digits++;
        status = input_get_limited(input, width, &used, &character);
    }

    if (status == SCAN_OK && character == '.') {
        status = append_character(&buffer, &buffer_length, &capacity, character);
        if (status != SCAN_OK) {
            free(buffer);
            return status;
        }
        status = input_get_limited(input, width, &used, &character);
        while (status == SCAN_OK && isdigit((unsigned char)character)) {
            status = append_character(&buffer, &buffer_length, &capacity, character);
            if (status != SCAN_OK) {
                free(buffer);
                return status;
            }
            digits++;
            status = input_get_limited(input, width, &used, &character);
        }
    }

    if (digits == 0) {
        if (status == SCAN_OK) {
            input_unget(input, character);
        }
        free(buffer);
        return SCAN_MATCH_FAILURE;
    }

    if (status == SCAN_OK && (character == 'e' || character == 'E')) {
        status = append_character(&buffer, &buffer_length, &capacity, character);
        if (status != SCAN_OK) {
            free(buffer);
            return status;
        }
        status = input_get_limited(input, width, &used, &character);
        if (status == SCAN_OK && (character == '+' || character == '-')) {
            status = append_character(&buffer, &buffer_length, &capacity, character);
            if (status != SCAN_OK) {
                free(buffer);
                return status;
            }
            status = input_get_limited(input, width, &used, &character);
        }

        int exponent_digits = 0;
        while (status == SCAN_OK && isdigit((unsigned char)character)) {
            status = append_character(&buffer, &buffer_length, &capacity, character);
            if (status != SCAN_OK) {
                free(buffer);
                return status;
            }
            exponent_digits++;
            status = input_get_limited(input, width, &used, &character);
        }
        if (exponent_digits == 0) {
            free(buffer);
            return SCAN_MATCH_FAILURE;
        }
    }

    if (status == SCAN_OK) {
        input_unget(input, character);
    }

    value = strtold(buffer, &end_pointer);
    if (end_pointer == buffer || *end_pointer != '\0' || !isfinite(value)) {
        free(buffer);
        return SCAN_OVERFLOW;
    }

    if (length == LENGTH_CAP_L) {
        *(long double *)destination = value;
    } else if (length == LENGTH_L) {
        if (fabsl(value) > DBL_MAX) {
            free(buffer);
            return SCAN_OVERFLOW;
        }
        *(double *)destination = (double)value;
    } else {
        if (fabsl(value) > FLT_MAX) {
            free(buffer);
            return SCAN_OVERFLOW;
        }
        *(float *)destination = (float)value;
    }

    free(buffer);
    return SCAN_OK;
}

static int scan_stop(int assigned, int status) {
    if (assigned == 0 && (status == SCAN_END_OF_INPUT || status == SCAN_INPUT_ERROR)) {
        return EOF;
    }
    return assigned;
}

static int scan_core(Input *input, const char *format, va_list *arguments) {
    int assigned = 0;

    if (input == NULL || format == NULL || arguments == NULL) {
        return 0;
    }

    while (*format != '\0') {
        int status;

        if (isspace((unsigned char)*format)) {
            while (isspace((unsigned char)*format)) {
                format++;
            }
            status = input_skip_spaces(input);
            if (status == SCAN_END_OF_INPUT || status == SCAN_INPUT_ERROR) {
                return assigned;
            }
            if (status != SCAN_OK) {
                return assigned;
            }
            continue;
        }

        if (*format != '%') {
            int character;
            status = input_get(input, &character);
            if (status != SCAN_OK) {
                return scan_stop(assigned, status);
            }
            if (character != (unsigned char)*format) {
                input_unget(input, character);
                return assigned;
            }
            format++;
            continue;
        }

        format++;
        if (*format == '\0') {
            return assigned;
        }
        if (*format == '%') {
            int character;
            status = input_get(input, &character);
            if (status != SCAN_OK) {
                return scan_stop(assigned, status);
            }
            if (character != '%') {
                input_unget(input, character);
                return assigned;
            }
            format++;
            continue;
        }

        if (format[0] == 'R' && format[1] == 'o') {
            int *destination = va_arg(*arguments, int *);
            status = input_read_roman(input, destination);
            format += 2;
            if (status != SCAN_OK) {
                return scan_stop(assigned, status);
            }
            assigned++;
            continue;
        }
        if (format[0] == 'Z' && format[1] == 'r') {
            unsigned int *destination = va_arg(*arguments, unsigned int *);
            status = input_read_zeckendorf(input, destination);
            format += 2;
            if (status != SCAN_OK) {
                return scan_stop(assigned, status);
            }
            assigned++;
            continue;
        }
        if (format[0] == 'C' && (format[1] == 'v' || format[1] == 'V')) {
            int *destination = va_arg(*arguments, int *);
            int base = va_arg(*arguments, int);
            uintmax_t magnitude;
            int negative;
            int letter_mode;

            if (format[1] == 'v') {
                letter_mode = 1;
            } else {
                letter_mode = 2;
            }

            if (base < 2 || base > 36) {
                base = 10;
            }
            status = input_read_integer(input, base, letter_mode, 0, &magnitude, &negative);
            if (status == SCAN_OK) {
                status = store_signed_integer(destination, LENGTH_DEFAULT, magnitude, negative);
            }
            format += 2;
            if (status != SCAN_OK) {
                return scan_stop(assigned, status);
            }
            assigned++;
            continue;
        }

        int suppress = 0;
        size_t width = 0;
        int length = LENGTH_DEFAULT;
        char conversion;

        if (*format == '*') {
            suppress = 1;
            format++;
        }
        while (*format >= '0' && *format <= '9') {
            unsigned int digit = (unsigned int)(*format - '0');
            if (width > (SIZE_MAX - digit) / 10) {
                return assigned;
            }
            width = width * 10 + digit;
            format++;
        }

        if (*format == 'h') {
            format++;
            if (*format == 'h') {
                length = LENGTH_HH;
                format++;
            } else {
                length = LENGTH_H;
            }
        } else if (*format == 'l') {
            format++;
            if (*format == 'l') {
                length = LENGTH_LL;
                format++;
            } else {
                length = LENGTH_L;
            }
        } else if (*format == 'j') {
            length = LENGTH_J;
            format++;
        } else if (*format == 'L') {
            length = LENGTH_CAP_L;
            format++;
        }

        conversion = *format;
        if (conversion == '\0') {
            return assigned;
        }
        format++;

        if (conversion == 'n') {
            if (!suppress) {
                if (length == LENGTH_HH) {
                    *(signed char *)va_arg(*arguments, signed char *) = (signed char)input->characters_read;
                } else if (length == LENGTH_H) {
                    *(short *)va_arg(*arguments, short *) = (short)input->characters_read;
                } else if (length == LENGTH_L) {
                    *(long *)va_arg(*arguments, long *) = (long)input->characters_read;
                } else if (length == LENGTH_LL) {
                    *(long long *)va_arg(*arguments, long long *) = (long long)input->characters_read;
                } else if (length == LENGTH_J) {
                    *(intmax_t *)va_arg(*arguments, intmax_t *) = (intmax_t)input->characters_read;
                } else {
                    *(int *)va_arg(*arguments, int *) = (int)input->characters_read;
                }
            }
            continue;
        }

        if (conversion == 'c') {
            char *destination = NULL;
            if (!suppress) {
                destination = va_arg(*arguments, char *);
            }
            size_t character_count = width;

            if (character_count == 0) {
                character_count = 1;
            }

            status = input_read_characters(input, destination, character_count, suppress);
        } else if (conversion == 's') {
            char *destination = NULL;
            if (!suppress) {
                destination = va_arg(*arguments, char *);
            }
            status = input_read_text(input, destination, width, suppress, NULL, 0, 0, 1);
        } else if (conversion == '[') {
            int invert = 0;
            const char *set_start;
            size_t set_length;
            char *destination = NULL;

            if (*format == '^') {
                invert = 1;
                format++;
            }
            set_start = format;
            if (*format == ']') {
                format++;
            }
            while (*format != '\0' && *format != ']') {
                format++;
            }
            if (*format != ']') {
                return assigned;
            }
            set_length = (size_t)(format - set_start);
            format++;
            if (set_length == 0) {
                return assigned;
            }
            if (!suppress) {
                destination = va_arg(*arguments, char *);
            }
            status = input_read_text(input, destination, width, suppress, set_start, set_length, invert, 0);
        } else if (conversion == 'd' || conversion == 'i') {
            uintmax_t magnitude;
            int negative;
            void *destination = NULL;

            if (!suppress) {
                if (length == LENGTH_HH) {
                    destination = va_arg(*arguments, signed char *);
                } else if (length == LENGTH_H) {
                    destination = va_arg(*arguments, short *);
                } else if (length == LENGTH_L) {
                    destination = va_arg(*arguments, long *);
                } else if (length == LENGTH_LL) {
                    destination = va_arg(*arguments, long long *);
                } else if (length == LENGTH_J) {
                    destination = va_arg(*arguments, intmax_t *);
                } else {
                    destination = va_arg(*arguments, int *);
                }
            }
            int base = 10;

            if (conversion == 'i') {
                base = 0;
            }

            status = input_read_integer(input, base, 0, width, &magnitude, &negative);
            if (status == SCAN_OK && !suppress) {
                status = store_signed_integer(destination, length, magnitude, negative);
            }
        } else if (conversion == 'u' || conversion == 'o' || conversion == 'x' || conversion == 'X') {
            uintmax_t magnitude;
            int negative;
            int base = 10;
            void *destination = NULL;

            if (conversion == 'o') {
                base = 8;
            } else if (conversion == 'x' || conversion == 'X') {
                base = 16;
            }
            if (!suppress) {
                if (length == LENGTH_HH) {
                    destination = va_arg(*arguments, unsigned char *);
                } else if (length == LENGTH_H) {
                    destination = va_arg(*arguments, unsigned short *);
                } else if (length == LENGTH_L) {
                    destination = va_arg(*arguments, unsigned long *);
                } else if (length == LENGTH_LL) {
                    destination = va_arg(*arguments, unsigned long long *);
                } else if (length == LENGTH_J) {
                    destination = va_arg(*arguments, uintmax_t *);
                } else {
                    destination = va_arg(*arguments, unsigned int *);
                }
            }
            status = input_read_integer(input, base, 0, width, &magnitude, &negative);
            if (status == SCAN_OK && !suppress) {
                status = store_unsigned_integer(destination, length, magnitude, negative);
            }
        } else if (conversion == 'p') {
            uintmax_t magnitude;
            int negative;
            void **destination = NULL;

            if (!suppress) {
                destination = va_arg(*arguments, void **);
            }
            status = input_read_integer(input, 16, 0, width, &magnitude, &negative);
            if (status == SCAN_OK && !suppress) {
                if (negative || magnitude > (uintmax_t)UINTPTR_MAX) {
                    status = SCAN_OVERFLOW;
                } else {
                    *destination = (void *)(uintptr_t)magnitude;
                }
            }
        } else if (conversion == 'f' || conversion == 'F' || conversion == 'e' || conversion == 'E' || conversion == 'g' || conversion == 'G') {
            void *destination = NULL;
            if (!suppress) {
                if (length == LENGTH_CAP_L) {
                    destination = va_arg(*arguments, long double *);
                } else if (length == LENGTH_L) {
                    destination = va_arg(*arguments, double *);
                } else {
                    destination = va_arg(*arguments, float *);
                }
            }
            if (suppress) {
                long double temporary;
                status = input_read_float(input, &temporary, LENGTH_CAP_L, width);
            } else {
                status = input_read_float(input, destination, length, width);
            }
        } else {
            return assigned;
        }

        if (status != SCAN_OK) {
            return scan_stop(assigned, status);
        }
        if (!suppress && conversion != 'n') {
            assigned++;
        }
    }

    return assigned;
}

int oversscanf(const char *source, const char *format, ...) {
    Input input;
    va_list arguments;
    int result;

    if (source == NULL || format == NULL) {
        return 0;
    }
    if (input_init_string(&input, source) != SCAN_OK) {
        return 0;
    }

    va_start(arguments, format);
    result = scan_core(&input, format, &arguments);
    va_end(arguments);
    return result;
}

int overfscanf(FILE *stream, const char *format, ...) {
    Input input;
    va_list arguments;
    int result;

    if (stream == NULL || format == NULL) {
        return 0;
    }
    if (input_init_file(&input, stream) != SCAN_OK) {
        return 0;
    }

    va_start(arguments, format);
    result = scan_core(&input, format, &arguments);
    va_end(arguments);
    return result;
}


int main(void) {
    int result;
    int number, roman, lower_base, upper_base, count;
    unsigned int zeckendorf, unsigned_number;
    short short_number;
    long long_number;
    long long long_long_number;
    intmax_t max_number;
    signed char signed_char_number;
    float float_number;
    double double_number;
    long double long_double_number;
    char character;
    char characters[4] = {0};
    char word[20], letters[20];
    void *pointer = NULL;
    FILE *file;

    printf("Custom conversions:\n");

    result = oversscanf("XIV 011 ff FF", "%Ro %Zr %Cv %CV", &roman, &zeckendorf, &lower_base, 16, &upper_base, 16);

    printf("Roman: %d, Zeckendorf: %u\n", roman, zeckendorf);
    printf("Cv: %d, CV: %d, assigned: %d\n", lower_base, upper_base, result);

    printf("\nInteger conversions:\n");

    result = oversscanf("-125 0x2A 255 17 FF", "%d %i %u %o %X", &number, &lower_base, &unsigned_number, &zeckendorf, &upper_base);

    printf("d: %d, i: %d, u: %u, o: %u, X: %u\n",
           number, lower_base, unsigned_number,
           zeckendorf, upper_base);
    printf("Assigned: %d\n", result);

    printf("\nLength modifiers:\n");

    result = oversscanf("12 345 6789 12345 98", "%hd %ld %lld %jd %hhd", &short_number, &long_number, &long_long_number, &max_number, &signed_char_number);

    printf("hd: %hd, ld: %ld, lld: %lld\n", short_number, long_number, long_long_number);
    printf("jd: %jd, hhd: %hhd, assigned: %d\n", max_number, signed_char_number, result);

    printf("\nFloating point:\n");

    result = oversscanf("3.14 2.5e2 1.75", "%f %lf %Lf", &float_number, &double_number, &long_double_number);

    printf("f: %f, lf: %f, Lf: %Lf, assigned: %d\n", float_number, double_number, long_double_number, result);

    printf("\nCharacters and strings:\n");

    result = oversscanf("A hello world abc123", "%c %s %s %3[a-z]", &character, word, letters, characters);

    printf("c: %c, s: %s, second s: %s, scanset: %s\n", character, word, letters, characters);
    printf("Assigned: %d\n", result);

    result = oversscanf("123abc", "%3[^a-z]", letters);
    printf("Inverted scanset: %s\n", letters);

    result = oversscanf("100 hello", "%*d %s", word);
    printf("Suppressed assignment: %s, assigned: %d\n", word, result);

    result = oversscanf("12345", "%3d", &number);
    printf("Field width: %d\n", number);

    result = oversscanf("%A 42", "%%%c %d", &character, &number);
    printf("Literal percent: %c, number: %d\n", character, number);

    result = oversscanf("123 abc", "%d%n", &number, &count);
    printf("n: %d, assigned: %d\n", count, result);

    result = oversscanf("0x1234", "%p", &pointer);
    printf("Pointer: %p, assigned: %d\n", pointer, result);

    printf("\nFile input:\n");

    file = tmpfile();

    if (file == NULL) {
        printf("Could not create temporary file\n");
        return 1;
    }

    fputs("42 XIV 011 ff", file);
    rewind(file);

    result = overfscanf(file, "%d %Ro %Zr %Cv", &number, &roman, &zeckendorf, &lower_base, 16);

    printf("Integer: %d, Roman: %d\n", number, roman);
    printf("Zeckendorf: %u, base conversion: %d\n", zeckendorf, lower_base);
    printf("Assigned: %d\n", result);

    fclose(file);

    return 0;
}