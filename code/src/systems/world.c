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

// Initialise le système de monde
WorldSystem* world_system_init(EntityManager* entity_manager) {
    if (!check_ptr(entity_manager, LOG_LEVEL_ERROR, "Entity manager NULL passé à world_system_init")) {
        return NULL;
    }

    WorldSystem* system = (WorldSystem*)calloc(1, sizeof(WorldSystem));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système de monde")) {
        return NULL;
    }

    system->entity_manager = entity_manager;
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
    return system;
}

// Libère les ressources du système de monde
void world_system_shutdown(WorldSystem* system) {
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

    // Libérer le système lui-même
    free(system);

    log_info("Système de monde libéré");
}

// Crée une tuile avec des valeurs par défaut
static Tile create_default_tile(TileType type) {
    Tile tile;
    tile.type = type;
    tile.variant = 0;
    
    // Définir les propriétés en fonction du type
    switch (type) {
        case TILE_GRASS:
            tile.is_walkable = true;
            tile.is_tillable = true;
            tile.is_watered = false;
            tile.is_tilled = false;
            break;
            
        case TILE_DIRT:
            tile.is_walkable = true;
            tile.is_tillable = true;
            tile.is_watered = false;
            tile.is_tilled = true; // Déjà labourée
            break;
            
        case TILE_WATER:
            tile.is_walkable = false;
            tile.is_tillable = false;
            tile.is_watered = true;
            tile.is_tilled = false;
            break;
            
        case TILE_STONE:
        case TILE_BUILDING:
            tile.is_walkable = false;
            tile.is_tillable = false;
            tile.is_watered = false;
            tile.is_tilled = false;
            break;
            
        case TILE_SAND:
            tile.is_walkable = true;
            tile.is_tillable = false;
            tile.is_watered = false;
            tile.is_tilled = false;
            break;
            
        case TILE_NONE:
        default:
            tile.is_walkable = true;
            tile.is_tillable = false;
            tile.is_watered = false;
            tile.is_tilled = false;
            break;
    }
    
    return tile;
}

// Crée une nouvelle carte vide
bool world_system_create_map(WorldSystem* system, int width, int height) {
    if (!system) return false;
    
    // Libérer la carte actuelle si elle existe
    if (system->current_map) {
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
    
    // Créer une nouvelle carte
    system->current_map = (Map*)calloc(1, sizeof(Map));
    if (!check_ptr(system->current_map, LOG_LEVEL_ERROR, "Échec d'allocation de la carte")) {
        return false;
    }
    
    system->current_map->chunks_x = width;
    system->current_map->chunks_y = height;
    system->current_map->chunk_size = DEFAULT_CHUNK_SIZE;
    system->current_map->tile_size = DEFAULT_TILE_SIZE;
    system->current_map->current_zone = ZONE_FARM;
    
    // Allouer les chunks
    system->current_map->chunks = (Chunk**)calloc(
        width * height, sizeof(Chunk*)
    );
    if (!check_ptr(system->current_map->chunks, LOG_LEVEL_ERROR, "Échec d'allocation des chunks")) {
        free(system->current_map);
        system->current_map = NULL;
        return false;
    }
    
    // Initialiser chaque chunk
    for (int i = 0; i < width * height; i++) {
        int chunk_x = i % width;
        int chunk_y = i / width;
        
        Chunk* chunk = (Chunk*)calloc(1, sizeof(Chunk));
        if (!check_ptr(chunk, LOG_LEVEL_ERROR, "Échec d'allocation du chunk (%d, %d)", chunk_x, chunk_y)) {
            // Nettoyer les chunks déjà alloués
            for (int j = 0; j < i; j++) {
                free(system->current_map->chunks[j]);
            }
            free(system->current_map->chunks);
            free(system->current_map);
            system->current_map = NULL;
            return false;
        }
        
        chunk->chunk_x = chunk_x;
        chunk->chunk_y = chunk_y;
        chunk->is_loaded = true;
        chunk->is_dirty = true;
        
        // Initialiser toutes les tuiles comme de l'herbe
        for (int x = 0; x < DEFAULT_CHUNK_SIZE; x++) {
            for (int y = 0; y < DEFAULT_CHUNK_SIZE; y++) {
                // Couche de sol: herbe par défaut
                chunk->tiles[x][y][LAYER_GROUND] = create_default_tile(TILE_GRASS);
                
                // Autres couches: vide par défaut
                for (int layer = LAYER_GROUND + 1; layer < LAYER_COUNT; layer++) {
                    chunk->tiles[x][y][layer] = create_default_tile(TILE_NONE);
                }
            }
        }
        
        system->current_map->chunks[i] = chunk;
    }
    
    // Initialiser la carte de collision
    memset(system->collision_map, 0, sizeof(system->collision_map));
    
    log_info("Nouvelle carte créée (%d x %d chunks)", width, height);
    return true;
}

// Initialise une nouvelle partie
bool world_system_init_new_game(WorldSystem* system) {
    if (!system) return false;
    
    // Créer une carte de taille par défaut
    if (!world_system_create_map(system, DEFAULT_MAP_WIDTH, DEFAULT_MAP_HEIGHT)) {
        log_error("Échec de création de la carte pour la nouvelle partie");
        return false;
    }
    
    // Créer l'entité du joueur
    EntityID player_id = entity_create(system->entity_manager);
    if (player_id == INVALID_ENTITY_ID) {
        log_error("Échec de création de l'entité du joueur");
        return false;
    }
    
    // Ajouter les composants nécessaires au joueur
    TransformComponent* transform = create_transform_component(player_id, 0.0f, 0.0f);
    if (!transform || !entity_add_component(system->entity_manager, player_id, transform)) {
        log_error("Échec d'ajout du composant Transform au joueur");
        entity_destroy(system->entity_manager, player_id);
        free(transform);
        return false;
    }
    
    PlayerComponent* player = create_player_component(player_id, DEFAULT_PLAYER_SPEED);
    if (!player || !entity_add_component(system->entity_manager, player_id, player)) {
        log_error("Échec d'ajout du composant Player au joueur");
        entity_destroy(system->entity_manager, player_id);
        free(player);
        return false;
    }
    
    ColliderComponent* collider = create_collider_component(player_id, 24.0f, 24.0f, false);
    if (!collider || !entity_add_component(system->entity_manager, player_id, collider)) {
        log_error("Échec d'ajout du composant Collider au joueur");
        entity_destroy(system->entity_manager, player_id);
        free(collider);
        return false;
    }
    
    // Positionner le joueur au centre de la carte
    float center_x = system->current_map->chunks_x * system->current_map->chunk_size * 
                    system->current_map->tile_size / 2.0f;
    float center_y = system->current_map->chunks_y * system->current_map->chunk_size * 
                    system->current_map->tile_size / 2.0f;
    
    transform->x = center_x;
    transform->y = center_y;
    
    // Enregistrer l'ID du joueur
    system->player_entity = player_id;
    
    log_info("Nouvelle partie initialisée avec succès");
    return true;
}

// Met à jour le système de temps
static void update_time_system(TimeSystem* time, float delta_time) {
    if (!time) return;
    
    // Un jour complet dure 20 minutes réelles (1200 secondes)
    // Donc 1 minute de jeu = 1200 / (24 * 60) = 0.833 secondes réelles
    const float SECONDS_PER_GAME_MINUTE = 0.833f;
    
    // Calculer le nombre de minutes à ajouter
    float minutes_to_add = delta_time / SECONDS_PER_GAME_MINUTE;
    int whole_minutes = (int)minutes_to_add;
    
    // Ajouter les minutes
    time->minute += whole_minutes;
    
    // Gérer le débordement des minutes
    while (time->minute >= MINUTES_PER_HOUR) {
        time->minute -= MINUTES_PER_HOUR;
        time->hour++;
        
        // Gérer le débordement des heures
        if (time->hour >= HOURS_PER_DAY) {
            time->hour = 0;
            time->day++;
            
            // Gérer le débordement des jours
            if (time->day > DAYS_PER_SEASON) {
                time->day = 1;
                time->season = (time->season + 1) % SEASONS_PER_YEAR;
                
                // Si on est passé à une nouvelle année
                if (time->season == SEASON_SPRING) {
                    time->year++;
                }
            }
        }
    }
    
    // Calculer le temps de la journée (0.0 = minuit, 0.5 = midi, 1.0 = minuit suivant)
    time->day_time = (float)(time->hour * MINUTES_PER_HOUR + time->minute) / 
                    (float)(HOURS_PER_DAY * MINUTES_PER_HOUR);
    
    // Déterminer s'il fait jour ou nuit
    time->is_night = (time->hour < DAY_START_HOUR || time->hour >= NIGHT_START_HOUR);
}

// Obtient la position du joueur
static bool get_player_position(WorldSystem* system, float* x, float* y) {
    if (!system || !x || !y || system->player_entity == INVALID_ENTITY_ID) return false;
    
    TransformComponent* transform = (TransformComponent*)entity_get_component(
        system->entity_manager, system->player_entity, COMPONENT_TRANSFORM
    );
    
    if (!transform) return false;
    
    *x = transform->x;
    *y = transform->y;
    return true;
}

// Met à jour la position du joueur en fonction des entrées
static void update_player_movement(WorldSystem* system, float delta_time) {
    if (!system || system->player_entity == INVALID_ENTITY_ID) return;
    
    // Obtenir les composants nécessaires
    TransformComponent* transform = (TransformComponent*)entity_get_component(
        system->entity_manager, system->player_entity, COMPONENT_TRANSFORM
    );
    
    PlayerComponent* player = (PlayerComponent*)entity_get_component(
        system->entity_manager, system->player_entity, COMPONENT_PLAYER
    );
    
    if (!transform || !player) return;
    
    // Vecteur de déplacement
    float move_x = 0.0f;
    float move_y = 0.0f;
    
    // Récupérer l'état du clavier
    const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
    
    // Mettre à jour le mouvement en fonction des touches
    if (keyboard_state[SDL_SCANCODE_UP] || keyboard_state[SDL_SCANCODE_W]) {
        move_y = -1.0f;
        system->player_direction = DIRECTION_UP;
        system->is_player_moving = true;
    } else if (keyboard_state[SDL_SCANCODE_DOWN] || keyboard_state[SDL_SCANCODE_S]) {
        move_y = 1.0f;
        system->player_direction = DIRECTION_DOWN;
        system->is_player_moving = true;
    }
    
    if (keyboard_state[SDL_SCANCODE_LEFT] || keyboard_state[SDL_SCANCODE_A]) {
        move_x = -1.0f;
        system->player_direction = DIRECTION_LEFT;
        system->is_player_moving = true;
    } else if (keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D]) {
        move_x = 1.0f;
        system->player_direction = DIRECTION_RIGHT;
        system->is_player_moving = true;
    }
    
    // Si aucune touche de mouvement n'est pressée, le joueur ne bouge pas
    if (move_x == 0.0f && move_y == 0.0f) {
        system->is_player_moving = false;
    } else {
        // Normaliser le vecteur de déplacement pour les diagonales
        if (move_x != 0.0f && move_y != 0.0f) {
            float length = sqrtf(move_x * move_x + move_y * move_y);
            move_x /= length;
            move_y /= length;
        }
        
        // Calculer la nouvelle position
        float new_x = transform->x + move_x * player->move_speed * delta_time;
        float new_y = transform->y + move_y * player->move_speed * delta_time;
        
        // Vérifier les collisions et mettre à jour la position si possible
        if (world_system_is_walkable(system, new_x, transform->y)) {
            transform->x = new_x;
        }
        
        if (world_system_is_walkable(system, transform->x, new_y)) {
            transform->y = new_y;
        }
    }
}

// Dessine une tuile à une position spécifique
static void draw_tile(RenderSystem* render_system, int texture_id, 
                     TileType type, int variant, float x, float y, 
                     int tile_size, bool is_tilled, bool is_watered) {
    if (!render_system) return;
    
    // Calculer la position source dans le tileset
    int src_x = variant * tile_size;
    int src_y = type * tile_size;
    
    // Dessiner la tuile de base
    render_system_draw_sprite(
        render_system, texture_id,
        x, y,
        tile_size, tile_size,
        src_x, src_y,
        tile_size, tile_size,
        0.0f, 1.0f, 1.0f
    );
    
    // Si la tuile est labourée, dessiner l'effet de labour par-dessus
    if (is_tilled) {
        // Exemple: décalage dans le tileset pour trouver l'effet de labour
        int tilled_src_x = 0;
        int tilled_src_y = TILE_TYPE_COUNT * tile_size; // Après toutes les tuiles normales
        
        render_system_draw_sprite(
            render_system, texture_id,
            x, y,
            tile_size, tile_size,
            tilled_src_x, tilled_src_y,
            tile_size, tile_size,
            0.0f, 1.0f, 1.0f
        );
    }
    
    // Si la tuile est arrosée, dessiner l'effet d'eau par-dessus
    if (is_watered) {
        // Exemple: décalage dans le tileset pour trouver l'effet d'arrosage
        int watered_src_x = tile_size;
        int watered_src_y = TILE_TYPE_COUNT * tile_size; // Après toutes les tuiles normales
        
        render_system_draw_sprite(
            render_system, texture_id,
            x, y,
            tile_size, tile_size,
            watered_src_x, watered_src_y,
            tile_size, tile_size,
            0.0f, 1.0f, 1.0f
        );
    }
}

// Dessine le joueur
static void draw_player(WorldSystem* system, RenderSystem* render_system) {
    if (!system || !render_system || system->player_entity == INVALID_ENTITY_ID) return;
    
    TransformComponent* transform = (TransformComponent*)entity_get_component(
        system->entity_manager, system->player_entity, COMPONENT_TRANSFORM
    );
    
    if (!transform) return;
    
    // Si la texture du joueur n'est pas chargée, utiliser un rectangle coloré
    if (system->player_texture_id < 0) {
        render_system_draw_rect(
            render_system,
            transform->x, transform->y,
            DEFAULT_TILE_SIZE, DEFAULT_TILE_SIZE,
            255, 0, 0, 255,
            true
        );
        return;
    }
    
    // Taille du joueur (légèrement plus grande qu'une tuile)
    int player_width = DEFAULT_TILE_SIZE;
    int player_height = DEFAULT_TILE_SIZE * 1.5f; // 1.5x plus haut pour les proportions
    
    // Calculer la frame d'animation
    int frame = system->is_player_moving ? 
                ((int)(system->world_elapsed_time * 5) % 4) : 0; // 4 frames d'animation
    
    // Calculer la position source dans la texture du joueur
    int src_x = frame * player_width;
    int src_y = system->player_direction * player_height;
    
    // Dessiner le joueur
    render_system_draw_sprite(
        render_system, system->player_texture_id,
        transform->x, transform->y,
        player_width, player_height,
        src_x, src_y,
        player_width, player_height,
        0.0f, 1.0f, 1.0f
    );
}

// Dessine la carte
static void draw_map(WorldSystem* system, RenderSystem* render_system) {
    if (!system || !render_system || !system->current_map) return;
    
    // Obtenir la position du joueur
    float player_x, player_y;
    if (!get_player_position(system, &player_x, &player_y)) {
        return;
    }
    
    // Centrer la caméra sur le joueur
    render_system_center_camera(render_system, player_x, player_y);
    
    // Calculer les chunks visibles autour du joueur
    int tile_size = system->current_map->tile_size;
    int chunk_size = system->current_map->chunk_size;
    
    int center_chunk_x = player_x / (tile_size * chunk_size);
    int center_chunk_y = player_y / (tile_size * chunk_size);
    
    // Dessiner les chunks visibles (1 chunk autour du joueur)
    const int VISIBLE_RADIUS = 1;
    
    for (int cy = center_chunk_y - VISIBLE_RADIUS; cy <= center_chunk_y + VISIBLE_RADIUS; cy++) {
        for (int cx = center_chunk_x - VISIBLE_RADIUS; cx <= center_chunk_x + VISIBLE_RADIUS; cx++) {
            // Vérifier si le chunk est dans les limites
            if (cx < 0 || cx >= system->current_map->chunks_x ||
                cy < 0 || cy >= system->current_map->chunks_y) {
                continue;
            }
            
            int chunk_index = cy * system->current_map->chunks_x + cx;
            Chunk* chunk = system->current_map->chunks[chunk_index];
            
            if (!chunk || !chunk->is_loaded) continue;
            
            // Dessiner toutes les tuiles du chunk
            for (int ty = 0; ty < chunk_size; ty++) {
                for (int tx = 0; tx < chunk_size; tx++) {
                    // Calculer la position monde de la tuile
                    float tile_x = (cx * chunk_size + tx) * tile_size + tile_size / 2.0f;
                    float tile_y = (cy * chunk_size + ty) * tile_size + tile_size / 2.0f;
                    
                    // Dessiner chaque couche
                    for (int layer = 0; layer < LAYER_COUNT; layer++) {
                        Tile tile = chunk->tiles[tx][ty][layer];
                        
                        if (tile.type != TILE_NONE) {
                            draw_tile(
                                render_system,
                                system->tileset_texture_id,
                                tile.type,
                                tile.variant,
                                tile_x, tile_y,
                                tile_size,
                                tile.is_tilled,
                                tile.is_watered
                            );
                        }
                    }
                }
            }
        }
    }
}

// Dessine l'interface utilisateur
static void draw_ui(WorldSystem* system, RenderSystem* render_system) {
    if (!system || !render_system) return;
    
    // Dessiner l'horloge
    char time_str[32];
    sprintf(time_str, "%02d:%02d", system->time_system.hour, system->time_system.minute);
    
    render_system_draw_text(
        render_system,
        time_str,
        10.0f, 10.0f,
        255, 255, 255, 255
    );
    
    // Dessiner la date
    char date_str[64];
    const char* season_names[] = {"Printemps", "Été", "Automne", "Hiver"};
    sprintf(date_str, "Jour %d, %s, An %d", 
            system->time_system.day,
            season_names[system->time_system.season],
            system->time_system.year);
    
    render_system_draw_text(
        render_system,
        date_str,
        10.0f, 30.0f,
        255, 255, 255, 255
    );
    
    // Dessiner les stats du joueur
    if (system->player_entity != INVALID_ENTITY_ID) {
        PlayerComponent* player = (PlayerComponent*)entity_get_component(
            system->entity_manager, system->player_entity, COMPONENT_PLAYER
        );
        
        if (player) {
            char stats_str[64];
            sprintf(stats_str, "Santé: %d/%d    Énergie: %d/%d",
                    player->health, player->max_health,
                    player->stamina, player->max_stamina);
            
            render_system_draw_text(
                render_system,
                stats_str,
                10.0f, 50.0f,
                255, 255, 255, 255
            );
        }
    }
}

// Met à jour le système de monde
void world_system_update(WorldSystem* system, float delta_time) {
    if (!system) return;
    
    // Mettre à jour le temps total écoulé
    system->world_elapsed_time += delta_time;
    
    // Mettre à jour le système de temps
    update_time_system(&system->time_system, delta_time);
    
    // Mettre à jour le mouvement du joueur
    update_player_movement(system, delta_time);
}

// Rend le monde à l'écran
void world_system_render(WorldSystem* system, RenderSystem* render_system) {
    if (!system || !render_system) return;
    
    // Commencer le rendu
    render_system_begin_frame(render_system);
    
    // Dessiner la carte
    draw_map(system, render_system);
    
    // Dessiner le joueur
    draw_player(system, render_system);
    
    // Dessiner l'interface utilisateur
    draw_ui(system, render_system);
    
    // Terminer le rendu
    render_system_end_frame(render_system);
}

// Gère les événements clavier pour le système de monde
void world_system_handle_keydown(WorldSystem* system, SDL_Keycode key) {
    if (!system) return;
    
    switch (key) {
        case SDLK_SPACE:
            // Exemple: Effectuer une action (utiliser un outil)
            break;
            
        case SDLK_e:
            // Exemple: Interagir avec un objet
            break;
            
        case SDLK_i:
            // Exemple: Ouvrir l'inventaire
            break;
            
        case SDLK_m:
            // Exemple: Ouvrir la carte
            break;
            
        case SDLK_t:
            // Exemple: Faire avancer le temps (pour le debug)
            world_system_advance_time(system, 10);
            break;
            
        case SDLK_n:
            // Exemple: Passer à la journée suivante
            world_system_advance_day(system);
            break;
    }
}

// Vérifie si une position est traversable
bool world_system_is_walkable(WorldSystem* system, float x, float y) {
    if (!system || !system->current_map) return false;
    
    // Convertir les coordonnées monde en coordonnées tuile
    int tile_x = (int)(x / system->current_map->tile_size);
    int tile_y = (int)(y / system->current_map->tile_size);
    
    // Vérifier les limites de la carte
    int total_width = system->current_map->chunks_x * system->current_map->chunk_size;
    int total_height = system->current_map->chunks_y * system->current_map->chunk_size;
    
    if (tile_x < 0 || tile_x >= total_width || tile_y < 0 || tile_y >= total_height) {
        return false;
    }
    
    // Calculer les coordonnées du chunk et de la tuile dans le chunk
    int chunk_x = tile_x / system->current_map->chunk_size;
    int chunk_y = tile_y / system->current_map->chunk_size;
    int local_x = tile_x % system->current_map->chunk_size;
    int local_y = tile_y % system->current_map->chunk_size;
    
    // Récupérer le chunk
    int chunk_index = chunk_y * system->current_map->chunks_x + chunk_x;
    Chunk* chunk = system->current_map->chunks[chunk_index];
    
    if (!chunk || !chunk->is_loaded) {
        return false;
    }
    
    // Vérifier si toutes les couches sont traversables
    for (int layer = 0; layer < LAYER_COUNT; layer++) {
        Tile tile = chunk->tiles[local_x][local_y][layer];
        if (tile.type != TILE_NONE && !tile.is_walkable) {
            return false;
        }
    }
    
    return true;
}

// Récupère une tuile de la carte
Tile world_system_get_tile(WorldSystem* system, int x, int y, MapLayer layer) {
    if (!system || !system->current_map) {
        return create_default_tile(TILE_NONE);
    }
    
    // Vérifier les limites de la carte
    int total_width = system->current_map->chunks_x * system->current_map->chunk_size;
    int total_height = system->current_map->chunks_y * system->current_map->chunk_size;
    
    if (x < 0 || x >= total_width || y < 0 || y >= total_height || layer < 0 || layer >= LAYER_COUNT) {
        return create_default_tile(TILE_NONE);
    }
    
    // Calculer les coordonnées du chunk et de la tuile dans le chunk
    int chunk_x = x / system->current_map->chunk_size;
    int chunk_y = y / system->current_map->chunk_size;
    int local_x = x % system->current_map->chunk_size;
    int local_y = y % system->current_map->chunk_size;
    
    // Récupérer le chunk
    int chunk_index = chunk_y * system->current_map->chunks_x + chunk_x;
    Chunk* chunk = system->current_map->chunks[chunk_index];
    
    if (!chunk || !chunk->is_loaded) {
        return create_default_tile(TILE_NONE);
    }
    
    return chunk->tiles[local_x][local_y][layer];
}

// Définit une tuile sur la carte
bool world_system_set_tile(WorldSystem* system, int x, int y, MapLayer layer, Tile tile) {
    if (!system || !system->current_map) {
        return false;
    }
    
    // Vérifier les limites de la carte
    int total_width = system->current_map->chunks_x * system->current_map->chunk_size;
    int total_height = system->current_map->chunks_y * system->current_map->chunk_size;
    
    if (x < 0 || x >= total_width || y < 0 || y >= total_height || layer < 0 || layer >= LAYER_COUNT) {
        return false;
    }
    
    // Calculer les coordonnées du chunk et de la tuile dans le chunk
    int chunk_x = x / system->current_map->chunk_size;
    int chunk_y = y / system->current_map->chunk_size;
    int local_x = x % system->current_map->chunk_size;
    int local_y = y % system->current_map->chunk_size;
    
    // Récupérer le chunk
    int chunk_index = chunk_y * system->current_map->chunks_x + chunk_x;
    Chunk* chunk = system->current_map->chunks[chunk_index];
    
    if (!chunk || !chunk->is_loaded) {
        return false;
    }
    
    // Mettre à jour la tuile
    chunk->tiles[local_x][local_y][layer] = tile;
    chunk->is_dirty = true;
    
    // Mettre à jour la carte de collision si nécessaire
    if (x < 1024 && y < 1024) {
        system->collision_map[x][y] = !tile.is_walkable;
    }
    
    return true;
}

// Vérifie si une tuile est labourable
bool world_system_is_tillable(WorldSystem* system, int x, int y) {
    if (!system) return false;
    
    Tile ground_tile = world_system_get_tile(system, x, y, LAYER_GROUND);
    return ground_tile.is_tillable && !ground_tile.is_tilled;
}

// Laboure une tuile
bool world_system_till_tile(WorldSystem* system, int x, int y) {
    if (!system || !world_system_is_tillable(system, x, y)) {
        return false;
    }
    
    Tile ground_tile = world_system_get_tile(system, x, y, LAYER_GROUND);
    ground_tile.is_tilled = true;
    
    return world_system_set_tile(system, x, y, LAYER_GROUND, ground_tile);
}

// Arrose une tuile
bool world_system_water_tile(WorldSystem* system, int x, int y) {
    if (!system) return false;
    
    Tile ground_tile = world_system_get_tile(system, x, y, LAYER_GROUND);
    
    // On ne peut arroser que les tuiles labourées
    if (!ground_tile.is_tilled) {
        return false;
    }
    
    ground_tile.is_watered = true;
    
    return world_system_set_tile(system, x, y, LAYER_GROUND, ground_tile);
}

// Avance le temps dans le monde
void world_system_advance_time(WorldSystem* system, int minutes) {
    if (!system) return;
    
    // Ajouter les minutes
    system->time_system.minute += minutes;
    
    // Gérer le débordement des minutes
    while (system->time_system.minute >= MINUTES_PER_HOUR) {
        system->time_system.minute -= MINUTES_PER_HOUR;
        system->time_system.hour++;
        
        // Gérer le débordement des heures
        if (system->time_system.hour >= HOURS_PER_DAY) {
            system->time_system.hour = 0;
            system->time_system.day++;
            
            // Gérer le débordement des jours
            if (system->time_system.day > DAYS_PER_SEASON) {
                system->time_system.day = 1;
                system->time_system.season = (system->time_system.season + 1) % SEASONS_PER_YEAR;
                
                // Si on est passé à une nouvelle année
                if (system->time_system.season == SEASON_SPRING) {
                    system->time_system.year++;
                }
            }
        }
    }
    
    // Mettre à jour le temps de la journée
    system->time_system.day_time = (float)(system->time_system.hour * MINUTES_PER_HOUR + system->time_system.minute) / 
                                  (float)(HOURS_PER_DAY * MINUTES_PER_HOUR);
    
    // Déterminer s'il fait jour ou nuit
    system->time_system.is_night = (system->time_system.hour < DAY_START_HOUR || 
                                  system->time_system.hour >= NIGHT_START_HOUR);
}

// Avance à la journée suivante
void world_system_advance_day(WorldSystem* system) {
    if (!system) return;
    
    // Passer à 6h du matin le jour suivant
    system->time_system.hour = DAY_START_HOUR;
    system->time_system.minute = 0;
    system->time_system.day++;
    
    // Gérer le débordement des jours
    if (system->time_system.day > DAYS_PER_SEASON) {
        system->time_system.day = 1;
        system->time_system.season = (system->time_system.season + 1) % SEASONS_PER_YEAR;
        
        // Si on est passé à une nouvelle année
        if (system->time_system.season == SEASON_SPRING) {
            system->time_system.year++;
        }
    }
    
    // Mettre à jour le temps de la journée
    system->time_system.day_time = (float)(system->time_system.hour * MINUTES_PER_HOUR) / 
                                  (float)(HOURS_PER_DAY * MINUTES_PER_HOUR);
    
    // Il fait jour
    system->time_system.is_night = false;
}

// Téléporte le joueur à une position
void world_system_teleport_player(WorldSystem* system, float x, float y) {
    if (!system || system->player_entity == INVALID_ENTITY_ID) return;
    
    TransformComponent* transform = (TransformComponent*)entity_get_component(
        system->entity_manager, system->player_entity, COMPONENT_TRANSFORM
    );
    
    if (!transform) return;
    
    transform->x = x;
    transform->y = y;
}

// Change la zone actuelle
bool world_system_change_zone(WorldSystem* system, ZoneType zone_type) {
    if (!system || !system->current_map) return false;
    
    // Changer la zone de la carte
    system->current_map->current_zone = zone_type;
    system->current_zone = zone_type;
    
    log_info("Zone changée pour: %d", zone_type);
    return true;
}