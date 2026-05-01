#include "editor_core.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define GAP_BUFFER_INITIAL_SIZE 1024
#define GAP_BUFFER_GROWTH_FACTOR 2

struct GapBuffer {
    char *buffer;
    size_t gap_start; // position du début du gap
    size_t gap_end;   // position de la fin du gap
    size_t total_size; // taille totale du buffer (y compris le gap)
    on_change_callback on_change; // callback pour notifier les changements

};

static void ensure_capacity(GapBuffer *gb, size_t neede) {
    size_t use = gb->total_size - (gb->gap_end - gb->gap_start);
    if (use + neede <= gb->total_size) return; // assez de place
    size_t new_size = gb->total_size * GAP_BUFFER_GROWTH_FACTOR;
    while (new_size < use + neede) new_size *= 2;
    char *new_buf = malloc(new_size);
    if (!new_buf) return; // échec de l'allocation

    // Copier le texte avant le gap
    memcpy(new_buf, gb->buffer, gb->gap_start);

    // Copier le texte après le gap
    size_t after_gap_len = gb->total_size - gb->gap_end;
     memcpy(new_buf + new_size - after_gap_len,
           gb->buffer + gb->gap_end,
           after_gap_len);
    free(gb->buffer);
    gb->buffer = new_buf;
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
    gb->gap_start = 0;
    gb->gap_end = gb->total_size;
    gb->on_change = NULL;
    return gb;
}

void editor_insert_text(GapBuffer *gb, const char *text) {
    if (!gb || !text) return;
    size_t len = strlen(text);
    if (len == 0) return;
    ensure_capacity(gb, len);

    // Copier dans le gap

    memcpy(gb->buffer + gb->gap_start, text, len);
    gb->gap_start += len;
    if (gb->on_change) gb->on_change();
}

void editor_delete_chars(GapBuffer *gb, size_t count) {
    if (!gb || count == 0) return;
    // Si count dépasse la zone avant le gap, on limite
    if (count > gb->gap_start) count = gb->gap_start;
    gb->gap_start -= count;
    if (gb->on_change) gb->on_change();
}

const char* editor_get_text(GapBuffer *gb) {
    if (!gb) return NULL;
    size_t text_len = gb->total_size - (gb->gap_end - gb->gap_start);
    char *result = malloc(text_len + 1);
    if (!result) return NULL;

    // Avant gap
    memcpy(result, gb->buffer, gb->gap_start);

    // Après gap
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
    size_t len = editor_get_text_length(gb);
    if (pos > len) pos = len;
    // Déplacer le gap à la nouvelle position
    if (pos < gb->gap_start) {
        // Décaler vers la gauche
        size_t move = gb->gap_start - pos;
        memmove(gb->buffer + pos + (gb->gap_end - gb->gap_start),
                gb->buffer + pos,
                move);
        gb->gap_start = pos;
        gb->gap_end = pos + (gb->gap_end - gb->gap_start);
    } else if (pos > gb->gap_start) {
        // Décaler vers la droite
        size_t move = pos - gb->gap_start;
        memmove(gb->buffer + gb->gap_start,
                gb->buffer + gb->gap_end,
                move);
        gb->gap_start += move;
        gb->gap_end += move;
    }
}

void editor_set_on_change_callback(GapBuffer *gb, on_change_callback callback){
    if (gb) gb -> on_change = callback;
}