/**
 * Initialise le système de farming
 * @param entity_manager Gestionnaire d'entités
 * @param world_system Système de monde
 * @return Pointeur vers le système de farming ou NULL en cas d'erreur
 */
FarmingSystem* farming_system_init(EntityManager* entity_manager, WorldSystem* world_system) {
    if (!entity_manager || !world_system) {
        log_error("Paramètres invalides pour farming_system_init");
        return NULL;
    }
    
    FarmingSystem* system = (FarmingSystem*)calloc(1, sizeof(FarmingSystem));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système de farming")) {
        return NULL;
    }
    
    system->entity_manager = entity_manager;
    system->world_system = world_system;
    
    // Initialiser la base de données des plantes
    system->plant_count = sizeof(default_plants) / sizeof(PlantData);
    system->plant_database = (PlantData*)calloc(MAX_PLANTS, sizeof(PlantData));
    
    if (!check_ptr(system->plant_database, LOG_LEVEL_ERROR, "Échec d'allocation de la base de données de plantes")) {
        free(system);
        return NULL;
    }
    
    // Copier les plantes par défaut
    memcpy(system->plant_database, default_plants, sizeof(default_plants));
    
    // Initialiser les types de tuiles pour le sol
    system->tilled_soil_type = TILE_DIRT; // À ajuster selon votre système
    system->watered_soil_type = TILE_DIRT; // À ajuster selon votre système
    
    log_info("Système de farming initialisé avec %d plantes", system->plant_count);
    
    return system;
}

/**
 * Libère les ressources du système de farming
 * @param system Système de farming
 */
void farming_system_shutdown(FarmingSystem* system) {
    if (!system) return;
    
    if (system->plant_database) {
        free(system->plant_database);
        system->plant_database = NULL;
    }
    
    free(system);
    log_info("Système de farming libéré");
}

/**
 * Met à jour le système de farming (croissance des plantes, etc.)
 * @param system Système de farming
 * @param days_elapsed Nombre de jours écoulés
 */
void farming_system_update(FarmingSystem* system, float days_elapsed) {
    if (!system || !system->world_system || !system->world_system->current_map) return;
    
    // Obtenir la saison actuelle
    Season current_season = system->world_system->time_system.season;
    SeasonFlags season_flag = 1 << current_season;
    
    // Parcourir toutes les tuiles du monde pour mettre à jour les plantes
    Map* map = system->world_system->current_map;
    
    for (int cy = 0; cy < map->chunks_y; cy++) {
        for (int cx = 0; cx < map->chunks_x; cx++) {
            int chunk_index = cy * map->chunks_x + cx;
            Chunk* chunk = map->chunks[chunk_index];
            
            if (!chunk || !chunk->is_loaded) continue;
            
            for (int x = 0; x < map->chunk_size; x++) {
                for (int y = 0; y < map->chunk_size; y++) {
                    // Vérifier s'il y a une plante sur cette tuile
                    Tile* tile = &chunk->tiles[x][y][LAYER_ITEMS];
                    if (tile->type == TILE_NONE) continue;
                    
                    // Obtenez l'état de la plante associée à cette tuile
                    PlantState state;
                    if (!farming_system_get_plant_state(system, cx * map->chunk_size + x, cy * map->chunk_size + y, &state)) {
                        continue;
                    }
                    
                    // Ne pas mettre à jour les plantes mortes
                    if (state.is_dead) continue;
                    
                    // Vérifier si la plante est dans une serre
                    bool is_in_greenhouse = state.is_in_greenhouse;
                    
                    // Obtenir les données de la plante
                    const PlantData* plant_data = farming_system_get_plant_data(system, state.plant_id);
                    if (!plant_data) continue;
                    
                    // Vérifier si la plante peut pousser dans cette saison
                    bool can_grow = is_in_greenhouse || (plant_data->seasons & season_flag);
                    
                    if (!can_grow) {
                        // La plante ne peut pas pousser dans cette saison, elle meurt
                        state.is_dead = true;
                        // Mettre à jour l'état de la plante sur la tuile
                        // TODO: Implémenter la mise à jour de l'état de la plante
                        continue;
                    }
                    
                    // Mettre à jour la croissance si la plante a été arrosée
                    if (state.is_watered || is_in_greenhouse) {
                        // Ajouter le temps écoulé à la croissance
                        state.days_growing += days_elapsed;
                        
                        // Mettre à jour l'étape de croissance
                        if (state.is_harvestable) {
                            // Plante déjà prête à être récoltée, ne rien faire
                        } else if (state.growth_stage < plant_data->growth_stages - 1) {
                            // Calculer la nouvelle étape de croissance
                            float growth_progress = state.days_growing / plant_data->days_to_mature;
                            int new_stage = (int)(growth_progress * (plant_data->growth_stages - 1));
                            
                            // Limiter l'étape de croissance au maximum
                            if (new_stage >= plant_data->growth_stages - 1) {
                                new_stage = plant_data->growth_stages - 1;
                                
                                // Marquer comme prête à être récoltée
                                state.is_harvestable = true;
                            }
                            
                            state.growth_stage = new_stage;
                        }
                        
                        // Réinitialiser l'état arrosé pour le jour suivant
                        state.is_watered = false;
                    }
                    
                    // Mettre à jour l'état de la plante sur la tuile
                    // TODO: Implémenter la mise à jour de l'état de la plante
                }
            }
        }
    }
}

/**
 * Laboure une tuile à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @return true si le labourage a réussi, false sinon
 */
bool farming_system_till_soil(FarmingSystem* system, int x, int y) {
    if (!system || !system->world_system) return false;
    
    // Vérifier si la tuile est labourable
    if (!world_system_is_tillable(system->world_system, x, y)) {
        return false;
    }
    
    // Obtenir la tuile actuelle
    Tile tile = world_system_get_tile(system->world_system, x, y, LAYER_GROUND);
    
    // Modifier la tuile pour qu'elle soit labourée
    tile.is_tilled = true;
    
    // Mettre à jour la tuile
    if (!world_system_set_tile(system->world_system, x, y, LAYER_GROUND, tile)) {
        return false;
    }
    
    log_debug("Tuile labourée en (%d, %d)", x, y);
    return true;
}

/**
 * Arrose une tuile à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @return true si l'arrosage a réussi, false sinon
 */
bool farming_system_water_soil(FarmingSystem* system, int x, int y) {
    if (!system || !system->world_system) return false;
    
    // Obtenir la tuile actuelle
    Tile tile = world_system_get_tile(system->world_system, x, y, LAYER_GROUND);
    
    // Vérifier si la tuile est labourée
    if (!tile.is_tilled) {
        return false;
    }
    
    // Vérifier si la tuile a déjà été arrosée
    if (tile.is_watered) {
        return true; // Déjà arrosée, considéré comme réussi
    }
    
    // Modifier la tuile pour qu'elle soit arrosée
    tile.is_watered = true;
    
    // Mettre à jour la tuile
    if (!world_system_set_tile(system->world_system, x, y, LAYER_GROUND, tile)) {
        return false;
    }
    
    // Mettre à jour l'état des plantes sur cette tuile
    PlantState state;
    if (farming_system_get_plant_state(system, x, y, &state)) {
        state.is_watered = true;
        // TODO: Sauvegarder l'état mis à jour
    }
    
    log_debug("Tuile arrosée en (%d, %d)", x, y);
    return true;
}

/**
 * Plante une graine à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @param plant_id ID de la plante à planter
 * @return true si la plantation a réussi, false sinon
 */
bool farming_system_plant_seed(FarmingSystem* system, int x, int y, int plant_id) {
    if (!system || !system->world_system) return false;
    
    // Vérifier si la plante existe
    const PlantData* plant = farming_system_get_plant_data(system, plant_id);
    if (!plant) {
        log_error("Plant ID invalide: %d", plant_id);
        return false;
    }
    
    // Vérifier si la tuile est labourée
    Tile ground_tile = world_system_get_tile(system->world_system, x, y, LAYER_GROUND);
    if (!ground_tile.is_tilled) {
        log_warning("Impossible de planter sur une tuile non labourée en (%d, %d)", x, y);
        return false;
    }
    
    // Vérifier si la tuile est vide
    Tile item_tile = world_system_get_tile(system->world_system, x, y, LAYER_ITEMS);
    if (item_tile.type != TILE_NONE) {
        log_warning("La tuile en (%d, %d) n'est pas vide", x, y);
        return false;
    }
    
    // Vérifier si la plante peut être plantée dans la saison actuelle
    Season current_season = system->world_system->time_system.season;
    SeasonFlags season_flag = 1 << current_season;
    
    // Les champignons peuvent être plantés n'importe quand dans un bâtiment spécifique
    bool is_mushroom = (plant->type == PLANT_TYPE_MUSHROOM);
    bool is_in_greenhouse = false; // TODO: Détecter si la tuile est dans une serre
    
    if (!is_mushroom && !is_in_greenhouse && !(plant->seasons & season_flag)) {
        log_warning("Cette plante ne peut pas être plantée pendant cette saison");
        return false;
    }
    
    // Créer une nouvelle tuile pour la plante
    Tile new_tile;
    new_tile.type = TILE_DIRT; // À adapter selon votre système de tuiles
    new_tile.variant = plant_id; // Stocker l'ID de la plante dans la variante
    new_tile.is_walkable = true;
    new_tile.is_tillable = false;
    new_tile.is_watered = ground_tile.is_watered;
    new_tile.is_tilled = true;
    
    // Placer la tuile sur la carte
    if (!world_system_set_tile(system->world_system, x, y, LAYER_ITEMS, new_tile)) {
        log_error("Échec de placement de la plante en (%d, %d)", x, y);
        return false;
    }
    
    // Créer un état initial pour la plante
    PlantState state;
    state.plant_id = plant_id;
    state.growth_stage = 0;
    state.days_growing = 0;
    state.is_watered = ground_tile.is_watered;
    state.is_harvestable = false;
    state.harvests_remaining = (plant->max_harvests > 0) ? plant->max_harvests : -1;
    state.is_dead = false;
    state.is_in_greenhouse = is_in_greenhouse;
    
    // TODO: Sauvegarder l'état de la plante
    
    log_info("Plante %s plantée en (%d, %d)", plant->name, x, y);
    return true;
}

/**
 * Récolte une plante à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @param harvested_items Tableau pour stocker les items récoltés
 * @param max_items Taille maximale du tableau
 * @return Nombre d'items récoltés ou -1 en cas d'erreur
 */
int farming_system_harvest_plant(FarmingSystem* system, int x, int y, int* harvested_items, int max_items) {
    if (!system || !system->world_system || !harvested_items || max_items <= 0) return -1;
    
    // Obtenir l'état de la plante
    PlantState state;
    if (!farming_system_get_plant_state(system, x, y, &state)) {
        log_warning("Aucune plante à récolter en (%d, %d)", x, y);
        return 0;
    }
    
    // Vérifier si la plante est récoltable
    if (!state.is_harvestable) {
        log_warning("La plante en (%d, %d) n'est pas encore prête à être récoltée", x, y);
        return 0;
    }
    
    // Obtenir les données de la plante
    const PlantData* plant = farming_system_get_plant_data(system, plant_id);
    if (!plant) return false;
    
    // Obtenir la saison actuelle
    Season current_season = system->world_system->time_system.season;
    SeasonFlags season_flag = 1 << current_season;
    
    // Vérifier si la plante peut être cultivée dans cette saison
    if (plant->type == PLANT_TYPE_MUSHROOM) {
        // Les champignons peuvent être plantés n'importe quand
        return true;
    } else {
        return (plant->seasons & season_flag) != 0;
    }
}

/**
 * Récupère les données d'une plante par son ID
 * @param system Système de farming
 * @param plant_id ID de la plante
 * @return Pointeur vers les données de la plante ou NULL si non trouvée
 */
const PlantData* farming_system_get_plant_data(FarmingSystem* system, int plant_id) {
    if (!system || !system->plant_database) return NULL;
    
    // Parcourir la base de données pour trouver la plante
    for (int i = 0; i < system->plant_count; i++) {
        if (system->plant_database[i].id == plant_id) {
            return &system->plant_database[i];
        }
    }
    
    return NULL;
}

/**
 * Récupère l'état d'une plante à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @param state Pointeur pour stocker l'état (peut être NULL)
 * @return true si une plante est présente, false sinon
 */
bool farming_system_get_plant_state(FarmingSystem* system, int x, int y, PlantState* state) {
    if (!system || !system->world_system) return false;
    
    // Obtenir la tuile à la position spécifiée
    Tile tile = world_system_get_tile(system->world_system, x, y, LAYER_ITEMS);
    
    // Vérifier si la tuile contient une plante
    if (tile.type == TILE_NONE) return false;
    
    // Dans cette implémentation simple, nous allons supposer que la variante de la tuile
    // est l'ID de la plante. Dans un système plus complexe, vous voudriez stocker les
    // états des plantes dans une structure de données séparée.
    int plant_id = tile.variant;
    
    // Obtenir les données de la plante
    const PlantData* plant_data = farming_system_get_plant_data(system, plant_id);
    if (!plant_data) return false;
    
    // Si un pointeur d'état est fourni, le remplir avec les informations actuelles
    // Dans un système réel, ces informations seraient lues à partir d'une structure
    // de données persistante, mais pour ce prototype nous les calculons à la volée
    if (state) {
        state->plant_id = plant_id;
        state->growth_stage = 0; // Normalement, cela viendrait de la structure de données
        state->days_growing = 0; // Normalement, cela viendrait de la structure de données
        state->is_watered = tile.is_watered;
        state->is_harvestable = false; // Normalement, cela viendrait de la structure de données
        state->harvests_remaining = plant_data->max_harvests;
        state->is_dead = false;
        state->is_in_greenhouse = false; // Normalement, cela viendrait de la détection de position
    }
    
    return true;
}_get_plant_data(system, state.plant_id);
    if (!plant) {
        log_error("Données de plante invalides pour l'ID %d", state.plant_id);
        return -1;
    }
    
    // Calculer le nombre d'items récoltés
    int harvested_count = 0;
    
    for (int i = 0; i < plant->product_count && harvested_count < max_items; i++) {
        // Déterminer le nombre aléatoire de produits
        int quantity = plant->products[i].min_quantity;
        if (plant->products[i].max_quantity > plant->products[i].min_quantity) {
            quantity += rand() % (plant->products[i].max_quantity - plant->products[i].min_quantity + 1);
        }
        
        // Ajouter les produits au tableau
        for (int j = 0; j < quantity && harvested_count < max_items; j++) {
            harvested_items[harvested_count++] = plant->products[i].item_id;
        }
    }
    
    // Mettre à jour l'état de la plante après la récolte
    switch (plant->type) {
        case PLANT_TYPE_SINGLE_HARVEST:
            // Supprimer la plante
            // Effacer la tuile
            Tile empty_tile;
            empty_tile.type = TILE_NONE;
            world_system_set_tile(system->world_system, x, y, LAYER_ITEMS, empty_tile);
            break;
            
        case PLANT_TYPE_REGROWABLE:
            // Décrémenter le compteur de récoltes si nécessaire
            if (state.harvests_remaining > 0) {
                state.harvests_remaining--;
                
                if (state.harvests_remaining == 0) {
                    // Plus de récoltes disponibles, supprimer la plante
                    Tile empty_tile;
                    empty_tile.type = TILE_NONE;
                    world_system_set_tile(system->world_system, x, y, LAYER_ITEMS, empty_tile);
                } else {
                    // Réinitialiser l'étape de croissance pour repousse
                    state.growth_stage = plant->growth_stages - 3; // Retour à l'état avant maturation
                    state.days_growing = (float)state.growth_stage * plant->days_to_mature / (plant->growth_stages - 1);
                    state.is_harvestable = false;
                    
                    // TODO: Sauvegarder l'état mis à jour
                }
            } else {
                // Récoltes illimitées
                // Réinitialiser l'étape de croissance pour repousse
                state.growth_stage = plant->growth_stages - 3; // Retour à l'état avant maturation
                state.days_growing = (float)state.growth_stage * plant->days_to_mature / (plant->growth_stages - 1);
                state.is_harvestable = false;
                
                // TODO: Sauvegarder l'état mis à jour
            }
            break;
            
        case PLANT_TYPE_FRUIT_TREE:
            // Les arbres restent mais sont marqués comme non récoltables
            state.is_harvestable = false;
            state.days_growing = 0; // Réinitialiser le compteur pour la prochaine récolte
            
            // TODO: Sauvegarder l'état mis à jour
            break;
            
        case PLANT_TYPE_MUSHROOM:
            // Les champignons peuvent être récoltés indéfiniment
            state.is_harvestable = false;
            state.days_growing = 0; // Réinitialiser le compteur pour la prochaine récolte
            
            // TODO: Sauvegarder l'état mis à jour
            break;
    }
    
    log_info("%d produits récoltés de %s en (%d, %d)", harvested_count, plant->name, x, y);
    return harvested_count;
}

/**
 * Vérifie si une plante peut être plantée en fonction de la saison actuelle
 * @param system Système de farming
 * @param plant_id ID de la plante
 * @return true si la plante peut être plantée, false sinon
 */
bool farming_system_can_plant(FarmingSystem* system, int plant_id) {
    if (!system || !system->world_system) return false;
    
    // Obtenir les données de la plante
    const PlantData* plant = farming_system/**
 * farming_system.c
 * Implémentation du système de gestion des cultures et des plantes
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../systems/farming_system.h"
#include "../utils/error_handler.h"

// Nombre maximal de plantes dans la base de données
#define MAX_PLANTS 100

// Macros pour convertir les saisons en flags
#define SEASON_STR_TO_FLAG(str) ( \
    (strstr(str, "Pr") ? SEASON_FLAG_SPRING : 0) | \
    (strstr(str, "Et") ? SEASON_FLAG_SUMMER : 0) | \
    (strstr(str, "Au") ? SEASON_FLAG_FALL : 0) | \
    (strstr(str, "Hi") ? SEASON_FLAG_WINTER : 0) \
)

// Données des plantes (à partir du fichier plants_data.xlsx)
static PlantData default_plants[] = {
    // [1] Plantes récoltables en une fois
    {1, "Potato", PLANT_TYPE_SINGLE_HARVEST, 60, 70, 10, SEASON_FLAG_SPRING | SEASON_FLAG_SUMMER | SEASON_FLAG_FALL, 6, 0, 1, {{1, 1, 3}}, 1, 4, 0},
    {2, "Wheat", PLANT_TYPE_SINGLE_HARVEST, 40, 45, 8, SEASON_FLAG_SPRING | SEASON_FLAG_SUMMER | SEASON_FLAG_FALL, 4, 0, 1, {{2, 1, 2}}, 1, 4, 0},
    {3, "Onion", PLANT_TYPE_SINGLE_HARVEST, 60, 80, 10, SEASON_FLAG_SPRING | SEASON_FLAG_SUMMER, 6, 0, 1, {{3, 1, 1}}, 1, 4, 0},
    {4, "Turnip", PLANT_TYPE_SINGLE_HARVEST, 50, 65, 9, SEASON_FLAG_SPRING, 4, 0, 1, {{4, 1, 1}}, 1, 4, 0},
    {5, "Cauliflower", PLANT_TYPE_SINGLE_HARVEST, 70, 100, 12, SEASON_FLAG_SPRING, 7, 0, 1, {{5, 1, 1}}, 1, 4, 0},
    {6, "Lettuce", PLANT_TYPE_SINGLE_HARVEST, 80, 105, 12, SEASON_FLAG_SPRING, 7, 0, 1, {{6, 1, 1}}, 1, 4, 0},
    {7, "Carrot", PLANT_TYPE_SINGLE_HARVEST, 55, 65, 9, SEASON_FLAG_SUMMER | SEASON_FLAG_FALL, 5, 0, 1, {{7, 1, 1}}, 1, 4, 0},
    {8, "Corn", PLANT_TYPE_SINGLE_HARVEST, 90, 60, 11, SEASON_FLAG_SUMMER, 8, 0, 1, {{8, 2, 3}}, 1, 4, 0},
    {9, "Pumpkin", PLANT_TYPE_SINGLE_HARVEST, 95, 130, 15, SEASON_FLAG_FALL, 5, 0, 1, {{9, 1, 1}}, 1, 4, 0},
    {10, "Spinach", PLANT_TYPE_SINGLE_HARVEST, 70, 95, 11, SEASON_FLAG_FALL, 4, 0, 1, {{10, 1, 1}}, 1, 4, 0},
    {11, "Leek", PLANT_TYPE_SINGLE_HARVEST, 60, 90, 10, SEASON_FLAG_FALL, 4, 0, 1, {{11, 1, 1}}, 1, 4, 0},
    {12, "Bok Choy", PLANT_TYPE_SINGLE_HARVEST, 50, 95, 11, SEASON_FLAG_FALL, 9, 0, 1, {{12, 1, 1}}, 1, 4, 0},
    {13, "Hellebore", PLANT_TYPE_SINGLE_HARVEST, 120, 60, 10, SEASON_FLAG_WINTER, 7, 0, 1, {{13, 1, 1}}, 1, 4, 0},
    
    // [2] Plantes qui repoussent
    {20, "Broccoli", PLANT_TYPE_REGROWABLE, 130, 70, 15, SEASON_FLAG_SPRING, 6, 3, 8, {{20, 1, 1}}, 1, 6, 0},
    {21, "Cucumber", PLANT_TYPE_REGROWABLE, 140, 100, 16, SEASON_FLAG_SPRING, 7, 4, 6, {{21, 1, 1}}, 1, 6, 0},
    {22, "Strawberry", PLANT_TYPE_REGROWABLE, 180, 240, 18, SEASON_FLAG_SPRING, 8, 3, 7, {{22, 1, 2}}, 1, 6, 0},
    {23, "Green Beans", PLANT_TYPE_REGROWABLE, 200, 325, 20, SEASON_FLAG_SPRING, 9, 3, 7, {{23, 1, 3}}, 1, 6, 0},
    {24, "Pepper", PLANT_TYPE_REGROWABLE, 110, 165, 16, SEASON_FLAG_SUMMER, 8, 5, 5, {{24, 1, 1}}, 1, 6, 0},
    {25, "Garlic", PLANT_TYPE_REGROWABLE, 120, 180, 17, SEASON_FLAG_SUMMER, 5, 5, 5, {{25, 1, 1}}, 1, 6, 0},
    {26, "Tomato", PLANT_TYPE_REGROWABLE, 130, 170, 17, SEASON_FLAG_SUMMER, 6, 4, 6, {{26, 1, 2}}, 1, 6, 0},
    {27, "Eggplant", PLANT_TYPE_REGROWABLE, 150, 120, 15, SEASON_FLAG_SUMMER, 7, 4, 6, {{27, 1, 1}}, 1, 6, 0},
    {28, "Melon", PLANT_TYPE_REGROWABLE, 170, 290, 20, SEASON_FLAG_SUMMER, 7, 6, 4, {{28, 1, 1}}, 1, 6, 0},
    {29, "Chili Pepper", PLANT_TYPE_REGROWABLE, 140, 260, 18, SEASON_FLAG_FALL, 9, 2, 10, {{29, 1, 1}}, 1, 6, 0},
    {30, "Sweet Potato", PLANT_TYPE_REGROWABLE, 160, 200, 18, SEASON_FLAG_FALL, 7, 4, 6, {{30, 1, 2}}, 1, 6, 0},
    
    // [3] Arbres fruitiers
    {40, "Orange", PLANT_TYPE_FRUIT_TREE, 2000, 250, 30, SEASON_FLAG_SPRING, 15, 10, -1, {{40, 1, 3}}, 1, 6, 0},
    {41, "Cherry", PLANT_TYPE_FRUIT_TREE, 2200, 275, 32, SEASON_FLAG_SPRING, 15, 10, -1, {{41, 1, 3}}, 1, 6, 0},
    {42, "Avocado", PLANT_TYPE_FRUIT_TREE, 2500, 310, 35, SEASON_FLAG_SPRING, 15, 10, -1, {{42, 1, 3}}, 1, 6, 0},
    {43, "Coffee Bean", PLANT_TYPE_FRUIT_TREE, 2800, 350, 38, SEASON_FLAG_SPRING, 15, 10, -1, {{43, 1, 3}}, 1, 6, 0},
    {44, "Lemon", PLANT_TYPE_FRUIT_TREE, 2000, 250, 30, SEASON_FLAG_SUMMER, 15, 10, -1, {{44, 1, 3}}, 1, 6, 0},
    {45, "Banana", PLANT_TYPE_FRUIT_TREE, 2200, 275, 32, SEASON_FLAG_SUMMER, 15, 10, -1, {{45, 1, 3}}, 1, 6, 0},
    {46, "Peach", PLANT_TYPE_FRUIT_TREE, 2500, 310, 35, SEASON_FLAG_SUMMER, 15, 10, -1, {{46, 1, 3}}, 1, 6, 0},
    {47, "Mango", PLANT_TYPE_FRUIT_TREE, 2500, 310, 35, SEASON_FLAG_SUMMER, 15, 10, -1, {{47, 1, 3}}, 1, 6, 0},
    {48, "Apple", PLANT_TYPE_FRUIT_TREE, 2000, 250, 30, SEASON_FLAG_FALL, 15, 10, -1, {{48, 1, 3}}, 1, 6, 0},
    {49, "Pear", PLANT_TYPE_FRUIT_TREE, 2000, 250, 30, SEASON_FLAG_FALL, 15, 10, -1, {{49, 1, 3}}, 1, 6, 0},
    {50, "Olive", PLANT_TYPE_FRUIT_TREE, 2200, 275, 32, SEASON_FLAG_FALL, 15, 10, -1, {{50, 1, 3}}, 1, 6, 0},
    {51, "Grape", PLANT_TYPE_FRUIT_TREE, 2900, 360, 40, SEASON_FLAG_FALL, 15, 10, -1, {{51, 1, 3}}, 1, 6, 0},
    
    // [4] Champignons
    {60, "Shittake", PLANT_TYPE_MUSHROOM, 500, 125, 15, SEASON_FLAG_ALL, 7, 4, -1, {{60, 1, 1}}, 1, 4, 0},
    {61, "Chanterelle", PLANT_TYPE_MUSHROOM, 500, 125, 15, SEASON_FLAG_ALL, 7, 4, -1, {{61, 1, 1}}, 1, 4, 0},
    {62, "Morel", PLANT_TYPE_MUSHROOM, 500, 250, 20, SEASON_FLAG_ALL, 7, 4, -1, {{62, 1, 1}}, 1, 4, 0},
    {63, "Paris Shroom", PLANT_TYPE_MUSHROOM, 500, 125, 15, SEASON_FLAG_ALL, 7, 4, -1, {{63, 1, 1}}, 1, 4, 0},
    {64, "Coral Shroom", PLANT_TYPE_MUSHROOM, 500, 125, 15, SEASON_FLAG_ALL, 7, 4, -1, {{64, 1, 1}}, 1, 4, 0}
};