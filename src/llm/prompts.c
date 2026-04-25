#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * CONTRAT MEMOIRE : toutes les fonctions de ce fichier retournent
 * un buffer alloue dynamiquement. L'appelant est responsable de
 * le liberer avec free() apres utilisation.
 * Retourne NULL si une entree est NULL ou si malloc echoue.
 */

/* Calcule la taille exacte puis alloue — pas de troncature silencieuse */
static char* build_prompt(const char* fmt, ...) {
    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);

    int needed = vsnprintf(NULL, 0, fmt, args1);
    va_end(args1);

    if (needed < 0) {
        va_end(args2);
        return NULL;
    }

    char* result = malloc(needed + 1);
    if (!result) {
        va_end(args2);
        return NULL;
    }

    vsnprintf(result, needed + 1, fmt, args2);
    va_end(args2);
    return result;
}

/* Correction grammaticale
 * @param phrase  : phrase a corriger (non NULL)
 * @return buffer a liberer avec free(), ou NULL si erreur
 */
char* prompt_grammaire(const char* phrase) {
    if (!phrase) return NULL;
    return build_prompt(
        "Tu es un correcteur grammatical français expert.\n"
        "Corrige uniquement les fautes de grammaire dans cette phrase.\n"
        "Réponds uniquement avec la phrase corrigée, rien d'autre.\n\n"
        "Phrase : %s\n"
        "Correction :",
        phrase
    );
}

/* Reformulation stylistique
 * @param phrase  : phrase a reformuler (non NULL)
 * @return buffer a liberer avec free(), ou NULL si erreur
 */
char* prompt_reformulation(const char* phrase) {
    if (!phrase) return NULL;
    return build_prompt(
        "Tu es un expert en rédaction académique française.\n"
        "Reformule cette phrase de manière plus claire et professionnelle.\n"
        "Réponds uniquement avec la phrase reformulée, rien d'autre.\n\n"
        "Phrase : %s\n"
        "Reformulation :",
        phrase
    );
}

/* Verification semantique : repond OUI ou NON
 * @param question : question posee au LLM (non NULL)
 * @param texte    : texte a analyser (non NULL)
 * @return buffer a liberer avec free(), ou NULL si erreur
 */
char* prompt_semantique(const char* question, const char* texte) {
    if (!question || !texte) return NULL;
    return build_prompt(
        "Tu es un assistant d'analyse de documents académiques.\n"
        "Réponds uniquement par OUI ou NON.\n\n"
        "Question : %s\n\n"
        "Texte à analyser :\n%s\n\n"
        "Réponse (OUI ou NON) :",
        question,
        texte
    );
}

/* Resume court d'un texte
 * @param texte : texte a resumer (non NULL)
 * @return buffer a liberer avec free(), ou NULL si erreur
 */
char* prompt_resume(const char* texte) {
    if (!texte) return NULL;
    return build_prompt(
        "Tu es un assistant de rédaction académique.\n"
        "Fais un résumé court (2-3 phrases maximum) de ce texte en français.\n\n"
        "Texte :\n%s\n\n"
        "Résumé :",
        texte
    );
}
