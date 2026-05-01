#include "editor_core.h"
#include <stdlib.h>
#include <string.h>

#define MAX_UNDO_STACK 100

typedef enum { ACTION_INSERT, ACTION_DELETE, ACTION_STYLE } ActionType;