#include "prompts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Templates de base pour les prompts */
static const char* PROMPT_TEMPLATES[] = {
    [PROMPT_CORRECTION_GRAMMAIRE] = "Corrige la grammaire et l'orthographe du texte suivant en français :\n\n%s\n\nTexte corrigé :",
    [PROMPT_SUGGESTION_AUTOCOMPLETION] = "Complète le texte suivant de manière cohérente :\n\n%s\n\nSuggestion :",
    [PROMPT_ANALYSE_STYLE] = "Analyse le style d'écriture du texte suivant :\n\n%s\n\nAnalyse :",
    [PROMPT_REFORMULATION] = "Reformule le texte suivant pour le rendre plus clair :\n\n%s\n\nTexte reformulé :",
    [PROMPT_TRADUCTION] = "Traduis le texte suivant en anglais :\n\n%s\n\nTraduction :"
};

char* generate_prompt(PromptType type, const char* text, const char* context) {
    if (type < 0 || type >= sizeof(PROMPT_TEMPLATES) / sizeof(PROMPT_TEMPLATES[0])) {
        return NULL;
    }

    const char* template = PROMPT_TEMPLATES[type];
    if (!template) {
        return NULL;
    }

    /* Si context est fourni, on pourrait l'intégrer, mais pour l'instant on l'ignore */
    (void)context;

    /* Allouer de la mémoire pour le prompt final */
    size_t len = strlen(template) + strlen(text) + 1;
    char* prompt = (char*)malloc(len);
    if (!prompt) {
        return NULL;
    }

    /* Formater le prompt */
    snprintf(prompt, len, template, text);
    return prompt;
}