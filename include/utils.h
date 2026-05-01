#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>

typedef struct Config Config;

Config *config_create(void);
Config *config_load(const char *filename);
bool config_save(const Config *cfg, const char *filename);
bool config_get(const Config *cfg, const char *key, const char **value);
bool config_set(Config *cfg, const char *key, const char *value);
void config_free(Config *cfg);

bool utf8_validate(const char *text);
size_t utf8_char_count(const char *text);
char *utf8_duplicate(const char *text);

#endif // UTILS_H
