/**
 * physics.c
 * Implémentation du système de physique et collision
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../systems/physics.h"
#include "../utils/error_handler.h"
#include "../systems/render.h"

#define MAX_COLLISION_RESULTS 16

// Initialise le système de physique
PhysicsSystem* physics_system_init(EntityManager* entity_manager) {
    if (!check_ptr(entity_manager, LOG_LEVEL_ERROR, "Entity manager NULL passé à physics_system_init")) {
        return NULL;
    }

    PhysicsSystem* system = (PhysicsSystem*)calloc(1, sizeof(PhysicsSystem));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système de physique")) {
        return NULL;
    }

    system->entity_manager = entity_manager;
    system->max_collision_results = MAX_COLLISION_RESULTS;
    system->collision_results = (CollisionResult*)calloc(system->max_collision_results, sizeof(CollisionResult));
    
    if (!check_ptr(system->collision_results, LOG_LEVEL_ERROR, "Échec d'allocation des résultats de collision")) {
        free(system);
        return NULL;
    }
    
    system->collision_results_count = 0;
    system->debug_draw = false;
    
    log_info("Système de physique initialisé avec succès");
    return system;
}

// Libère les ressources du système de physique
void physics_system_shutdown(PhysicsSystem* system) {
    if (!system) return;
    
    if (system->collision_results) {
        free(system->collision_results);
        system->collision_results = NULL;
    }
    
    free(system);
    
    log_info("Système de physique libéré");
}

// Calcule la boîte englobante d'une entité
bool physics_get_entity_bounds(PhysicsSystem* system, EntityID entity_id, BoundingBox* box) {
    if (!system || !box || entity_id == INVALID_ENTITY_ID) return false;
    
    // Récupérer les composants nécessaires
    TransformComponent* transform = (TransformComponent*)entity_get_component(
        system->entity_manager, entity_id, COMPONENT_TRANSFORM
    );
    
    ColliderComponent* collider = (ColliderComponent*)entity_get_component(
        system->entity_manager, entity_id, COMPONENT_COLLIDER
    );
    
    // Si l'entité n'a pas les composants requis, elle n'a pas de boîte englobante
    if (!transform || !collider) return false;
    
    // Calculer la boîte englobante
    box->x = transform->x + collider->offset_x - collider->width / 2.0f;
    box->y = transform->y + collider->offset_y - collider->height / 2.0f;
    box->width = collider->width;
    box->height = collider->height;
    
    return true;
}

// Vérifie si deux boîtes englobantes se chevauchent
bool physics_check_box_collision(
    const BoundingBox* a, 
    const BoundingBox* b, 
    CollisionResult* result
) {
    if (!a || !b) return false;
    
    // Calculer les bords des rectangles
    float a_left = a->x;
    float a_right = a->x + a->width;
    float a_top = a->y;
    float a_bottom = a->y + a->height;
    
    float b_left = b->x;
    float b_right = b->x + b->width;
    float b_top = b->y;
    float b_bottom = b->y + b->height;
    
    // Tester le chevauchement (si un rectangle est à gauche, à droite, au-dessus
    // ou en dessous de l'autre, il n'y a pas de collision)
    if (a_right <= b_left || a_left >= b_right || a_bottom <= b_top || a_top >= b_bottom) {
        return false;
    }
    
    // S'il y a collision et qu'un résultat est demandé, calculer la pénétration
    if (result) {
        result->collided = true;
        
        // Calculer la pénétration dans chaque axe
        float dx1 = b_right - a_left;   // Pénétration à droite de A
        float dx2 = a_right - b_left;   // Pénétration à gauche de A
        float dy1 = b_bottom - a_top;   // Pénétration en bas de A
        float dy2 = a_bottom - b_top;   // Pénétration en haut de A
        
        // Trouver la plus petite pénétration (axe de séparation minimale)
        float dx = (dx1 < dx2) ? -dx1 : dx2;
        float dy = (dy1 < dy2) ? -dy1 : dy2;
        
        // Utiliser l'axe avec la plus petite pénétration
        if (fabsf(dx) < fabsf(dy)) {
            result->penetration.x = dx;
            result->penetration.y = 0.0f;
        } else {
            result->penetration.x = 0.0f;
            result->penetration.y = dy;
        }
    }
    
    return true;
}

// Vérifie si une entité est en collision avec d'autres entités
int physics_check_entity_collisions(
    PhysicsSystem* system, 
    EntityID entity_id, 
    CollisionResult* results, 
    int max_results
) {
    if (!system || entity_id == INVALID_ENTITY_ID || !results || max_results <= 0) {
        return 0;
    }
    
    // Récupérer la boîte englobante de l'entité
    BoundingBox entity_box;
    if (!physics_get_entity_bounds(system, entity_id, &entity_box)) {
        return 0;
    }
    
    // Récupérer le composant de collision pour connaître le masque et la couche
    ColliderComponent* entity_collider = (ColliderComponent*)entity_get_component(
        system->entity_manager, entity_id, COMPONENT_COLLIDER
    );
    
    if (!entity_collider) {
        return 0;
    }
    
    // Créer un masque de requête pour trouver les entités avec un collider
    ComponentMask query_mask = COMPONENT_BIT(COMPONENT_TRANSFORM) | COMPONENT_BIT(COMPONENT_COLLIDER);
    
    // Tableau pour stocker les entités trouvées
    EntityID entities[1024];
    int entity_count = entity_find_with_components(
        system->entity_manager, query_mask, entities, 1024
    );
    
    // Nombre de collisions trouvées
    int collision_count = 0;
    
    // Vérifier les collisions avec chaque entité
    for (int i = 0; i < entity_count && collision_count < max_results; i++) {
        EntityID other_id = entities[i];
        
        // Ne pas vérifier la collision avec soi-même
        if (other_id == entity_id) {
            continue;
        }
        
        // Récupérer le collider de l'autre entité
        ColliderComponent* other_collider = (ColliderComponent*)entity_get_component(
            system->entity_manager, other_id, COMPONENT_COLLIDER
        );
        
        if (!other_collider) {
            continue;
        }
        
        // Vérifier si les couches de collision sont compatibles
        if ((entity_collider->collision_mask & other_collider->collision_layer) == 0) {
            continue;
        }
        
        // Récupérer la boîte englobante de l'autre entité
        BoundingBox other_box;
        if (!physics_get_entity_bounds(system, other_id, &other_box)) {
            continue;
        }
        
        // Vérifier la collision
        CollisionResult result;
        if (physics_check_box_collision(&entity_box, &other_box, &result)) {
            // Stocker les informations de collision
            result.entity = other_id;
            result.type = other_collider->type;
            
            // Ajouter le résultat au tableau
            results[collision_count++] = result;
        }
    }
    
    return collision_count;
}

// Vérifie si une position est valide pour une entité (sans collision)
bool physics_is_position_valid(
    PhysicsSystem* system, 
    EntityID entity_id, 
    float x, 
    float y, 
    EntityID ignore_entity_id
) {
    if (!system || entity_id == INVALID_ENTITY_ID) {
        return false;
    }
    
    // Sauvegarder la position actuelle
    TransformComponent* transform = (TransformComponent*)entity_get_component(
        system->entity_manager, entity_id, COMPONENT_TRANSFORM
    );
    
    if (!transform) {
        return false;
    }
    
    float original_x = transform->x;
    float original_y = transform->y;
    
    // Déplacer temporairement l'entité à la nouvelle position
    transform->x = x;
    transform->y = y;
    
    // Vérifier les collisions
    CollisionResult results[16];
    int collision_count = physics_check_entity_collisions(system, entity_id, results, 16);
    
    // Restaurer la position originale
    transform->x = original_x;
    transform->y = original_y;
    
    // Si aucune collision ou seulement avec l'entité à ignorer, la position est valide
    if (collision_count == 0) {
        return true;
    }
    
    // Vérifier si toutes les collisions sont des triggers ou avec l'entité à ignorer
    for (int i = 0; i < collision_count; i++) {
        if (results[i].entity != ignore_entity_id && results[i].type != COLLISION_TRIGGER) {
            return false;
        }
    }
    
    return true;
}

// Déplace une entité en tenant compte des collisions
bool physics_move_entity(PhysicsSystem* system, EntityID entity_id, float dx, float dy) {
    if (!system || entity_id == INVALID_ENTITY_ID) {
        return false;
    }
    
    // Récupérer les composants nécessaires
    TransformComponent* transform = (TransformComponent*)entity_get_component(
        system->entity_manager, entity_id, COMPONENT_TRANSFORM
    );
    
    if (!transform) {
        return false;
    }
    
    // Calculer la nouvelle position
    float new_x = transform->x + dx;
    float new_y = transform->y + dy;
    
    // Essayer d'abord de déplacer horizontalement
    if (physics_is_position_valid(system, entity_id, new_x, transform->y, INVALID_ENTITY_ID)) {
        transform->x = new_x;
    }
    
    // Puis essayer de déplacer verticalement
    if (physics_is_position_valid(system, entity_id, transform->x, new_y, INVALID_ENTITY_ID)) {
        transform->y = new_y;
    }
    
    return true;
}

// Met à jour le système de physique
void physics_system_update(PhysicsSystem* system, float delta_time) {
    if (!system) return;
    
    // Réinitialiser les résultats de collision
    system->collision_results_count = 0;
    
    // Cette fonction pourrait être étendue pour effectuer d'autres mises à jour
    // comme la simulation de la physique, la gravité, etc.
}

// Affiche les hitboxes pour le débogage
void physics_system_debug_render(PhysicsSystem* system, RenderSystem* render_system) {
    if (!system || !render_system || !system->debug_draw) return;
    
    // Créer un masque de requête pour trouver les entités avec un collider
    ComponentMask query_mask = COMPONENT_BIT(COMPONENT_TRANSFORM) | COMPONENT_BIT(COMPONENT_COLLIDER);
    
    // Tableau pour stocker les entités trouvées
    EntityID entities[1024];
    int entity_count = entity_find_with_components(
        system->entity_manager, query_mask, entities, 1024
    );
    
    // Dessiner les boîtes englobantes de chaque entité
    for (int i = 0; i < entity_count; i++) {
        EntityID entity_id = entities[i];
        
        // Récupérer la boîte englobante
        BoundingBox box;
        if (physics_get_entity_bounds(system, entity_id, &box)) {
            // Récupérer le type de collision pour choisir la couleur
            ColliderComponent* collider = (ColliderComponent*)entity_get_component(
                system->entity_manager, entity_id, COMPONENT_COLLIDER
            );
            
            if (!collider) continue;
            
            // Choisir la couleur selon le type de collision
            uint8_t r = 0, g = 0, b = 0, a = 255;
            switch (collider->type) {
                case COLLISION_STATIC:
                    r = 255; g = 0; b = 0;  // Rouge pour les objets statiques
                    break;
                case COLLISION_DYNAMIC:
                    r = 0; g = 255; b = 0;  // Vert pour les objets dynamiques
                    break;
                case COLLISION_TRIGGER:
                    r = 0; g = 0; b = 255;  // Bleu pour les triggers
                    break;
                default:
                    r = 255; g = 255; b = 0;  // Jaune pour les autres
                    break;
            }
            
            // Dessiner la boîte englobante
            render_system_draw_rect(
                render_system,
                box.x + box.width / 2.0f,  // Centrer sur la position
                box.y + box.height / 2.0f,
                box.width,
                box.height,
                r, g, b, a,
                false  // Non rempli
            );
        }
    }
}