#ifndef LLM_INTERFACE_H
#define LLM_INTERFACE_H

/* Interface principale du moteur LLM (DEV-C)
 * Charge un modèle GGUF et expose les fonctions
 * d'inférence pour le reste du projet.
 */

typedef struct LLMEngine LLMEngine;

/* Charge le modèle depuis model_path.
 * Retourne NULL en cas d'échec. */
LLMEngine* llm_init(const char* model_path, int n_threads, int n_ctx);

/* Envoie un prompt et retourne la réponse (allouée dynamiquement).
 * L'appelant doit libérer la mémoire avec free(). */
char* llm_ask(LLMEngine* engine, const char* prompt);

/* Libère toutes les ressources du moteur. */
void llm_free(LLMEngine* engine);

#endif /* LLM_INTERFACE_H */
