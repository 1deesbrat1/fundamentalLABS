#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>


typedef struct {
    unsigned int id;
    char* surname;
    char* name;
    char* group;
    unsigned char* grades;
} Student;


typedef struct {
    Student* data;
    size_t count;
    size_t capacity;
} StudentArray;

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_MEMORY_ERROR,
    STATUS_TOO_LONG,
    STATUS_FILE_ERROR
} Status;


Status init_student_array(StudentArray* array) {
    if (array == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    array->data = NULL;
    array->count = 0;
    array->capacity = 0;


    return STATUS_OK;
}

Status free_student(Student* student) {
    if (student == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    free(student->surname);
    free(student->name);
    free(student->group);
    free(student->grades);
    student->surname = NULL;
    student->name = NULL;
    student->group = NULL;
    student->grades = NULL;
    student->id = 0;

    return STATUS_OK;
}

Status free_student_array(StudentArray* array) {
    if (array == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }


    for (size_t i = 0; i < array->count; i++) {
        free_student(&array->data[i]);
    }
    free(array->data);
    array->capacity = 0;
    array->count = 0;
    array->data = NULL;

    return STATUS_OK;
}

Status append_student(StudentArray* array, Student* student) {
    if (array == NULL || student == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity > 0 && array->data == NULL) || (array->capacity == 0 && array->data != NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t new_capacity;
    if (array->count == array->capacity) {
        if (array->capacity == 0) {
            new_capacity = 2;
        } else if (array->capacity <= SIZE_MAX / 2) {
            new_capacity = array->capacity * 2;
        } else {
            return STATUS_TOO_LONG;
        }

        if (new_capacity > SIZE_MAX / sizeof(Student)) {
            return STATUS_TOO_LONG;
        }

        Student* temp = (Student *)realloc(array->data, new_capacity * sizeof(Student));

        if (temp == NULL) {
            return STATUS_MEMORY_ERROR;
        }

        array->data = temp;
        array->capacity = new_capacity;
    } 

    array->data[array->count] = *student;
    array->count++;

    student->name = NULL;
    student->surname = NULL;
    student->group = NULL;
    student->grades = NULL;
    student->id = 0;

    return STATUS_OK;
}

Status init_student(Student* student) {
    if (student == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    student->id = 0;
    student->name = NULL;
    student->surname = NULL;
    student->group = NULL;
    student->grades = NULL;

    return STATUS_OK;
}

Status copy_string(const char* source, char** destination) {
    if (source == NULL || destination == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*destination != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t length = strlen(source);
    if (length == SIZE_MAX) {
        return STATUS_TOO_LONG;
    }

    length++;
    char* temp = (char *)malloc(length);
    if (temp == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    memcpy(temp, source, length);

    *destination = temp;

    return STATUS_OK;
}


Status copy_grades(const unsigned char* source, unsigned char** destination) {
    if (source == NULL || destination == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*destination != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    unsigned char* temp = (unsigned char *)malloc(5 * sizeof(unsigned char));
    if (temp == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    memcpy(temp, source, 5 * sizeof(unsigned char));
    *destination = temp;

    return STATUS_OK;
}


Status create_student(Student* student, unsigned int id, const char* name, const char* surname, const char* group, const unsigned char* grades) {
    if (student == NULL || name == NULL || surname == NULL || group == NULL || grades == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (name[0] == '\0' || surname[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }
    size_t i = 0;
    while (name[i] != '\0') {
        if (!((name[i] >= 'A' && name[i] <= 'Z') || (name[i] >= 'a' && name[i] <= 'z'))) {
            return STATUS_INVALID_ARGUMENT;
        }
        i++;
    }
    i = 0;
    while (surname[i] != '\0') {
        if (!((surname[i] >= 'A' && surname[i] <= 'Z') || (surname[i] >= 'a' && surname[i] <= 'z'))) {
            return STATUS_INVALID_ARGUMENT;
        }
        i++;
    }

    if (group[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = init_student(student);
    if (status != STATUS_OK) {
        return status;
    }

    student->id = id;
    status = copy_string(name, &student->name);
    if (status != STATUS_OK) {
        free_student(student);
        return status;
    }
    status = copy_string(surname, &student->surname);
    if (status != STATUS_OK) {
        free_student(student);
        return status;
    }
    status = copy_string(group, &student->group);
    if (status != STATUS_OK) {
        free_student(student);
        return status;
    }
    status = copy_grades(grades, &student->grades);
    if (status != STATUS_OK) {
        free_student(student);
        return status;
    }

    return STATUS_OK;

}

Status read_line(FILE* file, char** line, int* end_of_file) {
    if (file == NULL || line == NULL || end_of_file == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*line != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *end_of_file = 0;

    char* buffer = NULL;
    size_t length = 0;
    size_t capacity = 0;
    size_t new_capacity;

    int ch = fgetc(file);
    while (ch != EOF && ch != '\n') {

        if (capacity == 0 || length >= capacity - 1) {
            if (capacity == 0) {
                new_capacity = 2;
            } else if (capacity <= SIZE_MAX / 2) {
                new_capacity = capacity * 2;
            } else {
                free(buffer);
                return STATUS_TOO_LONG;
            }

            char* temp = (char*)realloc(buffer, new_capacity);
            if (temp == NULL) {
                free(buffer);
                return STATUS_MEMORY_ERROR;
            }
            buffer = temp;
            capacity = new_capacity;
        }

        if (ch == '\0') {
            free(buffer);
            return STATUS_INVALID_ARGUMENT;
        }
        buffer[length] = ch;
        length++;

        ch = fgetc(file);
    }

    if (ch == EOF && ferror(file) != 0) {
        free(buffer);
        return STATUS_FILE_ERROR;
    }

    if (ch == EOF && length == 0) {
        *end_of_file = 1;
        return STATUS_OK;
    }

    if (buffer == NULL) {
        buffer = (char *)malloc(1);
        if (buffer == NULL) {
            return STATUS_MEMORY_ERROR;
        }
    }

    buffer[length] = '\0';
    *line = buffer;

    return STATUS_OK;

}


Status parse_unsigned(const char* text, unsigned int max_value, unsigned int* result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    } 

    if (text[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    unsigned int value = 0;
    size_t i = 0;
    while (text[i] != '\0') {
        if (text[i] < '0' || text[i] > '9') {
            return STATUS_INVALID_ARGUMENT;
        }
        unsigned int digit = (unsigned int)(text[i] - '0');
        if (value > max_value / 10 || (value == max_value / 10 && digit > max_value % 10)) {
            return STATUS_TOO_LONG;
        }
        value = value * 10 + digit;
        i++;
    }

    *result = value;

    return STATUS_OK;

}


Status parse_student(char* line, Student* student) {
    if (line == NULL || student == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    char* fields[9];
    char* current = line;
    size_t i = 0;
    for (i = 0; i < 8; i++) {
        char* separator = strchr(current, ';');
        if (separator == NULL) {
            return STATUS_INVALID_ARGUMENT;
        }

        fields[i] = current;
        *separator = '\0';
        current = separator + 1;
    }

    fields[i] = current;
    char* separator = strchr(current, ';');
    if (separator != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    unsigned int id;
    Status status;
    
    status = parse_unsigned(fields[0], UINT_MAX, &id);
    if (status != STATUS_OK) {
        return status;
    }

    unsigned char grades[5];
    unsigned int value;
    for (size_t i = 4; i < 9; i++) {
        status = parse_unsigned(fields[i], UCHAR_MAX, &value);
        if (status != STATUS_OK) {
            return status;
        }
        grades[i - 4] = (unsigned char) value;
    }

    status = create_student(student, id, fields[1], fields[2], fields[3], grades);
    if (status != STATUS_OK) {
        return status;
    }
    

    return STATUS_OK;
}

Status load_students(const char *path, StudentArray *array) {
    if (path == NULL || array == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (path[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->data != NULL || array->count != 0 || array->capacity != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return STATUS_FILE_ERROR;
    }

    char *line = NULL;
    int end_of_file = 0;
    Status status = STATUS_OK;

    while (end_of_file == 0 && status == STATUS_OK) {
        status = read_line(file, &line, &end_of_file);

        if (status != STATUS_OK || end_of_file == 1) {
            break;
        }

        Student student;
        status = init_student(&student);

        if (status == STATUS_OK) {
            status = parse_student(line, &student);

            if (status == STATUS_OK) {
                status = append_student(array, &student);
            }

            free_student(&student);
        }

        free(line);
        line = NULL;
    }

    if (fclose(file) == EOF && status == STATUS_OK) {
        status = STATUS_FILE_ERROR;
    }

    if (status != STATUS_OK) {
        free_student_array(array);
    }

    return status;
}

typedef struct {
    size_t count;
    size_t capacity;
    size_t* indices;
} SearchResult;


Status init_search_result(SearchResult* result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    result->count = 0;
    result->capacity = 0;
    result->indices = NULL;

    return STATUS_OK;
}


Status free_search_result(SearchResult* result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    free(result->indices);
    result->indices = NULL;
    result->count = 0;
    result->capacity = 0;
    
    return STATUS_OK;
}

Status append_search_result(SearchResult* result, size_t index) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->count > result->capacity || (result->capacity > 0 && result->indices == NULL) || (result->capacity == 0 && result->indices != NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t new_capacity;
    if (result->count == result->capacity) {
        if (result->capacity == 0) {
            new_capacity = 2;
        } else if (result->capacity <= SIZE_MAX / 2) {
            new_capacity = result->capacity * 2;
        } else {
            return STATUS_TOO_LONG;
        }

        if (new_capacity > SIZE_MAX / sizeof(size_t)) {
            return STATUS_TOO_LONG;
        }

        size_t* temp = (size_t*)realloc(result->indices, new_capacity * sizeof(size_t));
        if (temp == NULL) {
            return STATUS_MEMORY_ERROR;
        }

        result->indices = temp;
        result->capacity = new_capacity;
    }

    result->indices[result->count] = index;
    result->count++;

    return STATUS_OK;
}

Status find_by_id(const StudentArray* array, unsigned int id, SearchResult* result) {
    if (array == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity > 0 && array->data == NULL) || (array->capacity == 0 && array->data != NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->count != 0 || result->capacity != 0 || result->indices != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status;
    for (size_t i = 0; i < array->count; i++) {
        if (array->data[i].id == id) {
            status = append_search_result(result, i);
            if (status != STATUS_OK) {
                free_search_result(result);
                return status;
            }
        }
    }

    return STATUS_OK;
}

Status find_by_surname(const StudentArray* array, const char* surname, SearchResult* result) {
    if (array == NULL || result == NULL || surname == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (surname[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity == 0 && array->data != NULL) || (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->indices != NULL || result->count != 0 || result->capacity != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status;
    for (size_t i = 0; i < array->count; i++) {
        if (strcmp(array->data[i].surname, surname) == 0) {
            status = append_search_result(result, i);
            if (status != STATUS_OK) {
                free_search_result(result);
                return status;
            }
        }
    }

    return STATUS_OK;
}

Status find_by_name(const StudentArray* array, const char* name, SearchResult* result) {
    if (array == NULL || name == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (name[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity == 0 && array->data != NULL) || (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->capacity != 0 || result->count != 0 || result->indices != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status;
    for (size_t i = 0; i < array->count; i++) {
        if (strcmp(array->data[i].name, name) == 0) {
            status = append_search_result(result, i);
            if (status != STATUS_OK) {
                free_search_result(result);
                return status;
            }
        }
    }

    return STATUS_OK;
}

Status find_by_group(const StudentArray* array, const char* group, SearchResult* result) {

    if (array == NULL || group == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (group[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity == 0 && array->data != NULL) || (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->capacity != 0 || result->count != 0 || result->indices != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }   

    Status status;
    for (size_t i = 0; i < array->count; i++) {
        if (strcmp(array->data[i].group, group) == 0) {
            status = append_search_result(result, i);
            if (status != STATUS_OK) {
                free_search_result(result);
                return status;
            }
        }
    }

    return STATUS_OK;
}


int compare_by_id(const void* a, const void* b) {
    const Student* student_a = (const Student*)a;
    const Student* student_b = (const Student*)b;

    if (student_a->id < student_b->id) {
        return -1;
    }

    if (student_a->id > student_b->id) {
        return 1;
    }

    return 0;
}

int compare_by_surname(const void* a, const void* b) {
    const Student* student_a = (const Student*)a;
    const Student* student_b = (const Student*)b;

    if (strcmp(student_a->surname, student_b->surname) > 0) {
        return 1;
    } else if (strcmp(student_a->surname, student_b->surname) < 0) {
        return -1;
    } else {
        return 0;
    }

}

int compare_by_name(const void* a, const void* b) {
    const Student* student_a = (const Student*)a;
    const Student* student_b= (const Student*)b;


    if (strcmp(student_a->name, student_b->name) > 0) {
        return 1;
    } else if (strcmp(student_a->name, student_b->name) < 0) {
        return -1;
    } else {
        return 0;
    }

}

int compare_by_group(const void*a, const void* b) {
    const Student* student_a = (const Student*)a;
    const Student* student_b = (const Student*)b;

    if (strcmp(student_a->group, student_b->group) > 0) {
        return 1;
    } else if (strcmp(student_a->group, student_b->group) < 0) {
        return -1;
    } else {
        return 0;
    }
}

Status sort_students(StudentArray* array, int(*compare)(const void*, const void*)) {
    if (array == NULL || compare == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity == 0 && array->data != NULL) || (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count < 2) {
        return STATUS_OK;
    }

    qsort(array->data, array->count, sizeof(Student), compare);

    return STATUS_OK;
}

Status calculate_student_average(const Student* student, double* average) {
    if (student == NULL || average == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (student->grades == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    unsigned int sum = 0;
    for (size_t i = 0; i < 5; i++) {
        sum+=student->grades[i];
    }

    *average = (double)sum / 5.0;

    return STATUS_OK;

}

Status calculate_overall_average(const StudentArray* array, double* average) {
    if (array == NULL || average == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity == 0 && array->data != NULL) || (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    double sum = 0.0;
    Status status;
    for (size_t i = 0; i < array->count; i++) {
        double student_average;
        status = calculate_student_average(&array->data[i], &student_average);
        if (status != STATUS_OK) {
            return status;
        }
        sum += student_average;
    }

    *average = sum / (double)array->count;

    return STATUS_OK;
}

Status find_above_average(const StudentArray* array, SearchResult* result) {
    if (array == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity || (array->capacity == 0 && array->data != NULL) || (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->count != 0 || result->capacity != 0 || result->indices != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status;
    double overall_average;
    const double epsilon = 1e-9;
    status = calculate_overall_average(array, &overall_average);
    if (status != STATUS_OK) {
        return status;
    }

    for (size_t i = 0; i < array->count; i++) {
        double student_average;
        status = calculate_student_average(&array->data[i], &student_average);
        if (status != STATUS_OK) {
            free_search_result(result);
            return status;
        }

        if (student_average > overall_average + epsilon) {
            status = append_search_result(result, i);
            if (status != STATUS_OK) {
                free_search_result(result);
                return status;
            }
        }
    }

    return STATUS_OK;
}

Status write_student_info(FILE *file, const Student *student) {
    if (file == NULL || student == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (student->name == NULL || student->surname == NULL ||
        student->group == NULL || student->grades == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    double average;
    Status status = calculate_student_average(student, &average);
    if (status != STATUS_OK) {
        return status;
    }

    if (fprintf(file, "ID: %u\nИмя: %s\nФамилия: %s\nГруппа: %s\nСредний балл: %.2f\n\n",
                student->id, student->name, student->surname,
                student->group, average) < 0) {
        return STATUS_FILE_ERROR;
    }

    return STATUS_OK;
}

Status write_search_results(FILE *file, const StudentArray *array, const SearchResult *result) {
    if (file == NULL || array == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity ||
        (array->capacity == 0 && array->data != NULL) ||
        (array->capacity > 0 && array->data == NULL) ||
        result->count > result->capacity ||
        (result->capacity == 0 && result->indices != NULL) ||
        (result->capacity > 0 && result->indices == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (fprintf(file, "Найдено студентов: %zu\n", result->count) < 0) {
        return STATUS_FILE_ERROR;
    }

    for (size_t i = 0; i < result->count; i++) {
        if (result->indices[i] >= array->count) {
            return STATUS_INVALID_ARGUMENT;
        }

        Status status = write_student_info(file, &array->data[result->indices[i]]);
        if (status != STATUS_OK) {
            return status;
        }
    }

    return STATUS_OK;
}

Status write_all_students(FILE *file, const StudentArray *array) {
    if (file == NULL || array == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity ||
        (array->capacity == 0 && array->data != NULL) ||
        (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (fprintf(file, "Всего студентов: %zu\n", array->count) < 0) {
        return STATUS_FILE_ERROR;
    }

    for (size_t i = 0; i < array->count; i++) {
        Status status = write_student_info(file, &array->data[i]);
        if (status != STATUS_OK) {
            return status;
        }
    }

    return STATUS_OK;
}

Status write_above_average(FILE *file, const StudentArray *array) {
    if (file == NULL || array == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count > array->capacity ||
        (array->capacity == 0 && array->data != NULL) ||
        (array->capacity > 0 && array->data == NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (array->count == 0) {
        if (fprintf(file, "Список студентов пуст.\n") < 0) {
            return STATUS_FILE_ERROR;
        }
        return STATUS_OK;
    }

    SearchResult result;
    Status status = init_search_result(&result);
    if (status != STATUS_OK) {
        return status;
    }

    status = find_above_average(array, &result);
    if (status != STATUS_OK) {
        free_search_result(&result);
        return status;
    }

    double overall_average;
    status = calculate_overall_average(array, &overall_average);
    if (status == STATUS_OK) {
        if (fprintf(file, "Общий средний балл: %.2f\nСтуденты выше среднего: %zu\n",
                    overall_average, result.count) < 0) {
            status = STATUS_FILE_ERROR;
        }
    }

    for (size_t i = 0; i < result.count && status == STATUS_OK; i++) {
        if (result.indices[i] >= array->count) {
            status = STATUS_INVALID_ARGUMENT;
            break;
        }

        const Student *student = &array->data[result.indices[i]];

        if (student->name == NULL || student->surname == NULL) {
            status = STATUS_INVALID_ARGUMENT;
            break;
        }

        if (fprintf(file, "%s %s\n", student->name, student->surname) < 0) {
            status = STATUS_FILE_ERROR;
        }
    }

    free_search_result(&result);
    return status;
}

Status read_prompt(const char *prompt, char **line, int *end_of_file) {
    if (prompt == NULL || line == NULL || end_of_file == NULL || *line != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (fputs(prompt, stdout) == EOF) {
        return STATUS_FILE_ERROR;
    }

    return read_line(stdin, line, end_of_file);
}

Status process_id_search(const StudentArray *array, FILE *trace) {
    if (array == NULL || trace == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    char *line = NULL;
    int end_of_file = 0;
    Status status = read_prompt("Введите ID: ", &line, &end_of_file);

    if (status != STATUS_OK) {
        free(line);
        return status;
    }

    if (end_of_file != 0) {
        return STATUS_OK;
    }

    unsigned int id;
    status = parse_unsigned(line, UINT_MAX, &id);
    free(line);

    if (status != STATUS_OK) {
        if (fputs("Некорректный ID.\n", stdout) == EOF) {
            return STATUS_FILE_ERROR;
        }
        return STATUS_OK;
    }

    SearchResult result;
    status = init_search_result(&result);
    if (status != STATUS_OK) {
        return status;
    }

    status = find_by_id(array, id, &result);

    if (status == STATUS_OK) {
        status = write_search_results(stdout, array, &result);
    }

    if (status == STATUS_OK) {
        if (fprintf(trace, "\nРезультат поиска по ID %u:\n", id) < 0) {
            status = STATUS_FILE_ERROR;
        } else {
            status = write_search_results(trace, array, &result);
        }
    }

    free_search_result(&result);
    return status;
}

Status process_text_search(const StudentArray *array, const char *prompt, Status (*search)(const StudentArray *, const char *, SearchResult *)) {
    if (array == NULL || prompt == NULL || search == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    char *line = NULL;
    int end_of_file = 0;
    Status status = read_prompt(prompt, &line, &end_of_file);

    if (status != STATUS_OK) {
        free(line);
        return status;
    }

    if (end_of_file != 0) {
        return STATUS_OK;
    }

    if (line[0] == '\0') {
        free(line);

        if (fputs("Пустой запрос.\n", stdout) == EOF) {
            return STATUS_FILE_ERROR;
        }

        return STATUS_OK;
    }

    SearchResult result;
    status = init_search_result(&result);

    if (status == STATUS_OK) {
        status = search(array, line, &result);

        if (status == STATUS_OK) {
            status = write_search_results(stdout, array, &result);
        }

        free_search_result(&result);
    }

    free(line);
    return status;
}

Status process_sort(StudentArray *array, int (*compare)(const void *, const void *)) {
    Status status = sort_students(array, compare);

    if (status != STATUS_OK) {
        return status;
    }

    return write_all_students(stdout, array);
}

Status run_menu(StudentArray *array, FILE *trace) {
    if (array == NULL || trace == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    while (1) {
        if (fputs("\n0 - Выход\n1 - Показать всех студентов\n2 - Поиск по ID\n"
                  "3 - Поиск по имени\n4 - Поиск по фамилии\n5 - Поиск по группе\n"
                  "6 - Сортировка по ID\n7 - Сортировка по имени\n"
                  "8 - Сортировка по фамилии\n9 - Сортировка по группе\n"
                  "10 - Студенты выше среднего\nВыберите пункт: ", stdout) == EOF) {
            return STATUS_FILE_ERROR;
        }

        char *line = NULL;
        int end_of_file = 0;
        Status status = read_line(stdin, &line, &end_of_file);

        if (status != STATUS_OK) {
            free(line);
            return status;
        }

        if (end_of_file != 0) {
            return STATUS_OK;
        }

        unsigned int command;
        status = parse_unsigned(line, 10U, &command);
        free(line);

        if (status != STATUS_OK) {
            if (fputs("Некорректный пункт меню.\n", stdout) == EOF) {
                return STATUS_FILE_ERROR;
            }

            continue;
        }

        if (command == 0U) {
            return STATUS_OK;
        } else if (command == 1U) {
            status = write_all_students(stdout, array);
        } else if (command == 2U) {
            status = process_id_search(array, trace);
        } else if (command == 3U) {
            status = process_text_search(array, "Введите имя: ", find_by_name);
        } else if (command == 4U) {
            status = process_text_search(array, "Введите фамилию: ", find_by_surname);
        } else if (command == 5U) {
            status = process_text_search(array, "Введите группу: ", find_by_group);
        } else if (command == 6U) {
            status = process_sort(array, compare_by_id);
        } else if (command == 7U) {
            status = process_sort(array, compare_by_name);
        } else if (command == 8U) {
            status = process_sort(array, compare_by_surname);
        } else if (command == 9U) {
            status = process_sort(array, compare_by_group);
        } else {
            status = write_above_average(stdout, array);
        }

        if (status != STATUS_OK) {
            return status;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <файл_студентов> <файл_трассировки>\n", argv[0]);
        return 1;
    }

    if (argv[1][0] == '\0' || argv[2][0] == '\0' || strcmp(argv[1], argv[2]) == 0) {
        fputs("Некорректные пути к файлам.\n", stderr);
        return 1;
    }

    StudentArray array;
    Status status = init_student_array(&array);

    if (status != STATUS_OK) {
        fputs("Ошибка инициализации массива.\n", stderr);
        return 1;
    }

    status = load_students(argv[1], &array);

    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка чтения файла студентов (статус %d).\n", (int)status);
        free_student_array(&array);
        return 1;
    }

    FILE *trace = fopen(argv[2], "w");

    if (trace == NULL) {
        fputs("Не удалось открыть файл трассировки.\n", stderr);
        free_student_array(&array);
        return 1;
    }

    status = write_above_average(trace, &array);

    if (status == STATUS_OK) {
        status = run_menu(&array, trace);
    }

    if (fclose(trace) == EOF && status == STATUS_OK) {
        status = STATUS_FILE_ERROR;
    }

    free_student_array(&array);

    if (status != STATUS_OK) {
        fprintf(stderr, "Ошибка выполнения программы (статус %d).\n", (int)status);
        return 1;
    }

    return 0;
}
