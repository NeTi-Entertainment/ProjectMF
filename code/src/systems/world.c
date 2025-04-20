/**
 * Ajout des nouvelles fonctionnalités pour le support de Tiled et des transitions de zone
 * À ajouter à world.c
 */

// Nouvelles inclusions à ajouter en haut du fichier
#include "../utils/tiled_parser.h"

// ===== Fonctions à ajouter à world.c =====

// Charge une carte au format Tiled (JSON)
bool world_system_load_tiled_map(WorldSystem* system, const char* filename) {
    if (!system || !filename) return false;
    
    log_info("Chargement de la carte Tiled: %s", filename);
    
    // Charger la carte Tiled
    TiledMap* tiled_map = tiled_load_map(filename);
    if (!tiled_map) {
        log_error("Échec de chargement de la carte Tiled: %s", filename);
        return false;
    }
    
    // Mettre à jour la zone actuelle
    system->current_zone = tiled_map->zone_type;
    
    // Convertir la carte Tiled en carte du jeu
    bool result = tiled_convert_to_game_map(tiled_map, system);
    
    // Charger les tilesets dans le système de rendu
    if (result) {
        // Cette fonction serait appelée par le système de jeu qui a accès au système de rendu
        // Par exemple dans une fonction world_system_load_map_resources
        
        // Enregistrer le fichier de carte
        if (system->current_map) {
            if (system->current_map->map_file) {
                free(system->current_map->map_file);
            }
            system->current_map->map_file = strdup(filename);
        }
    }
    
    // Libérer la carte Tiled
    tiled_free_map(tiled_map);
    
    return result;
}

// Ajoute un point de transition à la carte actuelle
int world_system_add_transition_point(WorldSystem* system, 
                                     float x, float y, 
                                     float width, float height,
                                     ZoneType target_zone, 
                                     float target_x, float target_y,
                                     const char* target_map) {
    if (!system || !system->current_map) return -1;
    
    // Redimensionner le tableau des points de transition
    int new_id = system->current_map->transition_count;
    TransitionPoint* new_transitions = (TransitionPoint*)realloc(
        system->current_map->transitions,
        (new_id + 1) * sizeof(TransitionPoint)
    );
    
    if (!new_transitions) {
        log_error("Échec d'allocation pour le point de transition");
        return -1;
    }
    
    system->current_map->transitions = new_transitions;
    
    // Initialiser le nouveau point de transition
    TransitionPoint* point = &system->current_map->transitions[new_id];
    point->id = new_id;
    point->x = x;
    point->y = y;
    point->width = width;
    point->height = height;
    point->target_zone = target_zone;
    point->target_x = target_x;
    point->target_y = target_y;
    
    if (target_map) {
        point->target_map = strdup(target_map);
    } else {
        point->target_map = NULL;
    }
    
    // Incrémenter le compteur
    system->current_map->transition_count++;
    
    // Créer également une entité pour ce point de transition
    EntityID entity_id = entity_create(system->entity_manager);
    if (entity_id != INVALID_ENTITY_ID) {
        // Ajouter les composants nécessaires
        TransformComponent* transform = create_transform_component(entity_id, x, y);
        if (transform) {
            entity_add_component(system->entity_manager, entity_id, transform);
        }
        
        // Ajouter un collider déclenché par le joueur
        ColliderComponent* collider = create_collider_component(
            entity_id, width, height, true
        );
        if (collider) {
            entity_add_component(system->entity_manager, entity_id, collider);
        }
        
        // Ajouter un composant Interactable pour la transition
        int transition_type = 0; // Type de base pour les transitions
        float interaction_radius = 0.0f; // Pas besoin de rayon pour les colliders
        
        InteractableComponent* interactable = create_interactable_component(
            entity_id, transition_type, interaction_radius
        );
        if (interactable) {
            entity_add_component(system->entity_manager, entity_id, interactable);
        }
    }
    
    log_info("Point de transition ajouté (ID: %d) vers la zone %d", new_id, target_zone);
    return new_id;
}

// Supprime un point de transition
bool world_system_remove_transition_point(WorldSystem* system, int id) {
    if (!system || !system->current_map || id < 0 || id >= system->current_map->transition_count) {
        return false;
    }
    
    // Libérer le chemin de carte cible si nécessaire
    if (system->current_map->transitions[id].target_map) {
        free(system->current_map->transitions[id].target_map);
    }
    
    // Déplacer les points suivants pour maintenir un tableau compact
    for (int i = id; i < system->current_map->transition_count - 1; i++) {
        system->current_map->transitions[i] = system->current_map->transitions[i + 1];
        // Mettre à jour l'ID
        system->current_map->transitions[i].id = i;
    }
    
    // Décrémenter le compteur
    system->current_map->transition_count--;
    
    // Réduire la taille du tableau si nécessaire
    if (system->current_map->transition_count > 0) {
        TransitionPoint* new_transitions = (TransitionPoint*)realloc(
            system->current_map->transitions,
            system->current_map->transition_count * sizeof(TransitionPoint)
        );
        
        if (new_transitions || system->current_map->transition_count == 0) {
            system->current_map->transitions = new_transitions;
        }
    } else {
        free(system->current_map->transitions);
        system->current_map->transitions = NULL;
    }
    
    log_info("Point de transition supprimé (ID: %d)", id);
    return true;
}

// Vérifie si le joueur est entré dans une zone de transition
int world_system_check_transition(WorldSystem* system) {
    if (!system || !system->current_map || system->player_entity == INVALID_ENTITY_ID) {
        return -1;
    }
    
    // Obtenir la position du joueur
    TransformComponent* player_transform = (TransformComponent*)entity_get_component(
        system->entity_manager, system->player_entity, COMPONENT_TRANSFORM
    );
    
    if (!player_transform) {
        return -1;
    }
    
    float player_x = player_transform->x;
    float player_y = player_transform->y;
    
    // Vérifier tous les points de transition
    for (int i = 0; i < system->current_map->transition_count; i++) {
        TransitionPoint* point = &system->current_map->transitions[i];
        
        // Calculer les limites de la zone de transition
        float min_x = point->x - point->width / 2.0f;
        float max_x = point->x + point->width / 2.0f;
        float min_y = point->y - point->height / 2.0f;
        float max_y = point->y + point->height / 2.0f;
        
        // Vérifier si le joueur est dans la zone
        if (player_x >= min_x && player_x <= max_x && player_y >= min_y && player_y <= max_y) {
            return i;
        }
    }
    
    return -1;
}

// Ajoute un objet interactif au monde
int world_system_add_interactive_object(WorldSystem* system, 
                                       EntityID entity_id,
                                       int interaction_type,
                                       float x, float y) {
    if (!system || entity_id == INVALID_ENTITY_ID) {
        return -1;
    }
    
    // Redimensionner le tableau des objets interactifs
    int new_id = system->interactive_object_count;
    InteractiveObject* new_objects = (InteractiveObject*)realloc(
        system->interactive_objects,
        (new_id + 1) * sizeof(InteractiveObject)
    );
    
    if (!new_objects) {
        log_error("Échec d'allocation pour l'objet interactif");
        return -1;
    }
    
    system->interactive_objects = new_objects;
    
    // Initialiser le nouvel objet interactif
    InteractiveObject* object = &system->interactive_objects[new_id];
    object->id = new_id;
    object->entity_id = entity_id;
    object->interaction_type = interaction_type;
    object->x = x;
    object->y = y;
    object->is_active = true;
    
    // Incrémenter le compteur
    system->interactive_object_count++;
    
    log_info("Objet interactif ajouté (ID: %d, Entité: %u)", new_id, entity_id);
    return new_id;
}

// Supprime un objet interactif
bool world_system_remove_interactive_object(WorldSystem* system, int id) {
    if (!system || id < 0 || id >= system->interactive_object_count) {
        return false;
    }
    
    // Déplacer les objets suivants pour maintenir un tableau compact
    for (int i = id; i < system->interactive_object_count - 1; i++) {
        system->interactive_objects[i] = system->interactive_objects[i + 1];
        // Mettre à jour l'ID
        system->interactive_objects[i].id = i;
    }
    
    // Décrémenter le compteur
    system->interactive_object_count--;
    
    // Réduire la taille du tableau si nécessaire
    if (system->interactive_object_count > 0) {
        InteractiveObject* new_objects = (InteractiveObject*)realloc(
            system->interactive_objects,
            system->interactive_object_count * sizeof(InteractiveObject)
        );
        
        if (new_objects || system->interactive_object_count == 0) {
            system->interactive_objects = new_objects;
        }
    } else {
        free(system->interactive_objects);
        system->interactive_objects = NULL;
    }
    
    log_info("Objet interactif supprimé (ID: %d)", id);
    return true;
}

// Trouve l'objet interactif le plus proche du joueur
int world_system_find_nearest_interactive_object(WorldSystem* system, float max_distance) {
    if (!system || system->player_entity == INVALID_ENTITY_ID || system->interactive_object_count == 0) {
        return -1;
    }
    
    // Obtenir la position du joueur
    TransformComponent* player_transform = (TransformComponent*)entity_get_component(
        system->entity_manager, system->player_entity, COMPONENT_TRANSFORM
    );
    
    if (!player_transform) {
        return -1;
    }
    
    float player_x = player_transform->x;
    float player_y = player_transform->y;
    
    // Trouver l'objet le plus proche
    int nearest_id = -1;
    float nearest_distance = max_distance;
    
    for (int i = 0; i < system->interactive_object_count; i++) {
        InteractiveObject* object = &system->interactive_objects[i];
        
        if (!object->is_active) continue;
        
        // Calculer la distance
        float dx = object->x - player_x;
        float dy = object->y - player_y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        if (distance < nearest_distance) {
            nearest_distance = distance;
            nearest_id = i;
        }
    }
    
    return nearest_id;
}

// ===== Modifications à apporter aux fonctions existantes =====

// Modifier world_system_init pour initialiser les nouveaux champs
// Dans la fonction world_system_init, ajouter après l'initialisation des autres champs:
/*
    // Initialiser les points de transition
    system->current_map->transitions = NULL;
    system->current_map->transition_count = 0;
    
    // Initialiser les objets interactifs
    system->interactive_objects = NULL;
    system->interactive_object_count = 0;
*/

// Modifier world_system_shutdown pour libérer les nouvelles ressources
// Dans la fonction world_system_shutdown, ajouter avant de libérer la carte:
/*
    // Libérer les points de transition
    if (system->current_map && system->current_map->transitions) {
        for (int i = 0; i < system->current_map->transition_count; i++) {
            if (system->current_map->transitions[i].target_map) {
                free(system->current_map->transitions[i].target_map);
            }
        }
        free(system->current_map->transitions);
    }
    
    // Libérer les objets interactifs
    if (system->interactive_objects) {
        free(system->interactive_objects);
    }
*/

// Modifier world_system_create_map pour initialiser les nouveaux champs de la carte
// Dans la fonction world_system_create_map, ajouter après l'initialisation des autres champs:
/*
    // Initialiser les points de transition
    system->current_map->transitions = NULL;
    system->current_map->transition_count = 0;
    
    // Initialiser le fichier de carte
    system->current_map->map_file = NULL;
*/

// Modifier world_system_update pour vérifier les transitions
// Dans la fonction world_system_update, ajouter avant la fin de la fonction:
/*
    // Vérifier si le joueur a déclenché une transition
    int transition_id = world_system_check_transition(system);
    if (transition_id >= 0 && transition_id < system->current_map->transition_count) {
        TransitionPoint* point = &system->current_map->transitions[transition_id];
        
        // Changer de zone
        if (world_system_change_zone(system, point->target_zone)) {
            // Charger la nouvelle carte si spécifiée
            if (point->target_map) {
                world_system_load_tiled_map(system, point->target_map);
            }
            
            // Téléporter le joueur à la position cible
            world_system_teleport_player(system, point->target_x, point->target_y);
        }
    }
*/