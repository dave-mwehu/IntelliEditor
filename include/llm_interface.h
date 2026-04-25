#ifndef LLM_INTERFACE_H
#define LLM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * llm_interface.h — Contrat public du module LLM (DEV-C)
 *
 * THREAD-SAFETY :
 *   - llm_init / llm_free        : NON thread-safe, appeler depuis un seul thread
 *   - llm_ask                    : NON thread-safe, utiliser le thread worker
 *   - llm_thread_start/stop      : NON thread-safe, appeler depuis un seul thread
 *   - llm_thread_submit          : thread-safe
 *   - llm_task_is_done/wait/free : thread-safe
 *
 * ORDRE D'APPEL OBLIGATOIRE :
 *   llm_init()
 *       └→ llm_thread_start()
 *               └→ llm_thread_submit() / llm_task_wait() / llm_task_free()
 *           llm_thread_stop()   ← AVANT llm_free()
 *       llm_free()              ← llm_free() stoppe le thread si oubli
 *
 * PROPRIETE MEMOIRE :
 *   - llm_ask()       : retourne un buffer a liberer avec free()
 *   - llm_task_wait() : retourne un pointeur POSSEDE par la tache
 *                       Ne pas appeler free() dessus directement
 *                       Utiliser llm_task_free() pour tout liberer
 */

/* --- Moteur LLM (opaque) --- */
typedef struct LLMEngine LLMEngine;

/*
 * Charge un modele GGUF.
 * @param model_path : chemin vers le fichier .gguf (non NULL)
 * @param n_threads  : nombre de threads CPU (ex: 4)
 * @param n_ctx      : taille du contexte (ex: 4096)
 * @return LLMEngine* ou NULL si echec
 */
LLMEngine* llm_init(const char* model_path, int n_threads, int n_ctx);

/*
 * Envoie un prompt et retourne la reponse — BLOQUANT.
 * Preferer llm_thread_submit() pour ne pas bloquer l'UI.
 * @return buffer alloue a liberer avec free(), NULL si erreur
 */
char* llm_ask(LLMEngine* engine, const char* prompt);

/*
 * Libere le moteur. Stoppe automatiquement le thread worker
 * s'il est encore actif.
 */
void llm_free(LLMEngine* engine);

/* --- Tache asynchrone (opaque) --- */
typedef struct LLMTask LLMTask;

/*
 * Demarre le thread worker.
 * @param engine : moteur initialise (non NULL)
 * @return 0 si succes, -1 si echec
 */
int llm_thread_start(LLMEngine* engine);

/*
 * Soumet une question — NON bloquant, thread-safe.
 * @return LLMTask* ou NULL si file pleine / erreur
 */
LLMTask* llm_thread_submit(const char* prompt);

/*
 * Verifie si une tache est terminee — NON bloquant, thread-safe.
 * @return 1 si terminee, 0 sinon, -1 si task NULL
 */
int llm_task_is_done(LLMTask* task);

/*
 * Attend la fin et retourne la reponse — BLOQUANT, thread-safe.
 * Le pointeur retourne est possede par la tache.
 * Ne pas appeler free() dessus — utiliser llm_task_free().
 * @return reponse ou NULL si erreur
 */
char* llm_task_wait(LLMTask* task);

/*
 * Libere la tache et toutes ses ressources — thread-safe.
 * Attend la fin si la tache n'est pas encore terminee.
 */
void llm_task_free(LLMTask* task);

/*
 * Arrete le thread worker proprement.
 * Appele automatiquement par llm_free() si oubli.
 */
void llm_thread_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* LLM_INTERFACE_H */
