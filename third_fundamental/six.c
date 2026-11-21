#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_MEMORY_ERROR,
    STATUS_FILE_ERROR,
    STATUS_INVALID_DATA,
    STATUS_NOT_FOUND
} Status;

typedef struct {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
} DateTime;

typedef enum {
    STOP_START,
    STOP_INTERMEDIATE,
    STOP_END
} StopType;

typedef struct StopNode {
    double x;
    double y;
    DateTime arrival;
    DateTime departure;
    StopType type;
    struct StopNode *next;
} StopNode;

typedef struct RouteNode {
    char *vehicle_id;
    StopNode *stops;
    struct RouteNode *next;
} RouteNode;

typedef struct RecordNode {
    char *vehicle_id;
    StopNode *stop;
    struct RecordNode *next;
} RecordNode;

typedef enum {
    QUERY_MOST_ROUTES = 1,
    QUERY_FEWEST_ROUTES,
    QUERY_LONGEST_PATH,
    QUERY_SHORTEST_PATH,
    QUERY_LONGEST_ROUTE,
    QUERY_SHORTEST_ROUTE,
    QUERY_LONGEST_STOP,
    QUERY_SHORTEST_STOP,
    QUERY_MOST_IDLE
} Query;

typedef struct {
    long double distance;
    long long idle_seconds;
    long long longest_stop;
    long long shortest_stop;
    const StopNode *longest_stop_node;
    const StopNode *shortest_stop_node;
} RouteMetrics;

typedef struct {
    size_t route_count;
    long double path_distance;
    long long idle_seconds;
    long double longest_route;
    long double shortest_route;
    const RouteNode *longest_route_node;
    const RouteNode *shortest_route_node;
    long long longest_stop;
    long long shortest_stop;
    const StopNode *longest_stop_node;
    const StopNode *shortest_stop_node;
} VehicleMetrics;

typedef struct {
    const char *vehicle_id;
    size_t route_count;
    long double distance;
    long long seconds;
    const RouteNode *route;
    const StopNode *stop;
} SearchResult;

int is_leap_year(int year) {
    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) {
        return 1;
    } else {
        return 0;
    }
}

Status validate_datetime(const DateTime *dt) {
    if (dt == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (dt->year < 1) {
        return STATUS_INVALID_DATA;
    }

    if (dt->month < 1 || dt->month > 12) {
        return STATUS_INVALID_DATA;
    }

    const int days_in_month[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    int max_days = days_in_month[dt->month - 1];

    if (dt->month == 2 && is_leap_year(dt->year)) {
        max_days++;
    }

    if (dt->day < 1 || dt->day > max_days) {
        return STATUS_INVALID_DATA;
    }

    if (dt->hour < 0 || dt->hour > 23 ||
        dt->minute < 0 || dt->minute > 59 ||
        dt->second < 0 || dt->second > 59) {
        return STATUS_INVALID_DATA;
    }

    return STATUS_OK;
}

Status compare_datetime(const DateTime *a, const DateTime *b, int *result) {
    if (a == NULL || b == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status;
    status = validate_datetime(a);
    if (status != STATUS_OK) {
        return status;
    }
    status = validate_datetime(b);
    if (status != STATUS_OK) {
        return status;
    }

    if (a->year > b->year) {
        *result = 1;
        return STATUS_OK;
    } else if (a->year < b->year) {
        *result = -1;
        return STATUS_OK;
    }

    if (a->month > b->month) {
        *result = 1;
        return STATUS_OK;
    } else if (a->month < b->month) {
        *result = -1;
        return STATUS_OK;
    }

    if (a->day > b->day) {
        *result = 1;
        return STATUS_OK;
    } else if (a->day < b->day) {
        *result = -1;
        return STATUS_OK;
    }

    if (a->hour > b->hour) {
        *result = 1;
        return STATUS_OK;
    } else if (a->hour < b->hour) {
        *result = -1;
        return STATUS_OK;
    }

    if (a->minute > b->minute) {
        *result = 1;
        return STATUS_OK;
    } else if (a->minute < b->minute) {
        *result = -1;
        return STATUS_OK;
    }

    if (a->second > b->second) {
        *result = 1;
        return STATUS_OK;
    } else if (a->second < b->second) {
        *result = -1;
        return STATUS_OK;
    }

    *result = 0;
    return STATUS_OK;
}

Status datetime_to_seconds(const DateTime *dt, long long *result) {
    if (dt == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = validate_datetime(dt);
    if (status != STATUS_OK) {
        return status;
    }

    const int days_in_month[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    long long years = (long long)dt->year - 1;
    long long leap_years = years / 4 - years / 100 + years / 400;

    long long days = years * 365 + leap_years;

    for (int i = 0; i < dt->month - 1; i++) {
        days += days_in_month[i];
    }

    if (dt->month > 2 && is_leap_year(dt->year)) {
        days++;
    }

    days += dt->day - 1;

    long long seconds = days * 86400LL;

    seconds += (long long)dt->hour * 3600;
    seconds += (long long)dt->minute * 60;
    seconds += dt->second;

    *result = seconds;

    return STATUS_OK;
}

Status calculate_duration(const StopNode *stop, long long *result) {
    if (stop == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status;
    long long arrival_seconds;
    long long departure_seconds;
    status = datetime_to_seconds(&stop->arrival, &arrival_seconds);
    if (status != STATUS_OK) {
        return status;
    }

    status = datetime_to_seconds(&stop->departure, &departure_seconds);
    if (status != STATUS_OK) {
        return status;
    }

    if (arrival_seconds > departure_seconds) {
        return STATUS_INVALID_DATA;
    }

    *result = departure_seconds - arrival_seconds;
    return STATUS_OK;
}

Status parse_datetime(const char *text, DateTime *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (strlen(text) != 19) {
        return STATUS_INVALID_DATA;
    }

    if (text[2] != '.' || text[5] != '.' ||
        text[10] != ' ' || text[13] != ':' ||
        text[16] != ':') {
        return STATUS_INVALID_DATA;
    }

    for (int i = 0; i < 19; i++) {
        if (i != 2 && i != 5 && i != 10 && i != 13 && i != 16) {
            if (text[i] < '0' || text[i] > '9') {
                return STATUS_INVALID_DATA;
            }
        }
    }

    DateTime parsed;

    parsed.day = (text[0] - '0') * 10 + (text[1] - '0');

    parsed.month = (text[3] - '0') * 10 + (text[4] - '0');

    parsed.year = (text[6] - '0') * 1000 +
                  (text[7] - '0') * 100 +
                  (text[8] - '0') * 10 +
                  (text[9] - '0');

    parsed.hour = (text[11] - '0') * 10 + (text[12] - '0');

    parsed.minute = (text[14] - '0') * 10 + (text[15] - '0');

    parsed.second = (text[17] - '0') * 10 + (text[18] - '0');

    Status status = validate_datetime(&parsed);
    if (status != STATUS_OK) {
        return status;
    }

    *result = parsed;

    return STATUS_OK;
}

Status duplicate_string(const char *text, char **result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    size_t length = strlen(text);
    if (length == SIZE_MAX) {
        return STATUS_MEMORY_ERROR;
    }
    char *copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    memcpy(copy, text, length + 1);
    *result = copy;
    return STATUS_OK;
}

Status create_stop_node(double x, double y, const DateTime *arrival, const DateTime *departure, StopType type, StopNode **result) {
    if (arrival == NULL || departure == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *result = NULL;
    if (!isfinite(x) || !isfinite(y) || (type != STOP_START && type != STOP_INTERMEDIATE && type != STOP_END)) {
        return STATUS_INVALID_DATA;
    }
    int cmp;
    Status status = compare_datetime(arrival, departure, &cmp);
    if (status != STATUS_OK) {
        return status;
    }
    if (cmp > 0) {
        return STATUS_INVALID_DATA;
    }
    StopNode *node = (StopNode *)malloc(sizeof(StopNode));
    if (node == NULL) {
        return STATUS_MEMORY_ERROR;
    }
    node->x = x;
    node->y = y;
    node->arrival = *arrival;
    node->departure = *departure;
    node->type = type;
    node->next = NULL;
    *result = node;
    return STATUS_OK;
}

Status create_route_node(const char *vehicle_id, RouteNode **result) {
    if (vehicle_id == NULL || result == NULL || vehicle_id[0] == '\0') {
        return STATUS_INVALID_ARGUMENT;
    }
    *result = NULL;
    char *copy = NULL;
    Status status = duplicate_string(vehicle_id, &copy);
    if (status != STATUS_OK) {
        return status;
    }
    RouteNode *route = (RouteNode *)malloc(sizeof(RouteNode));
    if (route == NULL) {
        free(copy);
        return STATUS_MEMORY_ERROR;
    }
    route->vehicle_id = copy;
    route->stops = NULL;
    route->next = NULL;
    *result = route;
    return STATUS_OK;
}

void free_stop_list(StopNode *head) {
    while (head != NULL) {
        StopNode *next = head->next;
        free(head);
        head = next;
    }
}

void free_route_list(RouteNode *head) {
    while (head != NULL) {
        RouteNode *next = head->next;
        free_stop_list(head->stops);
        free(head->vehicle_id);
        free(head);
        head = next;
    }
}

void free_record_list(RecordNode *head) {
    while (head != NULL) {
        RecordNode *next = head->next;
        free_stop_list(head->stop);
        free(head->vehicle_id);
        free(head);
        head = next;
    }
}

Status read_line(FILE *stream, char **result, int *has_line) {
    if (stream == NULL || result == NULL || has_line == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *result = NULL;
    *has_line = 0;
    size_t length = 0;
    size_t capacity = 0;
    char *buffer = NULL;
    int ch;
    while ((ch = fgetc(stream)) != EOF) {
        if (ch == '\0') {
            free(buffer);
            return STATUS_INVALID_DATA;
        }
        if (capacity == 0) {
            capacity = 128;
            buffer = (char *)malloc(capacity);
            if (buffer == NULL) {
                return STATUS_MEMORY_ERROR;
            }
        }
        if (length >= capacity - 1) {
            if (capacity > SIZE_MAX / 2) {
                free(buffer);
                return STATUS_MEMORY_ERROR;
            }
            size_t new_capacity = capacity * 2;
            char *new_buffer = (char *)realloc(buffer, new_capacity);
            if (new_buffer == NULL) {
                free(buffer);
                return STATUS_MEMORY_ERROR;
            }
            buffer = new_buffer;
            capacity = new_capacity;
        }
        if (ch == '\n') {
            break;
        }
        buffer[length++] = (char)ch;
    }
    if (ferror(stream)) {
        free(buffer);
        return STATUS_FILE_ERROR;
    }
    if (ch == EOF && length == 0 && buffer == NULL) {
        return STATUS_OK;
    }
    if (buffer == NULL) {
        buffer = (char *)malloc(1);
        if (buffer == NULL) {
            return STATUS_MEMORY_ERROR;
        }
    }

    buffer[length] = '\0';
    *result = buffer;
    *has_line = 1;
    return STATUS_OK;
}

char *trim_spaces(char *text) {
    while (isspace((unsigned char)*text)) {
        text++;
    }
    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }
    return text;
}

int is_decimal_number(const char *text) {
    const unsigned char *p = (const unsigned char *)text;
    while (isspace(*p)) {
        p++;
    }
    if (*p == '+' || *p == '-') {
        p++;
    }
    int has_digit = 0;
    while (*p >= '0' && *p <= '9') {
        has_digit = 1;
        p++;
    }
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            has_digit = 1;
            p++;
        }
    }
    if (!has_digit) {
        return 0;
    }
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') {
            p++;
        }
        if (*p < '0' || *p > '9') {
            return 0;
        }
        while (*p >= '0' && *p <= '9') {
            p++;
        }
    }
    while (isspace(*p)) {
        p++;
    }
    return *p == '\0';
}

Status parse_double(const char *text, double *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (!is_decimal_number(text)) {
        return STATUS_INVALID_DATA;
    }
    char *end;
    double value = strtod(text, &end);
    if (text == end || !isfinite(value)) {
        return STATUS_INVALID_DATA;
    }
    while (isspace((unsigned char)*end)) {
        end++;
    }
    if (*end != '\0') {
        return STATUS_INVALID_DATA;
    }
    if (fpclassify(value) == FP_ZERO) {
        for (const char *p = text; *p != '\0' && *p != 'e' && *p != 'E'; p++) {
            if (*p >= '1' && *p <= '9') {
                return STATUS_INVALID_DATA;
            }
        }
    }
    *result = value;
    return STATUS_OK;
}

Status parse_coordinates(char *line, double *x, double *y) {
    if (line == NULL || x == NULL || y == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    char *separator = strchr(line, ';');
    if (separator == NULL || strchr(separator + 1, ';') != NULL) {
        return STATUS_INVALID_DATA;
    }
    *separator = '\0';
    double parsed_x;
    double parsed_y;
    Status status = parse_double(line, &parsed_x);
    if (status != STATUS_OK) {
        return status;
    }
    status = parse_double(separator + 1, &parsed_y);
    if (status != STATUS_OK) {
        return status;
    }
    *x = parsed_x;
    *y = parsed_y;
    return STATUS_OK;
}

Status parse_record(char *line, double x, double y, RecordNode **result) {
    if (line == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *result = NULL;
    char *separator1 = strchr(line, ';');
    if (separator1 == NULL) {
        return STATUS_INVALID_DATA;
    }
    *separator1 = '\0';
    char *separator2 = strchr(separator1 + 1, ';');
    if (separator2 == NULL) {
        return STATUS_INVALID_DATA;
    }
    *separator2 = '\0';
    char *separator3 = strchr(separator2 + 1, ';');
    if (separator3 == NULL || strchr(separator3 + 1, ';') != NULL) {
        return STATUS_INVALID_DATA;
    }
    *separator3 = '\0';
    char *vehicle_id = trim_spaces(line);
    if (*vehicle_id == '\0') {
        return STATUS_INVALID_DATA;
    }
    DateTime arrival;
    DateTime departure;
    Status status = parse_datetime(separator1 + 1, &arrival);
    if (status != STATUS_OK) {
        return status;
    }
    status = parse_datetime(separator2 + 1, &departure);
    if (status != STATUS_OK) {
        return status;
    }
    StopType type;
    const char *marker = separator3 + 1;
    if (strcmp(marker, "START") == 0) {
        type = STOP_START;
    } else if (strcmp(marker, "MIDDLE") == 0) {
        type = STOP_INTERMEDIATE;
    } else if (strcmp(marker, "END") == 0) {
        type = STOP_END;
    } else {
        return STATUS_INVALID_DATA;
    }
    char *copy = NULL;
    status = duplicate_string(vehicle_id, &copy);
    if (status != STATUS_OK) {
        return status;
    }
    StopNode *stop = NULL;
    status = create_stop_node(x, y, &arrival, &departure, type, &stop);
    if (status != STATUS_OK) {
        free(copy);
        return status;
    }
    RecordNode *record = (RecordNode *)malloc(sizeof(RecordNode));
    if (record == NULL) {
        free(stop);
        free(copy);
        return STATUS_MEMORY_ERROR;
    }
    record->vehicle_id = copy;
    record->stop = stop;
    record->next = NULL;
    *result = record;
    return STATUS_OK;
}

Status load_file(const char *path, RecordNode **head, RecordNode **tail, size_t *bad_line) {
    if (path == NULL || path[0] == '\0' || head == NULL || tail == NULL || bad_line == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    *bad_line = 0;
    FILE *stream = fopen(path, "rb");
    if (stream == NULL) {
        return STATUS_FILE_ERROR;
    }
    char *line = NULL;
    int has_line = 0;
    Status status = read_line(stream, &line, &has_line);
    if (status == STATUS_OK && !has_line) {
        status = STATUS_INVALID_DATA;
    }
    double x = 0;
    double y = 0;
    if (status == STATUS_OK) {
        *bad_line = 1;
        status = parse_coordinates(line, &x, &y);
    }
    free(line);
    size_t line_number = 1;
    while (status == STATUS_OK) {
        line = NULL;
        has_line = 0;
        status = read_line(stream, &line, &has_line);
        if (status != STATUS_OK) {
            *bad_line = line_number + 1;
            break;
        }
        if (!has_line) {
            break;
        }
        line_number++;
        *bad_line = line_number;
        RecordNode *record = NULL;
        status = parse_record(line, x, y, &record);
        free(line);
        if (status != STATUS_OK) {
            break;
        }
        if (*tail == NULL) {
            *head = record;
        } else {
            (*tail)->next = record;
        }
        *tail = record;
    }
    if (fclose(stream) != 0 && status == STATUS_OK) {
        status = STATUS_FILE_ERROR;
    }
    return status;
}

int compare_raw_dates(const DateTime *a, const DateTime *b) {
    const int av[6] = {a->year, a->month, a->day, a->hour, a->minute, a->second};
    const int bv[6] = {b->year, b->month, b->day, b->hour, b->minute, b->second};
    for (int i = 0; i < 6; i++) {
        if (av[i] < bv[i]) {
            return -1;
        }
        if (av[i] > bv[i]) {
            return 1;
        }
    }
    return 0;
}

int marker_rank(StopType type) {
    if (type == STOP_END) {
        return 0;
    }
    if (type == STOP_START) {
        return 1;
    }
    return 2;
}

int compare_records(const RecordNode *a, const RecordNode *b) {
    int cmp = strcmp(a->vehicle_id, b->vehicle_id);
    if (cmp != 0) {
        return cmp;
    }
    cmp = compare_raw_dates(&a->stop->arrival, &b->stop->arrival);
    if (cmp != 0) {
        return cmp;
    }
    int ar = marker_rank(a->stop->type);
    int br = marker_rank(b->stop->type);
    if (ar != br) {
        if (ar < br) {
            return -1;
        } else {
            return 1;
        }
    }
    return compare_raw_dates(&a->stop->departure, &b->stop->departure);
}

RecordNode *merge_records(RecordNode *a, RecordNode *b) {
    RecordNode dummy;
    RecordNode *tail = &dummy;
    dummy.next = NULL;
    while (a != NULL && b != NULL) {
        if (compare_records(a, b) <= 0) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    if (a != NULL) {
        tail->next = a;
    } else {
        tail->next = b;
    }
    return dummy.next;
}

RecordNode *sort_records(RecordNode *head) {
    if (head == NULL || head->next == NULL) {
        return head;
    }
    RecordNode *slow = head;
    RecordNode *fast = head->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    RecordNode *second = slow->next;
    slow->next = NULL;
    RecordNode *left = sort_records(head);
    RecordNode *right = sort_records(second);
    return merge_records(left, right);
}

Status build_routes(RecordNode **records, RouteNode **result) {
    if (records == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;
    *records = sort_records(*records);

    RouteNode *routes = NULL;
    RouteNode *last_route = NULL;
    RouteNode *current_route = NULL;
    StopNode *last_stop = NULL;
    StopNode *previous_stop = NULL;
    const char *previous_vehicle = NULL;
    Status status = STATUS_OK;

    while (*records != NULL) {
        RecordNode *record = *records;
        StopNode *stop = record->stop;

        if (previous_vehicle != NULL && strcmp(previous_vehicle, record->vehicle_id) != 0) {
            if (current_route != NULL) {
                status = STATUS_INVALID_DATA;
                break;
            }
            previous_stop = NULL;
        }

        if (previous_stop != NULL) {
            int comparison;
            status = compare_datetime(&previous_stop->departure, &stop->arrival, &comparison);
            if (status != STATUS_OK) {
                break;
            }
            if (comparison > 0) {
                status = STATUS_INVALID_DATA;
                break;
            }
        }

        if (stop->type == STOP_START) {
            if (current_route != NULL) {
                status = STATUS_INVALID_DATA;
                break;
            }

            RouteNode *new_route = NULL;
            status = create_route_node(record->vehicle_id, &new_route);
            if (status != STATUS_OK) {
                break;
            }

            if (last_route == NULL) {
                routes = new_route;
            } else {
                last_route->next = new_route;
            }

            last_route = new_route;
            current_route = new_route;
            last_stop = NULL;
            previous_vehicle = new_route->vehicle_id;
        } else if (current_route == NULL) {
            status = STATUS_INVALID_DATA;
            break;
        }

        *records = record->next;
        record->stop = NULL;
        stop->next = NULL;

        if (last_stop == NULL) {
            current_route->stops = stop;
        } else {
            last_stop->next = stop;
        }
        last_stop = stop;
        previous_stop = stop;

        free(record->vehicle_id);
        free(record);

        if (stop->type == STOP_END) {
            current_route = NULL;
            last_stop = NULL;
        }
    }

    if (status == STATUS_OK && current_route != NULL) {
        status = STATUS_INVALID_DATA;
    }

    if (status != STATUS_OK) {
        free_route_list(routes);
        return status;
    }

    *result = routes;
    return STATUS_OK;
}

Status calculate_route_metrics(const RouteNode *route, RouteMetrics *result) {
    if (route == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (route->stops == NULL) {
        return STATUS_INVALID_DATA;
    }
    RouteMetrics metrics;
    metrics.distance = 0;
    metrics.idle_seconds = 0;
    metrics.longest_stop = 0;
    metrics.shortest_stop = 0;
    metrics.longest_stop_node = NULL;
    metrics.shortest_stop_node = NULL;
    const StopNode *previous = NULL;
    for (const StopNode *stop = route->stops; stop != NULL; stop = stop->next) {
        long long duration;
        Status status = calculate_duration(stop, &duration);
        if (status != STATUS_OK) {
            return status;
        }
        if (duration > LLONG_MAX - metrics.idle_seconds) {
            return STATUS_INVALID_DATA;
        }
        metrics.idle_seconds += duration;
        if (metrics.longest_stop_node == NULL || duration > metrics.longest_stop) {
            metrics.longest_stop = duration;
            metrics.longest_stop_node = stop;
        }
        if (metrics.shortest_stop_node == NULL || duration < metrics.shortest_stop) {
            metrics.shortest_stop = duration;
            metrics.shortest_stop_node = stop;
        }
        if (previous != NULL) {
            long double dx = (long double)stop->x - (long double)previous->x;
            long double dy = (long double)stop->y - (long double)previous->y;
            long double segment = hypotl(dx, dy);
            if (!isfinite(segment) || !isfinite(metrics.distance + segment)) {
                return STATUS_INVALID_DATA;
            }
            metrics.distance += segment;
        }
        previous = stop;
    }
    *result = metrics;
    return STATUS_OK;
}

Status calculate_vehicle_metrics(const RouteNode *first, const RouteNode **after, long double epsilon, VehicleMetrics *result) {
    if (first == NULL || after == NULL || result == NULL || first->vehicle_id == NULL || !isfinite(epsilon) || epsilon < 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    VehicleMetrics metrics = {0};
    const RouteNode *route = first;
    while (route != NULL && strcmp(route->vehicle_id, first->vehicle_id) == 0) {
        RouteMetrics rm;
        Status status = calculate_route_metrics(route, &rm);
        if (status != STATUS_OK) {
            return status;
        }
        if (metrics.route_count == SIZE_MAX || rm.idle_seconds > LLONG_MAX - metrics.idle_seconds ||
            !isfinite(metrics.path_distance + rm.distance)) {
            return STATUS_INVALID_DATA;
        }
        if (metrics.route_count == 0 || rm.distance - metrics.longest_route > epsilon) {
            metrics.longest_route = rm.distance;
            metrics.longest_route_node = route;
        }
        if (metrics.route_count == 0 || metrics.shortest_route - rm.distance > epsilon) {
            metrics.shortest_route = rm.distance;
            metrics.shortest_route_node = route;
        }
        if (metrics.route_count == 0 || rm.longest_stop > metrics.longest_stop) {
            metrics.longest_stop = rm.longest_stop;
            metrics.longest_stop_node = rm.longest_stop_node;
        }
        if (metrics.route_count == 0 || rm.shortest_stop < metrics.shortest_stop) {
            metrics.shortest_stop = rm.shortest_stop;
            metrics.shortest_stop_node = rm.shortest_stop_node;
        }
        metrics.route_count++;
        metrics.path_distance += rm.distance;
        metrics.idle_seconds += rm.idle_seconds;
        route = route->next;
    }
    *after = route;
    *result = metrics;
    return STATUS_OK;
}

int is_better_result(const SearchResult *candidate, const SearchResult *best, Query query, long double epsilon) {
    if (query == QUERY_MOST_ROUTES) {
        return candidate->route_count > best->route_count;
    }
    if (query == QUERY_FEWEST_ROUTES) {
        return candidate->route_count < best->route_count;
    }
    if (query == QUERY_LONGEST_STOP || query == QUERY_MOST_IDLE) {
        return candidate->seconds > best->seconds;
    }
    if (query == QUERY_SHORTEST_STOP) {
        return candidate->seconds < best->seconds;
    }
    if (fabsl(candidate->distance - best->distance) <= epsilon) {
        return 0;
    }
    if (query == QUERY_LONGEST_PATH || query == QUERY_LONGEST_ROUTE) {
        return candidate->distance > best->distance;
    }
    return candidate->distance < best->distance;
}

Status find_extreme(const RouteNode *routes, Query query, long double epsilon, SearchResult *result) {
    if (result == NULL || query < QUERY_MOST_ROUTES || query > QUERY_MOST_IDLE || !isfinite(epsilon) || epsilon < 0) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (routes == NULL) {
        return STATUS_NOT_FOUND;
    }
    int found = 0;
    const RouteNode *route = routes;
    SearchResult best = {0};
    while (route != NULL) {
        VehicleMetrics vm;
        const RouteNode *after = NULL;
        Status status = calculate_vehicle_metrics(route, &after, epsilon, &vm);
        if (status != STATUS_OK) {
            return status;
        }
        SearchResult candidate = {0};
        candidate.vehicle_id = route->vehicle_id;
        if (query == QUERY_MOST_ROUTES || query == QUERY_FEWEST_ROUTES) {
            candidate.route_count = vm.route_count;
        } else if (query == QUERY_LONGEST_PATH || query == QUERY_SHORTEST_PATH) {
            candidate.distance = vm.path_distance;
        } else if (query == QUERY_LONGEST_ROUTE) {
            candidate.distance = vm.longest_route;
            candidate.route = vm.longest_route_node;
        } else if (query == QUERY_SHORTEST_ROUTE) {
            candidate.distance = vm.shortest_route;
            candidate.route = vm.shortest_route_node;
        } else if (query == QUERY_LONGEST_STOP) {
            candidate.seconds = vm.longest_stop;
            candidate.stop = vm.longest_stop_node;
        } else if (query == QUERY_SHORTEST_STOP) {
            candidate.seconds = vm.shortest_stop;
            candidate.stop = vm.shortest_stop_node;
        } else {
            candidate.seconds = vm.idle_seconds;
        }
        if (!found || is_better_result(&candidate, &best, query, epsilon)) {
            best = candidate;
            found = 1;
        }
        route = after;
    }
    *result = best;
    return STATUS_OK;
}

const char *status_name(Status status) {
    if (status == STATUS_OK) {
        return "успешно";
    }
    if (status == STATUS_INVALID_ARGUMENT) {
        return "некорректный аргумент";
    }
    if (status == STATUS_MEMORY_ERROR) {
        return "недостаточно памяти";
    }
    if (status == STATUS_FILE_ERROR) {
        return "ошибка файла или потока";
    }
    if (status == STATUS_INVALID_DATA) {
        return "некорректные данные";
    }
    return "данные не найдены";
}

Status print_menu(void) {
    if (printf("\nВыберите запрос:\n"
               "  1. Больше всего маршрутов\n"
               "  2. Меньше всего маршрутов\n"
               "  3. Самый длинный суммарный путь\n"
               "  4. Самый короткий суммарный путь\n"
               "  5. Самый длинный отдельный маршрут\n"
               "  6. Самый короткий отдельный маршрут\n"
               "  7. Самая долгая остановка\n"
               "  8. Самая короткая остановка\n"
               "  9. Самое большое суммарное время простоя\n"
               "  0. Выход\n"
               "Ваш выбор: ") < 0) {
        return STATUS_FILE_ERROR;
    }

    return STATUS_OK;
}

Status parse_choice(const char *text, int *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    const unsigned char *p = (const unsigned char *)text;
    while (isspace(*p)) {
        p++;
    }
    if (!isdigit(*p)) {
        return STATUS_INVALID_DATA;
    }
    int value = 0;
    while (isdigit(*p)) {
        value = value * 10 + (*p - '0');
        if (value > 9) {
            return STATUS_INVALID_DATA;
        }
        p++;
    }
    while (isspace(*p)) {
        p++;
    }
    if (*p != '\0') {
        return STATUS_INVALID_DATA;
    }
    *result = value;
    return STATUS_OK;
}

Status print_search_result(Query query, const SearchResult *result) {
    if (result == NULL || result->vehicle_id == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }
    if (printf("\nТранспорт: %s\n", result->vehicle_id) < 0) {
        return STATUS_FILE_ERROR;
    }
    if (query == QUERY_MOST_ROUTES || query == QUERY_FEWEST_ROUTES) {
        if (printf("Количество маршрутов: %zu\n", result->route_count) < 0) {
            return STATUS_FILE_ERROR;
        }
    } else if (query == QUERY_LONGEST_PATH || query == QUERY_SHORTEST_PATH ||
               query == QUERY_LONGEST_ROUTE || query == QUERY_SHORTEST_ROUTE) {
        if (printf("Длина: %.9Lf условных единиц\n", result->distance) < 0) {
            return STATUS_FILE_ERROR;
        }
        if (result->route != NULL && result->route->stops != NULL) {
            const DateTime *dt = &result->route->stops->arrival;
            if (printf("Начало маршрута: %02d.%02d.%04d %02d:%02d:%02d\n",
                       dt->day, dt->month, dt->year,
                       dt->hour, dt->minute, dt->second) < 0) {
                return STATUS_FILE_ERROR;
            }
        }
    } else {
        if (printf("Время: %lld секунд\n", result->seconds) < 0) {
            return STATUS_FILE_ERROR;
        }
        if (result->stop != NULL) {
            const DateTime *dt = &result->stop->arrival;
            if (printf("Остановка: (%.6f; %.6f), прибытие %02d.%02d.%04d %02d:%02d:%02d\n",
                       result->stop->x, result->stop->y,
                       dt->day, dt->month, dt->year,
                       dt->hour, dt->minute, dt->second) < 0) {
                return STATUS_FILE_ERROR;
            }
        }
    }
    return STATUS_OK;
}

Status run_dialog(const RouteNode *routes) {
    for (;;) {
        Status status = print_menu();
        if (status != STATUS_OK) {
            return status;
        }
        char *line = NULL;
        int has_line = 0;
        status = read_line(stdin, &line, &has_line);
        if (status != STATUS_OK) {
            return status;
        }
        if (!has_line) {
            return STATUS_OK;
        }
        int choice;
        status = parse_choice(line, &choice);
        free(line);
        if (status != STATUS_OK) {
            if (puts("Некорректный пункт меню.") == EOF) {
                return STATUS_FILE_ERROR;
            }
            continue;
        }
        if (choice == 0) {
            return STATUS_OK;
        }
        SearchResult result;
        status = find_extreme(routes, (Query)choice, 1e-9L, &result);
        if (status == STATUS_NOT_FOUND) {
            if (puts("В хранилище нет завершённых маршрутов.") == EOF) {
                return STATUS_FILE_ERROR;
            }
            continue;
        }
        if (status != STATUS_OK) {
            return status;
        }
        status = print_search_result((Query)choice, &result);
        if (status != STATUS_OK) {
            return status;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2 || argv == NULL) {
        fprintf(stderr, "Использование: %s файл1 [файл2 ...]\n",
                argv != NULL && argv[0] != NULL ? argv[0] : "transport");
        return EXIT_FAILURE;
    }
    RecordNode *records = NULL;
    RecordNode *tail = NULL;
    Status status = STATUS_OK;
    for (int i = 1; i < argc; i++) {
        size_t bad_line = 0;
        status = load_file(argv[i], &records, &tail, &bad_line);
        if (status != STATUS_OK) {
            if (bad_line != 0) {
                fprintf(stderr, "Ошибка в файле '%s', строка %zu: %s\n",
                        argv[i], bad_line, status_name(status));
            } else {
                fprintf(stderr, "Ошибка файла '%s': %s\n",
                        argv[i], status_name(status));
            }
            break;
        }
    }
    RouteNode *routes = NULL;
    if (status == STATUS_OK) {
        status = build_routes(&records, &routes);
        if (status != STATUS_OK) {
            fprintf(stderr, "Не удалось восстановить маршруты: %s\n",
                    status_name(status));
        }
    }
    free_record_list(records);
    if (status == STATUS_OK) {
        status = run_dialog(routes);
        if (status != STATUS_OK) {
            fprintf(stderr, "Ошибка диалога: %s\n", status_name(status));
        }
    }
    free_route_list(routes);
    if (status == STATUS_OK) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
