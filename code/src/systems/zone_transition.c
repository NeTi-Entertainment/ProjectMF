/**
 * zone_transition.c
 * Implémentation du système de gestion des transitions entre zones
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../systems/zone_transition.h"
#include "../utils/error_handler.h"
#include "../systems/tiled_parser.h"

#define INITIAL_TRANSITION_CAPACITY 16
#define DEFAULT_TRANSITION_DURATION 1.0f

// Initialise le système de transition
ZoneTransitionSystem* zone_transition_init(void) {
    ZoneTransitionSystem* system = (ZoneTransitionSystem*)calloc(1, sizeof(ZoneTransitionSystem));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système de transition")) {
        return NULL;
    }
    
    system->transition_capacity = INITIAL_TRANSITION_CAPACITY;
    system->transitions = (ZoneTransition*)calloc(system->transition_capacity, sizeof(ZoneTransition));
    if (!check_ptr(system->transitions, LOG_LEVEL_ERROR, "Échec d'allocation du tableau de transitions")) {
        free(system);
        return NULL;
    }
    
    system->transition_count = 0;
    system->active_transition = NULL;
    system->transition_progress = 0.0f;
    system->is_transitioning = false;
    
    log_info("Système de transition initialisé avec succès");
    return system;
}

// Libère les ressources du système de transition
void zone_transition_shutdown(ZoneTransitionSystem* system) {
    if (!system) return;
    
    if (system->transitions) {
        // Libérer les noms de fichiers
        for (int i = 0; i < system->transition_count; i++) {
            if (system->transitions[i].target_map) {
                free(system->transitions[i].target_map);
            }
        }
        
        free(system->transitions);
        system->transitions = NULL;
    }
    
    free(system);
    
    log_info("Système de transition libéré");
}

// Ajoute une transition
int zone_transition_add(
    ZoneTransitionSystem* system,
    ZoneType source_zone,
    ZoneType target_zone,
    float source_x, float source_y,
    float target_x, float target_y,
    const char* target_map
) {
    if (!system) return -1;
    
    // Vérifier si nous devons agrandir le tableau
    if (system->transition_count >= system->transition_capacity) {
        int new_capacity = system->transition_capacity * 2;
        ZoneTransition* new_transitions = (ZoneTransition*)realloc(
            system->transitions, new_capacity * sizeof(ZoneTransition)
        );
        
        if (!check_ptr(new_transitions, LOG_LEVEL_ERROR, "Échec de réallocation du tableau de transitions")) {
            return -1;
        }
        
        system->transitions = new_transitions;
        system->transition_capacity = new_capacity;
    }
    
    // Ajouter la nouvelle transition
    int transition_id = system->transition_count;
    ZoneTransition* transition = &system->transitions[transition_id];
    
    transition->id = transition_id;
    transition->source_zone = source_zone;
    transition->target_zone = target_zone;
    transition->source_x = source_x;
    transition->source_y = source_y;
    transition->target_x = target_x;
    transition->target_y = target_y;
    
    // Copier le chemin de la carte si fourni
    if (target_map) {
        transition->target_map = strdup(target_map);
    } else {
        transition->target_map = NULL;
    }
    
    transition->is_active = true;
    
    system->transition_count++;
    
    log_debug("Transition ajoutée: %d -> %d (%f,%f -> %f,%f)", 
             source_zone, target_zone,
             source_x, source_y,
             target_x, target_y);
    
    return transition_id;
}

// Active une transition
bool zone_transition_activate(ZoneTransitionSystem* system, int transition_id) {
    if (!system || transition_id < 0 || transition_id >= system->transition_count) {
        return false;
    }
    
    system->transitions[transition_id].is_active = true;
    return true;
}

// Désactive une transition
bool zone_transition_deactivate(ZoneTransitionSystem* system, int transition_id) {
    if (!system || transition_id < 0 || transition_id >= system->transition_count) {
        return false;
    }
    
    system->transitions[transition_id].is_active = false;
    return true;
}

// Trouve une transition à partir d'une position
int zone_transition_find_at_position(
    ZoneTransitionSystem* system,
    ZoneType zone,
    float x, float y,
    float radius
) {
    if (!system) return -1;
    
    // Parcourir toutes les transitions
    for (int i = 0; i < system->transition_count; i++) {
        ZoneTransition* transition = &system->transitions[i];
        
        // Vérifier si la transition est active et dans la bonne zone
        if (!transition->is_active || transition->source_zone != zone) {
            continue;
        }
        
        // Calculer la distance
        float dx = transition->source_x - x;
        float dy = transition->source_y - y;
        float distance = sqrtf(dx * dx + dy * dy);
        
        // Si la distance est inférieure au rayon, on a trouvé une transition
        if (distance <= radius) {
            return transition->id;
        }
    }
    
    return -1;
}

// Déclenche une transition
bool zone_transition_trigger(
    ZoneTransitionSystem* system,
    WorldSystem* world_system,
    int transition_id
) {
    if (!system || !world_system || transition_id < 0 || transition_id >= system->transition_count) {
        return false;
    }
    
    // Vérifier si une transition est déjà en cours
    if (system->is_transitioning) {
        return false;
    }
    
    // Récupérer la transition
    ZoneTransition* transition = &system->transitions[transition_id];
    
    // Vérifier si la transition est active
    if (!transition->is_active) {
        return false;
    }
    
    // Si nous n'avons pas besoin de changer de carte
    if (transition->source_zone == transition->target_zone) {
        // Téléporter directement le joueur
        world_system_teleport_player(world_system, transition->target_x, transition->target_y);
        return true;
    }
    
    // Sinon, déclencher la transition
    system->active_transition = transition;
    system->transition_progress = 0.0f;
    system->is_transitioning = true;
    
    log_info("Transition déclenchée: %d -> %d", 
            transition->source_zone, transition->target_zone);
    
    return true;
}

// Met à jour le système de transition
void zone_transition_update(
    ZoneTransitionSystem* system,
    WorldSystem* world_system,
    ResourceManager* resource_manager,
    float delta_time
) {
    if (!system || !world_system || !resource_manager) return;
    
    // Si aucune transition n'est en cours, rien à faire
    if (!system->is_transitioning || !system->active_transition) {
        return;
    }
    
    // Mettre à jour la progression
    system->transition_progress += delta_time / DEFAULT_TRANSITION_DURATION;
    
    // Si la transition est terminée
    if (system->transition_progress >= 1.0f) {
        ZoneTransition* transition = system->active_transition;
        
        // Changer de zone
        world_system_change_zone(world_system, transition->target_zone);
        
        // Si nous avons une carte cible
        if (transition->target_map) {
            // Charger la nouvelle carte (à implémenter)
            // Pour l'instant, on va simplement téléporter le joueur
        }
        
        // Téléporter le joueur
        world_system_teleport_player(world_system, transition->target_x, transition->target_y);
        
        // Réinitialiser l'état
        system->active_transition = NULL;
        system->transition_progress = 0.0f;
        system->is_transitioning = false;
        
        log_info("Transition terminée");
    }
}

// Rend le système de transition
void zone_transition_render(
    ZoneTransitionSystem* system,
    RenderSystem* render_system
) {
    if (!system || !render_system) return;
    
    // Si aucune transition n'est en cours, rien à rendre
    if (!system->is_transitioning) {
        return;
    }
    
    // Calculer l'opacité de l'effet de fondu
    uint8_t alpha = (uint8_t)(255.0f * system->transition_progress);
    
    // Dessiner un rectangle noir qui couvre tout l'écran
    render_system_draw_rect(
        render_system,
        render_system->screen_width / 2.0f,
        render_system->screen_height / 2.0f,
        (float)render_system->screen_width,
        (float)render_system->screen_height,
        0, 0, 0, alpha,
        true
    );
}

// Charge les transitions depuis une carte Tiled
int zone_transition_load_from_map(
    ZoneTransitionSystem* system,
    TiledMap* tiled_map,
    ZoneType zone_type
) {
    if (!system || !tiled_map) return -1;
    
    int loaded_count = 0;
    
    // Parcourir chaque couche d'objets
    for (int group_index = 0; group_index < tiled_map->object_group_count; group_index++) {
        TiledObjectGroup* group = tiled_map->object_groups[group_index];
        if (!group) continue;
        
        // Vérifier si c'est une couche de transitions
        if (strstr(group->name, "transition") == NULL && 
            strstr(group->name, "Transition") == NULL) {
            continue;
        }
        
        // Parcourir chaque objet
        for (int obj_index = 0; obj_index < group->object_count; obj_index++) {
            TiledObject* object = group->objects[obj_index];
            if (!object) continue;
            
            // Vérifier si c'est un objet de transition
            if (strcmp(object->type, "transition") != 0) {
                continue;
            }
            
            // Extraire les propriétés
            ZoneType target_zone = zone_type; // Par défaut, même zone
            float target_x = 0.0f;
            float target_y = 0.0f;
            const char* target_map = NULL;
            
            // Parcourir les propriétés
            for (int prop_index = 0; prop_index < object->property_count; prop_index++) {
                TiledProperty* property = object->properties[prop_index];
                if (!property || !property->name) continue;
                
                if (strcmp(property->name, "target_zone") == 0) {
                    if (strcmp(property->value, "farm") == 0) {
                        target_zone = ZONE_FARM;
                    } else if (strcmp(property->value, "village") == 0) {
                        target_zone = ZONE_VILLAGE;
                    } else if (strcmp(property->value, "forest") == 0) {
                        target_zone = ZONE_FOREST;
                    } else if (strcmp(property->value, "mine") == 0) {
                        target_zone = ZONE_MINE;
                    } else if (strcmp(property->value, "beach") == 0) {
                        target_zone = ZONE_BEACH;
                    }
                } else if (strcmp(property->name, "target_x") == 0) {
                    target_x = atof(property->value);
                } else if (strcmp(property->name, "target_y") == 0) {
                    target_y = atof(property->value);
                } else if (strcmp(property->name, "target_map") == 0) {
                    target_map = property->value;
                }
            }
            
            // Ajouter la transition
            int transition_id = zone_transition_add(
                system,
                zone_type,
                target_zone,
                object->x + object->width / 2.0f,
                object->y + object->height / 2.0f,
                target_x,
                target_y,
                target_map
            );
            
            if (transition_id >= 0) {
                loaded_count++;
            }
        }
    }
    
    log_info("%d transitions chargées depuis la carte", loaded_count);
    return loaded_count;
}

// Rend le système de transition prêt pour une nouvelle zone
void zone_transition_prepare_zone(
    ZoneTransitionSystem* system,
    ZoneType zone_type
) {
    if (!system) return;
    
    // Pour l'instant, rien de spécial à faire
    // Cette fonction pourrait être utilisée pour préparer des transitions spécifiques
    // ou pour réinitialiser l'état lors d'un changement de zone
    
    log_debug("Système de transition préparé pour la zone %d", zone_type);
}
