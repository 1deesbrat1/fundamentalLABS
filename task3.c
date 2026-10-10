#include <stddef.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int check_multiple(long long a, long long b, int* result) {
    if (result == NULL || a == 0 || b == 0) {
        return 1;
    }

    if (b == 1 || b == -1) {
        *result = 1;
        return 0;
    }

    if (a % b == 0) {
        *result = 1;
    } else {
        *result = 0;
    }


    return 0;
}

int check_right_triangle(double epsilon, double a, double b, double c, int *result) {
    long double x = a;
    long double y = b;
    long double z = c;
    long double temp;
    long double left, right;

    if (result == NULL ||
        !isfinite(epsilon) || epsilon <= 0.0 ||
        !isfinite(a) || !isfinite(b) || !isfinite(c)) {
        return 1;
    }

    if (a <= 0.0 || b <= 0.0 || c <= 0.0) {
        *result = 0;
        return 0;
    }

    if (x > y) {
        temp = y;
        y = x;
        x = temp;
    }

    if (y > z) {
        temp = y;
        y = z;
        z = temp;
    }

    if (x > y) {
        temp = x;
        x = y;
        y = temp;
    }

    if (x + y <= z) {
        *result = 0;
        return 0;
    }

    left = x * x + y * y;
    right = z * z;

    if (!isfinite(left) || !isfinite(right)) {
        return 2;
    }

    if (fabsl(left - right) <= epsilon) {
        *result = 1;
    } else {
        *result = 0;
    }


    return 0;
}


int solve_quadrantic(double epsilon, double a, double b, double c, double* x1, double* x2, int* root_count) {
    long double d;
    long double root1, root2;

    if (x1 == NULL || x2 == NULL || root_count == NULL || !isfinite(epsilon) || epsilon <= 0.0 || !isfinite(a) || !isfinite(b) || !isfinite(c)) {
        return 1;
    }

    if (fabs(a) <= epsilon) {

        if (fabs(b) <= epsilon) {
            
            if (fabs(c) <= epsilon) {
                *root_count = -1;
            } else {
                *root_count = 0;
            }

            return 0;
        }

        root1 = -(long double)c / b;

        if (fabsl(root1) <= epsilon) {
            root1 = 0.0L;
        }

        if (!isfinite(root1) || fabsl(root1) > DBL_MAX) {
            return 2;
        }

        *x1 = (double)root1;
        *root_count = 1;

        return 0;
    }

    d = (long double)b * b - 4.0L * (long double)a * c;

    if (!isfinite(d)) {
        return 2;
    }

    if (d < -epsilon) {
        *root_count = 0;
        return 0;
    }

    if (fabsl(d) <= epsilon) {
        root1 = -(long double)b / (2.0L * a);

        if (fabsl(root1) <= epsilon) {
            root1 = 0.0L;
        }

        if (!isfinite(root1) || fabsl(root1) > DBL_MAX) {
            return 2;
        }

        *x1 = (double)root1;
        *root_count = 1;

        return 0;
    }

    root1 = (-(long double)b + sqrtl(d)) / (2.0L * a);
    root2 = (-(long double)b - sqrtl(d)) / (2.0L * a);

    if (fabsl(root1) <= epsilon) {
        root1 = 0.0L;
    }

    if (fabsl(root2) <= epsilon) {
        root2 = 0.0L;
    }

    if (!isfinite(root1) || !isfinite(root2) || fabsl(root1) > DBL_MAX || fabsl(root2) > DBL_MAX) {
        return 2;
    }

    *x1 = root1;
    *x2 = root2;
    *root_count = 2;

    return 0;

}

int get_unique_permutations(double epsilon, double a, double b, double c, double permutations[6][3], int* count) {
    double all[6][3] = {
        {a, b, c},
        {a, c, b},
        {b, a, c},
        {b, c, a},
        {c, a, b},
        {c, b, a}
    };

    if (permutations == NULL || count == NULL || !isfinite(a) || !isfinite(b) || !isfinite(c) || !isfinite(epsilon) || epsilon <= 0.0) {
        return 1;
    } 

    *count = 0;


    for (int i = 0; i < 6; i++) {

        int repeated = 0;

        for (int j = 0; j < *count; j++) {
            if (fabs(all[i][0] - permutations[j][0]) <= epsilon &&
                fabs(all[i][1] - permutations[j][1]) <= epsilon &&
                fabs(all[i][2] - permutations[j][2]) <= epsilon) {
                    
                    repeated = 1;
                    break;
                }
        }

        if (repeated == 0) {
            permutations[*count][0] = all[i][0];
            permutations[*count][1] = all[i][1];
            permutations[*count][2] = all[i][2];

            (*count)++;
        }
    }

    return 0;
}


int parse_double(const char* str, double* result) {
    char* end;
    double value;

    if (str == NULL || result == NULL || str[0] == '\0') {
        return 1;
    }


    value = strtod(str, &end);

    if (end == str || *end != '\0') {
        return 1;
    }

    if (!isfinite(value)) {
        return 1;
    }

    *result = value;

    return 0;
}

int parse_long_long(const char* str, long long* result) {
    int negative = 0;
    unsigned long long value = 0;
    unsigned long long limit;

    int i = 0;

    if (str == NULL || result == NULL || str[0] == '\0') {
        return 1;
    }

    if (str[i] == '-') {
        negative = 1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }


    if (str[i] == '\0') {
        return 1;
    }


    if (negative) {
        limit = (unsigned long long)LLONG_MAX + 1ULL;
    } else {
        limit = (unsigned long long)LLONG_MAX;
    }

    while (str[i] != '\0') {
        int digit;

        if (str[i] < '0' || str[i] > '9') {
            return 1;
        }

        digit = str[i] - '0';

        if (value > (limit - digit) / 10ULL) {
            return 2;
        }


        value = value * 10ULL + digit;
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




int main(int argc, char* argv[]) {
    long long first;
    long long second;
    int result;

    if (argc < 2) {
        printf("Не указан флаг.\n");
        return 1;
    }

    if (strcmp(argv[1], "-m") == 0 || strcmp(argv[1], "/m") == 0) {
        
        if (argc != 4) {
            printf("Для флага -m нужны два целых числа.\n");
            return 1;
        }

        if (parse_long_long(argv[2], &first) != 0 || parse_long_long(argv[3], &second) != 0) {
            printf("Параметры должны быть целыми числами.\n");
            return 1;
        }

        if (first == 0 || second == 0) {
            printf("Числа должны быть ненулевыми.\n");
            return 1;
        }

        if (check_multiple(first, second, &result) != 0) {
            printf("Ошибка вычисления.\n");
            return 1;
        }

        if (result == 1) {
            printf("%lld кратно %lld.\n", first, second);
        } else {
            printf("%lld не кратно %lld.\n", first, second);
        }
    } else if (strcmp(argv[1], "-t") == 0 || strcmp(argv[1], "/t") == 0) {
        double epsilon;
        double a, b, c;
        int result;
        if (argc != 6) {
            printf("Для флага нужны эпсилон и три стороны.\n");
            return 1;
        }

        if (parse_double(argv[2], &epsilon) != 0 ||
            parse_double(argv[3], &a) != 0 ||
            parse_double(argv[4], &b) != 0 ||
            parse_double(argv[5], &c) != 0) {
                printf("Параметры долнжы быть вещественными числами.\n");
                return 1;
        }

        if (epsilon <= 0.0) {
            printf("Эпсилон должен быть положительным.\n");
            return 1;
        }

        if (check_right_triangle(epsilon, a, b, c, &result) != 0) {
            printf("Ошибка вычисления.\n");
            return 1;
        }

        if (result == 1) {
            printf("Можно построить прямоугольный треугольник.\n");
        } else {
            printf("Нельзя построить прямоугольный треугольник.\n");
        }


    } else if (strcmp(argv[1], "-q") == 0 || strcmp(argv[1], "/q") == 0) {
        double epsilon;
        double a, b, c;

        double permutations[6][3];
        int count;

        double x1, x2;
        int root_count;
        int status;

        if (argc != 6) {
            printf("Для флага нужны эпсилон и три коэффициенты.\n");
            return 1;
        }

        if (parse_double(argv[2], &epsilon) != 0 ||
            parse_double(argv[3], &a) != 0 ||
            parse_double(argv[4], &b) != 0 ||
            parse_double(argv[5], &c) != 0) {

                printf("Параметры должны быть вещественными числами.\n");
                return 1;
            }

        if (epsilon <= 0.0) {
            printf("Эпсилон должен быть положительным.\n");
            return 1;
        }

        status = get_unique_permutations(epsilon, a, b, c, permutations, &count);

        if (status != 0) {
            printf("Ошибка в перестановках.\n");
            return 1;
        }

        for (int i = 0; i < count; i++) {
            
            status = solve_quadrantic(epsilon, permutations[i][0], permutations[i][1], permutations[i][2], &x1, &x2, &root_count);

            printf("a = %.10g, b = %.10g, c = %.10g: ", permutations[i][0], permutations[i][1], permutations[i][2]);

            if (status != 0) {
                printf("Ошибка вычисления.\n");
            } else if (root_count == - 1) {
                printf("Бесконечно много решений.\n");
            } else if (root_count == 0) {
                printf("Действительных корней нет.\n");
            } else if (root_count == 1) {
                printf("x = %.10g\n", x1);
            } else if (root_count == 2) {
                printf("x1 = %.10g, x2 = %.10g\n", x1, x2);
            }
        }
    }

    else {
        printf("Неизвестный флаг.\n");
        return 1;
    }

    return 0;
}