#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#define STATUS_OK 0
#define STATUS_INVALID_ARGUMENTS 1
#define STATUS_INVALID_FRACTION 2
#define STATUS_PRECISION_LIMIT 3

static uintmax_t gcd_uintmax(const uintmax_t first, const uintmax_t second) {
    uintmax_t a = first;
    uintmax_t b = second;

    while (b != 0U) {
        uintmax_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

static int build_decimal_scale(const double epsilon, uintmax_t *scale) {
    uintmax_t current_scale = 1U;
    size_t decimal_places = 0U;
    const size_t max_decimal_places = 15U;

    if (scale == NULL || !isfinite(epsilon) || epsilon <= 0.0 || epsilon >= 0.5) {
        return STATUS_INVALID_ARGUMENTS;
    }

    while (0.5 / (double)current_scale > epsilon) {
        if (decimal_places >= max_decimal_places) {
            return STATUS_PRECISION_LIMIT;
        }
        current_scale *= 10U;
        decimal_places++;
    }

    *scale = current_scale;
    return STATUS_OK;
}

static int calculate_result(const double value, const unsigned int base, const double epsilon, const uintmax_t scale, int *result) {
    double scaled_value;
    double rounded_value;
    double restored_value;
    uintmax_t numerator;
    uintmax_t denominator;
    uintmax_t divisor;

    if (result == NULL || base < 2U || scale == 0U) {
        return STATUS_INVALID_ARGUMENTS;
    }

    if (!isfinite(value) || value <= 0.0 || value >= 1.0) {
        return STATUS_INVALID_FRACTION;
    }

    scaled_value = value * (double)scale;
    if (!isfinite(scaled_value)) {
        return STATUS_PRECISION_LIMIT;
    }

    rounded_value = floor(scaled_value + 0.5);
    if (rounded_value < 1.0 || rounded_value >= (double)scale) {
        return STATUS_PRECISION_LIMIT;
    }

    restored_value = rounded_value / (double)scale;
    if (fabs(value - restored_value) > epsilon) {
        return STATUS_PRECISION_LIMIT;
    }

    numerator = (uintmax_t)rounded_value;
    denominator = scale;
    divisor = gcd_uintmax(numerator, denominator);
    denominator /= divisor;

    while (denominator % 2U == 0U && base % 2U == 0U) {
        denominator /= 2U;
    }

    while (denominator % 5U == 0U && base % 5U == 0U) {
        denominator /= 5U;
    }

    *result = 0;
    if (denominator == 1U) {
        *result = 1;
    }

    return STATUS_OK;
}

int check_fractions(const unsigned int base, const size_t count, const double epsilon, int *results, ...) {
    va_list arguments;
    uintmax_t scale;
    size_t i;
    int status;
    int temporary_result;

    if (base < 2U || count == 0U || results == NULL) {
        return STATUS_INVALID_ARGUMENTS;
    }

    status = build_decimal_scale(epsilon, &scale);
    if (status != STATUS_OK) {
        return status;
    }

    va_start(arguments, results);
    status = STATUS_OK;
    for (i = 0U; i < count; i++) {
        double value = va_arg(arguments, double);
        status = calculate_result(value, base, epsilon, scale, &temporary_result);
        if (status != STATUS_OK) {
            break;
        }
    }
    va_end(arguments);

    if (status != STATUS_OK) {
        return status;
    }

    va_start(arguments, results);
    for (i = 0U; i < count; i++) {
        double value = va_arg(arguments, double);
        status = calculate_result(value, base, epsilon, scale, &results[i]);
        if (status != STATUS_OK) {
            break;
        }
    }
    va_end(arguments);

    return status;
}

int main(void) {
    const double epsilon = 1e-12;
    const size_t count = 4U;
    const double fractions_base_2[4] = {0.5, 0.25, 0.125, 0.1};
    const double fractions_base_5[4] = {0.2, 0.04, 0.125, 0.1};
    const double fractions_base_10[4] = {0.1, 0.2, 0.125, 0.375};
    int results_base_2[4];
    int results_base_5[4];
    int results_base_10[4];
    int status;
    size_t i;

    status = check_fractions(2U, count, epsilon, results_base_2, 0.5, 0.25, 0.125, 0.1);
    if (status != STATUS_OK) {
        printf("Ошибка при обработке основания 2. Код: %d\n", status);
        return 1;
    }

    status = check_fractions(5U, count, epsilon, results_base_5, 0.2, 0.04, 0.125, 0.1);
    if (status != STATUS_OK) {
        printf("Ошибка при обработке основания 5. Код: %d\n", status);
        return 1;
    }

    status = check_fractions(10U, count, epsilon, results_base_10, 0.1, 0.2, 0.125, 0.375);
    if (status != STATUS_OK) {
        printf("Ошибка при обработке основания 10. Код: %d\n", status);
        return 1;
    }

    printf("Основание 2:\n");
    for (i = 0U; i < count; i++) {
        printf("%.12g: ", fractions_base_2[i]);
        if (results_base_2[i] == 1) {
            printf("конечное представление\n");
        } else {
            printf("бесконечное представление\n");
        }
    }

    printf("\nОснование 5:\n");
    for (i = 0U; i < count; i++) {
        printf("%.12g: ", fractions_base_5[i]);
        if (results_base_5[i] == 1) {
            printf("конечное представление\n");
        } else {
            printf("бесконечное представление\n");
        }
    }

    printf("\nОснование 10:\n");
    for (i = 0U; i < count; i++) {
        printf("%.12g: ", fractions_base_10[i]);
        if (results_base_10[i] == 1) {
            printf("конечное представление\n");
        } else {
            printf("бесконечное представление\n");
        }
    }

    return 0;
}
