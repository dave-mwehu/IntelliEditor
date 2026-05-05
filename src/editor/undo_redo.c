#include "editor_core.h"
#include <stdlib.h>
#include <string.h>

#define MAX_UNDO_STACK 100

typedef enum { ACTION_INSERT, ACTION_DELETE, ACTION_STYLE } ActionType;

typedef struct {
    ActionType type;
    size_t pos;
    char *text;
    size_t length;
} Action;

typedef struct {
    GapBuffer *gb;
    Action undo_stack[MAX_UNDO_STACK];
    int undo_top;
    Action redo_stack[MAX_UNDO_STACK];
    int redo_top;
} BufferUndo;

static BufferUndo buffer_states[MAX_UNDO_STACK];
static int buffer_state_count = 0;
static int record_enabled = 1;

int editor_undo_is_record_enabled(void) {
    return record_enabled;
}

static void set_record_enabled(int enabled) {
    record_enabled = enabled;
}

static void clear_action(Action *action) {
    if (!action) return;
    free(action->text);
    action->text = NULL;
    action->length = 0;
}

static void clear_stack(Action *stack, int *top) {
    if (!stack || !top) return;
    for (int i = 0; i <= *top; ++i) {
        clear_action(&stack[i]);
    }
    *top = -1;
}

static BufferUndo *find_state(GapBuffer *gb) {
    for (int i = 0; i < buffer_state_count; ++i) {
        if (buffer_states[i].gb == gb) return &buffer_states[i];
    }
    return NULL;
}

static BufferUndo *get_state(GapBuffer *gb) {
    BufferUndo *state = find_state(gb);
    if (state) return state;

    if (buffer_state_count < MAX_UNDO_STACK) {
        state = &buffer_states[buffer_state_count++];
    } else {
        clear_stack(buffer_states[0].undo_stack, &buffer_states[0].undo_top);
        clear_stack(buffer_states[0].redo_stack, &buffer_states[0].redo_top);
        memmove(&buffer_states[0], &buffer_states[1], sizeof(BufferUndo) * (MAX_UNDO_STACK - 1));
        state = &buffer_states[MAX_UNDO_STACK - 1];
        memset(state, 0, sizeof(BufferUndo));
    }

    state->gb = gb;
    state->undo_top = -1;
    state->redo_top = -1;
    return state;
}

static void push_redo(BufferUndo *state, const Action *action) {
    if (!state || !action) return;
    if (state->redo_top + 1 >= MAX_UNDO_STACK) {
        clear_action(&state->redo_stack[0]);
        memmove(&state->redo_stack[0], &state->redo_stack[1], sizeof(Action) * (MAX_UNDO_STACK - 1));
        state->redo_top = MAX_UNDO_STACK - 2;
    }
    state->redo_stack[++state->redo_top] = *action;
}

static void push_undo(BufferUndo *state, const Action *action) {
    if (!state || !action) return;
    if (state->undo_top + 1 >= MAX_UNDO_STACK) {
        clear_action(&state->undo_stack[0]);
        memmove(&state->undo_stack[0], &state->undo_stack[1], sizeof(Action) * (MAX_UNDO_STACK - 1));
        state->undo_top = MAX_UNDO_STACK - 2;
    }
    state->undo_stack[++state->undo_top] = *action;
    clear_stack(state->redo_stack, &state->redo_top);
}

static Action copy_action(const Action *source) {
    Action copy = *source;
    if (source->text) {
        copy.text = malloc(source->length + 1);
        if (copy.text) {
            memcpy(copy.text, source->text, source->length + 1);
        } else {
            copy.length = 0;
        }
    }
    return copy;
}

void editor_record_insert(GapBuffer *gb, size_t pos, const char *text) {
    if (!gb || !text) return;
    BufferUndo *state = get_state(gb);
    if (!state) return;

    Action action = {
        .type = ACTION_INSERT,
        .pos = pos,
        .length = strlen(text),
        .text = malloc(strlen(text) + 1)
    };
    if (!action.text) return;
    memcpy(action.text, text, action.length + 1);
    push_undo(state, &action);
}

void editor_record_delete(GapBuffer *gb, size_t pos, const char *text) {
    if (!gb || !text) return;
    BufferUndo *state = get_state(gb);
    if (!state) return;

    Action action = {
        .type = ACTION_DELETE,
        .pos = pos,
        .length = strlen(text),
        .text = malloc(strlen(text) + 1)
    };
    if (!action.text) return;
    memcpy(action.text, text, action.length + 1);
    push_undo(state, &action);
}

void editor_undo(GapBuffer *gb) {
    if (!gb) return;
    BufferUndo *state = find_state(gb);
    if (!state || state->undo_top < 0) return;

    Action action = state->undo_stack[state->undo_top--];
    Action inverse = {0};
    set_record_enabled(0);

    if (action.type == ACTION_INSERT) {
        inverse.type = ACTION_DELETE;
        inverse.pos = action.pos;
        inverse.length = action.length;
        inverse.text = malloc(action.length + 1);
        if (inverse.text) memcpy(inverse.text, action.text, action.length + 1);

        editor_set_cursor_pos(gb, action.pos + action.length);
        editor_delete_chars(gb, action.length);
    } else if (action.type == ACTION_DELETE) {
        inverse.type = ACTION_INSERT;
        inverse.pos = action.pos;
        inverse.length = action.length;
        inverse.text = malloc(action.length + 1);
        if (inverse.text) memcpy(inverse.text, action.text, action.length + 1);

        editor_set_cursor_pos(gb, action.pos);
        editor_insert(gb, action.text);
    }

    set_record_enabled(1);
    push_redo(state, &inverse);
    clear_action(&action);
}

void editor_redo(GapBuffer *gb) {
    if (!gb) return;
    BufferUndo *state = find_state(gb);
    if (!state || state->redo_top < 0) return;

    Action action = state->redo_stack[state->redo_top--];
    Action inverse = {0};
    set_record_enabled(0);

    if (action.type == ACTION_INSERT) {
        inverse.type = ACTION_DELETE;
        inverse.pos = action.pos;
        inverse.length = action.length;
        inverse.text = malloc(action.length + 1);
        if (inverse.text) memcpy(inverse.text, action.text, action.length + 1);

        editor_set_cursor_pos(gb, action.pos);
        editor_insert(gb, action.text);
    } else if (action.type == ACTION_DELETE) {
        inverse.type = ACTION_INSERT;
        inverse.pos = action.pos;
        inverse.length = action.length;
        inverse.text = malloc(action.length + 1);
        if (inverse.text) memcpy(inverse.text, action.text, action.length + 1);

        editor_set_cursor_pos(gb, action.pos + action.length);
        editor_delete_chars(gb, action.length);
    }

    set_record_enabled(1);
    push_undo(state, &inverse);
    clear_action(&action);
}

void editor_begin_action_group(GapBuffer *gb) {
    (void)gb;
}

void editor_end_action_group(GapBuffer *gb) {
    (void)gb;
}
