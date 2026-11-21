#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>

typedef struct {
    char *data;
    int length;
} String;

typedef enum {
    STATUS_OK,
    STATUS_INVALID_ARGUMENT,
    STATUS_MEMORY_ERROR,
    STATUS_LENGTH_OVERFLOW,
    STATUS_TIME_ERROR,
    STATUS_DUPLICATE_ID,
    STATUS_NOT_FOUND,
    STATUS_IO_ERROR
} Status;

Status string_create(const char *source, String *result) {
    if (source == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->data != NULL || result->length != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t length = strlen(source);

    if (length > INT_MAX) {
        return STATUS_LENGTH_OVERFLOW;
    }

    char *data = (char *)malloc(length + 1);

    if (data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    memcpy(data, source, length + 1);

    result->data = data;
    result->length = (int)length;

    return STATUS_OK;
}

Status string_destroy(String *str) {
    if (str == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (str->length < 0 || (str->data == NULL && str->length != 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    free(str->data);

    str->data = NULL;
    str->length = 0;

    return STATUS_OK;
}

Status string_compare(const String *first, const String *second, int *comparison) {
    if (first == NULL || second == NULL || comparison == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first->length < 0 || second->length < 0 || (first->data == NULL && first->length != 0) || (second->data == NULL && second->length != 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first->length < second->length) {
        *comparison = -1;
        return STATUS_OK;
    }

    if (first->length > second->length) {
        *comparison = 1;
        return STATUS_OK;
    }

    for (int i = 0; i < first->length; i++) {
        unsigned char a = (unsigned char)first->data[i];
        unsigned char b = (unsigned char)second->data[i];

        if (a < b) {
            *comparison = -1;
            return STATUS_OK;
        }

        if (a > b) {
            *comparison = 1;
            return STATUS_OK;
        }
    }

    *comparison = 0;

    return STATUS_OK;
}

Status string_equal(const String *first, const String *second, int *equal) {
    if (first == NULL || second == NULL || equal == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first->length < 0 || second->length < 0 || (first->data == NULL && first->length != 0) || (second->data == NULL && second->length != 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first->length != second->length) {
        *equal = 0;
        return STATUS_OK;
    }

    for (int i = 0; i < first->length; i++) {
        if (first->data[i] != second->data[i]) {
            *equal = 0;
            return STATUS_OK;
        }
    }

    *equal = 1;

    return STATUS_OK;
}

Status string_copy(const String *source, String *destination) {
    if (source == NULL || destination == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (source->length < 0 || destination->length < 0 ||
        (source->data == NULL && source->length != 0) ||
        (destination->data == NULL && destination->length != 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (source == destination) {
        return STATUS_OK;
    }

    char *new_data = (char *)malloc((size_t)source->length + 1);

    if (new_data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i < source->length; i++) {
        new_data[i] = source->data[i];
    }

    new_data[source->length] = '\0';

    free(destination->data);

    destination->data = new_data;
    destination->length = source->length;

    return STATUS_OK;
}

Status string_clone(const String *source, String **result) {
    if (source == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (source->length < 0 || (source->data == NULL && source->length != 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*result != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    String *copy = (String *)malloc(sizeof(String));

    if (copy == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    copy->data = NULL;
    copy->length = 0;

    Status status = string_copy(source, copy);

    if (status != STATUS_OK) {
        free(copy);
        return status;
    }

    *result = copy;

    return STATUS_OK;
}

Status string_concat(String *first, const String *second) {
    if (first == NULL || second == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first->length < 0 || second->length < 0 || (first->data == NULL && first->length != 0) || (second->data == NULL && second->length != 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (first->length > INT_MAX - second->length) {
        return STATUS_LENGTH_OVERFLOW;
    }

    int total_length = first->length + second->length;

    char *new_data = (char *)malloc((size_t)total_length + 1);

    if (new_data == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    for (int i = 0; i < first->length; i++) {
        new_data[i] = first->data[i];
    }

    for (int i = 0; i < second->length; i++) {
        new_data[first->length + i] = second->data[i];
    }

    new_data[total_length] = '\0';

    free(first->data);

    first->data = new_data;
    first->length = total_length;

    return STATUS_OK;
}

typedef struct {
    String city;
    String street;
    int house;
    String building;
    int apartment;
    String postal_code;
} Address;

Status address_validate(const char *city, const char *street, int house, const char *building, int apartment, const char *postal_code) {
    if (city == NULL || street == NULL ||
        building == NULL || postal_code == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t city_length = strlen(city);
    size_t street_length = strlen(street);
    size_t building_length = strlen(building);

    if (city_length == 0 || street_length == 0 || house <= 0 || apartment <= 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (city_length > INT_MAX || street_length > INT_MAX || building_length > INT_MAX) {
        return STATUS_LENGTH_OVERFLOW;
    }

    int digits = 0;

    while (postal_code[digits] != '\0') {
        if (digits == 6) {
            return STATUS_INVALID_ARGUMENT;
        }

        if (postal_code[digits] < '0' || postal_code[digits] > '9') {
            return STATUS_INVALID_ARGUMENT;
        }

        digits++;
    }

    if (digits != 6) {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

Status address_create(const char *city, const char *street, int house, const char *building, int apartment, const char *postal_code, Address *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->city.data != NULL || result->city.length != 0 ||
        result->street.data != NULL || result->street.length != 0 ||
        result->building.data != NULL || result->building.length != 0 ||
        result->postal_code.data != NULL || result->postal_code.length != 0 ||
        result->house != 0 || result->apartment != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = address_validate(city, street, house, building, apartment, postal_code);

    if (status != STATUS_OK) {
        return status;
    }

    Address temp = {0};

    status = string_create(city, &temp.city);

    if (status == STATUS_OK) {
        status = string_create(street, &temp.street);
    }

    if (status == STATUS_OK) {
        status = string_create(building, &temp.building);
    }

    if (status == STATUS_OK) {
        status = string_create(postal_code, &temp.postal_code);
    }

    if (status != STATUS_OK) {
        string_destroy(&temp.city);
        string_destroy(&temp.street);
        string_destroy(&temp.building);
        string_destroy(&temp.postal_code);

        return status;
    }

    temp.house = house;
    temp.apartment = apartment;

    *result = temp;

    return STATUS_OK;
}

Status address_destroy(Address *address) {
    if (address == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status city_status = string_destroy(&address->city);
    Status street_status = string_destroy(&address->street);
    Status building_status = string_destroy(&address->building);
    Status postal_status = string_destroy(&address->postal_code);

    if (city_status != STATUS_OK ||
        street_status != STATUS_OK ||
        building_status != STATUS_OK ||
        postal_status != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }

    address->house = 0;
    address->apartment = 0;

    return STATUS_OK;
}

Status address_copy(const Address *source, Address *destination) {
    if (source == NULL || destination == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (source == destination) {
        return address_validate(
            source->city.data,
            source->street.data,
            source->house,
            source->building.data,
            source->apartment,
            source->postal_code.data
        );
    }

    Address temp = {0};

    Status status = address_create(
        source->city.data,
        source->street.data,
        source->house,
        source->building.data,
        source->apartment,
        source->postal_code.data,
        &temp
    );

    if (status != STATUS_OK) {
        return status;
    }

    status = address_destroy(destination);

    if (status != STATUS_OK) {
        address_destroy(&temp);
        return status;
    }

    *destination = temp;

    return STATUS_OK;
}

typedef struct {
    Address recipient;
    double weight;
    String id;
    String created_at;
    String delivered_at;
} Mail;

typedef struct {
    int day;
    int month;
    int year;
    int hour;
    int minute;
    int second;
} DateTime;

Status datetime_parse(const char *text, DateTime *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (strlen(text) != 19) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (int i = 0; i < 19; i++) {
        if (i == 2 || i == 5 || i == 13 || i == 16) {
            if (text[i] != ':') {
                return STATUS_INVALID_ARGUMENT;
            }
        } else if (i == 10) {
            if (text[i] != ' ') {
                return STATUS_INVALID_ARGUMENT;
            }
        } else {
            if (text[i] < '0' || text[i] > '9') {
                return STATUS_INVALID_ARGUMENT;
            }
        }
    }

    DateTime temp = {0};

    temp.day = (text[0] - '0') * 10 + (text[1] - '0');
    temp.month = (text[3] - '0') * 10 + (text[4] - '0');

    temp.year = (text[6] - '0') * 1000 + (text[7] - '0') * 100 + (text[8] - '0') * 10 + (text[9] - '0');

    temp.hour = (text[11] - '0') * 10 + (text[12] - '0');
    temp.minute = (text[14] - '0') * 10 + (text[15] - '0');
    temp.second = (text[17] - '0') * 10 + (text[18] - '0');

    if (temp.year == 0 || temp.month < 1 || temp.month > 12 ||
        temp.hour > 23 || temp.minute > 59 || temp.second > 59) {
        return STATUS_INVALID_ARGUMENT;
    }

    int max_day = 31;

    if (temp.month == 4 || temp.month == 6 || temp.month == 9 || temp.month == 11) {
        max_day = 30;
    } else if (temp.month == 2) {
        max_day = 28;

        if (temp.year % 400 == 0 || (temp.year % 4 == 0 && temp.year % 100 != 0)) {
            max_day = 29;
        }
    }

    if (temp.day < 1 || temp.day > max_day) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = temp;

    return STATUS_OK;
}

Status datetime_compare(const char *first, const char *second, int *comparison) {
    if (first == NULL || second == NULL || comparison == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    DateTime first_date = {0};
    DateTime second_date = {0};

    Status status = datetime_parse(first, &first_date);

    if (status != STATUS_OK) {
        return status;
    }

    status = datetime_parse(second, &second_date);

    if (status != STATUS_OK) {
        return status;
    }

    const int first_values[6] = {
        first_date.year,
        first_date.month,
        first_date.day,
        first_date.hour,
        first_date.minute,
        first_date.second
    };

    const int second_values[6] = {
        second_date.year,
        second_date.month,
        second_date.day,
        second_date.hour,
        second_date.minute,
        second_date.second
    };

    for (int i = 0; i < 6; i++) {
        if (first_values[i] < second_values[i]) {
            *comparison = -1;
            return STATUS_OK;
        }

        if (first_values[i] > second_values[i]) {
            *comparison = 1;
            return STATUS_OK;
        }
    }

    *comparison = 0;

    return STATUS_OK;
}

Status datetime_now(DateTime *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    time_t current_time = time(NULL);

    if (current_time == (time_t)-1) {
        return STATUS_TIME_ERROR;
    }

    struct tm *local_time = localtime(&current_time);

    if (local_time == NULL) {
        return STATUS_TIME_ERROR;
    }

    long long year = (long long)local_time->tm_year + 1900LL;

    if (year < 1 || year > 9999 ||
        local_time->tm_mon < 0 || local_time->tm_mon > 11 ||
        local_time->tm_mday < 1 || local_time->tm_mday > 31 ||
        local_time->tm_hour < 0 || local_time->tm_hour > 23 ||
        local_time->tm_min < 0 || local_time->tm_min > 59 ||
        local_time->tm_sec < 0 || local_time->tm_sec > 59) {
        return STATUS_TIME_ERROR;
    }

    DateTime temp = {0};

    temp.day = local_time->tm_mday;
    temp.month = local_time->tm_mon + 1;
    temp.year = (int)year;
    temp.hour = local_time->tm_hour;
    temp.minute = local_time->tm_min;
    temp.second = local_time->tm_sec;

    *result = temp;

    return STATUS_OK;
}

Status mail_validate(const Address *recipient, double weight, const char *id, const char *created_at, const char *delivered_at) {
    if (recipient == NULL || id == NULL ||
        created_at == NULL || delivered_at == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = address_validate(
        recipient->city.data,
        recipient->street.data,
        recipient->house,
        recipient->building.data,
        recipient->apartment,
        recipient->postal_code.data
    );

    if (status != STATUS_OK) {
        return status;
    }

    if (!isfinite(weight) || (signbit(weight) && fpclassify(weight) != FP_ZERO)) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (int i = 0; i < 14; i++) {
        if (id[i] < '0' || id[i] > '9') {
            return STATUS_INVALID_ARGUMENT;
        }
    }

    if (id[14] != '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    int comparison = 0;

    status = datetime_compare(created_at, delivered_at, &comparison);

    if (status != STATUS_OK) {
        return status;
    }

    if (comparison > 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

Status mail_create(const Address *recipient, double weight, const char *id, const char *created_at, const char *delivered_at, Mail *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->recipient.city.data != NULL ||
        result->recipient.city.length != 0 ||
        result->recipient.street.data != NULL ||
        result->recipient.street.length != 0 ||
        result->recipient.building.data != NULL ||
        result->recipient.building.length != 0 ||
        result->recipient.postal_code.data != NULL ||
        result->recipient.postal_code.length != 0 ||
        result->recipient.house != 0 ||
        result->recipient.apartment != 0 ||
        result->id.data != NULL ||
        result->id.length != 0 ||
        result->created_at.data != NULL ||
        result->created_at.length != 0 ||
        result->delivered_at.data != NULL ||
        result->delivered_at.length != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = mail_validate(recipient, weight, id, created_at, delivered_at);

    if (status != STATUS_OK) {
        return status;
    }

    Mail temp = {0};

    status = address_copy(recipient, &temp.recipient);

    if (status == STATUS_OK) {
        status = string_create(id, &temp.id);
    }

    if (status == STATUS_OK) {
        status = string_create(created_at, &temp.created_at);
    }

    if (status == STATUS_OK) {
        status = string_create(delivered_at, &temp.delivered_at);
    }

    if (status != STATUS_OK) {
        string_destroy(&temp.delivered_at);
        string_destroy(&temp.created_at);
        string_destroy(&temp.id);
        address_destroy(&temp.recipient);

        return status;
    }

    temp.weight = weight;

    *result = temp;

    return STATUS_OK;
}

Status mail_destroy(Mail *mail) {
    if (mail == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status address_status = address_destroy(&mail->recipient);
    Status id_status = string_destroy(&mail->id);
    Status created_status = string_destroy(&mail->created_at);
    Status delivered_status = string_destroy(&mail->delivered_at);

    if (address_status != STATUS_OK ||
        id_status != STATUS_OK ||
        created_status != STATUS_OK ||
        delivered_status != STATUS_OK) {
        return STATUS_INVALID_ARGUMENT;
    }

    mail->weight = 0.0;

    return STATUS_OK;
}

Status mail_copy(const Mail *source, Mail *destination) {
    if (source == NULL || destination == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (source == destination) {
        return mail_validate(
            &source->recipient,
            source->weight,
            source->id.data,
            source->created_at.data,
            source->delivered_at.data
        );
    }

    Mail temp = {0};

    Status status = mail_create(
        &source->recipient,
        source->weight,
        source->id.data,
        source->created_at.data,
        source->delivered_at.data,
        &temp
    );

    if (status != STATUS_OK) {
        return status;
    }

    status = mail_destroy(destination);

    if (status != STATUS_OK) {
        mail_destroy(&temp);
        return status;
    }

    *destination = temp;

    return STATUS_OK;
}

typedef struct {
    Address *office_address;
    Mail *mails;
    size_t count;
    size_t capacity;
} Post;

Status post_create(const Address *office_address, Post *result) {
    if (office_address == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (result->office_address != NULL ||
        result->mails != NULL ||
        result->count != 0 ||
        result->capacity != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    Address *new_address = (Address *)malloc(sizeof(Address));

    if (new_address == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    *new_address = (Address){0};

    Status status = address_copy(office_address, new_address);

    if (status != STATUS_OK) {
        free(new_address);
        return status;
    }

    result->office_address = new_address;
    result->mails = NULL;
    result->count = 0;
    result->capacity = 0;

    return STATUS_OK;
}


Status post_destroy(Post *post) {
    if (post == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (post->count > post->capacity || (post->mails == NULL && post->capacity != 0) || (post->mails != NULL && post->capacity == 0) || (post->office_address == NULL && post->mails != NULL)) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = STATUS_OK;

    for (size_t i = 0; i < post->count; i++) {
        Status current = mail_destroy(&post->mails[i]);

        if (status == STATUS_OK && current != STATUS_OK) {
            status = current;
        }
    }

    free(post->mails);

    post->mails = NULL;
    post->count = 0;
    post->capacity = 0;

    if (post->office_address != NULL) {
        Status current = address_destroy(post->office_address);

        if (status == STATUS_OK && current != STATUS_OK) {
            status = current;
        }

        free(post->office_address);
        post->office_address = NULL;
    }

    return status;
}

Status mail_compare(const Mail *first, const Mail *second, int *comparison) {
    if (first == NULL || second == NULL || comparison == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    const String *first_index = &first->recipient.postal_code;
    const String *second_index = &second->recipient.postal_code;

    if (first_index->data == NULL || second_index->data == NULL ||
        first->id.data == NULL || second->id.data == NULL ||
        first_index->length != 6 || second_index->length != 6 ||
        first->id.length != 14 || second->id.length != 14) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (int i = 0; i < 6; i++) {
        if (first_index->data[i] < '0' || first_index->data[i] > '9' ||
            second_index->data[i] < '0' || second_index->data[i] > '9') {
            return STATUS_INVALID_ARGUMENT;
        }
    }

    for (int i = 0; i < 14; i++) {
        if (first->id.data[i] < '0' || first->id.data[i] > '9' || second->id.data[i] < '0' || second->id.data[i] > '9') {
            return STATUS_INVALID_ARGUMENT;
        }
    }

    if (first_index->data[6] != '\0' || second_index->data[6] != '\0' || first->id.data[14] != '\0' || second->id.data[14] != '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    int result = 0;

    Status status = string_compare(first_index, second_index, &result);

    if (status != STATUS_OK) {
        return status;
    }

    if (result != 0) {
        *comparison = result;
        return STATUS_OK;
    }

    return string_compare(&first->id, &second->id, comparison);
}

Status post_add_mail(Post *post, const Mail *mail) {
    if (post == NULL || mail == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (post->office_address == NULL || post->count > post->capacity || (post->mails == NULL && post->capacity != 0) || (post->mails != NULL && post->capacity == 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (mail->id.data == NULL || mail->id.length != 14 || mail->created_at.data == NULL || mail->created_at.length != 19 || mail->delivered_at.data == NULL || mail->delivered_at.length != 19) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = mail_validate(
        &mail->recipient,
        mail->weight,
        mail->id.data,
        mail->created_at.data,
        mail->delivered_at.data
    );

    if (status != STATUS_OK) {
        return status;
    }

    size_t position = post->count;

    for (size_t i = 0; i < post->count; i++) {
        int equal = 0;

        status = string_equal(&post->mails[i].id, &mail->id, &equal);

        if (status != STATUS_OK) {
            return status;
        }

        if (equal) {
            return STATUS_DUPLICATE_ID;
        }

        int comparison = 0;

        status = mail_compare(&post->mails[i], mail, &comparison);

        if (status != STATUS_OK) {
            return status;
        }

        if (position == post->count && comparison > 0) {
            position = i;
        }
    }

    Mail copy = {0};

    status = mail_copy(mail, &copy);

    if (status != STATUS_OK) {
        return status;
    }

    if (post->count == post->capacity) {
        if (post->capacity > SIZE_MAX / 2) {
            mail_destroy(&copy);
            return STATUS_LENGTH_OVERFLOW;
        }

        size_t new_capacity;

        if (post->capacity == 0) {
            new_capacity = 4;
        } else {
            new_capacity = post->capacity * 2;
        }

        if (new_capacity > SIZE_MAX / sizeof(Mail)) {
            mail_destroy(&copy);
            return STATUS_LENGTH_OVERFLOW;
        }

        Mail *new_mails = (Mail *)realloc(post->mails, new_capacity * sizeof(Mail));

        if (new_mails == NULL) {
            mail_destroy(&copy);
            return STATUS_MEMORY_ERROR;
        }

        post->mails = new_mails;
        post->capacity = new_capacity;
    }

    for (size_t i = post->count; i > position; i--) {
        post->mails[i] = post->mails[i - 1];
    }

    post->mails[position] = copy;
    post->count++;

    return STATUS_OK;
}

Status post_find_mail(const Post *post, const char *id, const Mail **result) {
    if (post == NULL || id == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = NULL;

    if (post->office_address == NULL || post->count > post->capacity || (post->mails == NULL && post->capacity != 0) || (post->mails != NULL && post->capacity == 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (int i = 0; i < 14; i++) {
        if (id[i] < '0' || id[i] > '9') {
            return STATUS_INVALID_ARGUMENT;
        }
    }

    if (id[14] != '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < post->count; i++) {
        const String *current_id = &post->mails[i].id;

        if (current_id->data == NULL || current_id->length != 14) {
            return STATUS_INVALID_ARGUMENT;
        }

        if (current_id->data[14] != '\0') {
            return STATUS_INVALID_ARGUMENT;
        }

        int equal = 1;

        for (int j = 0; j < 14; j++) {
            if (current_id->data[j] != id[j]) {
                equal = 0;
                break;
            }
        }

        if (equal) {
            *result = &post->mails[i];
            return STATUS_OK;
        }
    }

    return STATUS_NOT_FOUND;
}

Status post_remove_mail(Post *post, const char *id) {
    if (post == NULL || id == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    const Mail *found = NULL;

    Status status = post_find_mail(post, id, &found);

    if (status != STATUS_OK) {
        return status;
    }

    size_t position = (size_t)(found - post->mails);

    status = mail_destroy(&post->mails[position]);

    if (status != STATUS_OK) {
        return status;
    }

    for (size_t i = position; i + 1 < post->count; i++) {
        post->mails[i] = post->mails[i + 1];
    }

    post->count--;

    post->mails[post->count] = (Mail){0};

    return STATUS_OK;
}

Status mail_sort_by_created(const Mail **mails, size_t count) {
    if (mails == NULL && count != 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < count; i++) {
        if (mails[i] == NULL) {
            return STATUS_INVALID_ARGUMENT;
        }
    }

    for (size_t i = 1; i < count; i++) {
        size_t j = i;

        while (j > 0) {
            int comparison = 0;

            Status status = datetime_compare(
                mails[j - 1]->created_at.data,
                mails[j]->created_at.data,
                &comparison
            );

            if (status != STATUS_OK) {
                return status;
            }

            if (comparison <= 0) {
                break;
            }

            const Mail *temp = mails[j - 1];
            mails[j - 1] = mails[j];
            mails[j] = temp;

            j--;
        }
    }

    return STATUS_OK;
}


Status mail_print(const Mail *mail) {
    if (mail == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Status status = mail_validate(
        &mail->recipient,
        mail->weight,
        mail->id.data,
        mail->created_at.data,
        mail->delivered_at.data
    );

    if (status != STATUS_OK) {
        return status;
    }

    int result = printf(
        "Идентификатор: %s\n"
        "Город: %s\n"
        "Улица: %s\n"
        "Дом: %d\n"
        "Корпус: %s\n"
        "Квартира: %d\n"
        "Почтовый индекс: %s\n"
        "Вес: %g\n"
        "Время создания: %s\n"
        "Время вручения: %s\n\n",
        mail->id.data,
        mail->recipient.city.data,
        mail->recipient.street.data,
        mail->recipient.house,
        mail->recipient.building.data,
        mail->recipient.apartment,
        mail->recipient.postal_code.data,
        mail->weight,
        mail->created_at.data,
        mail->delivered_at.data
    );

    if (result < 0) {
        return STATUS_IO_ERROR;
    }

    return STATUS_OK;
}

Status read_line(char *buffer, size_t capacity) {
    if (buffer == NULL || capacity == 0) {
        return STATUS_INVALID_ARGUMENT;
    }

    size_t length = 0;
    int too_long = 0;
    int invalid = 0;
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) {
        if (ch == '\0') {
            invalid = 1;
        } else if (length < capacity - 1) {
            buffer[length] = (char)ch;
            length++;
        } else {
            too_long = 1;
        }
    }

    buffer[length] = '\0';

    if (ch == EOF && ferror(stdin)) {
        return STATUS_IO_ERROR;
    }

    if (ch == EOF && length == 0 && !too_long && !invalid) {
        return STATUS_IO_ERROR;
    }

    if (too_long || invalid) {
        return STATUS_INVALID_ARGUMENT;
    }

    return STATUS_OK;
}

Status parse_int(const char *text, int min_value, int max_value, int *result) {
    if (text == NULL || result == NULL || min_value > max_value) {
        return STATUS_INVALID_ARGUMENT;
    }

    char *end = NULL;

    long long value = strtoll(text, &end, 10);

    if (end == text) {
        return STATUS_INVALID_ARGUMENT;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (value < (long long)min_value ||
        value > (long long)max_value) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = (int)value;

    return STATUS_OK;
}


Status parse_double(const char *text, double *result) {
    if (text == NULL || result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    char *end = NULL;

    double value = strtod(text, &end);

    if (end == text) {
        return STATUS_INVALID_ARGUMENT;
    }

    while (isspace((unsigned char)*end)) {
        end++;
    }

    if (*end != '\0') {
        return STATUS_INVALID_ARGUMENT;
    }

    if (!isfinite(value) || (signbit(value) && fpclassify(value) != FP_ZERO)) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result = value;

    return STATUS_OK;
}



Status address_read(Address *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    char city[256];
    char street[256];
    char building[256];
    char postal_code[16];
    char number_buffer[64];

    int house = 0;
    int apartment = 0;

    Status status;

    if (puts("Введите город:") == EOF) {
        return STATUS_IO_ERROR;
    }

    status = read_line(city, sizeof(city));

    if (status != STATUS_OK) {
        return status;
    }

    if (puts("Введите улицу:") == EOF) {
        return STATUS_IO_ERROR;
    }

    status = read_line(street, sizeof(street));

    if (status != STATUS_OK) {
        return status;
    }

    if (puts("Введите номер дома:") == EOF) {
        return STATUS_IO_ERROR;
    }

    status = read_line(number_buffer, sizeof(number_buffer));

    if (status != STATUS_OK) {
        return status;
    }

    status = parse_int(number_buffer, 1, INT_MAX, &house);

    if (status != STATUS_OK) {
        return status;
    }

    if (puts("Введите корпус:") == EOF) {
        return STATUS_IO_ERROR;
    }

    status = read_line(building, sizeof(building));

    if (status != STATUS_OK) {
        return status;
    }

    if (puts("Введите номер квартиры:") == EOF) {
        return STATUS_IO_ERROR;
    }

    status = read_line(number_buffer, sizeof(number_buffer));

    if (status != STATUS_OK) {
        return status;
    }

    status = parse_int(number_buffer, 1, INT_MAX, &apartment);

    if (status != STATUS_OK) {
        return status;
    }

    if (puts("Введите почтовый индекс (6 цифр):") == EOF) {
        return STATUS_IO_ERROR;
    }

    status = read_line(postal_code, sizeof(postal_code));

    if (status != STATUS_OK) {
        return status;
    }

    return address_create(
        city,
        street,
        house,
        building,
        apartment,
        postal_code,
        result
    );
}



Status mail_read(Mail *result) {
    if (result == NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    Address recipient = {0};

    char weight_text[128];
    char id[32];
    char created_at[32];
    char delivered_at[32];

    double weight = 0.0;

    Status status = address_read(&recipient);

    if (status != STATUS_OK) {
        return status;
    }

    if (puts("Введите вес отправления:") == EOF) {
        status = STATUS_IO_ERROR;
    }

    if (status == STATUS_OK) {
        status = read_line(weight_text, sizeof(weight_text));
    }

    if (status == STATUS_OK) {
        status = parse_double(weight_text, &weight);
    }

    if (status == STATUS_OK) {
        if (puts("Введите идентификатор (14 цифр):") == EOF) {
            status = STATUS_IO_ERROR;
        } else {
            status = read_line(id, sizeof(id));
        }
    }

    if (status == STATUS_OK) {
        if (puts("Введите время создания (dd:MM:yyyy hh:mm:ss):") == EOF) {
            status = STATUS_IO_ERROR;
        } else {
            status = read_line(created_at, sizeof(created_at));
        }
    }

    if (status == STATUS_OK) {
        if (puts("Введите время вручения (dd:MM:yyyy hh:mm:ss):") == EOF) {
            status = STATUS_IO_ERROR;
        } else {
            status = read_line(delivered_at, sizeof(delivered_at));
        }
    }

    if (status == STATUS_OK) {
        status = mail_create(
            &recipient,
            weight,
            id,
            created_at,
            delivered_at,
            result
        );
    }

    Status destroy_status = address_destroy(&recipient);

    if (status == STATUS_OK && destroy_status != STATUS_OK) {
        status = destroy_status;
    }

    return status;
}

Status post_filter_by_delivery_date(const Post *post, int past, const Mail ***result, size_t *result_count) {
    if (post == NULL || result == NULL || result_count == NULL ||
        (past != 0 && past != 1)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (*result != NULL) {
        return STATUS_INVALID_ARGUMENT;
    }

    *result_count = 0;

    if (post->office_address == NULL || post->count > post->capacity ||
        (post->mails == NULL && post->capacity != 0) ||
        (post->mails != NULL && post->capacity == 0)) {
        return STATUS_INVALID_ARGUMENT;
    }

    if (post->count == 0) {
        return STATUS_OK;
    }

    DateTime now = {0};
    Status status = datetime_now(&now);

    if (status != STATUS_OK) {
        return status;
    }

    char current_date[20];

    int length = snprintf(
        current_date, sizeof(current_date),
        "%02d:%02d:%04d %02d:%02d:%02d",
        now.day, now.month, now.year,
        now.hour, now.minute, now.second
    );

    if (length != 19) {
        return STATUS_TIME_ERROR;
    }

    if (post->count > SIZE_MAX / sizeof(const Mail *)) {
        return STATUS_LENGTH_OVERFLOW;
    }

    const Mail **selected = (const Mail **)malloc(post->count * sizeof(const Mail *)
    );

    if (selected == NULL) {
        return STATUS_MEMORY_ERROR;
    }

    size_t count = 0;

    for (size_t i = 0; i < post->count; i++) {
        int comparison = 0;

        status = datetime_compare(
            post->mails[i].delivered_at.data,
            current_date,
            &comparison
        );

        if (status != STATUS_OK) {
            free(selected);
            return status;
        }

        if ((past == 1 && comparison <= 0) ||
            (past == 0 && comparison > 0)) {
            selected[count] = &post->mails[i];
            count++;
        }
    }

    if (count == 0) {
        free(selected);
        return STATUS_OK;
    }

    status = mail_sort_by_created(selected, count);

    if (status != STATUS_OK) {
        free(selected);
        return status;
    }

    *result = selected;
    *result_count = count;

    return STATUS_OK;
}


int main(void) {
    Address office = {0};
    Post post = {0};
    int exit_code = 0;

    puts("Введите адрес почтового отделения:");

    Status status = address_read(&office);

    if (status != STATUS_OK) {
        address_destroy(&office);
        fputs("Не удалось ввести адрес отделения.\n", stderr);
        return 1;
    }

    status = post_create(&office, &post);
    address_destroy(&office);

    if (status != STATUS_OK) {
        fputs("Не удалось создать почтовое отделение.\n", stderr);
        return 1;
    }

    while (1) {
        if (printf(
            "\n1. Добавить отправление\n"
            "2. Удалить отправление\n"
            "3. Найти отправление\n"
            "4. Дата вручения наступила\n"
            "5. Дата вручения ещё не наступила\n"
            "0. Выход\n"
            "Выберите действие:\n"
        ) < 0) {
            exit_code = 1;
            break;
        }

        char buffer[256];
        status = read_line(buffer, sizeof(buffer));

        if (status == STATUS_IO_ERROR) {
            if (ferror(stdin)) {
                exit_code = 1;
            }
            break;
        }

        int command = -1;

        if (status != STATUS_OK ||
            parse_int(buffer, 0, 5, &command) != STATUS_OK) {
            puts("Неверная команда.");
            continue;
        }

        if (command == 0) {
            break;
        }

        if (command == 1) {
            Mail mail = {0};

            status = mail_read(&mail);

            if (status == STATUS_OK) {
                status = post_add_mail(&post, &mail);
            }

            Status destroy_status = mail_destroy(&mail);

            if (status == STATUS_OK && destroy_status != STATUS_OK) {
                status = destroy_status;
            }

            if (status == STATUS_OK) {
                puts("Отправление добавлено.");
            }
        } else if (command == 2 || command == 3) {
            char id[32];

            if (puts("Введите идентификатор (14 цифр):") == EOF) {
                status = STATUS_IO_ERROR;
            } else {
                status = read_line(id, sizeof(id));
            }

            if (status == STATUS_OK && command == 2) {
                status = post_remove_mail(&post, id);

                if (status == STATUS_OK) {
                    puts("Отправление удалено.");
                }
            } else if (status == STATUS_OK) {
                const Mail *found = NULL;

                status = post_find_mail(&post, id, &found);

                if (status == STATUS_OK) {
                    status = mail_print(found);
                }
            }
        } else if (command == 4 || command == 5) {
            int past = 0;

            if (command == 4) {
                past = 1;
            }

            const Mail **selected = NULL;
            size_t selected_count = 0;

            status = post_filter_by_delivery_date(
                &post, past, &selected, &selected_count
            );

            if (status == STATUS_OK && selected_count == 0) {
                puts("Подходящих отправлений нет.");
            }

            if (status == STATUS_OK) {
                for (size_t i = 0; i < selected_count; i++) {
                    status = mail_print(selected[i]);

                    if (status != STATUS_OK) {
                        break;
                    }
                }
            }

            free(selected);
        }

        if (status == STATUS_IO_ERROR) {
            exit_code = 1;
            break;
        }

        if (status == STATUS_NOT_FOUND) {
            puts("Отправление не найдено.");
        } else if (status == STATUS_DUPLICATE_ID) {
            puts("Отправление с таким идентификатором уже существует.");
        } else if (status != STATUS_OK) {
            puts("Ошибка выполнения команды.");
        }
    }

    status = post_destroy(&post);

    if (status != STATUS_OK) {
        exit_code = 1;
    }

    return exit_code;
}
