/**
 * entity_manager.c
 * Implémentation du gestionnaire d'entités
 */

#include <stdlib.h>
#include <string.h>
#include "../systems/entity_manager.h"
#include "../utils/error_handler.h"

// Taille des composants par type
static const size_t component_sizes[COMPONENT_TYPE_COUNT] = {
    sizeof(TransformComponent),    // COMPONENT_TRANSFORM
    sizeof(SpriteComponent),       // COMPONENT_SPRITE
    sizeof(ColliderComponent),     // COMPONENT_COLLIDER
    sizeof(PlayerComponent),       // COMPONENT_PLAYER
    sizeof(ItemComponent),         // COMPONENT_ITEM
    sizeof(FarmingComponent),      // COMPONENT_FARMING
    sizeof(InteractableComponent), // COMPONENT_INTERACTABLE
};

// Initialise le gestionnaire d'entités
EntityManager* entity_manager_init(void) {
    EntityManager* manager = (EntityManager*)calloc(1, sizeof(EntityManager));
    if (!check_ptr(manager, LOG_LEVEL_ERROR, "Échec d'allocation du gestionnaire d'entités")) {
        return NULL;
    }

    // Initialiser les tableaux de composants
    for (int i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        manager->component_arrays[i] = calloc(MAX_COMPONENTS_PER_TYPE, component_sizes[i]);
        if (!check_ptr(manager->component_arrays[i], LOG_LEVEL_ERROR, \
                    "Échec d'allocation des tableaux de composants")) {
            // Libérer les tableaux déjà alloués
            for (int j = 0; j < i; j++) {
                free(manager->component_arrays[j]);
            }
            free(manager);
            return NULL;
        }
        manager->component_counts[i] = 0;
    }

    // Initialiser les autres membres
    manager->entity_count = 0;
    manager->next_entity_id = 1; // Commencer à 1 car 0 est INVALID_ENTITY_ID

    log_info("Gestionnaire d'entités initialisé avec succès");
    return manager;
}

// Libère les ressources du gestionnaire d'entités
void entity_manager_shutdown(EntityManager* manager) {
    if (!manager) return;

    // Libérer tous les tableaux de composants
    for (int i = 0; i < COMPONENT_TYPE_COUNT; i++) {
        if (manager->component_arrays[i]) {
            free(manager->component_arrays[i]);
            manager->component_arrays[i] = NULL;
        }
    }

    // Libérer le gestionnaire lui-même
    free(manager);

    log_info("Gestionnaire d'entités libéré");
}

// Crée une nouvelle entité
EntityID entity_create(EntityManager* manager) {
    if (!manager) return INVALID_ENTITY_ID;

    if (manager->entity_count >= MAX_ENTITIES) {
        log_error("Impossible de créer une entité : limite atteinte (%d)", MAX_ENTITIES);
        return INVALID_ENTITY_ID;
    }

    // Attribuer un nouvel ID
    EntityID new_id = manager->next_entity_id++;

    // Ajouter l'ID à la liste des entités
    manager->entities[manager->entity_count] = new_id;

    // Initialiser le masque de composants
    manager->entity_masks[manager->entity_count] = 0;

    // Incrémenter le compteur d'entités
    manager->entity_count++;

    log_debug("Entité créée avec ID %u", new_id);
    return new_id;
}

// Fonction interne pour trouver l'index d'une entité
static int find_entity_index(EntityManager* manager, EntityID entity_id) {
    if (!manager) return -1;

    for (int i = 0; i < manager->entity_count; i++) {
        if (manager->entities[i] == entity_id) {
            return i;
        }
    }

    return -1;
}

// Détruit une entité et tous ses composants
bool entity_destroy(EntityManager* manager, EntityID entity_id) {
    if (!manager || entity_id == INVALID_ENTITY_ID) return false;

    int entity_index = find_entity_index(manager, entity_id);
    if (entity_index == -1) {
        log_warning("Tentative de destruction d'une entité inexistante (ID: %u)", entity_id);
        return false;
    }

    // Récupérer le masque de composants
    ComponentMask mask = manager->entity_masks[entity_index];

    // Supprimer tous les composants de l'entité
    for (int type = 0; type < COMPONENT_TYPE_COUNT; type++) {
        if (HAS_COMPONENT(mask, type)) {
            entity_remove_component(manager, entity_id, (ComponentType)type);
        }
    }

    // Remplacer cette entité par la dernière dans la liste pour maintenir un tableau compact
    if (entity_index < manager->entity_count - 1) {
        manager->entities[entity_index] = manager->entities[manager->entity_count - 1];
        manager->entity_masks[entity_index] = manager->entity_masks[manager->entity_count - 1];
    }

    // Décrémenter le compteur d'entités
    manager->entity_count--;

    log_debug("Entité détruite (ID: %u)", entity_id);
    return true;
}

// Vérifie si une entité existe
bool entity_exists(EntityManager* manager, EntityID entity_id) {
    return find_entity_index(manager, entity_id) != -1;
}

// Fonction interne pour trouver l'index d'un composant
static int find_component_index(EntityManager* manager, EntityID entity_id, ComponentType type) {
    if (!manager) return -1;

    void* components_array = manager->component_arrays[type];
    int component_count = manager->component_counts[type];

    for (int i = 0; i < component_count; i++) {
        // Calculer le pointeur vers le composant
        Component* component = (Component*)((char*)components_array + i * component_sizes[type]);

        if (component->entity == entity_id) {
            return i;
        }
    }

    return -1;
}

// Ajoute un composant à une entité
bool entity_add_component(EntityManager* manager, EntityID entity_id, void* component) {
    if (!manager || !component || entity_id == INVALID_ENTITY_ID) return false;

    int entity_index = find_entity_index(manager, entity_id);
    if (entity_index == -1) {
        log_warning("Tentative d'ajout d'un composant à une entité inexistante (ID: %u)", entity_id);
        return false;
    }

    // Récupérer le type de composant
    Component* base_component = (Component*)component;
    ComponentType type = base_component->type;

    // Vérifier si l'entité a déjà ce type de composant
    if (HAS_COMPONENT(manager->entity_masks[entity_index], type)) {
        log_warning("L'entité %u possède déjà un composant de type %d", entity_id, type);
        return false;
    }

    // Vérifier si on a atteint la limite de composants pour ce type
    if (manager->component_counts[type] >= MAX_COMPONENTS_PER_TYPE) {
        log_error("Limite de composants atteinte pour le type %d", type);
        return false;
    }

    // Calculer l'adresse où le composant sera stocké
    void* dest = (char*)manager->component_arrays[type] + \
                 manager->component_counts[type] * component_sizes[type];

    // Copier le composant dans le tableau de composants
    memcpy(dest, component, component_sizes[type]);

    // Mettre à jour le masque de composants de l'entité
    manager->entity_masks[entity_index] = ADD_COMPONENT(manager->entity_masks[entity_index], type);

    // Incrémenter le nombre de composants
    manager->component_counts[type]++;

    log_debug("Composant de type %d ajouté à l'entité %u", type, entity_id);
    return true;
}

// Supprime un composant d'une entité
bool entity_remove_component(EntityManager* manager, EntityID entity_id, ComponentType type) {
    if (!manager || entity_id == INVALID_ENTITY_ID || type >= COMPONENT_TYPE_COUNT) return false;

    int entity_index = find_entity_index(manager, entity_id);
    if (entity_index == -1) {
        log_warning("Tentative de suppression d'un composant d'une entité inexistante (ID: %u)", entity_id);
        return false;
    }

    // Vérifier si l'entité possède ce type de composant
    if (!HAS_COMPONENT(manager->entity_masks[entity_index], type)) {
        log_warning("L'entité %u ne possède pas de composant de type %d", entity_id, type);
        return false;
    }

    // Trouver l'index du composant
    int component_index = find_component_index(manager, entity_id, type);
    if (component_index == -1) {
        log_error("Incohérence : composant introuvable pour l'entité %u (type %d)", entity_id, type);
        return false;
    }

    // Calculer les adresses des composants
    void* components_array = manager->component_arrays[type];
    int component_count = manager->component_counts[type];

    // Si ce n'est pas le dernier composant, déplacer le dernier à la place de celui-ci
    if (component_index < component_count - 1) {
        void* src = (char*)components_array + (component_count - 1) * component_sizes[type];
        void* dest = (char*)components_array + component_index * component_sizes[type];
        memcpy(dest, src, component_sizes[type]);
    }

    // Décrémenter le nombre de composants
    manager->component_counts[type]--;

    // Mettre à jour le masque de composants de l'entité
    manager->entity_masks[entity_index] = REMOVE_COMPONENT(manager->entity_masks[entity_index], type);

    log_debug("Composant de type %d supprimé de l'entité %u", type, entity_id);
    return true;
}

// Récupère un composant d'une entité
void* entity_get_component(EntityManager* manager, EntityID entity_id, ComponentType type) {
    if (!manager || entity_id == INVALID_ENTITY_ID || type >= COMPONENT_TYPE_COUNT) return NULL;

    int entity_index = find_entity_index(manager, entity_id);
    if (entity_index == -1) {
        log_warning("Tentative d'accès à un composant d'une entité inexistante (ID: %u)", entity_id);
        return NULL;
    }

    // Vérifier si l'entité possède ce type de composant
    if (!HAS_COMPONENT(manager->entity_masks[entity_index], type)) {
        return NULL;
    }

    // Trouver l'index du composant
    int component_index = find_component_index(manager, entity_id, type);
    if (component_index == -1) {
        log_error("Incohérence : composant introuvable pour l'entité %u (type %d)", entity_id, type);
        return NULL;
    }

    // Calculer l'adresse du composant
    return (char*)manager->component_arrays[type] + component_index * component_sizes[type];
}

// Vérifie si une entité possède un composant spécifique
bool entity_has_component(EntityManager* manager, EntityID entity_id, ComponentType type) {
    if (!manager || entity_id == INVALID_ENTITY_ID || type >= COMPONENT_TYPE_COUNT) return false;

    int entity_index = find_entity_index(manager, entity_id);
    if (entity_index == -1) return false;

    return HAS_COMPONENT(manager->entity_masks[entity_index], type);
}

// Récupère le masque de composants d'une entité
ComponentMask entity_get_mask(EntityManager* manager, EntityID entity_id) {
    if (!manager || entity_id == INVALID_ENTITY_ID) return 0;

    int entity_index = find_entity_index(manager, entity_id);
    if (entity_index == -1) return 0;

    return manager->entity_masks[entity_index];
}

// Récupère toutes les entités qui possèdent un ensemble spécifique de composants
int entity_find_with_components(EntityManager* manager, ComponentMask mask, \
                               EntityID* out_entities, int max_count) {
    if (!manager || !out_entities || max_count <= 0) return 0;

    int found_count = 0;

    for (int i = 0; i < manager->entity_count && found_count < max_count; i++) {
        ComponentMask entity_mask = manager->entity_masks[i];

        // Vérifier si tous les composants requis sont présents
        if ((entity_mask & mask) == mask) {
            out_entities[found_count++] = manager->entities[i];
        }
    }

    return found_count;
}
