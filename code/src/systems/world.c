/**
 * world.c
 * Implémentation du système de gestion du monde
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "../systems/world.h"
#include "../utils/error_handler.h"
#include "../core/entity.h"
#include "../systems/physics.h"
#include "../core/resource_manager.h"

// Constantes pour le système de monde
#define DEFAULT_CHUNK_SIZE 16
#define DEFAULT_TILE_SIZE 32
#define DEFAULT_MAP_WIDTH 4
#define DEFAULT_MAP_HEIGHT 4
#define DEFAULT_PLAYER_SPEED 150.0f  // Pixels par seconde

// Constantes pour le système de temps
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24
#define DAYS_PER_SEASON 30
#define SEASONS_PER_YEAR 4

// Constantes pour le cycle jour/nuit
#define DAY_START_HOUR 6     // 6h du matin
#define NIGHT_START_HOUR 20  // 20h du soir

// Structure du système de monde étendue avec les nouveaux systèmes
typedef struct {
    EntityManager* entity_manager;    // Gestionnaire d'entités
    PhysicsSystem* physics_system;    // Système de physique
    ResourceManager* resource_manager;// Gestionnaire de ressources
    Map* current_map;                 // Carte actuelle
    TimeSystem time_system;           // Système de temps
    EntityID player_entity;           // Entité du joueur
    bool is_player_moving;            // Le joueur est-il en mouvement
    Direction player_direction;       // Direction du joueur
    float world_elapsed_time;         // Temps total écoulé
    int current_zone;                 // Zone actuelle
    
    // Texture IDs pour les différents éléments du monde
    int tileset_texture_id;           // ID de la texture du tileset
    int player_texture_id;            // ID de la texture du joueur
    int objects_texture_id;           // ID de la texture des objets
    
    // Paramètres de collision
    bool collision_map[1024][1024];   // Carte de collision temporaire (à améliorer plus tard)
} WorldSystemInternal;

// Wrapper pour le système de monde
WorldSystem* world_system_init(EntityManager* entity_manager) {
    if (!check_ptr(entity_manager, LOG_LEVEL_ERROR, "Entity manager NULL passé à world_system_init")) {
        return NULL;
    }

    WorldSystemInternal* system = (WorldSystemInternal*)calloc(1, sizeof(WorldSystemInternal));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système de monde")) {
        return NULL;
    }

    system->entity_manager = entity_manager;
    
    // Initialiser le système de physique
    system->physics_system = physics_system_init(entity_manager);
    if (!check_ptr(system->physics_system, LOG_LEVEL_ERROR, "Échec d'initialisation du système de physique")) {
        free(system);
        return NULL;
    }
    
    // Le gestionnaire de ressources sera initialisé plus tard, lors de l'appel à world_system_init_renderer
    system->resource_manager = NULL;
    
    system->current_map = NULL;
    system->player_entity = INVALID_ENTITY_ID;
    system->is_player_moving = false;
    system->player_direction = DIRECTION_DOWN;
    system->world_elapsed_time = 0.0f;
    system->current_zone = ZONE_FARM;

    // Initialiser le système de temps
    system->time_system.day = 1;
    system->time_system.season = SEASON_SPRING;
    system->time_system.year = 1;
    system->time_system.hour = 6;  // Commencer à 6h du matin
    system->time_system.minute = 0;
    system->time_system.day_time = 0.0f;  // Début de journée
    system->time_system.is_night = false;

    // Initialiser les IDs de texture à -1 (non chargé)
    system->tileset_texture_id = -1;
    system->player_texture_id = -1;
    system->objects_texture_id = -1;

    log_info("Système de monde initialisé avec succès");
    return (WorldSystem*)system;
}

// Initialise le gestionnaire de ressources avec le renderer
void world_system_init_renderer(WorldSystem* _system, SDL_Renderer* renderer) {
    WorldSystemInternal* system = (WorldSystemInternal*)_system;
    if (!system || !renderer) return;
    
    // Créer le gestionnaire de ressources
    system->resource_manager = resource_manager_init(renderer);
    if (!check_ptr(system->resource_manager, LOG_LEVEL_ERROR, "Échec d'initialisation du gestionnaire de ressources")) {
        return;
    }
    
    log_info("Gestionnaire de ressources du monde initialisé avec succès");
}

// Libère les ressources du système de monde
void world_system_shutdown(WorldSystem* _system) {
    WorldSystemInternal* system = (WorldSystemInternal*)_system;
    if (!system) return;

    // Libérer la carte actuelle
    if (system->current_map) {
        // Libérer chaque chunk
        if (system->current_map->chunks) {
            for (int i = 0; i < system->current_map->chunks_x * system->current_map->chunks_y; i++) {
                if (system->current_map->chunks[i]) {
                    free(system->current_map->chunks[i]);
                }
            }
            free(system->current_map->chunks);
        }
        free(system->current_map);
    }
    
    // Libérer le système de physique
    if (system->physics_system) {
        physics_system_shutdown(system->physics_system);
    }
    
    // Libérer le gestionnaire de ressources
    if (system->resource_manager) {
        resource_manager_shutdown(system->resource_manager);
    }

    // Libérer le système lui-même
    free(system);

    log_info("Système de monde libéré");
}
    