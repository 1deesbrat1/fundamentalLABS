#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_INPUT_ERROR,
    STATUS_MEMORY_ERROR
} Status;

typedef struct {
    int id;
    int start;
    int finish;
} Report;

Status parse_time(const char *text, int *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (strlen(text) != 5) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (text[2] != ':') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (text[0] < '0' || text[0] > '9' ||
        text[1] < '0' || text[1] > '9' ||
        text[3] < '0' || text[3] > '9' ||
        text[4] < '0' || text[4] > '9') {
        return STATUS_INVALID_ARGUMENT;
    }

    int hours = (text[0] - '0') * 10 + (text[1] - '0');
    int minutes = (text[3] - '0') * 10 + (text[4] - '0');

    if (hours > 23 || minutes > 59) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = hours * 60 + minutes;

    return STATUS_OK;
}

Status allocate_reports(size_t n, Report **reports) {
    if (reports == NULL || n == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    *reports = NULL;

    if (n > SIZE_MAX / sizeof(Report)) {
        return STATUS_MEMORY_ERROR;
    }

    *reports = (Report *)malloc(n * sizeof(Report));

    if (*reports == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    return STATUS_OK;
}

Status parse_report(const char *line, Report *report) {
    if (line == NULL || report == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    char id_text[32];
    char start_text[6];
    char finish_text[6];
    char extra;

    int count = sscanf(
        line,
        "%31s %5s %5s %c",
        id_text,
        start_text,
        finish_text,
        &extra
    );

    if (count != 3) {
        return STATUS_INVALID_ARGUMENT;
    }

    char *end;
    long long id = strtoll(id_text, &end, 10);

    if (*end != '\0' || id <= 0 || id > INT32_MAX) {
        return STATUS_INVALID_ARGUMENT;
    }

    Report temp;
    temp.id = (int)id;

    Status status = parse_time(start_text, &temp.start);

    if (status != STATUS_OK) {
        return status;
    }

    status = parse_time(finish_text, &temp.finish);

    if (status != STATUS_OK) {
        return status;
    }

    if (temp.start >= temp.finish) {
        return STATUS_INVALID_ARGUMENT;
    }

    *report = temp;

    return STATUS_OK;
}

Status read_reports(Report *reports, size_t n) {
    if (reports == NULL || n == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    char line[128];

    for (size_t i = 0; i < n; ++i) {
        if (fgets(line, sizeof(line), stdin) == NULL) {
            return STATUS_INPUT_ERROR;
        }

        char *newline = strchr(line, '\n');

        if (newline != NULL) {
            *newline = '\0';
        }

        Status status = parse_report(line, &reports[i]);

        if (status != STATUS_OK) {
            return status;
        }
    }

    int ch;

    while ((ch = getchar()) != EOF) {
        if (ch != ' ' && ch != '\n' &&
            ch != '\t' && ch != '\r') {
            return STATUS_INPUT_ERROR;
        }
    }

    return STATUS_OK;
}


Status parse_positive_size(const char *text, size_t *result) {
    if (text == NULL || result == NULL || *text == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t value = 0;

    while (*text != '\0') {
        if (*text < '0' || *text > '9') {
            return STATUS_INVALID_ARGUMENT;
        }

        size_t digit = (size_t)(*text - '0');

        if (value > (SIZE_MAX - digit) / 10) {
            return STATUS_INVALID_ARGUMENT;
        }

        value = value * 10 + digit;
        ++text;
    }

    if (value == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = value;

    return STATUS_OK;
}

Status read_input(Report **reports, size_t *n, size_t *k) {
    if (reports == NULL || n == NULL || k == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *reports = NULL;
    *n = 0;
    *k = 0;

    char line[128];
    char n_text[32];
    char k_text[32];
    char extra;

    if (fgets(line, sizeof(line), stdin) == NULL) {
        return STATUS_INPUT_ERROR;
    }

    char *newline = strchr(line, '\n');

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

    int count = sscanf(line, "%31s %31s %c", n_text, k_text, &extra);

    if (count != 2) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t report_count;
    size_t hall_count;

    Status status = parse_positive_size(n_text, &report_count);

    if (status != STATUS_OK) {
        return status;
    }

    status = parse_positive_size(k_text, &hall_count);

    if (status != STATUS_OK) {
        return status;
    }

    status = allocate_reports(report_count, reports);

    if (status != STATUS_OK) {
        return status;
    }

    status = read_reports(*reports, report_count);

    if (status != STATUS_OK) {
        free(*reports);
        *reports = NULL;
        return status;
    }

    *n = report_count;
    *k = hall_count;

    return STATUS_OK;
}

int compare_ids(const void *a, const void *b) {
    const int *first = (const int *)a;
    const int *second = (const int *)b;

    if (*first < *second) {
        return -1;
    }
    else if (*first > *second) {
        return 1;
    }

    return 0;
}

Status validate_reports(const Report *reports, size_t n) {
    if (reports == NULL || n == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (n > SIZE_MAX / sizeof(int)) {
        return STATUS_MEMORY_ERROR;
    }

    int *ids = (int *)malloc(n * sizeof(int));

    if (ids == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    for (size_t i = 0; i < n; ++i) {
        if (reports[i].id <= 0 ||
            reports[i].start < 0 ||
            reports[i].start > 1439 ||
            reports[i].finish < 0 ||
            reports[i].finish > 1439 ||
            reports[i].start >= reports[i].finish) {
            free(ids);
            return STATUS_INVALID_ARGUMENT;
        }

        ids[i] = reports[i].id;
    }

    qsort(ids, n, sizeof(int), compare_ids);

    for (size_t i = 1; i < n; ++i) {
        if (ids[i] == ids[i - 1]) {
            free(ids);
            return STATUS_INVALID_ARGUMENT;
        }
    }

    free(ids);

    return STATUS_OK;
}

int compare_reports(const void *a, const void *b) {
    const Report *first = (const Report *)a;
    const Report *second = (const Report *)b;

    if (first->finish < second->finish) {
        return -1;
    }
    else if (first->finish > second->finish) {
        return 1;
    }

    return 0;
}

Status sort_reports(Report *reports, size_t n) {
    if (reports == NULL || n == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    qsort(reports, n, sizeof(Report), compare_reports);

    return STATUS_OK;
}

Status max_reports(const Report *reports, size_t n, size_t k, size_t *result) {
    if (reports == NULL || result == NULL || n == 0 || k == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = validate_reports(reports, n);

    if (status != STATUS_OK) {
        return status;
    }

    if (n > SIZE_MAX / sizeof(Report)) {
        return STATUS_MEMORY_ERROR;
    }

    Report *sorted = (Report *)malloc(n * sizeof(Report));

    if (sorted == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    memcpy(sorted, reports, n * sizeof(Report));

    status = sort_reports(sorted, n);

    if (status != STATUS_OK) {
        free(sorted);
        return status;
    }

    size_t rooms;
    if (k < n) {
        rooms = k;
    } else {
        rooms = n;
    }
    if (rooms > SIZE_MAX / sizeof(int)) {
        free(sorted);
        return STATUS_MEMORY_ERROR;
    }

    int *room_end = (int *)calloc(rooms, sizeof(int));

    if (room_end == NULL) {
        free(sorted);
        return STATUS_MEMORY_ERROR;
    }

    size_t count = 0;

    for (size_t i = 0; i < n; ++i) {
        size_t best = rooms;

        for (size_t j = 0; j < rooms; ++j) {
            if (room_end[j] <= sorted[i].start) {
                if (best == rooms || room_end[j] > room_end[best]) {
                    best = j;
                }
            }
        }

        if (best != rooms) {
            room_end[best] = sorted[i].finish;
            ++count;
        }
    }

    *result = count;

    free(room_end);
    free(sorted);

    return STATUS_OK;
}

int main(void) {
    Report *reports = NULL;

    size_t n = 0;
    size_t k = 0;
    size_t result = 0;

    Status status = read_input(&reports, &n, &k);

    if (status != STATUS_OK) {
        fprintf(stderr, "Input error\n");
        return 1;
    }

    status = max_reports(reports, n, k, &result);

    free(reports);

    if (status == STATUS_OK) {
        printf("%zu\n", result);
        return 0;
    }
    else if (status == STATUS_INVALID_ARGUMENT) {
        fprintf(stderr, "Invalid argument\n");
    }
    else if (status == STATUS_MEMORY_ERROR) {
        fprintf(stderr, "Memory allocation error\n");
    }
    else {
        fprintf(stderr, "Unknown error\n");
    }

    return 1;
}