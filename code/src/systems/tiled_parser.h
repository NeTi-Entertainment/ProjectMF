/**
 * tiled_parser.h
 * Système de chargement et parsage des cartes créées avec Tiled
 */

#ifndef TILED_PARSER_H
#define TILED_PARSER_H

#include <stdbool.h>
#include "../systems/world.h"
#include "../core/resource_manager.h"

// Structure des propriétés personnalisées
typedef struct {
    char* name;
    char* type;
    char* value;
} TiledProperty;

// Structure d'un tileset
typedef struct {
    int firstgid;           // Premier GID du tileset
    char* name;             // Nom du tileset
    char* source;           // Chemin de la source (si externe)
    char* image_source;     // Chemin de l'image
    int tile_width;         // Largeur d'une tuile
    int tile_height;        // Hauteur d'une tuile
    int spacing;            // Espacement entre les tuiles
    int margin;             // Marge autour des tuiles
    int columns;            // Nombre de colonnes
    int image_width;        // Largeur de l'image
    int image_height;       // Hauteur de l'image
    int tile_count;         // Nombre total de tuiles
    int texture_id;         // ID de la texture (après chargement)
    TiledProperty** properties; // Propriétés du tileset
    int property_count;     // Nombre de propriétés
} TiledTileset;

// Structure d'une couche de tuiles
typedef struct {
    char* name;             // Nom de la couche
    int width;              // Largeur en tuiles
    int height;             // Hauteur en tuiles
    float opacity;          // Opacité (0.0-1.0)
    bool visible;           // Visibilité
    int* data;              // Données des tuiles (indices)
    TiledProperty** properties; // Propriétés de la couche
    int property_count;     // Nombre de propriétés
} TiledLayer;

// Structure d'un objet
typedef struct {
    int id;                 // ID de l'objet
    char* name;             // Nom de l'objet
    char* type;             // Type de l'objet
    float x, y;             // Position
    float width, height;    // Dimensions
    float rotation;         // Rotation en degrés
    bool visible;           // Visibilité
    int gid;                // GID (pour les objets tuiles)
    TiledProperty** properties; // Propriétés de l'objet
    int property_count;     // Nombre de propriétés
} TiledObject;

// Structure d'une couche d'objets
typedef struct {
    char* name;             // Nom de la couche
    float opacity;          // Opacité (0.0-1.0)
    bool visible;           // Visibilité
    TiledObject** objects;  // Objets dans la couche
    int object_count;       // Nombre d'objets
    TiledProperty** properties; // Propriétés de la couche
    int property_count;     // Nombre de propriétés
} TiledObjectGroup;

// Structure principale d'une carte Tiled
typedef struct {
    int width;              // Largeur de la carte en tuiles
    int height;             // Hauteur de la carte en tuiles
    int tile_width;         // Largeur d'une tuile en pixels
    int tile_height;        // Hauteur d'une tuile en pixels
    TiledTileset** tilesets;// Tilesets utilisés
    int tileset_count;      // Nombre de tilesets
    TiledLayer** layers;    // Couches de tuiles
    int layer_count;        // Nombre de couches de tuiles
    TiledObjectGroup** object_groups; // Couches d'objets
    int object_group_count; // Nombre de couches d'objets
    TiledProperty** properties; // Propriétés de la carte
    int property_count;     // Nombre de propriétés
} TiledMap;

/**
 * Charge une carte Tiled depuis un fichier JSON
 * @param filename Chemin du fichier JSON
 * @return Pointeur vers la structure TiledMap ou NULL en cas d'erreur
 */
TiledMap* tiled_load_map(const char* filename);

/**
 * Libère les ressources utilisées par une carte Tiled
 * @param map Carte à libérer
 */
void tiled_free_map(TiledMap* map);

/**
 * Convertit une carte Tiled en carte du jeu
 * @param tiled_map Carte Tiled à convertir
 * @param resource_manager Gestionnaire de ressources pour charger les textures
 * @return Pointeur vers la carte du jeu ou NULL en cas d'erreur
 */
Map* tiled_convert_to_game_map(TiledMap* tiled_map, ResourceManager* resource_manager);

/**
 * Crée des entités à partir des objets d'une carte Tiled
 * @param tiled_map Carte Tiled source
 * @param entity_manager Gestionnaire d'entités
 * @param origin_x Décalage X (position de la carte)
 * @param origin_y Décalage Y (position de la carte)
 * @return Nombre d'entités créées ou -1 en cas d'erreur
 */
int tiled_create_entities(TiledMap* tiled_map, EntityManager* entity_manager, float origin_x, float origin_y);

/**
 * Obtient une propriété par son nom
 * @param properties Tableau de propriétés
 * @param property_count Nombre de propriétés
 * @param name Nom de la propriété recherchée
 * @return Pointeur vers la propriété ou NULL si non trouvée
 */
TiledProperty* tiled_get_property(TiledProperty** properties, int property_count, const char* name);

/**
 * Charge des tilesets externes
 * @param tiled_map Carte Tiled
 * @param base_path Chemin de base pour les chemins relatifs
 * @return true si chargement réussi, false sinon
 */
bool tiled_load_external_tilesets(TiledMap* tiled_map, const char* base_path);

#endif /* TILED_PARSER_H */
