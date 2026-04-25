#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../../include/llm_interface.h"

/* Definition complete de LLMTask — opaque pour les autres modules */
struct LLMTask {
    char*           prompt;
    char*           response;
    int             done;
    int             error;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
};

typedef struct {
    LLMEngine*      engine;
    struct LLMTask* queue[32];
    int             head;
    int             tail;
    int             count;
    pthread_t       thread;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int             running;
} LLMThreadPool;

static LLMThreadPool*   g_pool       = NULL;
static pthread_mutex_t  g_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

static void* llm_worker(void* arg) {
    LLMThreadPool* pool = (LLMThreadPool*)arg;

    while (1) {
        pthread_mutex_lock(&pool->mutex);
        while (pool->count == 0 && pool->running)
            pthread_cond_wait(&pool->cond, &pool->mutex);

        if (!pool->running && pool->count == 0) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        struct LLMTask* task = pool->queue[pool->head];
        pool->head = (pool->head + 1) % 32;
        pool->count--;
        pthread_mutex_unlock(&pool->mutex);

        char* result = llm_ask(pool->engine, task->prompt);

        pthread_mutex_lock(&task->mutex);
        task->response = result;
        task->error    = (result == NULL) ? 1 : 0;
        task->done     = 1;
        pthread_cond_signal(&task->cond);
        pthread_mutex_unlock(&task->mutex);

        printf("[LLM Thread] Tache traitee.\n");
    }

    return NULL;
}

int llm_thread_start(LLMEngine* engine) {
    if (!engine) {
        fprintf(stderr, "[LLM Thread] engine NULL\n");
        return -1;
    }

    pthread_mutex_lock(&g_pool_mutex);
    if (g_pool) {
        pthread_mutex_unlock(&g_pool_mutex);
        return 0;
    }

    LLMThreadPool* pool = malloc(sizeof(LLMThreadPool));
    if (!pool) {
        fprintf(stderr, "[LLM Thread] Erreur malloc pool\n");
        pthread_mutex_unlock(&g_pool_mutex);
        return -1;
    }

    pool->engine  = engine;
    pool->head    = 0;
    pool->tail    = 0;
    pool->count   = 0;
    pool->running = 1;

    if (pthread_mutex_init(&pool->mutex, NULL) != 0 ||
        pthread_cond_init(&pool->cond, NULL)   != 0 ||
        pthread_create(&pool->thread, NULL, llm_worker, pool) != 0) {
        fprintf(stderr, "[LLM Thread] Erreur init thread\n");
        free(pool);
        pthread_mutex_unlock(&g_pool_mutex);
        return -1;
    }

    g_pool = pool;
    pthread_mutex_unlock(&g_pool_mutex);
    printf("[LLM Thread] Thread demarre.\n");
    return 0;
}

LLMTask* llm_thread_submit(const char* prompt) {
    if (!prompt) return NULL;

    pthread_mutex_lock(&g_pool_mutex);
    if (!g_pool || !g_pool->running) {
        pthread_mutex_unlock(&g_pool_mutex);
        fprintf(stderr, "[LLM Thread] Pool non demarre\n");
        return NULL;
    }
    pthread_mutex_unlock(&g_pool_mutex);

    struct LLMTask* task = malloc(sizeof(struct LLMTask));
    if (!task) return NULL;

    task->prompt = strdup(prompt);
    if (!task->prompt) { free(task); return NULL; }

    task->response = NULL;
    task->done     = 0;
    task->error    = 0;
    pthread_mutex_init(&task->mutex, NULL);
    pthread_cond_init(&task->cond, NULL);

    pthread_mutex_lock(&g_pool->mutex);
    if (g_pool->count >= 32) {
        pthread_mutex_unlock(&g_pool->mutex);
        fprintf(stderr, "[LLM Thread] File pleine\n");
        free(task->prompt);
        pthread_mutex_destroy(&task->mutex);
        pthread_cond_destroy(&task->cond);
        free(task);
        return NULL;
    }
    g_pool->queue[g_pool->tail] = task;
    g_pool->tail = (g_pool->tail + 1) % 32;
    g_pool->count++;
    pthread_cond_signal(&g_pool->cond);
    pthread_mutex_unlock(&g_pool->mutex);

    return (LLMTask*)task;
}

int llm_task_is_done(LLMTask* task) {
    if (!task) return -1;
    pthread_mutex_lock(&task->mutex);
    int done = task->done;
    pthread_mutex_unlock(&task->mutex);
    return done;
}

char* llm_task_wait(LLMTask* task) {
    if (!task) return NULL;
    pthread_mutex_lock(&task->mutex);
    while (!task->done)
        pthread_cond_wait(&task->cond, &task->mutex);
    char* response = task->response;
    pthread_mutex_unlock(&task->mutex);
    return response;
}

void llm_task_free(LLMTask* task) {
    if (!task) return;
    pthread_mutex_lock(&task->mutex);
    while (!task->done)
        pthread_cond_wait(&task->cond, &task->mutex);
    pthread_mutex_unlock(&task->mutex);

    free(task->prompt);
    free(task->response);
    pthread_mutex_destroy(&task->mutex);
    pthread_cond_destroy(&task->cond);
    free(task);
}

void llm_thread_stop(void) {
    pthread_mutex_lock(&g_pool_mutex);
    if (!g_pool) {
        pthread_mutex_unlock(&g_pool_mutex);
        return;
    }
    LLMThreadPool* pool = g_pool;
    g_pool = NULL;
    pthread_mutex_unlock(&g_pool_mutex);

    pthread_mutex_lock(&pool->mutex);
    pool->running = 0;
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    pthread_join(pool->thread, NULL);
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
    free(pool);
    printf("[LLM Thread] Thread arrete.\n");
}
