#ifndef RULE_ENGINE_H
#define RULE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * rule_engine.h — Contrat public du moteur de règles (DEV-D)
 *
 * Utilisation typique :
 *   1. rule_engine_init()       → cree le moteur
 *   2. rule_engine_charger()    → charge un fichier JSON de regles
 *   3. rule_engine_evaluer()    → evalue les regles sur un texte
 *   4. rule_rapport_afficher()  → affiche les resultats
 *   5. rule_engine_free()       → libere les ressources
 */

#include "llm_interface.h"

/* --- Statuts d'une règle --- */
typedef enum {
    RULE_CONFORME,      /* regle respectee      ✅ */
    RULE_NON_CONFORME,  /* regle violee         ❌ */
    RULE_AVERTISSEMENT, /* partiellement OK     ⚠️ */
    RULE_EN_COURS,      /* verification LLM     🔄 */
    RULE_ERREUR         /* erreur interne       */
} RuleStatut;

/* --- Severite d'une règle --- */
typedef enum {
    RULE_SEVERITY_ERROR,
    RULE_SEVERITY_WARNING,
    RULE_SEVERITY_INFO
} RuleSeverity;

/* --- Resultat d'une règle --- */
typedef struct {
    char*        id;          /* ex: "R001"              */
    char*        description; /* description de la regle */
    RuleStatut   statut;
    RuleSeverity severity;
    int          position;    /* offset dans le texte, -1 si N/A */
    char*        detail;      /* message explicatif      */
} RuleResult;

/* --- Rapport complet --- */
typedef struct {
    RuleResult* resultats;
    int         count;
    int         conformes;      /* nombre de regles OK  */
    int         non_conformes;  /* nombre de regles KO  */
    int         avertissements;
} RuleRapport;

/* --- Moteur (opaque) --- */
typedef struct RuleEngine RuleEngine;

/* --- API publique --- */

/*
 * Cree un nouveau moteur de regles.
 * @param engine_llm : moteur LLM pour les verifications semantiques
 *                     (peut etre NULL — desactive les regles LLM)
 * @return RuleEngine* ou NULL si erreur
 */
RuleEngine* rule_engine_init(LLMEngine* engine_llm);

/*
 * Charge un fichier JSON de regles.
 * @param chemin : chemin vers le fichier .json (non NULL)
 * @return 0 si succes, -1 si erreur
 */
int rule_engine_charger(RuleEngine* engine, const char* chemin);

/*
 * Evalue toutes les regles chargees sur un texte.
 * @param texte : texte a analyser (non NULL)
 * @return RuleRapport a liberer avec rule_rapport_free()
 */
RuleRapport rule_engine_evaluer(RuleEngine* engine, const char* texte);

/*
 * Retourne le nombre de regles chargees.
 */
int rule_engine_count(const RuleEngine* engine);

/*
 * Libere un RuleRapport.
 */
void rule_rapport_free(RuleRapport* rapport);

/*
 * Affiche un rapport dans le terminal (pour debug).
 */
void rule_rapport_afficher(const RuleRapport* rapport);

/*
 * Libere toutes les ressources du moteur.
 */
void rule_engine_free(RuleEngine* engine);

#ifdef __cplusplus
}
#endif

#endif /* RULE_ENGINE_H */
