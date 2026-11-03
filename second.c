#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#define STATUS_OK 0
#define STATUS_ARGUMENTS 1
#define STATUS_PATH 2
#define STATUS_FLAG 3
#define STATUS_INPUT_OPEN 4
#define STATUS_INPUT_READ 5
#define STATUS_INPUT_CLOSE 6
#define STATUS_OUTPUT_OPEN 7
#define STATUS_OUTPUT_WRITE 8
#define STATUS_OUTPUT_CLOSE 9
#define STATUS_MEMORY 10
#define STATUS_SIZE 11
#define STATUS_INVALID_DATA 12
#define STATUS_RECORD 13
#define STATUS_ID 14
#define STATUS_NAME 15
#define STATUS_SALARY 16
#define STATUS_RANGE 17
#define STATUS_PARAMETER 18

#define INITIAL_CAPACITY 4

typedef struct {
    unsigned long long id;
    char *name;
    char *surname;
    double salary;
} Employee;

typedef struct {
    Employee *items;
    size_t size;
    size_t capacity;
} EmployeeArray;

static int is_separator(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f';
}

static int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static int is_latin_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static int validate_name(const char *text, size_t length) {
    size_t i;

    if (text == NULL || length == 0) {
        return STATUS_NAME;
    }

    for (i = 0; i < length; ++i) {
        if (!is_latin_letter(text[i])) {
            return STATUS_NAME;
        }
    }

    return STATUS_OK;
}

static int parse_id(const char *text, size_t length, unsigned long long *id) {
    size_t i;
    unsigned long long value = 0;
    unsigned long long digit;

    if (text == NULL || id == NULL) {
        return STATUS_PARAMETER;
    }

    if (length == 0) {
        return STATUS_ID;
    }

    for (i = 0; i < length; ++i) {
        if (!is_digit(text[i])) {
            return STATUS_ID;
        }

        digit = (unsigned long long)(text[i] - '0');
        if (value > (ULLONG_MAX - digit) / 10ULL) {
            return STATUS_RANGE;
        }

        value = value * 10ULL + digit;
    }

    *id = value;
    return STATUS_OK;
}

static int parse_salary(const char *text, size_t length, double *salary) {
    char *end;
    size_t i = 0;
    size_t digits = 0;
    size_t exponent_digits = 0;
    int has_nonzero_digit = 0;
    double value;

    if (text == NULL || salary == NULL) {
        return STATUS_PARAMETER;
    }

    if (length == 0) {
        return STATUS_SALARY;
    }

    if (text[i] == '+') {
        ++i;
    }

    while (i < length && is_digit(text[i])) {
        if (text[i] != '0') {
            has_nonzero_digit = 1;
        }
        ++digits;
        ++i;
    }

    if (i < length && text[i] == '.') {
        ++i;
        while (i < length && is_digit(text[i])) {
            if (text[i] != '0') {
                has_nonzero_digit = 1;
            }
            ++digits;
            ++i;
        }
    }

    if (digits == 0) {
        return STATUS_SALARY;
    }

    if (i < length && (text[i] == 'e' || text[i] == 'E')) {
        ++i;
        if (i < length && (text[i] == '+' || text[i] == '-')) {
            ++i;
        }
        while (i < length && is_digit(text[i])) {
            ++exponent_digits;
            ++i;
        }
        if (exponent_digits == 0) {
            return STATUS_SALARY;
        }
    }

    if (i != length) {
        return STATUS_SALARY;
    }

    value = strtod(text, &end);

    if (end != text + length || !isfinite(value)) {
        return STATUS_RANGE;
    }

    if (value < 0.0 || (value == 0.0 && has_nonzero_digit)) {
        return STATUS_RANGE;
    }

    *salary = value;
    return STATUS_OK;
}

static void employee_destroy(Employee *employee) {
    if (employee != NULL) {
        free(employee->name);
        free(employee->surname);
        employee->name = NULL;
        employee->surname = NULL;
    }
}

static int parse_employee_line(const char *line, size_t length, Employee *employee) {
    const char *tokens[4];
    size_t token_lengths[4];
    size_t position = 0;
    size_t count = 0;
    size_t start;
    int status;

    if (line == NULL || employee == NULL) {
        return STATUS_PARAMETER;
    }

    employee->id = 0;
    employee->name = NULL;
    employee->surname = NULL;
    employee->salary = 0.0;

    while (position < length) {
        while (position < length && is_separator(line[position])) {
            ++position;
        }

        if (position == length) {
            break;
        }

        if (count == 4) {
            return STATUS_RECORD;
        }

        start = position;
        while (position < length && !is_separator(line[position])) {
            ++position;
        }

        tokens[count] = line + start;
        token_lengths[count] = position - start;
        ++count;
    }

    if (count != 4) {
        return STATUS_RECORD;
    }

    status = parse_id(tokens[0], token_lengths[0], &employee->id);
    if (status != STATUS_OK) {
        return status;
    }

    status = validate_name(tokens[1], token_lengths[1]);
    if (status != STATUS_OK) {
        return status;
    }

    status = validate_name(tokens[2], token_lengths[2]);
    if (status != STATUS_OK) {
        return status;
    }

    status = parse_salary(tokens[3], token_lengths[3], &employee->salary);
    if (status != STATUS_OK) {
        return status;
    }

    employee->name = malloc(token_lengths[1] + 1);
    if (employee->name == NULL) {
        return STATUS_MEMORY;
    }

    employee->surname = malloc(token_lengths[2] + 1);
    if (employee->surname == NULL) {
        employee_destroy(employee);
        return STATUS_MEMORY;
    }

    memcpy(employee->name, tokens[1], token_lengths[1]);
    employee->name[token_lengths[1]] = '\0';
    memcpy(employee->surname, tokens[2], token_lengths[2]);
    employee->surname[token_lengths[2]] = '\0';

    return STATUS_OK;
}

static int employee_array_add(EmployeeArray *array, Employee *employee) {
    size_t new_capacity;
    Employee *new_items;

    if (array == NULL || employee == NULL || employee->name == NULL || employee->surname == NULL) {
        return STATUS_PARAMETER;
    }

    if (array->size > array->capacity || ((array->items == NULL) != (array->capacity == 0))) {
        return STATUS_PARAMETER;
    }

    if (array->size == array->capacity) {
        if (array->capacity == 0) {
            new_capacity = INITIAL_CAPACITY;
        } else {
            if (array->capacity > SIZE_MAX / 2) {
                return STATUS_SIZE;
            }
            new_capacity = array->capacity * 2;
        }

        if (new_capacity > SIZE_MAX / sizeof(*array->items)) {
            return STATUS_SIZE;
        }

        new_items = realloc(array->items, new_capacity * sizeof(*array->items));
        if (new_items == NULL) {
            return STATUS_MEMORY;
        }

        array->items = new_items;
        array->capacity = new_capacity;
    }

    array->items[array->size] = *employee;
    ++array->size;
    employee->name = NULL;
    employee->surname = NULL;

    return STATUS_OK;
}

static void employee_array_destroy(EmployeeArray *array) {
    size_t i;

    if (array != NULL) {
        for (i = 0; i < array->size; ++i) {
            employee_destroy(&array->items[i]);
        }
        free(array->items);
        array->items = NULL;
        array->size = 0;
        array->capacity = 0;
    }
}

static int read_line(FILE *input, char **line, size_t *capacity, size_t *length, int *has_line) {
    int character;
    size_t required;
    size_t new_capacity;
    char *new_line;

    if (input == NULL || line == NULL || capacity == NULL || length == NULL || has_line == NULL) {
        return STATUS_PARAMETER;
    }

    *length = 0;
    *has_line = 0;

    while (1) {
        character = fgetc(input);
        if (character == EOF || character == '\n') {
            break;
        }

        if (character == '\0') {
            return STATUS_INVALID_DATA;
        }

        if (*length > SIZE_MAX - 2) {
            return STATUS_SIZE;
        }

        required = *length + 2;
        if (required > *capacity) {
            if (*capacity == 0) {
                new_capacity = 128;
            } else if (*capacity > SIZE_MAX / 2) {
                new_capacity = required;
            } else {
                new_capacity = *capacity * 2;
            }

            if (new_capacity < required) {
                new_capacity = required;
            }

            new_line = realloc(*line, new_capacity);
            if (new_line == NULL) {
                return STATUS_MEMORY;
            }

            *line = new_line;
            *capacity = new_capacity;
        }

        (*line)[*length] = (char)character;
        ++(*length);
    }

    if (character == EOF && ferror(input)) {
        return STATUS_INPUT_READ;
    }

    if (character == EOF && *length == 0) {
        return STATUS_OK;
    }

    *has_line = 1;

    if (*length > 0 && (*line)[*length - 1] == '\r') {
        --(*length);
    }

    if (*capacity < *length + 1) {
        new_line = realloc(*line, *length + 1);
        if (new_line == NULL) {
            return STATUS_MEMORY;
        }
        *line = new_line;
        *capacity = *length + 1;
    }

    (*line)[*length] = '\0';
    return STATUS_OK;
}

static int read_employees(const char *path, EmployeeArray *employees) {
    FILE *input;
    char *line = NULL;
    size_t capacity = 0;
    size_t length = 0;
    int has_line = 0;
    int status = STATUS_OK;

    if (path == NULL || employees == NULL) {
        return STATUS_PARAMETER;
    }

    if (path[0] == '\0') {
        return STATUS_PATH;
    }

    if (employees->size > employees->capacity || ((employees->items == NULL) != (employees->capacity == 0))) {
        return STATUS_PARAMETER;
    }

    input = fopen(path, "r");
    if (input == NULL) {
        return STATUS_INPUT_OPEN;
    }

    while (status == STATUS_OK) {
        status = read_line(input, &line, &capacity, &length, &has_line);
        if (status != STATUS_OK || !has_line) {
            break;
        }

        {
            Employee employee = {0, NULL, NULL, 0.0};
            status = parse_employee_line(line, length, &employee);
            if (status == STATUS_OK) {
                status = employee_array_add(employees, &employee);
            }
            employee_destroy(&employee);
        }
    }

    free(line);

    if (fclose(input) != 0 && status == STATUS_OK) {
        status = STATUS_INPUT_CLOSE;
    }

    return status;
}

static int compare_employee_ascending(const void *left_pointer, const void *right_pointer) {
    const Employee *left = left_pointer;
    const Employee *right = right_pointer;
    int result;

    if (left->salary < right->salary) {
        return -1;
    }
    if (left->salary > right->salary) {
        return 1;
    }

    result = strcmp(left->surname, right->surname);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }

    result = strcmp(left->name, right->name);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }

    if (left->id < right->id) {
        return -1;
    }
    if (left->id > right->id) {
        return 1;
    }

    return 0;
}

static int compare_employee_descending(const void *left, const void *right) {
    return -compare_employee_ascending(left, right);
}

static int sort_employees(EmployeeArray *employees, int descending) {
    if (employees == NULL || (descending != 0 && descending != 1)) {
        return STATUS_PARAMETER;
    }

    if (employees->size > employees->capacity || ((employees->items == NULL) != (employees->capacity == 0))) {
        return STATUS_PARAMETER;
    }

    {
        size_t i;
        for (i = 0; i < employees->size; ++i) {
            if (employees->items[i].name == NULL || employees->items[i].surname == NULL ||
                !isfinite(employees->items[i].salary) || employees->items[i].salary < 0.0) {
                return STATUS_RECORD;
            }
        }
    }

    if (employees->size > 1) {
        if (descending == 1) {
            qsort(employees->items, employees->size, sizeof(employees->items[0]), compare_employee_descending);
        } else {
            qsort(employees->items, employees->size, sizeof(employees->items[0]), compare_employee_ascending);
        }
    }

    return STATUS_OK;
}

static int write_employees(const char *path, const EmployeeArray *employees) {
    FILE *output;
    size_t i;
    int status = STATUS_OK;

    if (path == NULL || employees == NULL) {
        return STATUS_PARAMETER;
    }
    if (path[0] == '\0') {
        return STATUS_PATH;
    }

    if (employees->size > employees->capacity || ((employees->items == NULL) != (employees->capacity == 0))) {
        return STATUS_PARAMETER;
    }

    output = fopen(path, "w");
    if (output == NULL) {
        return STATUS_OUTPUT_OPEN;
    }

    for (i = 0; i < employees->size; ++i) {
        if (fprintf(output, "%llu %s %s %.17g\n", employees->items[i].id,
                    employees->items[i].name, employees->items[i].surname,
                    employees->items[i].salary) < 0) {
            status = STATUS_OUTPUT_WRITE;
            break;
        }
    }

    if (fclose(output) != 0 && status == STATUS_OK) {
        status = STATUS_OUTPUT_CLOSE;
    }

    return status;
}

static const char *status_message(int status) {
    if (status == STATUS_ARGUMENTS) {
        return "Неверное количество аргументов. Использование: employee_sort <input> <-a|-d|/a|/d> <output>";
    }
    if (status == STATUS_PATH) {
        return "Путь к файлу не должен быть пустым.";
    }
    if (status == STATUS_FLAG) {
        return "Недопустимый флаг сортировки.";
    }
    if (status == STATUS_INPUT_OPEN) {
        return "Не удалось открыть входной файл.";
    }
    if (status == STATUS_INPUT_READ) {
        return "Ошибка чтения входного файла.";
    }
    if (status == STATUS_INPUT_CLOSE) {
        return "Ошибка закрытия входного файла.";
    }
    if (status == STATUS_OUTPUT_OPEN) {
        return "Не удалось открыть выходной файл.";
    }
    if (status == STATUS_OUTPUT_WRITE) {
        return "Ошибка записи в выходной файл.";
    }
    if (status == STATUS_OUTPUT_CLOSE) {
        return "Ошибка закрытия выходного файла.";
    }
    if (status == STATUS_MEMORY) {
        return "Не удалось выделить память.";
    }
    if (status == STATUS_SIZE) {
        return "Превышен допустимый размер данных.";
    }
    if (status == STATUS_INVALID_DATA) {
        return "Входной файл содержит нулевой байт.";
    }
    if (status == STATUS_RECORD) {
        return "Запись сотрудника должна содержать ровно четыре поля.";
    }
    if (status == STATUS_ID) {
        return "Некорректный идентификатор сотрудника.";
    }
    if (status == STATUS_NAME) {
        return "Имя и фамилия должны содержать только латинские буквы.";
    }
    if (status == STATUS_SALARY) {
        return "Некорректный формат заработной платы.";
    }
    if (status == STATUS_RANGE) {
        return "Числовое значение выходит за допустимый диапазон.";
    }
    if (status == STATUS_PARAMETER) {
        return "Некорректный параметр функции.";
    }
    return "Неизвестная ошибка.";
}

int main(int argc, char *argv[]) {
    EmployeeArray employees = {NULL, 0, 0};
    int descending = 0;
    int status = STATUS_OK;

    if (argc != 4) {
        status = STATUS_ARGUMENTS;
    } else if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
        status = STATUS_PARAMETER;
    } else if (argv[1][0] == '\0' || argv[3][0] == '\0') {
        status = STATUS_PATH;
    } else if (strcmp(argv[2], "-a") == 0 || strcmp(argv[2], "/a") == 0) {
        descending = 0;
    } else if (strcmp(argv[2], "-d") == 0 || strcmp(argv[2], "/d") == 0) {
        descending = 1;
    } else {
        status = STATUS_FLAG;
    }

    if (status == STATUS_OK) {
        status = read_employees(argv[1], &employees);
    }

    if (status == STATUS_OK) {
        status = sort_employees(&employees, descending);
    }

    if (status == STATUS_OK) {
        status = write_employees(argv[3], &employees);
    }

    employee_array_destroy(&employees);

    if (status != STATUS_OK) {
        fprintf(stderr, "%s\n", status_message(status));
        return 1;
    }

    return 0;
}
