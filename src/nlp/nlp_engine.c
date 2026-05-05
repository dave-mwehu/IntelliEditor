#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/nlp_engine.h"

/* --- Fonctions internes --- */

static NLPResult nlp_result_init(void) {
    NLPResult r;
    r.capacite      = 16;
    r.count         = 0;
    r.erreur_interne = 0;
    r.erreurs       = malloc(sizeof(NLPErreur) * r.capacite);
    if (!r.erreurs) {
        r.capacite      = 0;
        r.erreur_interne = 1;
    }
    return r;
}

/* Retourne 1 si succes, 0 si echec — marque erreur_interne */
static int nlp_result_ajouter(NLPResult* r, NLPErreurType type,
                               int start, int end,
                               const char* original,
                               const char* suggestion) {
    if (!r || r->erreur_interne) return 0;

    if (r->count >= r->capacite) {
        int new_cap = r->capacite * 2;
        NLPErreur* tmp = realloc(r->erreurs,
                                  sizeof(NLPErreur) * new_cap);
        if (!tmp) { r->erreur_interne = 1; return 0; }
        r->erreurs  = tmp;
        r->capacite = new_cap;
    }

    NLPErreur* e  = &r->erreurs[r->count];
    e->type       = type;
    e->start      = start;
    e->end        = end;
    e->original   = original   ? strdup(original)   : NULL;
    e->suggestion = suggestion ? strdup(suggestion) : NULL;

    if ((original   && !e->original) ||
        (suggestion && !e->suggestion)) {
        free(e->original);
        free(e->suggestion);
        r->erreur_interne = 1;
        return 0;
    }

    r->count++;
    return 1;
}

/* Calcule l'offset caractere reel d'un token dans le texte source */
static int trouver_offset(const char* texte, const char* token,
                           int depuis) {
    const char* p = strstr(texte + depuis, token);
    return p ? (int)(p - texte) : depuis;
}

/* Ponctuation française : espace obligatoire avant : ; ! ?
 * Accepte aussi l'espace insecable (0xC2 0xA0 en UTF-8) */
static void verifier_ponctuation(const char* texte, NLPResult* r) {
    if (!texte || !r) return;
    const char* ponctuations = ":;!?";

    for (int i = 1; texte[i]; i++) {
        if (!strchr(ponctuations, texte[i])) continue;

        /* Verifier espace normale ou insecable (UTF-8: 0xC2 0xA0) */
        int espace_ok = (texte[i-1] == ' ');
        if (!espace_ok && i >= 2)
            espace_ok = ((unsigned char)texte[i-2] == 0xC2 &&
                         (unsigned char)texte[i-1] == 0xA0);

        if (!espace_ok) {
            char original[3]  = { texte[i-1], texte[i], '\0' };
            char suggestion[4]= { texte[i-1], ' ', texte[i], '\0' };
            nlp_result_ajouter(r, NLP_PONCTUATION,
                                i - 1, i + 1,
                                original, suggestion);
        }
    }
}

/* Repetitions : fenetre de 3 mots */
static void verifier_repetitions(const char* texte,
                                  NLPResult* r) {
    if (!texte || !r) return;
    TokenList mots = tokenizer_mots(texte);
    if (!mots.tokens) return;

    /* count - 1 pour couvrir aussi "mot mot" (2 mots seulement) */
    int limite = mots.count - 1;
    for (int i = 0; i < limite; i++) {
        int rep = 0;
        if (strcasecmp(mots.tokens[i], mots.tokens[i+1]) == 0)
            rep = 1;
        else if (i + 2 < mots.count &&
                 strcasecmp(mots.tokens[i], mots.tokens[i+2]) == 0)
            rep = 1;

        if (rep) {
            int start = trouver_offset(texte, mots.tokens[i], 0);
            int end   = start + (int)strlen(mots.tokens[i]);
            nlp_result_ajouter(r, NLP_STYLE, start, end,
                                mots.tokens[i],
                                "mot répété trop proche");
        }
    }

    tokenizer_free_list(&mots);
}

/* --- API publique --- */

NLPResult nlp_analyser(const char* texte,
                        LLMEngine* engine,
                        HunspellEngine* hs) {
    NLPResult result = nlp_result_init();
    if (!texte || !hs || result.erreur_interne) return result;

    /* 1. Orthographe mot par mot */
    TokenList mots = tokenizer_mots(texte);
    if (mots.tokens) {
        int offset = 0;
        for (int i = 0; i < mots.count; i++) {
            int start = trouver_offset(texte, mots.tokens[i], offset);
            int end   = start + (int)strlen(mots.tokens[i]);
            offset    = end;

            if (hunspell_verifier(hs, mots.tokens[i]) == 0) {
                int    nb_sugg = 0;
                char** sugg    = hunspell_suggestions(hs,
                                                       mots.tokens[i],
                                                       &nb_sugg);
                nlp_result_ajouter(&result, NLP_ORTHO, start, end,
                                    mots.tokens[i],
                                    nb_sugg > 0 ? sugg[0] : NULL);
                if (sugg)
                    hunspell_free_suggestions(hs, sugg, nb_sugg);
            }
        }
        tokenizer_free_list(&mots);
    }

    /* 2. Ponctuation */
    verifier_ponctuation(texte, &result);

    /* 3. Repetitions */
    verifier_repetitions(texte, &result);

    /* 4. Grammaire via LLM — soumettre TOUTES les phrases d'abord */
    if (engine) {
        TokenList phrases = tokenizer_phrases(texte);
        if (phrases.tokens) {

            /* Tableau de taches — on soumet tout avant d'attendre */
            LLMTask** taches = malloc(sizeof(LLMTask*) * phrases.count);
            char**    prompts = malloc(sizeof(char*)   * phrases.count);

            if (taches && prompts) {
                /* Soumission de toutes les phrases */
                for (int i = 0; i < phrases.count; i++) {
                    prompts[i] = prompt_grammaire(phrases.tokens[i]);
                    taches[i]  = prompts[i]
                                 ? llm_thread_submit(prompts[i])
                                 : NULL;
                }

                /* Attente et collecte des resultats */
                for (int i = 0; i < phrases.count; i++) {
                    if (!taches[i]) { free(prompts[i]); continue; }

                    char* correction = llm_task_wait(taches[i]);
                    if (correction &&
                        strcmp(correction, phrases.tokens[i]) != 0) {
                        int start = trouver_offset(texte,
                                                    phrases.tokens[i],
                                                    0);
                        int end = start +
                                  (int)strlen(phrases.tokens[i]);
                        nlp_result_ajouter(&result, NLP_GRAMMAIRE,
                                            start, end,
                                            phrases.tokens[i],
                                            correction);
                    }
                    free(prompts[i]);
                    llm_task_free(taches[i]);
                }
            }

            free(taches);
            free(prompts);
            tokenizer_free_list(&phrases);
        }
    }

    return result;
}

void nlp_result_afficher(const NLPResult* r) {
    if (!r) return;
    const char* types[] = {"ORTHO","GRAMMAIRE","STYLE","PONCTUATION"};
    printf("[NLP] %d erreur(s)%s :\n",
           r->count,
           r->erreur_interne ? " (analyse incomplete — erreur memoire)"
                             : "");
    for (int i = 0; i < r->count; i++) {
        const NLPErreur* e = &r->erreurs[i];
        printf("  [%s] [%d-%d] '%s' → '%s'\n",
               types[e->type], e->start, e->end,
               e->original   ? e->original   : "?",
               e->suggestion ? e->suggestion : "?");
    }
}

void nlp_result_free(NLPResult* r) {
    if (!r || !r->erreurs) return;
    for (int i = 0; i < r->count; i++) {
        free(r->erreurs[i].original);
        free(r->erreurs[i].suggestion);
    }
    free(r->erreurs);
    r->erreurs       = NULL;
    r->count         = 0;
    r->capacite      = 0;
    r->erreur_interne = 0;
}
