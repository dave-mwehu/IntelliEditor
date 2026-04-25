#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

/*
 * CONTRAT MEMOIRE : les fonctions retournant des TokenList allouent
 * dynamiquement. L'appelant doit liberer avec tokenizer_free_list().
 * En cas d'erreur, tokens == NULL et count == 0 TOUJOURS.
 */

/* --- Structures --- */

typedef struct {
    char** tokens;
    int    count;
} TokenList;

/* --- Prototypes internes --- */
static void tokenlist_liberer_partielle(TokenList* liste);
static int  tokenlist_agrandir(TokenList* liste, int* capacite);

/* --- Fonctions internes --- */

/* Libere proprement une liste partielle et remet a zero */
static void tokenlist_liberer_partielle(TokenList* liste) {
    if (!liste) return;
    for (int i = 0; i < liste->count; i++)
        free(liste->tokens[i]);
    free(liste->tokens);
    liste->tokens = NULL;
    liste->count  = 0;
}

/* Agrandit le tableau de tokens — retourne 0 si echec */
static int tokenlist_agrandir(TokenList* liste, int* capacite) {
    int nouvelle_capacite = (*capacite) * 2;
    char** tmp = realloc(liste->tokens, sizeof(char*) * nouvelle_capacite);
    if (!tmp) return 0;
    liste->tokens = tmp;
    *capacite = nouvelle_capacite;
    return 1;
}

/* Ajoute un token — retourne 0 si echec, libere la liste entiere */
static int tokenlist_ajouter(TokenList* liste, int* capacite,
                              const char* debut, int longueur) {
    if (longueur <= 0) return 1; /* rien a ajouter, pas une erreur */

    if (liste->count >= *capacite) {
        if (!tokenlist_agrandir(liste, capacite)) {
            tokenlist_liberer_partielle(liste);
            return 0;
        }
    }

    liste->tokens[liste->count] = malloc(longueur + 1);
    if (!liste->tokens[liste->count]) {
        tokenlist_liberer_partielle(liste);
        return 0;
    }
    memcpy(liste->tokens[liste->count], debut, longueur);
    liste->tokens[liste->count][longueur] = '\0';
    liste->count++;
    return 1;
}

/*
 * En français, l'apostrophe et le tiret font partie des mots
 * (l'homme, aujourd'hui, porte-monnaie).
 * On ne les traite PAS comme separateurs.
 */
static int est_separateur_mot(char c) {
    return isspace((unsigned char)c) ||
           c == ',' || c == ';' || c == ':' ||
           c == '(' || c == ')' || c == '"' ||
           c == '.' || c == '!' || c == '?' ||
           c == '\n';
}

/* Retourne le nombre de caracteres du separateur de phrase en p
 * (1 pour . ! ?, 3 pour ...) ou 0 si ce n'est pas un separateur */
static int longueur_separateur_phrase(const char* p) {
    if (strncmp(p, "...", 3) == 0) return 3;
    if (*p == '.' || *p == '!' || *p == '?') return 1;
    return 0;
}

/* --- API publique --- */

void tokenizer_free_list(TokenList* liste);  /* prototype avance */

/*
 * Decoupe un texte en mots.
 * @param texte : texte source (non NULL)
 * @return TokenList — tokens==NULL et count==0 si erreur ou texte vide
 */
TokenList tokenizer_mots(const char* texte) {
    TokenList result = {NULL, 0};
    if (!texte) return result;

    int capacite = 64;
    result.tokens = malloc(sizeof(char*) * capacite);
    if (!result.tokens) return result;

    const char* p = texte;
    while (*p) {
        while (*p && est_separateur_mot(*p)) p++;
        if (!*p) break;

        const char* debut = p;
        while (*p && !est_separateur_mot(*p)) p++;

        int longueur = (int)(p - debut);
        if (!tokenlist_ajouter(&result, &capacite, debut, longueur))
            return result; /* deja remise a zero par ajouter */
    }

    return result;
}

/*
 * Decoupe un texte en phrases.
 * @param texte : texte source (non NULL)
 * @return TokenList — tokens==NULL et count==0 si erreur ou texte vide
 */
TokenList tokenizer_phrases(const char* texte) {
    TokenList result = {NULL, 0};
    if (!texte) return result;

    int capacite = 32;
    result.tokens = malloc(sizeof(char*) * capacite);
    if (!result.tokens) return result;

    const char* debut = texte;
    const char* p     = texte;

    while (*p) {
        int sep_len = longueur_separateur_phrase(p);
        if (sep_len > 0) {
            /* Inclure le separateur dans la phrase */
            int longueur = (int)(p - debut) + sep_len;

            /* Sauter espaces en debut */
            const char* d = debut;
            while (isspace((unsigned char)*d) && d < p) {
                d++;
                longueur--;
            }

            if (longueur > sep_len) { /* phrase non vide */
                if (!tokenlist_ajouter(&result, &capacite, d, longueur))
                    return result;
            }

            p    += sep_len; /* consommer tout le separateur (1 ou 3 chars) */
            debut = p;
        } else {
            p++;
        }
    }

    /* Derniere phrase sans ponctuation finale */
    const char* d = debut;
    while (isspace((unsigned char)*d) && d < p) d++;
    int longueur = (int)(p - d);
    if (longueur > 0) {
        if (!tokenlist_ajouter(&result, &capacite, d, longueur))
            return result;
    }

    return result;
}

/*
 * Compte le nombre de mots dans un texte.
 * @param texte : texte source (non NULL)
 * @return nombre de mots, -1 si erreur
 */
int tokenizer_compter_mots(const char* texte) {
    if (!texte) return -1;
    TokenList liste = tokenizer_mots(texte);
    if (!liste.tokens) return -1;
    int count = liste.count;
    tokenizer_free_list(&liste);
    return count;
}

/*
 * Libere une TokenList.
 */
void tokenizer_free_list(TokenList* liste) {
    if (!liste) return;
    tokenlist_liberer_partielle(liste);
}
