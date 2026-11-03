#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <float.h>
#include <math.h>

#define STATUS_OK 0
#define STATUS_INVALID_ARGUMENT 1
#define STATUS_INVALID_VECTOR 2
#define STATUS_INVALID_PARAMETER 3
#define STATUS_MEMORY_ERROR 4
#define STATUS_NUMERIC_ERROR 5
#define STATUS_NOT_POSITIVE_DEFINITE 6
#define STATUS_CALLBACK_ERROR 7
#define STATUS_OUTPUT_ERROR 8

typedef struct {
    size_t dimension;
    const double *coordinates;
} Vector;

typedef struct {
    size_t dimension;
    const double *elements;
} MatrixContext;

typedef int (*NormFunction)(const Vector *, const void *, double, double *);

typedef struct {
    NormFunction function;
    const void *context;
} NormTask;

typedef struct {
    double norm_value;
    size_t count;
    size_t *indices;
    Vector *vectors;
} NormResult;

static int validate_vector(const Vector *vector, size_t dimension) {
    size_t i;

    if (vector == NULL || vector->coordinates == NULL || dimension == 0 || vector->dimension != dimension) {
        return STATUS_INVALID_VECTOR;
    }

    for (i = 0; i < dimension; ++i) {
        if (!isfinite(vector->coordinates[i])) {
            return STATUS_INVALID_VECTOR;
        }
    }

    return STATUS_OK;
}

int norm_infinity(const Vector *vector, const void *context, double epsilon, double *result) {
    size_t i;
    double maximum;

    if (vector == NULL || result == NULL || context != NULL || !isfinite(epsilon) || epsilon < 0.0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_vector(vector, vector->dimension) != STATUS_OK) {
        return STATUS_INVALID_VECTOR;
    }

    maximum = 0.0;

    for (i = 0; i < vector->dimension; ++i) {
        double current = fabs(vector->coordinates[i]);
        double difference = current - maximum;

        if (difference > epsilon) {
            maximum = current;
        } else if (fabs(difference) <= epsilon) {
            maximum = fmax(maximum, current);
        }
    }

    *result = maximum;
    return STATUS_OK;
}

int norm_p(const Vector *vector, const void *context, double epsilon, double *result) {
    const double *p;
    long double maximum;
    long double sum;
    long double norm_value;
    size_t i;

    if (vector == NULL || context == NULL || result == NULL || !isfinite(epsilon) || epsilon < 0.0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_vector(vector, vector->dimension) != STATUS_OK) {
        return STATUS_INVALID_VECTOR;
    }

    p = (const double *)context;

    if (!isfinite(*p) || *p < 1.0) {
        return STATUS_INVALID_PARAMETER;
    }

    maximum = 0.0L;

    for (i = 0; i < vector->dimension; ++i) {
        maximum = fmaxl(maximum, fabsl((long double)vector->coordinates[i]));
    }

    if (maximum == 0.0L) {
        *result = 0.0;
        return STATUS_OK;
    }

    sum = 0.0L;

    for (i = 0; i < vector->dimension; ++i) {
        long double scaled_coordinate = fabsl((long double)vector->coordinates[i]) / maximum;
        long double term = powl(scaled_coordinate, (long double)*p);

        if (!isfinite(term)) {
            return STATUS_NUMERIC_ERROR;
        }

        sum += term;

        if (!isfinite(sum)) {
            return STATUS_NUMERIC_ERROR;
        }
    }

    norm_value = maximum * powl(sum, 1.0L / (long double)*p);

    if (!isfinite(norm_value) || norm_value > (long double)DBL_MAX) {
        return STATUS_NUMERIC_ERROR;
    }

    *result = (double)norm_value;
    return STATUS_OK;
}

static int validate_positive_definite_matrix(const MatrixContext *matrix, size_t dimension, double epsilon) {
    size_t element_count;
    size_t i;
    size_t j;
    size_t k;
    long double scale;
    long double *lower;
    int status;

    if (matrix == NULL || matrix->elements == NULL || matrix->dimension != dimension || dimension == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    if (dimension > SIZE_MAX / dimension) {
        return STATUS_INVALID_ARGUMENT;
    }

    element_count = dimension * dimension;

    if (element_count > SIZE_MAX / sizeof(long double)) {
        return STATUS_MEMORY_ERROR;
    }

    scale = 0.0L;

    for (i = 0; i < element_count; ++i) {
        if (!isfinite(matrix->elements[i])) {
            return STATUS_INVALID_PARAMETER;
        }

        scale = fmaxl(scale, fabsl((long double)matrix->elements[i]));
    }

    if (scale == 0.0L) {
        return STATUS_NOT_POSITIVE_DEFINITE;
    }

    for (i = 0; i < dimension; ++i) {
        for (j = i + 1; j < dimension; ++j) {
            long double left = (long double)matrix->elements[i * dimension + j] / scale;
            long double right = (long double)matrix->elements[j * dimension + i] / scale;

            if (fabsl(left - right) > (long double)epsilon) {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    lower = (long double *)calloc(element_count, sizeof(long double));

    if (lower == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    status = STATUS_OK;

    for (i = 0; i < dimension && status == STATUS_OK; ++i) {
        for (j = 0; j <= i && status == STATUS_OK; ++j) {
            long double sum = 0.5L * (
                (long double)matrix->elements[i * dimension + j] / scale +
                (long double)matrix->elements[j * dimension + i] / scale
            );

            for (k = 0; k < j; ++k) {
                sum -= lower[i * dimension + k] * lower[j * dimension + k];
            }

            if (i == j) {
                if (sum <= (long double)epsilon) {
                    status = STATUS_NOT_POSITIVE_DEFINITE;
                } else {
                    lower[i * dimension + j] = sqrtl(sum);
                }
            } else {
                if (lower[j * dimension + j] <= 0.0L) {
                    status = STATUS_NOT_POSITIVE_DEFINITE;
                } else {
                    lower[i * dimension + j] = sum / lower[j * dimension + j];
                }
            }
        }
    }

    free(lower);
    return status;
}

int norm_A(const Vector *vector, const void *context, double epsilon, double *result) {
    const MatrixContext *matrix;
    long double scale;
    long double quadratic_form;
    long double vector_square_sum;
    long double norm_value;
    size_t i;
    size_t j;
    size_t dimension;
    int status;

    if (vector == NULL || context == NULL || result == NULL || !isfinite(epsilon) || epsilon < 0.0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (validate_vector(vector, vector->dimension) != STATUS_OK) {
        return STATUS_INVALID_VECTOR;
    }

    matrix = (const MatrixContext *)context;
    dimension = vector->dimension;
    status = validate_positive_definite_matrix(matrix, dimension, epsilon);

    if (status != STATUS_OK) {
        return status;
    }

    scale = 0.0L;

    for (i = 0; i < dimension * dimension; ++i) {
        scale = fmaxl(scale, fabsl((long double)matrix->elements[i]));
    }

    quadratic_form = 0.0L;
    vector_square_sum = 0.0L;

    for (i = 0; i < dimension; ++i) {
        long double coordinate = (long double)vector->coordinates[i];
        vector_square_sum += coordinate * coordinate;

        for (j = 0; j < dimension; ++j) {
            long double a_ij = (long double)matrix->elements[i * dimension + j] / scale;
            long double a_ji = (long double)matrix->elements[j * dimension + i] / scale;
            long double symmetric_element = 0.5L * (a_ij + a_ji);

            quadratic_form += coordinate * symmetric_element * (long double)vector->coordinates[j];
        }
    }

    if (!isfinite(quadratic_form) || !isfinite(vector_square_sum)) {
        return STATUS_NUMERIC_ERROR;
    }

    if (quadratic_form < 0.0L) {
        if (fabsl(quadratic_form) <= (long double)epsilon * vector_square_sum) {
            quadratic_form = 0.0L;
        } else {
            return STATUS_NUMERIC_ERROR;
        }
    }

    quadratic_form *= scale;

    if (!isfinite(quadratic_form) || quadratic_form < 0.0L) {
        return STATUS_NUMERIC_ERROR;
    }

    norm_value = sqrtl(quadratic_form);

    if (!isfinite(norm_value) || norm_value > (long double)DBL_MAX) {
        return STATUS_NUMERIC_ERROR;
    }

    *result = (double)norm_value;
    return STATUS_OK;
}

void free_norm_results(NormResult *results, size_t norm_count) {
    size_t i;

    if (results != NULL) {
        for (i = 0; i < norm_count; ++i) {
            free(results[i].indices);
            free(results[i].vectors);
        }

        free(results);
    }
}

int find_longest_vectors(size_t dimension, size_t vector_count, size_t norm_count, double epsilon, NormResult **output, ...) {
    Vector *vectors;
    NormTask *tasks;
    double *values;
    NormResult *results;
    va_list arguments;
    size_t i;
    size_t j;
    int status;

    if (output == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *output = NULL;

    if (dimension == 0 || vector_count == 0 || norm_count == 0 || !isfinite(epsilon) || epsilon < 0.0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (vector_count > SIZE_MAX / sizeof(Vector) || norm_count > SIZE_MAX / sizeof(NormTask) || vector_count > SIZE_MAX / sizeof(double) || norm_count > SIZE_MAX / sizeof(NormResult)) {
        return STATUS_MEMORY_ERROR;
    }

    vectors = (Vector *)calloc(vector_count, sizeof(Vector));
    tasks = (NormTask *)calloc(norm_count, sizeof(NormTask));
    values = NULL;
    results = NULL;

    if (vectors == NULL || tasks == NULL) {
        free(vectors);
        free(tasks);
        return STATUS_MEMORY_ERROR;
    }

    va_start(arguments, output);

    status = STATUS_OK;

    for (i = 0; i < vector_count && status == STATUS_OK; ++i) {
        vectors[i] = va_arg(arguments, Vector);
        status = validate_vector(&vectors[i], dimension);
    }

    for (i = 0; i < norm_count && status == STATUS_OK; ++i) {
        tasks[i].function = va_arg(arguments, NormFunction);
        tasks[i].context = va_arg(arguments, const void *);

        if (tasks[i].function == NULL) {
            status = STATUS_INVALID_ARGUMENT;
        }
    }

    va_end(arguments);

    if (status == STATUS_OK) {
        values = (double *)malloc(vector_count * sizeof(double));
        results = (NormResult *)calloc(norm_count, sizeof(NormResult));

        if (values == NULL || results == NULL) {
            status = STATUS_MEMORY_ERROR;
        }
    }

    for (i = 0; i < norm_count && status == STATUS_OK; ++i) {
        double maximum;
        size_t result_count;

        for (j = 0; j < vector_count && status == STATUS_OK; ++j) {
            status = tasks[i].function(&vectors[j], tasks[i].context, epsilon, &values[j]);

            if (status == STATUS_OK && (!isfinite(values[j]) || values[j] < 0.0)) {
                status = STATUS_CALLBACK_ERROR;
            }
        }

        if (status == STATUS_OK) {
            maximum = values[0];

            for (j = 1; j < vector_count; ++j) {
                double difference = values[j] - maximum;

                if (difference > epsilon) {
                    maximum = values[j];
                } else if (fabs(difference) <= epsilon) {
                    maximum = fmax(maximum, values[j]);
                }
            }

            result_count = 0;

            for (j = 0; j < vector_count; ++j) {
                if (fabs(values[j] - maximum) <= epsilon) {
                    ++result_count;
                }
            }

            if (result_count > SIZE_MAX / sizeof(size_t) || result_count > SIZE_MAX / sizeof(Vector)) {
                status = STATUS_MEMORY_ERROR;
            } else {
                results[i].indices = (size_t *)malloc(result_count * sizeof(size_t));
                results[i].vectors = (Vector *)malloc(result_count * sizeof(Vector));

                if (results[i].indices == NULL || results[i].vectors == NULL) {
                    status = STATUS_MEMORY_ERROR;
                } else {
                    size_t position = 0;

                    for (j = 0; j < vector_count; ++j) {
                        if (fabs(values[j] - maximum) <= epsilon) {
                            results[i].indices[position] = j;
                            results[i].vectors[position] = vectors[j];
                            ++position;
                        }
                    }

                    results[i].norm_value = maximum;
                    results[i].count = result_count;
                }
            }
        }
    }

    free(vectors);
    free(tasks);
    free(values);

    if (status != STATUS_OK) {
        free_norm_results(results, norm_count);
        return status;
    }

    *output = results;
    return STATUS_OK;
}

const char *status_message(int status)
{
    if (status == STATUS_OK) {
        return "успешно";
    } else if (status == STATUS_INVALID_ARGUMENT) {
        return "неверные аргументы функции";
    } else if (status == STATUS_INVALID_VECTOR) {
        return "некорректный вектор";
    } else if (status == STATUS_INVALID_PARAMETER) {
        return "некорректные параметры нормы или матрицы";
    } else if (status == STATUS_MEMORY_ERROR) {
        return "ошибка выделения памяти";
    } else if (status == STATUS_NUMERIC_ERROR) {
        return "численная ошибка или переполнение";
    } else if (status == STATUS_NOT_POSITIVE_DEFINITE) {
        return "матрица не является положительно определенной";
    } else if (status == STATUS_CALLBACK_ERROR) {
        return "функция нормы вернула некорректное значение";
    } else if (status == STATUS_OUTPUT_ERROR) {
        return "ошибка записи в поток вывода";
    }

    return "неизвестная ошибка";
}

int print_results(const NormResult *results, size_t norm_count, const char *const *names) {
    size_t i;
    size_t j;
    size_t k;

    if (results == NULL || names == NULL || norm_count == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < norm_count; ++i) {
        if (names[i] == NULL || !isfinite(results[i].norm_value) || results[i].count == 0 || results[i].indices == NULL || results[i].vectors == NULL) {
            return STATUS_INVALID_ARGUMENT;
        }

        for (j = 0; j < results[i].count; ++j) {
            if (results[i].vectors[j].coordinates == NULL || results[i].vectors[j].dimension == 0) {
                return STATUS_INVALID_VECTOR;
            }

            for (k = 0; k < results[i].vectors[j].dimension; ++k) {
                if (!isfinite(results[i].vectors[j].coordinates[k])) {
                    return STATUS_INVALID_VECTOR;
                }
            }
        }
    }

    for (i = 0; i < norm_count; ++i) {
        if (printf("Норма %s, максимальное значение: %.10f\n", names[i], results[i].norm_value) < 0) {
            return STATUS_OUTPUT_ERROR;
        }

        for (j = 0; j < results[i].count; ++j) {
            if (printf("Вектор %zu: (", results[i].indices[j] + 1) < 0) {
                return STATUS_OUTPUT_ERROR;
            }

            for (k = 0; k < results[i].vectors[j].dimension; ++k) {
                if (printf("%.3f", results[i].vectors[j].coordinates[k]) < 0) {
                    return STATUS_OUTPUT_ERROR;
                }

                if (k + 1 < results[i].vectors[j].dimension && printf(", ") < 0) {
                    return STATUS_OUTPUT_ERROR;
                }
            }

            if (printf(")\n") < 0) {
                return STATUS_OUTPUT_ERROR;
            }
        }
    }

    return STATUS_OK;
}

int main(void) {
    const double coordinates_1[] = {3.0, 0.0, 0.0};
    const double coordinates_2[] = {0.0, 3.0, 0.0};
    const double coordinates_3[] = {1.0, 2.0, 2.0};
    const double coordinates_4[] = {0.0, 0.0, -3.0};
    const double matrix_elements[] = {
        2.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    };
    const double p = 2.0;
    const void *infinity_context = NULL;
    const void *p_context = &p;
    const MatrixContext matrix = {3, matrix_elements};
    const void *matrix_context = &matrix;
    const Vector vector_1 = {3, coordinates_1};
    const Vector vector_2 = {3, coordinates_2};
    const Vector vector_3 = {3, coordinates_3};
    const Vector vector_4 = {3, coordinates_4};
    const char *names[] = {"бесконечности", "p = 2", "A"};
    const double epsilon = 1e-9;
    NormResult *results = NULL;
    int status;

    status = find_longest_vectors(
        3,
        4,
        3,
        epsilon,
        &results,
        vector_1,
        vector_2,
        vector_3,
        vector_4,
        norm_infinity, infinity_context,
        norm_p, p_context,
        norm_A, matrix_context
    );

    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка: %s\n", status_message(status));
        free_norm_results(results, 3);
        return 1;
    }

    status = print_results(results, 3, names);
    free_norm_results(results, 3);

    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка: %s\n", status_message(status));
        return 1;
    }

    return 0;
}
