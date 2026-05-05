#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Config {
    char **keys;
    char **values;
    size_t count;
};

static char *trim(char *text) {
    if (!text) return NULL;
    char *start = text;
    while (*start && isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return start;
}

static char *read_line(FILE *file) {
    size_t capacity = 128;
    size_t length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) return NULL;

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char *next = realloc(buffer, capacity);
            if (!next) {
                free(buffer);
                return NULL;
            }
            buffer = next;
        }
        if (ch == '\r') continue;
        buffer[length++] = (char)ch;
        if (ch == '\n') break;
    }

    if (length == 0 && ch == EOF) {
        free(buffer);
        return NULL;
    }

    buffer[length] = '\0';
    return buffer;
}

Config *config_create(void) {
    Config *cfg = malloc(sizeof(Config));
    if (!cfg) return NULL;
    cfg->keys = NULL;
    cfg->values = NULL;
    cfg->count = 0;
    return cfg;
}

void config_free(Config *cfg) {
    if (!cfg) return;
    for (size_t i = 0; i < cfg->count; ++i) {
        free(cfg->keys[i]);
        free(cfg->values[i]);
    }
    free(cfg->keys);
    free(cfg->values);
    free(cfg);
}

bool config_get(const Config *cfg, const char *key, const char **value) {
    if (!cfg || !key || !value) return false;
    for (size_t i = 0; i < cfg->count; ++i) {
        if (strcmp(cfg->keys[i], key) == 0) {
            *value = cfg->values[i];
            return true;
        }
    }
    return false;
}

bool config_set(Config *cfg, const char *key, const char *value) {
    if (!cfg || !key || !value) return false;
    for (size_t i = 0; i < cfg->count; ++i) {
        if (strcmp(cfg->keys[i], key) == 0) {
            char *new_value = strdup(value);
            if (!new_value) return false;
            free(cfg->values[i]);
            cfg->values[i] = new_value;
            return true;
        }
    }

    char **new_keys = realloc(cfg->keys, (cfg->count + 1) * sizeof(char *));
    char **new_values = realloc(cfg->values, (cfg->count + 1) * sizeof(char *));
    if (!new_keys || !new_values) {
        free(new_keys);
        free(new_values);
        return false;
    }

    cfg->keys = new_keys;
    cfg->values = new_values;
    cfg->keys[cfg->count] = strdup(key);
    cfg->values[cfg->count] = strdup(value);
    if (!cfg->keys[cfg->count] || !cfg->values[cfg->count]) {
        free(cfg->keys[cfg->count]);
        free(cfg->values[cfg->count]);
        return false;
    }
    cfg->count += 1;
    return true;
}

Config *config_load(const char *filename) {
    if (!filename) return NULL;
    FILE *file = fopen(filename, "r");
    if (!file) return NULL;

    Config *cfg = config_create();
    if (!cfg) {
        fclose(file);
        return NULL;
    }

    char *line;
    while ((line = read_line(file)) != NULL) {
        char *text = trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') {
            free(line);
            continue;
        }

        char *equals = strchr(text, '=');
        if (!equals) {
            free(line);
            continue;
        }

        *equals = '\0';
        char *key = trim(text);
        char *value = trim(equals + 1);
        if (*key && *value) {
            config_set(cfg, key, value);
        }
        free(line);
    }

    fclose(file);
    return cfg;
}

bool config_save(const Config *cfg, const char *filename) {
    if (!cfg || !filename) return false;
    FILE *file = fopen(filename, "w");
    if (!file) return false;

    for (size_t i = 0; i < cfg->count; ++i) {
        fprintf(file, "%s=%s\n", cfg->keys[i], cfg->values[i]);
    }

    fclose(file);
    return true;
}
