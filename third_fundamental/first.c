#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_BUFFER_TOO_SMALL,
    STATUS_INPUT_ERROR
} Status;

Status convert_number(int32_t number, unsigned int r, char *result, size_t result_size) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (r < 1 || r > 5) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result_size < 34) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUV";

    uint32_t mask = ~(UINT32_MAX << r);
    uint32_t value = (uint32_t)number;

    if (number < 0) {
        value = ~value;

        uint32_t bit = 1U;

        while ((value & bit) != 0U) {
            value ^= bit;
            bit <<= 1;
        }

        value ^= bit;
    }

    char *cursor = &result[33];
    *cursor = '\0';

    do {
        uint32_t digit = value & mask;

        --cursor;
        *cursor = digits[digit];

        value >>= r;

    } while (value != 0);

    if (number < 0) {
        --cursor;
        *cursor = '-';
    }

    char *source = cursor;
    char *destination = result;

    while (*source != '\0') {
        *destination = *source;
        ++destination;
        ++source;
    }

    *destination = '\0';

    return STATUS_OK;
}

Status read_line(char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < 2) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (fgets(buffer, buffer_size, stdin) == NULL) {
        return STATUS_INPUT_ERROR;
    }

    char *newline = strchr(buffer, '\n');

    if (newline != NULL) {
        *newline = '\0';
    } else {
        int ch = getchar();

        if (ch != '\n' && ch != EOF) {
            do {
                ch = getchar();
            } while (ch != '\n' && ch != EOF);

            return STATUS_INPUT_ERROR;
        }

    }

    return STATUS_OK;
}

Status parse_int32(const char *text, int32_t min_value, int32_t max_value, int32_t *result) {
    if (text == NULL || result == NULL || min_value > max_value) {
        return STATUS_INVALID_ARGUMENT;
    }

    char *end;
    long long value = strtoll(text, &end, 10);

    if (text == end) {
        return STATUS_INVALID_ARGUMENT;
    }

    while (isspace((unsigned char)*end)) {
        ++end;
    }

    if (*end != '\0' || value < min_value || value > max_value) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = (int32_t)value;

    return STATUS_OK;
}

int main(void) {
    char buffer[128];
    char result[34];

    int32_t number;
    int32_t r_input;

    Status status;

    printf("Enter number: ");

    status = read_line(buffer, sizeof(buffer));

    if (status != STATUS_OK) {
        fprintf(stderr, "Input error\n");
        return 1;
    }

    status = parse_int32(buffer, INT32_MIN, INT32_MAX, &number);

    if (status != STATUS_OK) {
        fprintf(stderr, "Invalid number\n");
        return 1;
    }

    printf("Enter r (1-5): ");

    status = read_line(buffer, sizeof(buffer));

    if (status != STATUS_OK) {
        fprintf(stderr, "Input error\n");
        return 1;
    }

    status = parse_int32(buffer, 1, 5, &r_input);

    if (status != STATUS_OK) {
        fprintf(stderr, "Invalid r\n");
        return 1;
    }

    status = convert_number(
        number,
        (unsigned int)r_input,
        result,
        sizeof(result)
    );

    if (status == STATUS_OK) {
        printf("Result: %s\n", result);
        return 0;
    }
    else if (status == STATUS_INVALID_ARGUMENT) {
        fprintf(stderr, "Invalid argument\n");
    }
    else if (status == STATUS_BUFFER_TOO_SMALL) {
        fprintf(stderr, "Buffer is too small\n");
    }
    else {
        fprintf(stderr, "Unknown error\n");
    }

    return 1;
}