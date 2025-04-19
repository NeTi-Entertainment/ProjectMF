/**
 * physics.h
 * Système de physique et collision
 */

#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdbool.h>
#include "../core/entity.h"
#include "../systems/entity_manager.h"

// Représente une position dans l'espace
typedef struct {
    float x, y;
} Vector2;

// Représente une zone de collision rectangulaire
typedef struct {
    float x, y;
    float width, height;
} BoundingBox;

// Représente un résultat de collision
typedef struct {
    bool collided;           // Y a-t-il eu collision
    Vector2 penetration;     // Vecteur de pénétration (combien et dans quelle direction)
    EntityID entity;         // Entité avec laquelle il y a eu collision
    CollisionType type;      // Type de collision
} CollisionResult;

// Système de physique
typedef struct {
    EntityManager* entity_manager;         // Gestionnaire d'entités
    CollisionResult* collision_results;    // Résultats de collision
    int max_collision_results;             // Nombre maximum de résultats de collision
    int collision_results_count;           // Nombre actuel de résultats de collision
    bool debug_draw;                       // Afficher les boîtes de collision
} PhysicsSystem;

/**
 * Initialise le système de physique
 * @param entity_manager Gestionnaire d'entités
 * @return Pointeur vers le système de physique ou NULL en cas d'erreur
 */
PhysicsSystem* physics_system_init(EntityManager* entity_manager);

/**
 * Libère les ressources du système de physique
 * @param system Système de physique
 */
void physics_system_shutdown(PhysicsSystem* system);

/**
 * Met à jour le système de physique
 * @param system Système de physique
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void physics_system_update(PhysicsSystem* system, float delta_time);

/**
 * Affiche les hitboxes pour le débogage
 * @param system Système de physique
 * @param render_system Système de rendu
 */
void physics_system_debug_render(PhysicsSystem* system, RenderSystem* render_system);

/**
 * Vérifie si une entité est en collision avec d'autres entités
 * @param system Système de physique
 * @param entity_id ID de l'entité à vérifier
 * @param results Tableau pour stocker les résultats de collision
 * @param max_results Taille maximum du tableau de résultats
 * @return Nombre de collisions trouvées
 */
int physics_check_entity_collisions(
    PhysicsSystem* system, 
    EntityID entity_id, 
    CollisionResult* results, 
    int max_results
);

/**
 * Vérifie si une position est valide pour une entité (sans collision)
 * @param system Système de physique
 * @param entity_id ID de l'entité
 * @param x Position X à vérifier
 * @param y Position Y à vérifier
 * @param ignore_entity_id ID d'une entité à ignorer lors des vérifications (ou INVALID_ENTITY_ID)
 * @return true si la position est valide, false sinon
 */
bool physics_is_position_valid(
    PhysicsSystem* system, 
    EntityID entity_id, 
    float x, 
    float y, 
    EntityID ignore_entity_id
);

/**
 * Déplace une entité en tenant compte des collisions
 * @param system Système de physique
 * @param entity_id ID de l'entité
 * @param dx Déplacement en X
 * @param dy Déplacement en Y
 * @return true si le déplacement a été effectué, false sinon
 */
bool physics_move_entity(PhysicsSystem* system, EntityID entity_id, float dx, float dy);

/**
 * Calcule la boîte englobante d'une entité
 * @param system Système de physique
 * @param entity_id ID de l'entité
 * @param box Pointeur vers la boîte englobante à remplir
 * @return true si la boîte a été calculée, false sinon
 */
bool physics_get_entity_bounds(PhysicsSystem* system, EntityID entity_id, BoundingBox* box);

/**
 * Vérifie si deux boîtes englobantes se chevauchent
 * @param a Première boîte
 * @param b Seconde boîte
 * @param result Résultat de la collision (peut être NULL)
 * @return true s'il y a collision, false sinon
 */
bool physics_check_box_collision(
    const BoundingBox* a, 
    const BoundingBox* b, 
    CollisionResult* result
);

#endif /* PHYSICS_H */