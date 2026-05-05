#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hunspell/hunspell.h>

/*
 * CONTRAT MEMOIRE :
 * - hunspell_init()        : alloue et retourne un HunspellEngine*
 * - hunspell_suggestions() : retourne un tableau a liberer avec
 *                            hunspell_free_suggestions() AVANT hunspell_free()
 * - hunspell_free()        : libere le moteur — ne pas utiliser apres
 */

/* --- Structure opaque --- */

typedef struct {
    Hunhandle* handle;
    char*      path_aff;  /* chemin .aff sauvegarde pour debug */
    char*      path_dic;  /* chemin .dic sauvegarde pour debug */
} HunspellEngine;

/* Chemins par defaut — surchargeable via parametre */
#define DEFAULT_AFF "/usr/share/hunspell/fr_FR.aff"
#define DEFAULT_DIC "/usr/share/hunspell/fr_FR.dic"

/* --- API publique --- */

/*
 * Initialise le moteur Hunspell.
 * @param path_aff : chemin vers fr_FR.aff, ou NULL pour le chemin par defaut
 * @param path_dic : chemin vers fr_FR.dic, ou NULL pour le chemin par defaut
 * @return HunspellEngine* a passer aux autres fonctions, NULL si echec
 */
HunspellEngine* hunspell_init(const char* path_aff, const char* path_dic) {
    const char* aff = path_aff ? path_aff : DEFAULT_AFF;
    const char* dic = path_dic ? path_dic : DEFAULT_DIC;

    HunspellEngine* engine = malloc(sizeof(HunspellEngine));
    if (!engine) {
        fprintf(stderr, "[Hunspell] Erreur malloc engine\n");
        return NULL;
    }

    engine->path_aff = strdup(aff);
    engine->path_dic = strdup(dic);

    if (!engine->path_aff || !engine->path_dic) {
        fprintf(stderr, "[Hunspell] Erreur strdup chemins\n");
        free(engine->path_aff);
        free(engine->path_dic);
        free(engine);
        return NULL;
    }

    engine->handle = Hunspell_create(aff, dic);
    if (!engine->handle) {
        fprintf(stderr, "[Hunspell] Erreur chargement dictionnaire :\n"
                        "  .aff : %s\n"
                        "  .dic : %s\n", aff, dic);
        free(engine->path_aff);
        free(engine->path_dic);
        free(engine);
        return NULL;
    }

    printf("[Hunspell] Dictionnaire charge :\n"
           "  .aff : %s\n"
           "  .dic : %s\n", aff, dic);
    return engine;
}

/*
 * Verifie si un mot est correctement orthographie.
 * @param engine : moteur initialise (non NULL)
 * @param mot    : mot a verifier (non NULL)
 * @return 1 si correct, 0 si incorrect, -1 si erreur
 */
int hunspell_verifier(HunspellEngine* engine, const char* mot) {
    if (!engine || !mot) return -1;
    return Hunspell_spell(engine->handle, mot) ? 1 : 0;
}

/*
 * Retourne des suggestions pour un mot mal orthographie.
 * @param engine  : moteur initialise (non NULL)
 * @param mot     : mot incorrect (non NULL)
 * @param nb_sugg : nombre de suggestions retournees (sortie)
 * @return tableau de suggestions a liberer avec hunspell_free_suggestions()
 *         AVANT d'appeler hunspell_free(). NULL si erreur.
 */
char** hunspell_suggestions(HunspellEngine* engine,
                             const char* mot, int* nb_sugg) {
    if (!engine || !mot || !nb_sugg) return NULL;

    char** liste = NULL;
    *nb_sugg = Hunspell_suggest(engine->handle, &liste, mot);

    if (*nb_sugg <= 0) {
        *nb_sugg = 0;
        return NULL;
    }

    return liste;
}

/*
 * Libere les suggestions. A appeler AVANT hunspell_free().
 * @param engine  : moteur initialise (non NULL)
 * @param liste   : tableau retourne par hunspell_suggestions()
 * @param nb_sugg : nombre de suggestions
 */
void hunspell_free_suggestions(HunspellEngine* engine,
                                char** liste, int nb_sugg) {
    if (!engine || !liste) return;
    Hunspell_free_list(engine->handle, &liste, nb_sugg);
}

/*
 * Libere toutes les ressources du moteur.
 * Appeler hunspell_free_suggestions() avant cette fonction.
 * @param engine : moteur a liberer
 */
void hunspell_free(HunspellEngine* engine) {
    if (!engine) return;
    Hunspell_destroy(engine->handle);
    free(engine->path_aff);
    free(engine->path_dic);
    free(engine);
    printf("[Hunspell] Ressources liberees.\n");
}
