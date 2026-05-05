#include <stdio.h>
#include <stdlib.h>
#include "../include/nlp_engine.h"

int main(void) {
    printf("=== Test module NLP ===\n\n");

    /* 1. Test Hunspell */
    printf("-- Test Hunspell --\n");
    HunspellEngine* hs = hunspell_init(NULL, NULL);
    if (!hs) {
        fprintf(stderr, "ECHEC : hunspell_init\n");
        return 1;
    }

    const char* mots[] = { "bonjour", "langague", "université", "tset" };
    for (int i = 0; i < 4; i++) {
        int ok = hunspell_verifier(hs, mots[i]);
        printf("  '%s' : %s\n", mots[i], ok ? "correct" : "incorrect");

        if (!ok) {
            int nb = 0;
            char** sugg = hunspell_suggestions(hs, mots[i], &nb);
            if (sugg && nb > 0)
                printf("    suggestion : %s\n", sugg[0]);
            if (sugg)
                hunspell_free_suggestions(hs, sugg, nb);
        }
    }

    /* 2. Test tokenizer */
    printf("\n-- Test tokenizer --\n");
    const char* texte = "Bonjour le monde. Comment allez-vous aujourd'hui ?";
    printf("  Texte : %s\n", texte);

    TokenList mots_liste = tokenizer_mots(texte);
    printf("  Mots (%d) :\n", mots_liste.count);
    for (int i = 0; i < mots_liste.count; i++)
        printf("    [%d] %s\n", i, mots_liste.tokens[i]);
    tokenizer_free_list(&mots_liste);

    TokenList phrases = tokenizer_phrases(texte);
    printf("  Phrases (%d) :\n", phrases.count);
    for (int i = 0; i < phrases.count; i++)
        printf("    [%d] %s\n", i, phrases.tokens[i]);
    tokenizer_free_list(&phrases);

    /* 3. Test NLP complet sans LLM */
    printf("\n-- Test NLP complet (sans LLM) --\n");
    const char* texte2 = "Je mange mange une pomme.Il fait beau!";
    printf("  Texte : %s\n", texte2);

    NLPResult result = nlp_analyser(texte2, NULL, hs);
    nlp_result_afficher(&result);
    nlp_result_free(&result);

    hunspell_free(hs);

    printf("\n=== Tests termines ===\n");
    return 0;
}
