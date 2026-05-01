#ifndef PROMPTS_H
#define PROMPTS_H

#include <stdlib.h>

/* Module de gestion des prompts pour l'IA (DEV-A)
 * Fournit des templates de prompts pour différentes tâches
 * d'édition intelligente.
 */

/* Types de prompts disponibles */
typedef enum {
    PROMPT_CORRECTION_GRAMMAIRE,
    PROMPT_SUGGESTION_AUTOCOMPLETION,
    PROMPT_ANALYSE_STYLE,
    PROMPT_REFORMULATION,
    PROMPT_TRADUCTION
} PromptType;

/* Génère un prompt complet à partir d'un type et de paramètres.
 * Retourne une chaîne allouée dynamiquement que l'appelant doit libérer.
 */
char* generate_prompt(PromptType type, const char* text, const char* context);

#endif /* PROMPTS_H */