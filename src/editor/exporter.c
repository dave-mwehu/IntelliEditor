#include "editor_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static bool write_bytes(const char *filename, const void *data, size_t size) {
    if (!filename || !data) return false;
    FILE *file = fopen(filename, "wb");
    if (!file) return false;
    bool ok = fwrite(data, 1, size, file) == size;
    fclose(file);
    return ok;
}

bool editor_export_txt(GapBuffer *gb, const char *filename) {
    if (!gb || !filename) return false;
    const char *text = editor_get_text(gb);
    if (!text) return false;

    bool result = write_bytes(filename, text, strlen(text));
    free((void *)text);
    return result;
}

bool editor_export_rtf(GapBuffer *gb, const char *filename) {
    if (!gb || !filename) return false;
    const char *text = editor_get_text(gb);
    if (!text) return false;

    FILE *file = fopen(filename, "wb");
    if (!file) {
        free((void *)text);
        return false;
    }

    fputs("{\\rtf1\\ansi\\deff0\n", file);
    for (const char *p = text; *p; ++p) {
        switch (*p) {
            case '\\': fputs("\\\\", file); break;
            case '{': fputs("\\{", file); break;
            case '}': fputs("\\}", file); break;
            case '\n': fputs("\\par\n", file); break;
            default: fputc(*p, file); break;
        }
    }
    fputs("}\n", file);
    fclose(file);
    free((void *)text);
    return true;
}

bool editor_export_ie(GapBuffer *gb, const char *filename) {
    if (!gb || !filename) return false;
    const char *text = editor_get_text(gb);
    if (!text) return false;

    size_t length = strlen(text);
    FILE *file = fopen(filename, "wb");
    if (!file) {
        free((void *)text);
        return false;
    }

    const uint32_t magic = 0x46454249; // 'IEBF'
    const uint64_t original_size = (uint64_t)length;
    if (fwrite(&magic, sizeof(magic), 1, file) != 1 ||
        fwrite(&original_size, sizeof(original_size), 1, file) != 1) {
        fclose(file);
        free((void *)text);
        return false;
    }

    size_t i = 0;
    while (i < length) {
        unsigned char value = (unsigned char)text[i];
        unsigned char run = 1;
        while (i + run < length && text[i + run] == text[i] && run < 255) {
            run++;
        }
        if (fwrite(&run, 1, 1, file) != 1 || fwrite(&value, 1, 1, file) != 1) {
            fclose(file);
            free((void *)text);
            return false;
        }
        i += run;
    }

    fclose(file);
    free((void *)text);
    return true;
}
