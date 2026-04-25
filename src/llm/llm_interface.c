#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../llama.cpp/include/llama.h"
#include "../../include/llm_interface.h"

/* Variable globale pour eviter d'init/free le backend plusieurs fois */
static int backend_initialized = 0;

struct LLMEngine {
    struct llama_model*   model;
    struct llama_context* ctx;
    int                   n_threads;
};

LLMEngine* llm_init(const char* model_path, int n_threads, int n_ctx) {
    if (!backend_initialized) {
        llama_backend_init();
        backend_initialized = 1;
    }

    struct llama_model_params mparams = llama_model_default_params();
    struct llama_model* model = llama_model_load_from_file(model_path, mparams);
    if (!model) {
        fprintf(stderr, "[LLM] Erreur : impossible de charger %s\n", model_path);
        return NULL;
    }

    struct llama_context_params cparams = llama_context_default_params();
    cparams.n_ctx     = n_ctx;
    cparams.n_threads = n_threads;

    struct llama_context* ctx = llama_init_from_model(model, cparams);
    if (!ctx) {
        fprintf(stderr, "[LLM] Erreur : impossible de creer le contexte\n");
        llama_model_free(model);
        return NULL;
    }

    LLMEngine* engine = malloc(sizeof(LLMEngine));
    if (!engine) {
        fprintf(stderr, "[LLM] Erreur : malloc engine echoue\n");
        llama_free(ctx);
        llama_model_free(model);
        return NULL;
    }

    engine->model     = model;
    engine->ctx       = ctx;
    engine->n_threads = n_threads;

    printf("[LLM] Modele charge avec succes.\n");
    return engine;
}

char* llm_ask(LLMEngine* engine, const char* prompt) {
    if (!engine || !prompt) return NULL;

    /* 1. Reset du contexte pour eviter l'accumulation de la memoire */
    llama_memory_clear(llama_get_memory(engine->ctx), false);

    /* 2. Tokeniser le prompt */
    int n_tokens_max = llama_n_ctx(engine->ctx);
    llama_token* tokens = malloc(sizeof(llama_token) * n_tokens_max);
    if (!tokens) {
        fprintf(stderr, "[LLM] Erreur : malloc tokens echoue\n");
        return NULL;
    }

    const struct llama_vocab* vocab = llama_model_get_vocab(engine->model);
    int n_tokens = llama_tokenize(vocab, prompt, strlen(prompt),
                                  tokens, n_tokens_max, true, false);
    if (n_tokens < 0) {
        fprintf(stderr, "[LLM] Erreur tokenisation\n");
        free(tokens);
        return NULL;
    }

    /* 3. Evaluer le prompt */
    struct llama_batch batch = llama_batch_get_one(tokens, n_tokens);
    if (llama_decode(engine->ctx, batch) != 0) {
        fprintf(stderr, "[LLM] Erreur decode prompt\n");
        free(tokens);
        return NULL;
    }

    /* 4. Generer la reponse token par token */
    size_t buf_size = 4096;
    char* response = malloc(buf_size);
    if (!response) {
        fprintf(stderr, "[LLM] Erreur : malloc response echoue\n");
        free(tokens);
        return NULL;
    }
    response[0] = '\0';
    size_t pos = 0;

    struct llama_sampler* sampler = llama_sampler_chain_init(
        llama_sampler_chain_default_params()
    );
    llama_sampler_chain_add(sampler, llama_sampler_init_greedy());

    for (int i = 0; i < 512; i++) {
        llama_token token_id = llama_sampler_sample(sampler, engine->ctx, -1);

        if (llama_vocab_is_eog(vocab, token_id)) break;

        char piece[32];
        int n = llama_token_to_piece(vocab, token_id, piece, sizeof(piece), 0, false);
        if (n > 0) {
            /* Agrandir le buffer si necessaire */
            if (pos + n + 1 >= buf_size) {
                buf_size *= 2;
                char* tmp = realloc(response, buf_size);
                if (!tmp) {
                    fprintf(stderr, "[LLM] Erreur : realloc echoue\n");
                    break;
                }
                response = tmp;
            }
            memcpy(response + pos, piece, n);
            pos += n;
            response[pos] = '\0';
        }

        struct llama_batch next = llama_batch_get_one(&token_id, 1);
        if (llama_decode(engine->ctx, next) != 0) {
            fprintf(stderr, "[LLM] Erreur decode token %d\n", i);
            break;
        }
    }

    llama_sampler_free(sampler);
    free(tokens);
    return response;
}

void llm_free(LLMEngine* engine) {
    if (!engine) return;
    llama_free(engine->ctx);
    llama_model_free(engine->model);
    free(engine);
    /* On ne free pas le backend ici car il peut y avoir
       d'autres instances LLMEngine encore actives */
    printf("[LLM] Ressources liberees.\n");
}
