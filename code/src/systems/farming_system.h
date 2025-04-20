/**
 * farming_system.h
 * Système de gestion des cultures et des plantes
 */

#ifndef FARMING_SYSTEM_H
#define FARMING_SYSTEM_H

#include <stdbool.h>
#include "../core/entity.h"
#include "../systems/entity_manager.h"
#include "../systems/world.h"

// Types de plantes
typedef enum {
    PLANT_TYPE_SINGLE_HARVEST,    // Plantes récoltables en une fois (ex: oignons)
    PLANT_TYPE_REGROWABLE,        // Plantes qui repoussent sur leur plant (ex: fraises)
    PLANT_TYPE_FRUIT_TREE,        // Arbres fruitiers
    PLANT_TYPE_MUSHROOM           // Champignons
} PlantType;

// Saisons pendant lesquelles les plantes peuvent être cultivées
typedef enum {
    SEASON_FLAG_NONE      = 0,
    SEASON_FLAG_SPRING    = 1 << 0,
    SEASON_FLAG_SUMMER    = 1 << 1,
    SEASON_FLAG_FALL      = 1 << 2,
    SEASON_FLAG_WINTER    = 1 << 3,
    SEASON_FLAG_ALL       = SEASON_FLAG_SPRING | SEASON_FLAG_SUMMER | SEASON_FLAG_FALL | SEASON_FLAG_WINTER
} SeasonFlags;

// Structure pour un produit récolté
typedef struct {
    int item_id;              // ID du produit
    int min_quantity;         // Quantité minimale récoltée
    int max_quantity;         // Quantité maximale récoltée
} HarvestProduct;

// Définition d'une plante
typedef struct {
    int id;                   // ID unique de la plante
    char name[32];            // Nom de la plante
    PlantType type;           // Type de plante
    int seed_price;           // Prix de la graine
    int sale_price;           // Prix de vente du produit
    int exp_gain;             // Gain d'expérience à la récolte
    SeasonFlags seasons;      // Saisons de croissance
    int days_to_mature;       // Jours jusqu'à maturité
    int regrow_days;          // Jours pour repousser (si applicable)
    int max_harvests;         // Nombre maximum de récoltes (-1 pour illimité)
    HarvestProduct products[3]; // Produits récoltés (jusqu'à 3)
    int product_count;        // Nombre de produits différents
    int growth_stages;        // Nombre d'étapes de croissance
    int sprite_id;            // ID du sprite pour la plante
} PlantData;

// État d'une plante sur la carte
typedef struct {
    int plant_id;             // ID de la plante
    int growth_stage;         // Étape de croissance actuelle
    int days_growing;         // Jours de croissance
    bool is_watered;          // Est arrosée aujourd'hui
    bool is_harvestable;      // Prête à être récoltée
    int harvests_remaining;   // Récoltes restantes
    bool is_dead;             // Plante morte (hors saison)
    bool is_in_greenhouse;    // Est dans une serre
} PlantState;

// Système de farming
typedef struct {
    EntityManager* entity_manager;  // Référence au gestionnaire d'entités
    WorldSystem* world_system;      // Référence au système de monde
    
    PlantData* plant_database;      // Base de données des plantes
    int plant_count;                // Nombre de plantes dans la base
    
    // Tables de conversion entre types de tuiles et états
    TileType tilled_soil_type;      // Type de tuile pour sol labouré
    TileType watered_soil_type;     // Type de tuile pour sol arrosé
} FarmingSystem;

/**
 * Initialise le système de farming
 * @param entity_manager Gestionnaire d'entités
 * @param world_system Système de monde
 * @return Pointeur vers le système de farming ou NULL en cas d'erreur
 */
FarmingSystem* farming_system_init(EntityManager* entity_manager, WorldSystem* world_system);

/**
 * Libère les ressources du système de farming
 * @param system Système de farming
 */
void farming_system_shutdown(FarmingSystem* system);

/**
 * Met à jour le système de farming (croissance des plantes, etc.)
 * @param system Système de farming
 * @param days_elapsed Nombre de jours écoulés
 */
void farming_system_update(FarmingSystem* system, float days_elapsed);

/**
 * Laboure une tuile à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @return true si le labourage a réussi, false sinon
 */
bool farming_system_till_soil(FarmingSystem* system, int x, int y);

/**
 * Arrose une tuile à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @return true si l'arrosage a réussi, false sinon
 */
bool farming_system_water_soil(FarmingSystem* system, int x, int y);

/**
 * Plante une graine à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @param plant_id ID de la plante à planter
 * @return true si la plantation a réussi, false sinon
 */
bool farming_system_plant_seed(FarmingSystem* system, int x, int y, int plant_id);

/**
 * Récolte une plante à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @param harvested_items Tableau pour stocker les items récoltés
 * @param max_items Taille maximale du tableau
 * @return Nombre d'items récoltés ou -1 en cas d'erreur
 */
int farming_system_harvest_plant(FarmingSystem* system, int x, int y, int* harvested_items, int max_items);

/**
 * Vérifie si une plante peut être plantée en fonction de la saison actuelle
 * @param system Système de farming
 * @param plant_id ID de la plante
 * @return true si la plante peut être plantée, false sinon
 */
bool farming_system_can_plant(FarmingSystem* system, int plant_id);

/**
 * Récupère les données d'une plante par son ID
 * @param system Système de farming
 * @param plant_id ID de la plante
 * @return Pointeur vers les données de la plante ou NULL si non trouvée
 */
const PlantData* farming_system_get_plant_data(FarmingSystem* system, int plant_id);

/**
 * Récupère l'état d'une plante à la position spécifiée
 * @param system Système de farming
 * @param x Position X
 * @param y Position Y
 * @param state Pointeur pour stocker l'état (peut être NULL)
 * @return true si une plante est présente, false sinon
 */
bool farming_system_get_plant_state(FarmingSystem* system, int x, int y, PlantState* state);

#endif /* FARMING_SYSTEM_H */
