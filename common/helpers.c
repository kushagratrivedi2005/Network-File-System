#include "common_headers.h"

char** split(const char* str, const char* delimiter, int* count) {
    char* str_copy = strdup(str);
    int capacity = 10;
    char** result = malloc(capacity * sizeof(char*));
    *count = 0;

    char* token = strtok(str_copy, delimiter);
    while (token != NULL) {
        if (*count >= capacity) {
            capacity *= 2;
            result = realloc(result, capacity * sizeof(char*));
        }
        result[*count] = strdup(token);
        (*count)++;
        token = strtok(NULL, delimiter);
    }

    free(str_copy);
    return result;
}