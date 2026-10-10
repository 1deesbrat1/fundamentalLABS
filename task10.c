#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

int digit_value(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    if (ch >= 'A' && ch <= 'Z') {
        return ch - 'A' + 10;
    }

    return -1;
}


int read_line(char **text) {
    size_t capacity = 16;
    size_t length = 0;
    int ch;
    char *buffer;

    if (text == NULL) {
        return 2;
    }

    *text = NULL;

    buffer = (char *)malloc(capacity);

    if (buffer == NULL) {
        return 2;
    }

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (length + 1 >= capacity) {
            char *temp;

            if (capacity > (size_t)-1 / 2) {
                free(buffer);
                return 2;
            }

            size_t new_capacity = capacity * 2;
            temp = (char *)realloc(buffer, new_capacity);

            if (temp == NULL) {
                free(buffer);
                return 2;
            }

            buffer = temp;
            capacity = new_capacity;
        }

        buffer[length] = (char)ch;
        length++;
    }

    if (ch == EOF && length == 0) {
        free(buffer);
        return 1;
    }

    buffer[length] = '\0';
    *text = buffer;

    return 0;
}


int parse_number(const char *text, int base, long long *result) {
    int i;
    int negative = 0;
    int digit;
    unsigned long long value = 0;
    unsigned long long limit;

    if (text == NULL || result == NULL || base < 2 || base > 36) {
        return 2;
    }

    if (text[0] == '\0') {
        return 1;
    }

    i = 0;

    if (text[i] == '-') {
        negative = 1;
        i++;
    } else if (text[i] == '+') {
        i++;
    }

    if (text[i] == '\0') {
        return 1;
    }

    if (negative) {
        limit = (unsigned long long)LLONG_MAX + 1ULL;
    } else {
        limit = (unsigned long long)LLONG_MAX;
    }

    while (text[i] != '\0') {
        digit = digit_value(text[i]);

        if (digit < 0 || digit >= base) {
            return 1;
        }

        if (value > limit / (unsigned long long)base || (value == limit / (unsigned long long)base && (unsigned long long)digit > limit % (unsigned long long)base)) {
            return 1;
        }

        value = value * (unsigned long long)base + (unsigned long long)digit;

        i++;
    }

    if (negative) {
        if (value == (unsigned long long)LLONG_MAX + 1ULL) {
            *result = LLONG_MIN;
        } else {
            *result = -(long long)value;
        }
    } else {
        *result = (long long)value;
    }

    return 0;
}

unsigned long long absolute_value(long long number) {
    if (number < 0) {
        return 0ULL - (unsigned long long)number;
    }

    return (unsigned long long)number;
}

void print_in_base(long long number, int base) {
    char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char result[70];
    unsigned long long value;
    int i = 0;
    int negative = number < 0;

    value = absolute_value(number);

    if (value == 0) {
        printf("0");
        return;
    }

    while (value > 0) {
        result[i] = digits[value % (unsigned long long)base];
        value /= (unsigned long long)base;
        i++;
    }

    if (negative) {
        printf("-");
    }

    while (i > 0) {
        i--;
        printf("%c", result[i]);
    }
}

int print_results(const char *name, long long number) {
    int bases[] = {9, 18, 27, 36};
    int i;

    printf("%s\n", name);

    for (i = 0; i < 4; i++) {
        printf("Base %d: ", bases[i]);
        print_in_base(number, bases[i]);
        printf("\n");
    }

    return 0;
}


int process_numbers(int base) {
    char *text = NULL;
    long long number;
    long long sum = 0;
    long long max_abs_number = 0;
    int has_number = 0;
    int read_status;

    printf("Enter numbers in base %d. Enter Stop to finish.\n", base);

    while ((read_status = read_line(&text)) == 0) {
        if (strcmp(text, "Stop") == 0) {
            free(text);
            text = NULL;
            break;
        }

        int parse_status = parse_number(text, base, &number);

        if (parse_status == 1) {
            printf("Invalid number\n");
            free(text);
            text = NULL;
            continue;
        }

        if (parse_status == 2) {
            free(text);
            return 1;
        }

        free(text);
        text = NULL;

        if ((number > 0 && sum > LLONG_MAX - number) ||
            (number < 0 && sum < LLONG_MIN - number)) {
            printf("The sum is too big.\n");
            return 1;
        }

        sum += number;

        if (!has_number ||
            absolute_value(number) > absolute_value(max_abs_number)) {
            max_abs_number = number;
        }

        has_number = 1;
    }

    if (read_status == 2) {
        printf("Memory allocation error.\n");
        return 1;
    }

    if (!has_number) {
        printf("No numbers were entered.\n");
        return 1;
    }

    printf("\nMaximum by absolute value\n");
    print_results("Representations:", max_abs_number);

    printf("\nSum of all numbers\n");
    print_results("Representations:", sum);

    return 0;
}


int main(void) {
    int base;

    printf("Enter the base (2..36): ");

    int ch;

    if (scanf("%d", &base) != 1 || base < 2 || base > 36) {
        printf("Please enter a base from 2 to 36.\n");
        return 1;
    }

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }

    if (process_numbers(base) != 0) {
        return 1;
    }

    printf("\nAll done!\n");

    return 0;
}
