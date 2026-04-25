#ifndef EDITOR_CORE_H
#define EDITOR_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * editor_core.h — Contrat public du module éditeur (DEV-A)
 *
 * THREAD-SAFETY : aucune fonction n'est thread-safe.
 * Toutes les appels doivent venir du thread UI principal.
 *
 * PROPRIETE MEMOIRE :
 * - editor_get_text() retourne un buffer possédé par l'éditeur
 *   Ne pas le libérer — il est invalidé au prochain appel
 * - editor_get_selection() idem
 */

/* --- Structure opaque --- */
typedef struct EditorCore EditorCore;

/* --- Styles de texte --- */
typedef enum {
    STYLE_NORMAL     = 0,
    STYLE_BOLD       = 1 << 0,
    STYLE_ITALIC     = 1 << 1,
    STYLE_UNDERLINE  = 1 << 2,
    STYLE_HEADING_1  = 1 << 3,
    STYLE_HEADING_2  = 1 << 4,
    STYLE_HEADING_3  = 1 << 5,
    STYLE_HEADING_4  = 1 << 6,
    STYLE_LIST_BULLET   = 1 << 7,
    STYLE_LIST_NUMBERED = 1 << 8
} EditorStyle;

/* --- Initialisation --- */

/*
 * Cree un nouvel éditeur vide.
 * @return EditorCore* ou NULL si erreur
 */
EditorCore* editor_init(void);

/*
 * Libere toutes les ressources de l'éditeur.
 */
void editor_free(EditorCore* editor);

/* --- Edition de base --- */

/*
 * Insere du texte a la position du curseur.
 * @param texte : texte a inserer (non NULL)
 * @return 0 si succes, -1 si erreur
 */
int editor_inserer(EditorCore* editor, const char* texte);

/*
 * Supprime n caracteres avant le curseur.
 * @param n : nombre de caracteres a supprimer
 */
void editor_supprimer(EditorCore* editor, int n);

/*
 * Deplace le curseur.
 * @param position : offset depuis le debut du texte
 */
void editor_set_curseur(EditorCore* editor, int position);

/*
 * Retourne la position actuelle du curseur.
 */
int editor_get_curseur(const EditorCore* editor);

/* --- Contenu --- */

/*
 * Retourne le texte complet.
 * Pointeur possede par l'éditeur — ne pas liberer.
 * @return texte ou NULL si erreur
 */
const char* editor_get_text(const EditorCore* editor);

/*
 * Retourne le nombre de mots du document.
 */
int editor_compter_mots(const EditorCore* editor);

/*
 * Retourne le nombre de caracteres du document.
 */
int editor_compter_chars(const EditorCore* editor);

/* --- Styles --- */

/*
 * Applique un style a la selection courante.
 * @param style : un ou plusieurs STYLE_* combines avec |
 */
void editor_appliquer_style(EditorCore* editor, EditorStyle style);

/* --- Undo / Redo --- */

/*
 * Annule la derniere action.
 * @return 1 si une action a ete annulee, 0 sinon
 */
int editor_undo(EditorCore* editor);

/*
 * Retablit la derniere action annulee.
 * @return 1 si une action a ete retablie, 0 sinon
 */
int editor_redo(EditorCore* editor);

/* --- Rechercher / Remplacer --- */

/*
 * Cherche la prochaine occurrence de motif.
 * @param motif   : texte a chercher (non NULL)
 * @param depuis  : offset de depart
 * @return offset de l'occurrence, -1 si non trouve
 */
int editor_chercher(const EditorCore* editor,
                    const char* motif, int depuis);

/*
 * Remplace toutes les occurrences de motif par remplacement.
 * @return nombre de remplacements effectues
 */
int editor_remplacer_tout(EditorCore* editor,
                           const char* motif,
                           const char* remplacement);

/* --- Export --- */

/*
 * Exporte le document en fichier texte brut.
 * @param chemin : chemin du fichier de sortie (non NULL)
 * @return 0 si succes, -1 si erreur
 */
int editor_exporter_txt(const EditorCore* editor, const char* chemin);

/*
 * Exporte le document en fichier RTF.
 * @param chemin : chemin du fichier de sortie (non NULL)
 * @return 0 si succes, -1 si erreur
 */
int editor_exporter_rtf(const EditorCore* editor, const char* chemin);

#ifdef __cplusplus
}
#endif

#endif /* EDITOR_CORE_H */
