#include "utils.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

bool utf8_validate(const char *text) {
    if (!text) return false;
    const unsigned char *ptr = (const unsigned char *)text;
    while (*ptr) {
        unsigned char c = *ptr;
        if (c < 0x80) {
            ptr += 1;
            continue;
        }

        size_t expected = 0;
        uint32_t code_point = 0;

        if ((c & 0xE0) == 0xC0) {
            expected = 1;
            code_point = c & 0x1F;
        } else if ((c & 0xF0) == 0xE0) {
            expected = 2;
            code_point = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            expected = 3;
            code_point = c & 0x07;
        } else {
            return false;
        }

        ptr++;
        for (size_t i = 0; i < expected; ++i) {
            if ((ptr[i] & 0xC0) != 0x80) return false;
            code_point = (code_point << 6) | (ptr[i] & 0x3F);
        }

        if (code_point < 0x80) return false;
        if (code_point >= 0xD800 && code_point <= 0xDFFF) return false;
        if (code_point > 0x10FFFF) return false;

        ptr += expected;
    }
    return true;
}

size_t utf8_char_count(const char *text) {
    if (!text) return 0;
    const unsigned char *ptr = (const unsigned char *)text;
    size_t count = 0;
    while (*ptr) {
        if ((*ptr & 0xC0) != 0x80) {
            count++;
        }
        ptr++;
    }
    return count;
}

char *utf8_duplicate(const char *text) {
    if (!text) return NULL;
    size_t len = strlen(text);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, text, len + 1);
    return copy;
}
