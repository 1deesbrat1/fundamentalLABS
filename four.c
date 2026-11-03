#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#define STATUS_OK 0
#define STATUS_INVALID_ARGUMENT 1
#define STATUS_NO_MEMORY 2
#define STATUS_SIZE_OVERFLOW 3
#define STATUS_IMPORTANCE_OVERFLOW 4
#define STATUS_INVALID_INPUT 5
#define STATUS_INPUT_ERROR 6
#define STATUS_OUTPUT_ERROR 7
#define STATUS_END_OF_INPUT 8
#define STATUS_NUMBER_OVERFLOW 9


#define INPUT_BUFFER_SIZE 4096
#define OUTPUT_CHUNK_SIZE 4096



typedef struct {
    char *name;
    size_t size;
    uint64_t importance;
} File;

typedef struct {
    const File **selected_files;
    size_t count;
    size_t total_size;
    uint64_t total_importance;
} BackupResult;

int select_backup_files(
    const File *files,
    size_t count,
    size_t capacity,
    BackupResult *result
);

typedef struct {
    unsigned char buffer[INPUT_BUFFER_SIZE];
    size_t position;
    size_t length;
    int eof_reached;
} InputReader;


int backup_result_init(BackupResult* result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    result->selected_files = NULL;
    result->count = 0;
    result->total_size = 0;
    result->total_importance = 0;

    return STATUS_OK;
}


int backup_result_destroy(BackupResult* result) {

    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    free(result->selected_files);

    result->selected_files = NULL;
    result->count = 0;
    result->total_size = 0;
    result->total_importance = 0;

    return STATUS_OK;
}


int files_create(size_t count, File** files_out) {
    File* created_files = NULL;
    size_t i;

    if (files_out == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *files_out = NULL;

    if (count == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (count > SIZE_MAX / sizeof(File)) {
        return STATUS_SIZE_OVERFLOW;
    }

    created_files = malloc(count * sizeof(File));

    if (created_files == NULL) {
        return STATUS_NO_MEMORY;
    }

    for (i = 0; i < count; ++i) {
        created_files[i].name = NULL;
        created_files[i].size = 0;
        created_files[i].importance = 0;
    }


    *files_out = created_files;

    return STATUS_OK;
}

int files_destroy(File** files, size_t count) {
    size_t i;

    if (files == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*files == NULL) {
        return STATUS_OK;
    }

    if (count == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (i = 0; i < count; i++) {
        free((*files)[i].name);
        (*files)[i].name = NULL;
    }

    free(*files);
    *files = NULL;

    return STATUS_OK;
}


int file_set_name(File *file, const char *name) {
    size_t length;
    size_t i;
    char *copy;

    if (file == NULL || name == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (file->name != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    length = strlen(name);

    if (length == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (length == SIZE_MAX) {
        return STATUS_SIZE_OVERFLOW;
    }

    for (i = 0; i < length; ++i) {
        if (isspace((unsigned char)name[i])) {
            return STATUS_INVALID_ARGUMENT;
        }
    }

    copy = malloc(length + 1);

    if (copy == NULL) {
        return STATUS_NO_MEMORY;
    }

    memcpy(copy, name, length + 1);

    file->name = copy;

    return STATUS_OK;
}

int file_set_size(File *file, size_t size) {
    if (file == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (size == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    file->size = size;

    return STATUS_OK;
}

int file_set_importance(File *file, uint64_t importance) {
    if (file == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    file->importance = importance;

    return STATUS_OK;
}

int file_validate(const File *file) {
    const char *cursor;

    if (file == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (file->name == NULL || file->name[0] == '\0') {
        return STATUS_INVALID_INPUT;
    }

    if (file->size == 0) {
        return STATUS_INVALID_INPUT;
    }

    cursor = file->name;

    while (*cursor != '\0') {
        if (isspace((unsigned char)*cursor)) {
            return STATUS_INVALID_INPUT;
        }

        ++cursor;
    }

    return STATUS_OK;
}

int files_validate(const File *files, size_t count) {
    size_t i;
    size_t j;
    int status;

    if (files == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (count == 0) {
        return STATUS_INVALID_INPUT;
    }

    for (i = 0; i < count; ++i) {
        status = file_validate(&files[i]);

        if (status != STATUS_OK) {
            return status;
        }
    }

    for (i = 0; i < count; ++i) {
        for (j = 0; j < i; ++j) {
            if (strcmp(files[i].name, files[j].name) == 0) {
                return STATUS_INVALID_INPUT;
            }
        }
    }

    return STATUS_OK;
}

int input_reader_init(InputReader *reader) {
    if (reader == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    reader->position = 0;
    reader->length = 0;
    reader->eof_reached = 0;

    return STATUS_OK;
}

int input_read_char(InputReader *reader, unsigned char *character) {
    ssize_t bytes_read;

    if (reader == NULL || character == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (reader->position >= reader->length) {
        if (reader->eof_reached) {
            return STATUS_END_OF_INPUT;
        }

        bytes_read = read(STDIN_FILENO, reader->buffer, sizeof(reader->buffer));

        if (bytes_read < 0) {
            return STATUS_INPUT_ERROR;
        }

        if (bytes_read == 0) {
            reader->eof_reached = 1;
            return STATUS_END_OF_INPUT;
        }

        reader->position = 0;
        reader->length = (size_t)bytes_read;
    }

    *character = reader->buffer[reader->position];

    ++reader->position;

    return STATUS_OK;
}

int input_read_token(InputReader *reader, char **token_out) {
    char *token = NULL;
    size_t length = 0;
    size_t capacity = 0;
    unsigned char character;
    int token_started = 0;
    int status;

    if (reader == NULL || token_out == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *token_out = NULL;

    while (1) {
        status = input_read_char(reader, &character);

        if (status == STATUS_END_OF_INPUT) {
            if (!token_started) {
                free(token);
                return STATUS_END_OF_INPUT;
            }

            break;
        }

        if (status != STATUS_OK) {
            free(token);
            return status;
        }

        if (character == '\0') {
            free(token);
            return STATUS_INVALID_INPUT;
        }

        if (isspace((unsigned char)character)) {
            if (token_started) {
                break;
            }

            continue;
        }

        token_started = 1;

        if (length > SIZE_MAX - 2) {
            free(token);
            return STATUS_SIZE_OVERFLOW;
        }

        if (capacity < length + 2) {
            size_t required = length + 2;
            size_t new_capacity;
            char *temporary;

            if (capacity == 0) {
                new_capacity = 16;
            } else if (capacity > SIZE_MAX / 2) {
                new_capacity = required;
            } else {
                new_capacity = capacity * 2;
            }

            if (new_capacity < required) {
                new_capacity = required;
            }

            temporary = realloc(token, new_capacity);

            if (temporary == NULL) {
                free(token);
                return STATUS_NO_MEMORY;
            }

            token = temporary;
            capacity = new_capacity;
        }

        token[length] = (char)character;
        ++length;
        token[length] = '\0';
    }

    *token_out = token;

    return STATUS_OK;
}

int parse_uintmax(const char *token, uintmax_t *value_out) {
    size_t i = 0;
    uintmax_t value = 0;

    if (token == NULL || value_out == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (token[0] == '\0') {
        return STATUS_INVALID_INPUT;
    }

    while (token[i] != '\0') {
        uintmax_t digit;

        if (token[i] < '0' || token[i] > '9') {
            return STATUS_INVALID_INPUT;
        }

        digit = (uintmax_t)(token[i] - '0');

        if (value > (UINTMAX_MAX - digit) / 10) {
            return STATUS_NUMBER_OVERFLOW;
        }

        value = value * 10 + digit;
        ++i;
    }

    *value_out = value;

    return STATUS_OK;
}

int input_read_required_token(InputReader *reader, char **token_out) {
    int status;

    if (reader == NULL || token_out == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    status = input_read_token(reader, token_out);

    if (status == STATUS_END_OF_INPUT) {
        return STATUS_INVALID_INPUT;
    }

    return status;
}

int input_read_uintmax(InputReader *reader, uintmax_t *value_out) {
    char *token = NULL;
    int status;

    if (reader == NULL || value_out == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    status = input_read_required_token(reader, &token);

    if (status != STATUS_OK) {
        free(token);
        return status;
    }

    status = parse_uintmax(token, value_out);

    free(token);

    return status;
}

static int files_read_failure(File **files, size_t count, int original_status) {
    int cleanup_status;

    if (files != NULL && *files != NULL) {
        cleanup_status = files_destroy(files, count);

        if (cleanup_status != STATUS_OK) {
            return cleanup_status;
        }
    }

    return original_status;
}

int files_read(InputReader *reader, File **files_out, size_t *count_out, size_t *capacity_out) {
    
    uintmax_t count_value;
    uintmax_t capacity_value;
    uintmax_t size_value;
    uintmax_t importance_value;

    size_t count;
    size_t capacity;
    size_t i;

    File *files = NULL;
    char *name = NULL;
    int status;

    if (reader == NULL || files_out == NULL || count_out == NULL || capacity_out == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *files_out = NULL;
    *count_out = 0;
    *capacity_out = 0;

    status = input_read_uintmax(reader, &count_value);

    if (status != STATUS_OK) {
        return status;
    }

    if (count_value == 0) {
        return STATUS_INVALID_INPUT;
    }

    if (count_value > SIZE_MAX) {
        return STATUS_SIZE_OVERFLOW;
    }

    count = (size_t)count_value;

    status = input_read_uintmax(reader, &capacity_value);

    if (status != STATUS_OK) {
        return status;
    }

    if (capacity_value > SIZE_MAX) {
        return STATUS_SIZE_OVERFLOW;
    }

    capacity = (size_t)capacity_value;

    status = files_create(count, &files);

    if (status != STATUS_OK) {
        return status;
    }

    for (i = 0; i < count; ++i) {
        status = input_read_required_token(reader, &name);

        if (status != STATUS_OK) {
            free(name);
            return files_read_failure(&files, count, status);
        }

        status = file_set_name(&files[i], name);

        free(name);
        name = NULL;

        if (status != STATUS_OK) {
            return files_read_failure(&files, count, status);
        }

        status = input_read_uintmax(reader, &size_value);

        if (status != STATUS_OK) {
            return files_read_failure(&files, count, status);
        }

        if (size_value == 0) {
            return files_read_failure(&files, count, STATUS_INVALID_INPUT);
        }

        if (size_value > SIZE_MAX) {
            return files_read_failure(&files, count, STATUS_SIZE_OVERFLOW);
        }

        status = file_set_size(&files[i], (size_t)size_value);

        if (status != STATUS_OK) {
            return files_read_failure(&files, count, status);
        }

        status = input_read_uintmax(reader, &importance_value);

        if (status != STATUS_OK) {
            return files_read_failure(&files, count, status);
        }

        if (importance_value > UINT64_MAX) {
            return files_read_failure(&files, count, STATUS_IMPORTANCE_OVERFLOW);
        }

        status = file_set_importance(&files[i], (uint64_t)importance_value);

        if (status != STATUS_OK) {
            return files_read_failure(&files, count, status);
        }
    }

    status = files_validate(files, count);

    if (status != STATUS_OK) {
        return files_read_failure(&files, count, status);
    }

    status = input_read_token(reader, &name);

    if (status == STATUS_OK) {
        free(name);

        return files_read_failure(&files, count, STATUS_INVALID_INPUT);
    }

    if (status != STATUS_END_OF_INPUT) {
        return files_read_failure(&files, count, status);
    }

    *files_out = files;
    *count_out = count;
    *capacity_out = capacity;

    return STATUS_OK;
}

int select_backup_files(const File *files, size_t count, size_t capacity, BackupResult *result) {
    uint64_t *dp = NULL;
    const File **selected = NULL;

    size_t rows;
    size_t columns;
    size_t cells;

    size_t i;
    size_t c;
    size_t remaining_capacity;
    size_t selected_count = 0;
    size_t selected_position;
    size_t total_size = 0;

    int status;

    if (files == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (count == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->selected_files != NULL || result->count != 0 || result->total_size != 0 || result->total_importance != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    status = files_validate(files, count);

    if (status != STATUS_OK) {
        return status;
    }

    if (count == SIZE_MAX || capacity == SIZE_MAX) {
        return STATUS_SIZE_OVERFLOW;
    }

    rows = count + 1;
    columns = capacity + 1;

    if (rows > SIZE_MAX / columns) {
        return STATUS_SIZE_OVERFLOW;
    }

    cells = rows * columns;

    if (cells > SIZE_MAX / sizeof(*dp)) {
        return STATUS_SIZE_OVERFLOW;
    }

    dp = calloc(cells, sizeof(*dp));

    if (dp == NULL) {
        return STATUS_NO_MEMORY;
    }

    for (i = 1; i <= count; ++i) {
        const File *file = &files[i - 1];

        for (c = 0; c <= capacity; ++c) {
            uint64_t best;
            uint64_t previous;

            best = dp[(i - 1) * columns + c];

            if (file->size <= c) {
                previous = dp[
                    (i - 1) * columns + (c - file->size)
                ];

                if (previous > UINT64_MAX - file->importance) {
                    free(dp);
                    return STATUS_IMPORTANCE_OVERFLOW;
                }

                previous += file->importance;

                if (previous > best) {
                    best = previous;
                }
            }

            dp[i * columns + c] = best;
        }
    }

    remaining_capacity = capacity;

    for (i = count; i > 0; --i) {
        uint64_t current;
        uint64_t previous;

        current = dp[i * columns + remaining_capacity];

        previous = dp[
            (i - 1) * columns + remaining_capacity
        ];

        if (current != previous) {
            if (files[i - 1].size > remaining_capacity) {
                free(dp);
                return STATUS_INVALID_INPUT;
            }

            remaining_capacity -= files[i - 1].size;
            ++selected_count;
        }
    }

    if (selected_count > SIZE_MAX / sizeof(*selected)) {
        free(dp);
        return STATUS_SIZE_OVERFLOW;
    }

    if (selected_count > 0) {
        selected = malloc(selected_count * sizeof(*selected));

        if (selected == NULL) {
            free(dp);
            return STATUS_NO_MEMORY;
        }
    }

    remaining_capacity = capacity;
    selected_position = selected_count;

    for (i = count; i > 0; --i) {
        uint64_t current;
        uint64_t previous;

        current = dp[i * columns + remaining_capacity];

        previous = dp[
            (i - 1) * columns + remaining_capacity
        ];

        if (current != previous) {
            if (selected_position == 0 ||
                files[i - 1].size > remaining_capacity) {
                free(selected);
                free(dp);
                return STATUS_INVALID_INPUT;
            }

            --selected_position;
            selected[selected_position] = &files[i - 1];

            remaining_capacity -= files[i - 1].size;
        }
    }

    if (selected_position != 0) {
        free(selected);
        free(dp);
        return STATUS_INVALID_INPUT;
    }

    for (i = 0; i < selected_count; ++i) {
        if (selected[i]->size > capacity - total_size) {
            free(selected);
            free(dp);
            return STATUS_SIZE_OVERFLOW;
        }

        total_size += selected[i]->size;
    }

    result->selected_files = selected;
    result->count = selected_count;
    result->total_size = total_size;
    result->total_importance =
        dp[count * columns + capacity];

    free(dp);

    return STATUS_OK;
}


static int output_write_all(const char *data, size_t length) {
    size_t offset = 0;

    if (data == NULL && length != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    while (offset < length) {
        size_t chunk_size = length - offset;
        ssize_t written;

        if (chunk_size > OUTPUT_CHUNK_SIZE) {
            chunk_size = OUTPUT_CHUNK_SIZE;
        }

        written = write(STDOUT_FILENO,data + offset,chunk_size);

        if (written <= 0) {
            return STATUS_OUTPUT_ERROR;
        }

        offset += (size_t)written;
    }

    return STATUS_OK;
}

static int write_all_to_fd(int fd, const char *data, size_t length) {
    size_t offset = 0;

    if (data == NULL && length != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    while (offset < length) {
        size_t remaining = length - offset;
        ssize_t written;

        written = write(
            fd,
            data + offset,
            remaining
        );

        if (written <= 0) {
            return STATUS_OUTPUT_ERROR;
        }

        offset += (size_t)written;
    }

    return STATUS_OK;
}

static int output_write_text(const char *text) {
    if (text == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    return output_write_all(text, strlen(text));
}

static int output_write_uintmax(uintmax_t value) {
    char buffer[sizeof(uintmax_t) * CHAR_BIT];
    size_t position = sizeof(buffer);

    do {
        unsigned int digit = (unsigned int)(value % 10);

        buffer[--position] = (char)('0' + digit);
        value /= 10;
    } while (value != 0);

    return output_write_all(buffer + position, sizeof(buffer) - position);
}

int backup_result_write(const BackupResult *result) {
    size_t i;
    size_t j;
    size_t calculated_size = 0;
    uint64_t calculated_importance = 0;
    int status;

    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->count == 0) {
        if (result->selected_files != NULL || result->total_size != 0 || result->total_importance != 0) {
            return STATUS_INVALID_INPUT;
        }
    } else {
        if (result->selected_files == NULL) {
            return STATUS_INVALID_ARGUMENT;
        }

        for (i = 0; i < result->count; ++i) {
            const File *file = result->selected_files[i];

            if (file == NULL) {
                return STATUS_INVALID_INPUT;
            }

            status = file_validate(file);

            if (status != STATUS_OK) {
                return status;
            }

            for (j = 0; j < i; ++j) {
                if (strcmp(file->name, result->selected_files[j]->name) == 0) {
                    return STATUS_INVALID_INPUT;
                }
            }

            if (file->size > SIZE_MAX - calculated_size) {
                return STATUS_SIZE_OVERFLOW;
            }

            calculated_size += file->size;

            if (file->importance > UINT64_MAX - calculated_importance) {
                return STATUS_IMPORTANCE_OVERFLOW;
            }

            calculated_importance += file->importance;
        }

        if (calculated_size != result->total_size || calculated_importance != result->total_importance) {
            return STATUS_INVALID_INPUT;
        }
    }

    status = output_write_text("TOTAL SIZE ");

    if (status != STATUS_OK) {
        return status;
    }

    status = output_write_uintmax((uintmax_t)result->total_size);

    if (status != STATUS_OK) {
        return status;
    }

    status = output_write_text("\nTOTAL IMPORTANCE ");

    if (status != STATUS_OK) {
        return status;
    }

    status = output_write_uintmax((uintmax_t)result->total_importance);

    if (status != STATUS_OK) {
        return status;
    }

    status = output_write_text("\nFILES ");

    if (status != STATUS_OK) {
        return status;
    }

    status = output_write_uintmax((uintmax_t)result->count);

    if (status != STATUS_OK) {
        return status;
    }

    status = output_write_text("\n");

    if (status != STATUS_OK) {
        return status;
    }

    for (i = 0; i < result->count; ++i) {
        status = output_write_text(result->selected_files[i]->name);

        if (status != STATUS_OK) {
            return status;
        }

        status = output_write_text("\n");

        if (status != STATUS_OK) {
            return status;
        }
    }

    return STATUS_OK;
}

static const char *status_message(int status) {
    if (status == STATUS_OK) {
        return "Success";
    }

    if (status == STATUS_INVALID_ARGUMENT) {
        return "invalid argument";
    }

    if (status == STATUS_NO_MEMORY) {
        return "not enough memory";
    }

    if (status == STATUS_SIZE_OVERFLOW) {
        return "size overflow";
    }

    if (status == STATUS_IMPORTANCE_OVERFLOW) {
        return "importance overflow";
    }

    if (status == STATUS_INVALID_INPUT) {
        return "invalid input";
    }

    if (status == STATUS_INPUT_ERROR) {
        return "input read failed";
    }

    if (status == STATUS_OUTPUT_ERROR) {
        return "output write failed";
    }

    if (status == STATUS_END_OF_INPUT) {
        return "unexpected end of input";
    }

    if (status == STATUS_NUMBER_OVERFLOW) {
        return "integer is too large";
    }

    return "unknown failure";
}

int main(void) {
    InputReader reader;
    File *files = NULL;
    size_t count = 0;
    size_t capacity = 0;
    BackupResult result = { NULL, 0, 0, 0 };
    int status;
    int cleanup_status;

    status = input_reader_init(&reader);

    if (status == STATUS_OK) {
        status = files_read(&reader, &files, &count, &capacity);
    }

    if (status == STATUS_OK) {
        status = backup_result_init(&result);
    }

    if (status == STATUS_OK) {
        status = select_backup_files(files, count, capacity, &result);
    }

    if (status == STATUS_OK) {
        status = backup_result_write(&result);
    }

    cleanup_status = backup_result_destroy(&result);

    if (status == STATUS_OK && cleanup_status != STATUS_OK) {
        status = cleanup_status;
    }

    cleanup_status = files_destroy(&files, count);

    if (status == STATUS_OK && cleanup_status != STATUS_OK) {
        status = cleanup_status;
    }

    if (status != STATUS_OK) {
        const char *message = status_message(status);
        const char prefix[] = "Error: ";

        (void)write_all_to_fd(STDERR_FILENO, prefix, sizeof(prefix) - 1);

        (void)write_all_to_fd(STDERR_FILENO, message, strlen(message));

        (void)write_all_to_fd(STDERR_FILENO, "\n", 1);
        write_all_to_fd(STDERR_FILENO, "\n", 1);

        return 1;
    }

    return 0;
}