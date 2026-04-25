#ifndef NLP_ENGINE_H
#define NLP_ENGINE_H

/*
 * nlp_engine.h — Contrat public du module NLP (DEV-C)
 * Toutes les positions sont des offsets caracteres UTF-8
 * depuis le debut du texte (start inclusif, end exclusif).
 */

#include "llm_interface.h"

/* Forward declaration — defini dans hunspell_wrap.c */
typedef struct HunspellEngine HunspellEngine;

/* --- Types d'erreurs --- */
typedef enum {
    NLP_ORTHO,       /* faute orthographique        */
    NLP_GRAMMAIRE,   /* faute grammaticale          */
    NLP_STYLE,       /* repetition / style          */
    NLP_PONCTUATION  /* erreur de ponctuation       */
} NLPErreurType;

/* --- Erreur individuelle --- */
typedef struct {
    NLPErreurType type;
    int           start;      /* offset debut (inclusif) */
    int           end;        /* offset fin   (exclusif) */
    char*         original;   /* texte original          */
    char*         suggestion; /* correction proposee     */
} NLPErreur;

/* --- Resultat d'analyse --- */
typedef struct {
    NLPErreur* erreurs;
    int        count;
    int        capacite;
    int        erreur_interne; /* 1 si malloc a echoue en cours d'analyse */
} NLPResult;

/* --- Hunspell --- */
HunspellEngine* hunspell_init(const char* path_aff, const char* path_dic);
int             hunspell_verifier(HunspellEngine* engine, const char* mot);
char**          hunspell_suggestions(HunspellEngine* engine,
                                     const char* mot, int* nb_sugg);
void            hunspell_free_suggestions(HunspellEngine* engine,
                                          char** liste, int nb_sugg);
void            hunspell_free(HunspellEngine* engine);

/* --- Tokenizer --- */
typedef struct { char** tokens; int count; } TokenList;
TokenList tokenizer_mots(const char* texte);
TokenList tokenizer_phrases(const char* texte);
int       tokenizer_compter_mots(const char* texte);
void      tokenizer_free_list(TokenList* liste);

/* --- Prompts --- */
char* prompt_grammaire(const char* phrase);
char* prompt_reformulation(const char* phrase);
char* prompt_semantique(const char* question, const char* texte);
char* prompt_resume(const char* texte);

/* --- NLP Engine --- */
NLPResult nlp_analyser(const char* texte,
                        LLMEngine* engine,
                        HunspellEngine* hs);
void      nlp_result_afficher(const NLPResult* r);
void      nlp_result_free(NLPResult* r);

#endif /* NLP_ENGINE_H */
