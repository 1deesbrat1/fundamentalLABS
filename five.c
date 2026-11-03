#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

#define STATUS_OK 0
#define STATUS_INVALID_ARGUMENT 1
#define STATUS_OUTPUT_ERROR 2
#define STATUS_TOO_LONG 3
#define STATUS_INVALID_FORMAT 4
#define STATUS_INVALID_NUMBER 5
#define STATUS_NUMBER_OVERFLOW 6
#define STATUS_UNSUPPORTED_TYPE 7

typedef struct {
    FILE* stream;
    char* buffer;
    size_t length;
} Output;

int overfprintf(FILE* stream, const char* format, ...);

int oversprintf(char* buffer, const char* format, ...);


static int output_write(Output* output, const char* data, size_t count) {
    if (output == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (data == NULL && count > 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if ((output->stream == NULL && output->buffer == NULL) || (output->stream != NULL && output->buffer != NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (output->length > (size_t)INT_MAX) {
        return STATUS_TOO_LONG;
    }

    if (count > (size_t)INT_MAX - output->length) {
        return STATUS_TOO_LONG;
    }

    if (count == 0) {
        return STATUS_OK;
    }

    if (output->stream != NULL) {
        size_t written = fwrite(data, 1, count, output->stream);

        output->length += written;

        if (written != count) {
            return STATUS_OUTPUT_ERROR;
        }

        return STATUS_OK;
    }

    for (size_t i = 0; i < count; i++) {
        output->buffer[output->length + i] = data[i];
    }

    output->length += count;
    output->buffer[output->length] = '\0';

    return STATUS_OK;
}


static int output_write_integer(Output *output, unsigned int magnitude, int negative, unsigned int base, int uppercase) {
    if (base < 2 || base > 36) {
        return STATUS_INVALID_ARGUMENT;
    }

    char digits[sizeof(unsigned int) * CHAR_BIT + 1];

    size_t length = 0;

    do {
        unsigned int remainder = magnitude % base;

        if (remainder < 10) {
            digits[length] = (char)('0' + remainder);
        }
        else if (uppercase) {
            digits[length] = (char)('A' + remainder - 10);
        }
        else {
            digits[length] = (char)('a' + remainder - 10);
        }

        length++;
        magnitude /= base;

    } while (magnitude > 0);

    if (negative) {
        digits[length] = '-';
        length++;
    }

    for (size_t i = 0; i < length / 2; i++) {
        char temp = digits[i];

        digits[i] = digits[length - 1 - i];
        digits[length - 1 - i] = temp;
    }

    return output_write(output, digits, length);
}


static int output_write_roman(Output *output, int value) {
    if (output == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    int negative = value < 0;

    unsigned int magnitude = (unsigned int)value;

    if (negative) {
        magnitude = 0U - magnitude;
    }

    if (negative) {
        int status = output_write(output, "-", 1);

        if (status != STATUS_OK) {
            return status;
        }
    }

    if (magnitude == 0) {
        return output_write(output, "N", 1);
    }

    char thousands_chunk[64];

    for (size_t i = 0; i < sizeof(thousands_chunk); i++) {
        thousands_chunk[i] = 'M';
    }

    while (magnitude >= 1000U) {
        unsigned int count = magnitude / 1000U;

        if (count > sizeof(thousands_chunk)) {
            count = (unsigned int)sizeof(thousands_chunk);
        }

        int status = output_write(output, thousands_chunk, (size_t)count);

        if (status != STATUS_OK) {
            return status;
        }

        magnitude -= count * 1000U;
    }

    const unsigned int values[] = {
        900, 500, 400, 100, 90, 50,
        40, 10, 9, 5, 4, 1
    };

    const char *symbols[] = {
        "CM", "D", "CD", "C", "XC", "L",
        "XL", "X", "IX", "V", "IV", "I"
    };

    size_t size = sizeof(values) / sizeof(values[0]);

    for (size_t i = 0; i < size; i++) {

        while (magnitude >= values[i]) {

            int status = output_write(output, symbols[i], strlen(symbols[i]));

            if (status != STATUS_OK) {
                return status;
            }

            magnitude -= values[i];
        }
    }

    return STATUS_OK;
}


static int output_write_zeckendorf(Output *output, unsigned int value) {
    if (output == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    unsigned int fibonacci[sizeof(unsigned int) * CHAR_BIT * 2];

    char representation[sizeof(unsigned int) * CHAR_BIT * 2 + 1];

    size_t capacity = sizeof(fibonacci) / sizeof(fibonacci[0]);

    size_t fib_count = 1;

    fibonacci[0] = 1U;

    if (value >= 2U) {
        fibonacci[fib_count] = 2U;
        fib_count++;
    }

    while (fib_count >= 2 && fib_count < capacity) {

        unsigned int previous = fibonacci[fib_count - 2];
        unsigned int current = fibonacci[fib_count - 1];

        if (current > UINT_MAX - previous) {
            break;
        }

        unsigned int next = previous + current;

        if (next > value) {
            break;
        }

        fibonacci[fib_count] = next;
        fib_count++;
    }

    for (size_t i = 0; i < fib_count; i++) {
        representation[i] = '0';
    }

    unsigned int remainder = value;

    for (size_t i = fib_count; i > 0; i--) {

        size_t index = i - 1;

        if (fibonacci[index] <= remainder) {

            representation[index] = '1';

            remainder -= fibonacci[index];
        }
    }

    representation[fib_count] = '1';

    return output_write(output, representation, fib_count + 1);
}


static int output_write_custom_base(Output *output, int value, int base, int uppercase) {
    if (output == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (base < 2 || base > 36) {
        base = 10;
    }

    int negative = value < 0;

    unsigned int magnitude = (unsigned int)value;

    if (negative) {
        magnitude = 0U - magnitude;
    }

    return output_write_integer(output, magnitude, negative, (unsigned int)base, uppercase);
}


static int output_write_parsed_number(Output *output, const char *text, int base, int uppercase) {
    if (output == NULL || text == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (base < 2 || base > 36) {
        base = 10;
    }
    const char *cursor = text;

    int negative = 0;
    if (*cursor == '-' || *cursor == '+') {
        negative = (*cursor == '-');
        cursor++;
    }

    if (*cursor == '\0') {
        return STATUS_INVALID_NUMBER;
    }

    unsigned int limit = (unsigned int)INT_MAX;

    if (negative && INT_MIN < -INT_MAX) {
        limit++;
    }

    unsigned int result = 0;

    while (*cursor != '\0') {

        unsigned int digit;

        if (*cursor >= '0' && *cursor <= '9') {
            digit = (unsigned int)(*cursor - '0');
        }

        else if (uppercase && *cursor >= 'A' && *cursor <= 'Z') {
            digit = (unsigned int)(*cursor - 'A' + 10);
        }

        else if (!uppercase && *cursor >= 'a' && *cursor <= 'z') {
            digit = (unsigned int)(*cursor - 'a' + 10);
        }

        else {
            return STATUS_INVALID_NUMBER;
        }

        if (digit >= (unsigned int)base) {
            return STATUS_INVALID_NUMBER;
        }

        if (digit > limit || result > (limit - digit) / (unsigned int)base) {

            return STATUS_NUMBER_OVERFLOW;
        }

        result = result * (unsigned int)base + digit;
        cursor++;
    }

    return output_write_integer(output, result, negative && result != 0, 10, 0);
}


static int output_write_memory(Output *output, const void *value, size_t size) {
    if (output == NULL || value == NULL || size == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (CHAR_BIT != 8) {
        return STATUS_UNSUPPORTED_TYPE;
    }

    const unsigned char *bytes = (const unsigned char *)value;

    for (size_t i = 0; i < size; i++) {
        char binary[8];

        for (size_t j = 0; j < 8; j++) {
            unsigned int shift = 7U - (unsigned int)j;

            unsigned int bit = ((unsigned int)bytes[i] >> shift) & 1U;

            if (bit == 1U) {
                binary[j] = '1';
            }
            else {
                binary[j] = '0';
            }
        }   

        int status = output_write(output, binary, 8);

        if (status != STATUS_OK) {
            return status;
        }

        if (i + 1 < size) {
            status = output_write(output, " ", 1);
            if (status != STATUS_OK) {
                return status;
            }
        }
    }

    return STATUS_OK;
}


static int format_engine(Output *output, const char *format, va_list *arguments) {
    if (output == NULL || format == NULL || arguments == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if ((output->stream == NULL && output->buffer == NULL) ||
        (output->stream != NULL && output->buffer != NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    const char *cursor = format;

    while (*cursor != '\0') {

        if (*cursor != '%') {
            const char *start = cursor;
            while (*cursor != '\0' && *cursor != '%') {
                cursor++;
            }
            size_t count = (size_t)(cursor - start);
            int status = output_write(output, start, count);
            if (status != STATUS_OK) {
                return status;
            }

        } else {
            cursor++;
            if (*cursor == '\0') {
                return STATUS_INVALID_FORMAT;
            }
            int status = STATUS_OK;

            if (*cursor == '%') {
                status = output_write(output, "%", 1);
            }

            else if (*cursor == 'R' && cursor[1] == 'o') {
                int value = va_arg(*arguments, int);
                status = output_write_roman(output, value);
                cursor++;
            }

            else if (*cursor == 'Z' && cursor[1] == 'r') {

                unsigned int value = va_arg(*arguments, unsigned int);

                status = output_write_zeckendorf(output, value);
                cursor++;
            }

            else if (*cursor == 'C' && (cursor[1] == 'v' || cursor[1] == 'V')) {
                int value = va_arg(*arguments, int);
                int base = va_arg(*arguments, int);
                int uppercase = (cursor[1] == 'V');
                status = output_write_custom_base(output, value, base, uppercase);

                cursor++;
            }

            else if ((*cursor == 't' && cursor[1] == 'o') || (*cursor == 'T' && cursor[1] == 'O')) {
                char *text = va_arg(*arguments, char *);
                int base = va_arg(*arguments, int);
                int uppercase = (*cursor == 'T');
                status = output_write_parsed_number(output, text, base, uppercase);

                cursor++;
            }

            else if (*cursor == 'm' && cursor[1] == 'i') {
               int value = va_arg(*arguments, int);

                if (sizeof(value) != 4) {
                    return STATUS_INVALID_ARGUMENT;
                }

                status = output_write_memory(output, &value, sizeof(value));
                cursor++;
            }

            else if (*cursor == 'm' && cursor[1] == 'u') {
                unsigned int value = va_arg(*arguments, unsigned int);

                if (sizeof(value) != 4) {
                    return STATUS_INVALID_ARGUMENT;
                }

                status = output_write_memory(output, &value, sizeof(value));
                cursor++;
            }

            else if (*cursor == 'm' && cursor[1] == 'f') {
                double promoted_value = va_arg(*arguments, double);
                float value = (float)promoted_value;
                status = output_write_memory(output, &value, sizeof(value));

                cursor++;
            }

            else if (*cursor == 'm' && cursor[1] == 'd') {
                double value = va_arg(*arguments, double);
                status = output_write_memory(output, &value, sizeof(value));
                cursor++;
            }

            else if (*cursor == 'd' || *cursor == 'i') {
                int value = va_arg(*arguments, int);
                int negative = value < 0;
                unsigned int magnitude = (unsigned int)value;

                if (negative) {
                    magnitude = 0U - magnitude;
                }

                status = output_write_integer(output, magnitude, negative, 10, 0);
            }

            else if (*cursor == 'u') {
                unsigned int value = va_arg(*arguments, unsigned int);
                status = output_write_integer(output, value, 0, 10, 0);
            }

            else if (*cursor == 'o') {
                unsigned int value = va_arg(*arguments, unsigned int);

                status = output_write_integer(output, value, 0, 8, 0);
            }

            else if (*cursor == 'x') {
                unsigned int value = va_arg(*arguments, unsigned int);
                status = output_write_integer(output, value, 0, 16, 0);
            }

            else if (*cursor == 'X') {
                unsigned int value = va_arg(*arguments, unsigned int);

                status = output_write_integer(output, value, 0, 16, 1);
            }

            else if (*cursor == 'c') {
                unsigned char character = (unsigned char)va_arg(*arguments, int);
                status = output_write(output, (const char *)&character, 1);
            }

            else if (*cursor == 's') {
                char *text = va_arg(*arguments, char *);
                if (text == NULL) {
                    return STATUS_INVALID_ARGUMENT;
                }

                status = output_write(output, text, strlen(text));
            }

            else {
                return STATUS_INVALID_FORMAT;
            }

            if (status != STATUS_OK) {
                return status;
            }

            cursor++;
        }
    }

    return STATUS_OK;
}



int overfprintf(FILE *stream, const char *format, ...) {
    if (stream == NULL || format == NULL) {
        return -1;
    }

    Output output = {stream, NULL, 0};

    va_list arguments;
    va_start(arguments, format);
    int status = format_engine(&output, format, &arguments);

    va_end(arguments);
    if (status != STATUS_OK) {
        return -1;
    }

    return (int)output.length;
}



int oversprintf(char *buffer, const char *format, ...) {
    if (buffer == NULL || format == NULL) {
        return -1;
    }

    buffer[0] = '\0';

    Output output = {NULL, buffer, 0};

    va_list arguments;
    va_start(arguments, format);

    int status = format_engine(&output, format, &arguments);

    va_end(arguments);

    if (status != STATUS_OK) {
        return -1;
    }

    return (int)output.length;
}


int main(void) {
    char buffer[1024];

    int result = oversprintf(
        buffer,
        "Standard types: %d %u %o %x %c %s\n"
        "Roman number: %Ro\n"
        "Zeckendorf number: %Zr\n"
        "Custom bases: %Cv %CV\n"
        "Parsed numbers: %to %TO\n"
        "Memory of int: %mi\n"
        "Memory of unsigned int: %mu\n"
        "Memory of float: %mf\n"
        "Memory of double: %md\n"
        "Percent: %%\n",

        -25, 100U, 64U, 255U, 'A', "Hello",
        1994,
        100U,
        -255, 16,
        255, 16,
        "ff", 16,
        "FF", 16,
        5,
        5U,
        (float)1.5,
        3.14
    );

    if (result < 0) {
        fputs("Error in oversprintf\n", stderr);
        return 1;
    }

    fputs("Result of oversprintf:\n", stdout);
    fputs(buffer, stdout);


    int stream_result = overfprintf(
        stdout,
        "\nResult of overfprintf:\n"
        "Roman number: %Ro\n"
        "Custom base: %CV\n"
        "Parsed number: %to\n",
        2026,
        255, 16,
        "1010", 2
    );

    if (stream_result < 0) {
        fputs("Error in overfprintf\n", stderr);
        return 1;
    }


    FILE *file = fopen("five_output.txt", "w");

    if (file == NULL) {
        fputs("Error opening file\n", stderr);
        return 1;
    }

    int file_result = overfprintf(
        file,
        "File output:\n"
        "Decimal: %d\n"
        "Roman: %Ro\n"
        "Zeckendorf: %Zr\n"
        "Custom base: %CV\n"
        "Parsed number: %to\n"
        "Memory: %mi\n"
        "Percent: %%\n",
        123,
        2026,
        50U,
        255, 16,
        "ff", 16,
        10
    );

    int close_result = fclose(file);

    if (file_result < 0) {
        fputs("Error writing to file\n", stderr);
        return 1;
    }

    if (close_result != 0) {
        fputs("Error closing file\n", stderr);
        return 1;
    }

    fputs("\nFile five_output.txt created successfully.\n", stdout);

    return 0;
}