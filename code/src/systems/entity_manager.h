/**
 * entity_manager.h
 * Gestionnaire des entités et des composants
 */

#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/entity.h"

// Nombre maximum d'entités gérées simultanément
#define MAX_ENTITIES 1000

// Nombre maximum de composants par type
#define MAX_COMPONENTS_PER_TYPE 1000

// Structure de gestion des entités
typedef struct {
    // Liste des ID d'entités utilisés
    EntityID entities[MAX_ENTITIES];
    // Masques de composants pour chaque entité
    ComponentMask entity_masks[MAX_ENTITIES];
    // Nombre d'entités actives
    int entity_count;
    // Prochain ID d'entité à attribuer
    EntityID next_entity_id;

    // Tableaux de composants par type
    void* component_arrays[COMPONENT_TYPE_COUNT];
    // Nombre de composants par type
    int component_counts[COMPONENT_TYPE_COUNT];
} EntityManager;

/**
 * Initialise le gestionnaire d'entités
 * @return Pointeur vers le gestionnaire initialisé ou NULL en cas d'erreur
 */
EntityManager* entity_manager_init(void);

/**
 * Libère les ressources du gestionnaire d'entités
 * @param manager Gestionnaire d'entités à libérer
 */
void entity_manager_shutdown(EntityManager* manager);

/**
 * Crée une nouvelle entité
 * @param manager Gestionnaire d'entités
 * @return ID de la nouvelle entité ou INVALID_ENTITY_ID en cas d'erreur
 */
EntityID entity_create(EntityManager* manager);

/**
 * Détruit une entité et tous ses composants
 * @param manager Gestionnaire d'entités
 * @param entity_id ID de l'entité à détruire
 * @return true si l'entité a été détruite, false sinon
 */
bool entity_destroy(EntityManager* manager, EntityID entity_id);

/**
 * Vérifie si une entité existe
 * @param manager Gestionnaire d'entités
 * @param entity_id ID de l'entité à vérifier
 * @return true si l'entité existe, false sinon
 */
bool entity_exists(EntityManager* manager, EntityID entity_id);

/**
 * Ajoute un composant à une entité
 * @param manager Gestionnaire d'entités
 * @param entity_id ID de l'entité
 * @param component Pointeur vers le composant à ajouter
 * @return true si le composant a été ajouté, false sinon
 */
bool entity_add_component(EntityManager* manager, EntityID entity_id, void* component);

/**
 * Supprime un composant d'une entité
 * @param manager Gestionnaire d'entités
 * @param entity_id ID de l'entité
 * @param component_type Type du composant à supprimer
 * @return true si le composant a été supprimé, false sinon
 */
bool entity_remove_component(EntityManager* manager, EntityID entity_id, ComponentType component_type);

/**
 * Récupère un composant d'une entité
 * @param manager Gestionnaire d'entités
 * @param entity_id ID de l'entité
 * @param component_type Type du composant à récupérer
 * @return Pointeur vers le composant ou NULL s'il n'existe pas
 */
void* entity_get_component(EntityManager* manager, EntityID entity_id, ComponentType component_type);

/**
 * Vérifie si une entité possède un composant spécifique
 * @param manager Gestionnaire d'entités
 * @param entity_id ID de l'entité
 * @param component_type Type du composant à vérifier
 * @return true si l'entité possède le composant, false sinon
 */
bool entity_has_component(EntityManager* manager, EntityID entity_id, ComponentType component_type);

/**
 * Récupère le masque de composants d'une entité
 * @param manager Gestionnaire d'entités
 * @param entity_id ID de l'entité
 * @return Masque de composants ou 0 si l'entité n'existe pas
 */
ComponentMask entity_get_mask(EntityManager* manager, EntityID entity_id);

/**
 * Récupère toutes les entités qui possèdent un ensemble spécifique de composants
 * @param manager Gestionnaire d'entités
 * @param mask Masque de composants à rechercher
 * @param out_entities Tableau pour stocker les ID d'entités trouvées
 * @param max_count Nombre maximal d'entités à récupérer
 * @return Nombre d'entités trouvées
 */
int entity_find_with_components(EntityManager* manager, ComponentMask mask, \
                               EntityID* out_entities, int max_count);

#endif /* ENTITY_MANAGER_H */
