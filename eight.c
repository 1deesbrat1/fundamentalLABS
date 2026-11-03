#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#define STATUS_OK 0
#define STATUS_INVALID_ARGUMENT 1
#define STATUS_INVALID_BASE 2
#define STATUS_INVALID_NUMBER 3
#define STATUS_MEMORY_ERROR 4
#define STATUS_SIZE_ERROR 5

static int digit_value(char character) {
    if (character >= '0' && character <= '9') {
        return character - '0';
    }

    if (character >= 'A' && character <= 'Z') {
        return character - 'A' + 10;
    }

    if (character >= 'a' && character <= 'z') {
        return character - 'a' + 10;
    }

    return -1;
}

static char digit_symbol(int value) {
    if (value < 10) {
        return (char)('0' + value);
    }

    return (char)('A' + value - 10);
}

static int validate_number(const char *number, int base) {
    size_t i;

    if (number == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (number[0] == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    i = 0;
    while (number[i] != '\0') {
        int value = digit_value(number[i]);

        if (value < 0 || value >= base) {
            return STATUS_INVALID_NUMBER;
        }

        i++;
    }

    return STATUS_OK;
}

int add_in_base(const char *first, const char *second, int base, char **result) {
    size_t first_length;
    size_t second_length;
    size_t max_length;
    size_t position;
    size_t output_length;
    size_t i;
    int carry;
    char *buffer;

    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;

    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }

    if (first == NULL || second == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    {
        int status = validate_number(first, base);
        if (status != STATUS_OK) {
            return status;
        }

        status = validate_number(second, base);
        if (status != STATUS_OK) {
            return status;
        }
    }

    first_length = strlen(first);
    second_length = strlen(second);

    max_length = first_length;
    if (second_length > max_length) {
        max_length = second_length;
    }

    if (max_length > (size_t)-1 - 2) {
        return STATUS_SIZE_ERROR;
    }

    buffer = (char *)malloc(max_length + 2);
    if (buffer == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    position = 0;
    output_length = 0;
    carry = 0;

    while (position < max_length || carry != 0) {
        int first_digit = 0;
        int second_digit = 0;
        int sum;

        if (position < first_length) {
            first_digit = digit_value(first[first_length - position - 1]);
        }

        if (position < second_length) {
            second_digit = digit_value(second[second_length - position - 1]);
        }

        sum = first_digit + second_digit + carry;
        buffer[output_length] = digit_symbol(sum % base);
        output_length++;
        carry = sum / base;
        position++;
    }

    while (output_length > 1 && buffer[output_length - 1] == '0') {
        output_length--;
    }

    buffer[output_length] = '\0';

    for (i = 0; i < output_length / 2; i++) {
        char temporary = buffer[i];
        buffer[i] = buffer[output_length - i - 1];
        buffer[output_length - i - 1] = temporary;
    }

    *result = buffer;
    return STATUS_OK;
}

int sum_in_base(int base, int count, char **result, ...) {
    va_list arguments;
    const char *current_number;
    char *total;
    char *next_total;
    int status;
    int i;

    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;

    if (base < 2 || base > 36) {
        return STATUS_INVALID_BASE;
    }

    if (count < 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    total = (char *)malloc(2);
    if (total == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    total[0] = '0';
    total[1] = '\0';

    va_start(arguments, result);

    i = 0;
    while (i < count) {
        current_number = va_arg(arguments, const char *);

        next_total = NULL;
        status = add_in_base(total, current_number, base, &next_total);
        if (status != STATUS_OK) {
            va_end(arguments);
            free(total);
            free(next_total);
            return status;
        }

        free(total);
        total = next_total;
        i++;
    }

    va_end(arguments);

    *result = total;
    return STATUS_OK;
}

static const char *status_text(int status) {
    if (status == STATUS_OK) {
        return "успешно";
    }

    if (status == STATUS_INVALID_ARGUMENT) {
        return "неверный аргумент";
    }

    if (status == STATUS_INVALID_BASE) {
        return "основание системы счисления должно быть от 2 до 36";
    }

    if (status == STATUS_INVALID_NUMBER) {
        return "число содержит недопустимые символы или цифры";
    }

    if (status == STATUS_MEMORY_ERROR) {
        return "ошибка выделения памяти";
    }

    if (status == STATUS_SIZE_ERROR) {
        return "слишком большой размер числа";
    }

    return "неизвестная ошибка";
}

int main(void) {
    char *result = NULL;
    int status;
    const char *decimal_first = "999999999999999999999999999999";
    const char *decimal_second = "1";
    const char *decimal_third = "123456789012345678901234567890";
    const char *hex_first = "FFFFFFFFFFFFFFFFFFFF";
    const char *hex_second = "1";
    const char *hex_third = "ABCDEF";
    const char *binary_first = "11111111111111111111";
    const char *binary_second = "1";
    const char *binary_third = "101010";

    status = sum_in_base(10, 3, &result, decimal_first, decimal_second, decimal_third);

    if (status == STATUS_OK) {
        printf("Основание 10: %s\n", result);
    } else {
        printf("Ошибка: %s\n", status_text(status));
    }
    free(result);
    result = NULL;

    status = sum_in_base(16, 3, &result, hex_first, hex_second, hex_third);

    if (status == STATUS_OK) {
        printf("Основание 16: %s\n", result);
    } else {
        printf("Ошибка: %s\n", status_text(status));
    }
    free(result);
    result = NULL;

    status = sum_in_base(2, 3, &result, binary_first, binary_second, binary_third);

    if (status == STATUS_OK) {
        printf("Основание 2: %s\n", result);
    } else {
        printf("Ошибка: %s\n", status_text(status));
    }
    free(result);

    return 0;
}
