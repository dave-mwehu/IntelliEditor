#include "editor_core.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define GAP_BUFFER_INITIAL_SIZE 1024
#define GAP_BUFFER_GROWTH_FACTOR 2

struct GapBuffer {
    char *buffer;
    TextStyle *style_buffer;
    size_t gap_start; // position du début du gap
    size_t gap_end;   // position de la fin du gap
    size_t total_size; // taille totale du buffer (y compris le gap)
    on_change_callback on_change; // callback pour notifier les changements
};

extern void editor_record_insert(GapBuffer *gb, size_t pos, const char *text);
extern void editor_record_delete(GapBuffer *gb, size_t pos, const char *text);
extern int editor_undo_is_record_enabled(void);

static size_t storage_index(const GapBuffer *gb, size_t pos) {
    if (pos <= gb->gap_start) return pos;
    return pos + (gb->gap_end - gb->gap_start);
}

static void ensure_capacity(GapBuffer *gb, size_t neede) {
    size_t use = gb->total_size - (gb->gap_end - gb->gap_start);
    if (use + neede <= gb->total_size) return; // assez de place

    size_t new_size = gb->total_size * GAP_BUFFER_GROWTH_FACTOR;
    while (new_size < use + neede) new_size *= 2;

    char *new_buf = malloc(new_size);
    TextStyle *new_style = malloc(new_size * sizeof(TextStyle));
    if (!new_buf || !new_style) {
        free(new_buf);
        free(new_style);
        return;
    }

    size_t after_gap_len = gb->total_size - gb->gap_end;

    memcpy(new_buf, gb->buffer, gb->gap_start);
    memcpy(new_style, gb->style_buffer, gb->gap_start * sizeof(TextStyle));

    memcpy(new_buf + new_size - after_gap_len,
           gb->buffer + gb->gap_end,
           after_gap_len);
    memcpy(new_style + new_size - after_gap_len,
           gb->style_buffer + gb->gap_end,
           after_gap_len * sizeof(TextStyle));

    free(gb->buffer);
    free(gb->style_buffer);

    gb->buffer = new_buf;
    gb->style_buffer = new_style;
    gb->gap_end = new_size - after_gap_len;
    gb->total_size = new_size;
}

GapBuffer* editor_create(void) {
    GapBuffer *gb = malloc(sizeof(GapBuffer));
    if (!gb) return NULL;

    gb->total_size = GAP_BUFFER_INITIAL_SIZE;
    gb->buffer = malloc(gb->total_size);
    if (!gb->buffer) {
        free(gb);
        return NULL;
    }

    gb->style_buffer = malloc(gb->total_size * sizeof(TextStyle));
    if (!gb->style_buffer) {
        free(gb->buffer);
        free(gb);
        return NULL;
    }

    gb->gap_start = 0;
    gb->gap_end = gb->total_size;
    gb->on_change = NULL;
    for (size_t i = 0; i < gb->total_size; ++i) {
        gb->style_buffer[i] = STYLE_NORMAL;
    }

    return gb;
}

void editor_destroy(GapBuffer *gb) {
    if (!gb) return;
    free(gb->buffer);
    free(gb->style_buffer);
    free(gb);
}

void editor_insert(GapBuffer *gb, const char *text) {
    if (!gb || !text) return;
    size_t len = strlen(text);
    if (len == 0) return;

    ensure_capacity(gb, len);
    size_t insert_pos = gb->gap_start;

    memcpy(gb->buffer + gb->gap_start, text, len);
    for (size_t i = 0; i < len; ++i) {
        gb->style_buffer[gb->gap_start + i] = STYLE_NORMAL;
    }

    gb->gap_start += len;

    if (editor_undo_is_record_enabled()) {
        editor_record_insert(gb, insert_pos, text);
    }

    if (gb->on_change) gb->on_change();
}

void editor_delete_chars(GapBuffer *gb, size_t count) {
    if (!gb || count == 0) return;

    if (count > gb->gap_start) count = gb->gap_start;
    size_t delete_pos = gb->gap_start - count;

    char *deleted = malloc(count + 1);
    if (!deleted) return;
    memcpy(deleted, gb->buffer + delete_pos, count);
    deleted[count] = '\0';

    gb->gap_start -= count;

    if (editor_undo_is_record_enabled()) {
        editor_record_delete(gb, delete_pos, deleted);
    }

    free(deleted);
    if (gb->on_change) gb->on_change();
}

void editor_delete_selection(GapBuffer *gb, size_t start, size_t end) {
    if (!gb || start >= end) return;

    size_t len = editor_get_text_length(gb);
    if (start > len) start = len;
    if (end > len) end = len;
    if (start >= end) return;

    editor_set_cursor_pos(gb, end);
    editor_delete_chars(gb, end - start);
}

const char* editor_get_text(GapBuffer *gb) {
    if (!gb) return NULL;
    size_t text_len = gb->total_size - (gb->gap_end - gb->gap_start);
    char *result = malloc(text_len + 1);
    if (!result) return NULL;

    memcpy(result, gb->buffer, gb->gap_start);
    memcpy(result + gb->gap_start,
           gb->buffer + gb->gap_end,
           gb->total_size - gb->gap_end);
    result[text_len] = '\0';
    return result;
}

size_t editor_get_text_length(GapBuffer *gb) {
    if (!gb) return 0;
    return gb->total_size - (gb->gap_end - gb->gap_start);
}

size_t editor_get_cursor_pos(GapBuffer *gb) {
    return gb ? gb->gap_start : 0;
}

void editor_set_cursor_pos(GapBuffer *gb, size_t pos) {
    if (!gb) return;

    size_t text_len = editor_get_text_length(gb);
    if (pos > text_len) pos = text_len;

    if (pos < gb->gap_start) {
        size_t move = gb->gap_start - pos;
        memmove(gb->buffer + gb->gap_end - move,
                gb->buffer + pos,
                move);
        memmove(gb->style_buffer + gb->gap_end - move,
                gb->style_buffer + pos,
                move * sizeof(TextStyle));
        gb->gap_start = pos;
        gb->gap_end -= move;
    } else if (pos > gb->gap_start) {
        size_t move = pos - gb->gap_start;
        memmove(gb->buffer + gb->gap_start,
                gb->buffer + gb->gap_end,
                move);
        memmove(gb->style_buffer + gb->gap_start,
                gb->style_buffer + gb->gap_end,
                move * sizeof(TextStyle));
        gb->gap_start += move;
        gb->gap_end += move;
    }
}

void editor_move_cursor(GapBuffer *gb, long delta) {
    if (!gb) return;

    size_t pos = editor_get_cursor_pos(gb);
    if (delta < 0) {
        size_t abs_delta = (size_t)(-delta);
        pos = abs_delta > pos ? 0 : pos - abs_delta;
    } else {
        pos += (size_t)delta;
    }
    editor_set_cursor_pos(gb, pos);
}

void editor_set_style(GapBuffer *gb, size_t pos, size_t len, TextStyle style, bool enable) {
    if (!gb || len == 0) return;

    size_t text_len = editor_get_text_length(gb);
    if (pos >= text_len) return;
    if (pos + len > text_len) len = text_len - pos;

    for (size_t i = 0; i < len; ++i) {
        size_t index = storage_index(gb, pos + i);
        if (enable) {
            gb->style_buffer[index] |= style;
        } else {
            gb->style_buffer[index] &= ~style;
        }
    }
}

TextStyle editor_get_style_at(GapBuffer *gb, size_t pos) {
    if (!gb) return STYLE_NORMAL;

    size_t text_len = editor_get_text_length(gb);
    if (pos >= text_len) return STYLE_NORMAL;
    return gb->style_buffer[storage_index(gb, pos)];
}

void editor_set_on_change(GapBuffer *gb, on_change_callback callback) {
    if (gb) gb->on_change = callback;
}
