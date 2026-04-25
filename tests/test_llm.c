#include <stdio.h>
#include <stdlib.h>
#include "../include/llm_interface.h"

int main(void) {
    printf("=== Test module LLM ===\n\n");

    /* 1. Charger le modele */
    printf("-- Chargement du modele --\n");
    LLMEngine* engine = llm_init(
        "data/models/qwen2.5-3b-instruct-q4_k_m.gguf",
        4,    /* threads */
        2048  /* contexte */
    );
    if (!engine) {
        fprintf(stderr, "ECHEC : llm_init\n");
        return 1;
    }

    /* 2. Demarrer le thread */
    if (llm_thread_start(engine) != 0) {
        fprintf(stderr, "ECHEC : llm_thread_start\n");
        llm_free(engine);
        return 1;
    }

    /* 3. Poser une question */
    printf("\n-- Test llm_ask --\n");
    const char* prompt = "Corrige cette phrase en français : "
                         "Les enfant mange des pomme.\n"
                         "Correction :";

    printf("Prompt : %s\n", prompt);
    printf("Réponse : ");
    fflush(stdout);

    LLMTask* task = llm_thread_submit(prompt);
    if (!task) {
        fprintf(stderr, "ECHEC : llm_thread_submit\n");
    } else {
        char* reponse = llm_task_wait(task);
        printf("%s\n", reponse ? reponse : "(vide)");
        llm_task_free(task);
    }

    /* 4. Nettoyage */
    llm_thread_stop();
    llm_free(engine);

    printf("\n=== Test termine ===\n");
    return 0;
}
