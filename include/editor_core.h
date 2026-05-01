#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

#include <stddef.h>
#include <stdbool.h>

// Types de style (bitmask)
typedef enum {
    STYLE_NORMAL     = 0,
    STYLE_BOLD       = 1 << 0,
    STYLE_ITALIC     = 1 << 1,
    STYLE_UNDERLINE  = 1 << 2,
    STYLE_H1         = 1 << 3,
    STYLE_H2         = 1 << 4,
    STYLE_H3         = 1 << 5,
    STYLE_H4         = 1 << 6,
    STYLE_LIST_BULLET= 1 << 7,
    STYLE_LIST_NUM   = 1 << 8
} TextStyle;

// Structure principale de l'éditeur (gap buffer)
typedef struct GapBuffer GapBuffer;

// Initialisation et libération du gap buffer
GapBuffer* editor_create(void);
void editor_destroy(GapBuffer* gb);

// Opérations de base
void editor_insert(GapBuffer* gb, const char *utf8_text);
void editor_delete_chars(GapBuffer* gb, size_t count);
void editor_delete_selection(GapBuffer* gb, size_t start, size_t end);

// gestion du curseur
size_t editor_get_cursor_pos(GapBuffer* gb);
void editor_set_cursor_pos(GapBuffer* gb, size_t pos);
void editor_move_cursor(GapBuffer* gb, long delta);

// Accès au texte
const char* editor_get_text(GapBuffer* gb);
size_t editor_get_text_length(GapBuffer* gb);

// Style du texte (tableau parallele au texte)
void editor_set_style(GapBuffer *gb, size_t pos, size_t len, TextStyle style, bool enable);
TextStyle editor_get_style_at(GapBuffer *gb, size_t pos);

// Undo / Redo
void editor_undo(GapBuffer* gb);
void editor_redo(GapBuffer* gb);
void editor_begin_action_group(GapBuffer* gb);
void editor_end_action_group(GapBuffer* gb);

// Export
bool editor_export_txt(GapBuffer *gb, const char *filename);
bool edidtor_export_rtf(GapBuffer *gb, const char *filename);
bool editor_export_ie(GapBuffer *gb, const char *filename); // format binaire compressé

// Callbacks pour l'interface utilisateur
typedef void (*on_change_callback)(void);
void editor_set_on_change(GapBuffer* gb, on_change_callback callback);

#endif // EDITOR_CORE_H