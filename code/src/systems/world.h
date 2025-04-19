/**
 * world.h
 * Système de gestion du monde de jeu
 */

#ifndef WORLD_H
#define WORLD_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "../systems/entity_manager.h"
#include "../systems/render.h"

// Type de tuile
typedef enum {
    TILE_NONE,        // Aucune tuile (transparent)
    TILE_GRASS,       // Herbe
    TILE_DIRT,        // Terre
    TILE_WATER,       // Eau
    TILE_STONE,       // Pierre
    TILE_SAND,        // Sable
    TILE_BUILDING,    // Bâtiment
    
    // Toujours ajouter avant cette ligne
    TILE_TYPE_COUNT
} TileType;

// Structure de tuile
typedef struct {
    TileType type;        // Type de tuile
    int variant;          // Variante de la tuile (pour les variations visuelles)
    bool is_walkable;     // La tuile est-elle traversable
    bool is_tillable;     // La tuile peut-elle être labourée
    bool is_watered;      // La tuile est-elle arrosée
    bool is_tilled;       // La tuile est-elle labourée
} Tile;

// Couche de tuiles
typedef enum {
    LAYER_GROUND,     // Sol de base
    LAYER_OBJECTS,    // Objets statiques (rochers, arbres)
    LAYER_ITEMS,      // Objets interactifs (cultures, items)
    LAYER_BUILDINGS,  // Bâtiments
    
    // Toujours ajouter avant cette ligne
    LAYER_COUNT
} MapLayer;

// Type de zone
typedef enum {
    ZONE_FARM,        // Ferme du joueur
    ZONE_VILLAGE,     // Village
    ZONE_FOREST,      // Forêt
    ZONE_MINE,        // Mine
    ZONE_BEACH,       // Plage
    
    // Toujours ajouter avant cette ligne
    ZONE_COUNT
} ZoneType;

// Structure de chunk (section de carte)
typedef struct {
    int chunk_x;                  // Coordonnée X du chunk dans le monde
    int chunk_y;                  // Coordonnée Y du chunk dans le monde
    Tile tiles[16][16][LAYER_COUNT]; // Tuiles du chunk (16x16 par défaut, sur plusieurs couches)
    bool is_loaded;               // Le chunk est-il chargé
    bool is_dirty;                // Le chunk a-t-il été modifié
} Chunk;

// Structure de carte
typedef struct {
    Chunk** chunks;           // Tableau de chunks
    int chunks_x;             // Nombre de chunks en largeur
    int chunks_y;             // Nombre de chunks en hauteur
    int chunk_size;           // Taille d'un chunk en tuiles (généralement 16)
    int tile_size;            // Taille d'une tuile en pixels
    ZoneType current_zone;    // Zone actuelle
} Map;

// Paramètres de saison
typedef enum {
    SEASON_SPRING,    // Printemps
    SEASON_SUMMER,    // Été
    SEASON_FALL,      // Automne
    SEASON_WINTER     // Hiver
} Season;

// Paramètres de temps et de date
typedef struct {
    int day;           // Jour du mois (1-30)
    int season;        // Saison actuelle (0-3)
    int year;          // Année
    int hour;          // Heure (0-23)
    int minute;        // Minute (0-59)
    float day_time;    // Temps écoulé dans la journée (0.0-1.0)
    bool is_night;     // Est-ce la nuit
} TimeSystem;

// Direction de déplacement
typedef enum {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT,
    
    DIRECTION_COUNT
} Direction;

// Système de monde (opaque)
typedef struct WorldSystemInternal WorldSystem;

/**
 * Initialise le système de monde
 * @param entity_manager Gestionnaire d'entités
 * @return Pointeur vers le système de monde ou NULL en cas d'erreur
 */
WorldSystem* world_system_init(EntityManager* entity_manager);

/**
 * Initialise le gestionnaire de ressources avec le renderer
 * @param system Système de monde
 * @param renderer Renderer SDL
 */
void world_system_init_renderer(WorldSystem* system, SDL_Renderer* renderer);

/**
 * Libère les ressources du système de monde
 * @param system Système de monde à libérer
 */
void world_system_shutdown(WorldSystem* system);

/**
 * Initialise une nouvelle partie
 * @param system Système de monde
 * @return true si l'initialisation a réussi, false sinon
 */
bool world_system_init_new_game(WorldSystem* system);

/**
 * Met à jour le système de monde
 * @param system Système de monde
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void world_system_update(WorldSystem* system, float delta_time);

/**
 * Rend le monde à l'écran
 * @param system Système de monde
 * @param render_system Système de rendu
 */
void world_system_render(WorldSystem* system, RenderSystem* render_system);

/**
 * Gère les événements clavier pour le système de monde
 * @param system Système de monde
 * @param key Touche appuyée
 */
void world_system_handle_keydown(WorldSystem* system, SDL_Keycode key);

/**
 * Charge une carte depuis un fichier
 * @param system Système de monde
 * @param filename Nom du fichier de carte
 * @return true si le chargement a réussi, false sinon
 */
bool world_system_load_map(WorldSystem* system, const char* filename);

/**
 * Sauvegarde une carte dans un fichier
 * @param system Système de monde
 * @param filename Nom du fichier de carte
 * @return true si la sauvegarde a réussi, false sinon
 */
bool world_system_save_map(WorldSystem* system, const char* filename);

/**
 * Crée une nouvelle carte vide
 * @param system Système de monde
 * @param width Largeur de la carte en chunks
 * @param height Hauteur de la carte en chunks
 * @return true si la création a réussi, false sinon
 */
bool world_system_create_map(WorldSystem* system, int width, int height);

/**
 * Définit une tuile sur la carte
 * @param system Système de monde
 * @param x Position X de la tuile
 * @param y Position Y de la tuile
 * @param layer Couche de la tuile
 * @param tile Tuile à définir
 * @return true si la tuile a été définie, false sinon
 */
bool world_system_set_tile(WorldSystem* system, int x, int y, MapLayer layer, Tile tile);

/**
 * Récupère une tuile de la carte
 * @param system Système de monde
 * @param x Position X de la tuile
 * @param y Position Y de la tuile
 * @param layer Couche de la tuile
 * @return Tuile récupérée (TILE_NONE si hors limites)
 */
Tile world_system_get_tile(WorldSystem* system, int x, int y, MapLayer layer);

/**
 * Vérifie si une position est traversable
 * @param system Système de monde
 * @param x Position X à vérifier
 * @param y Position Y à vérifier
 * @return true si la position est traversable, false sinon
 */
bool world_system_is_walkable(WorldSystem* system, float x, float y);

/**
 * Vérifie si une tuile est labourable
 * @param system Système de monde
 * @param x Position X de la tuile
 * @param y Position Y de la tuile
 * @return true si la tuile peut être labourée, false sinon
 */
bool world_system_is_tillable(WorldSystem* system, int x, int y);

/**
 * Laboure une tuile
 * @param system Système de monde
 * @param x Position X de la tuile
 * @param y Position Y de la tuile
 * @return true si la tuile a été labourée avec succès, false sinon
 */
bool world_system_till_tile(WorldSystem* system, int x, int y);

/**
 * Arrose une tuile
 * @param system Système de monde
 * @param x Position X de la tuile
 * @param y Position Y de la tuile
 * @return true si la tuile a été arrosée avec succès, false sinon
 */
bool world_system_water_tile(WorldSystem* system, int x, int y);

/**
 * Avance le temps dans le monde
 * @param system Système de monde
 * @param minutes Nombre de minutes à avancer
 */
void world_system_advance_time(WorldSystem* system, int minutes);

/**
 * Avance à la journée suivante
 * @param system Système de monde
 */
void world_system_advance_day(WorldSystem* system);

/**
 * Téléporte le joueur à une position
 * @param system Système de monde
 * @param x Position X
 * @param y Position Y
 */
void world_system_teleport_player(WorldSystem* system, float x, float y);

/**
 * Change la zone actuelle
 * @param system Système de monde
 * @param zone_type Type de zone
 * @return true si le changement a réussi, false sinon
 */
bool world_system_change_zone(WorldSystem* system, ZoneType zone_type);

#endif /* WORLD_H */