
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stddef.h>

#define POLY_SUCCESS 0
#define POLY_INVALID_ARGUMENT 1
#define POLY_MEMORY_ERROR 2
#define POLY_INVALID_COEFFICIENT 3
#define POLY_ARITHMETIC_ERROR 4

int reexpand_polynomial(double epsilon, double a, double **result, int degree, ...) {
    if (result == NULL)
    {
        return POLY_INVALID_ARGUMENT;
    }

    *result = NULL;

    if (!isfinite(epsilon) || epsilon <= 0.0 || !isfinite(a) || degree < 0) {
        return POLY_INVALID_ARGUMENT;
    }

    size_t count = (size_t)degree + 1;

    if (count > (size_t)-1 / sizeof(double)) {
        return POLY_INVALID_ARGUMENT;
    }

    double *coefficients = malloc(count * sizeof(double));

    if (coefficients == NULL) {
        return POLY_MEMORY_ERROR;
    }

    va_list arguments;
    va_start(arguments, degree);

    int valid = 1;

    for (size_t i = 0; i < count; ++i) {
        double coefficient = va_arg(arguments, double);

        if (!isfinite(coefficient)) {
            valid = 0;
            break;
        }

        coefficients[i] = coefficient;
    }

    va_end(arguments);

    if (valid == 0) {
        free(coefficients);
        return POLY_INVALID_COEFFICIENT;
    }

    for (size_t i = count - 1; i > 0; --i) {
        size_t first = i - 1;

        for (size_t j = first; j < count - 1; ++j) {
            double value = coefficients[j] + a * coefficients[j + 1];
            if (!isfinite(value)) {
                free(coefficients);
                return POLY_ARITHMETIC_ERROR;
            }

            coefficients[j] = value;
        }
    }

    for (size_t i = 0; i < count; ++i) {
        if (fabs(coefficients[i]) <= epsilon) {
            coefficients[i] = 0.0;
        }
    }

    *result = coefficients;

    return POLY_SUCCESS;
}

int main(void) {
    const double epsilon = 1e-9;
    const double a = 2.0;
    const int degree = 2;

    double *result = NULL;

    int status = reexpand_polynomial(
        epsilon,
        a,
        &result,
        degree,
        1.0,
        2.0,
        3.0
    );

    if (status == POLY_SUCCESS) {
        printf("Coefficients of the resulting polynomial:\n");

        for (size_t i = 0; i <= (size_t)degree; ++i) {
            printf("g[%zu] = %.10g\n", i, result[i]);
        }

        printf("\nResulting polynomial:\n");

        printf("f(x) = %.10g", result[0]);

        for (size_t i = 1; i <= (size_t)degree; ++i) {
            printf(" + %.10g * (x - %.10g)^%zu",
                   result[i], a, i);
        }

        printf("\n");
    }
    else if (status == POLY_INVALID_ARGUMENT) {
        printf("Error: invalid function arguments.\n");
    }
    else if (status == POLY_MEMORY_ERROR) {
        printf("Error: memory allocation failed.\n");
    }
    else if (status == POLY_INVALID_COEFFICIENT) {
        printf("Error: invalid polynomial coefficient.\n");
    }
    else if (status == POLY_ARITHMETIC_ERROR) {
        printf("Error: arithmetic overflow.\n");
    }

    free(result);

    if (status == POLY_SUCCESS) {
        return 0;
    }

    return 1;
}
