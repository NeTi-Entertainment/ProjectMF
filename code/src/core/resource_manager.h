/**
 * resource_manager.h
 * Système de gestion des ressources (textures, animations, sons, etc.)
 */

#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include "../core/entity.h"

// Structure pour stocker les données d'une texture
typedef struct {
    SDL_Texture* texture;
    char* path;
    int width;
    int height;
    bool is_loaded;
} TextureResource;

// Structure pour stocker les données d'une animation
typedef struct {
    Animation animation;
    char* name;
    bool is_loaded;
} AnimationResource;

// Gestionnaire de ressources
typedef struct {
    SDL_Renderer* renderer;

    // Gestion des textures
    TextureResource* textures;
    int texture_count;
    int texture_capacity;

    // Gestion des animations
    AnimationResource* animations;
    int animation_count;
    int animation_capacity;

    // Caches pour accès rapide (pourraient être remplacés par des tables de hachage)
    int texture_lookup_cache[256]; // Cache simple pour les chemins fréquents
} ResourceManager;

/**
 * Initialise le gestionnaire de ressources
 * @param renderer Renderer SDL
 * @return Pointeur vers le gestionnaire de ressources ou NULL en cas d'erreur
 */
ResourceManager* resource_manager_init(SDL_Renderer* renderer);

/**
 * Libère le gestionnaire de ressources
 * @param manager Gestionnaire de ressources
 */
void resource_manager_shutdown(ResourceManager* manager);

/**
 * Charge une texture, ou renvoie l'ID si déjà chargée
 * @param manager Gestionnaire de ressources
 * @param path Chemin du fichier
 * @return ID de la texture ou -1 si erreur
 */
int resource_load_texture(ResourceManager* manager, const char* path);

/**
 * Récupère une texture par ID
 * @param manager Gestionnaire de ressources
 * @param id ID de la texture
 * @return Pointeur vers la texture ou NULL
 */
SDL_Texture* resource_get_texture(ResourceManager* manager, int id);

/**
 * Récupère les dimensions d'une texture
 * @param manager Gestionnaire de ressources
 * @param id ID de la texture
 * @param width Pointeur pour stocker la largeur (peut être NULL)
 * @param height Pointeur pour stocker la hauteur (peut être NULL)
 * @return true si la texture existe, false sinon
 */
bool resource_get_texture_size(ResourceManager* manager, int id, int* width, int* height);

/**
 * Crée une animation à partir d'une texture
 * @param manager Gestionnaire de ressources
 * @param name Nom de l'animation
 * @param texture_id ID de la texture
 * @param frame_width Largeur d'une frame
 * @param frame_height Hauteur d'une frame
 * @param frames_per_row Nombre de frames par ligne
 * @param frame_count Nombre total de frames
 * @param frame_duration Durée d'une frame en secondes
 * @return ID de l'animation ou -1 si erreur
 */
int resource_create_animation(
    ResourceManager* manager,
    const char* name,
    int texture_id,
    int frame_width,
    int frame_height,
    int frames_per_row,
    int frame_count,
    float frame_duration
);

/**
 * Récupère une animation par ID
 * @param manager Gestionnaire de ressources
 * @param id ID de l'animation
 * @return Pointeur vers l'animation ou NULL
 */
Animation* resource_get_animation(ResourceManager* manager, int id);

/**
 * Trouve une animation par nom
 * @param manager Gestionnaire de ressources
 * @param name Nom de l'animation
 * @return ID de l'animation ou -1 si non trouvée
 */
int resource_find_animation(ResourceManager* manager, const char* name);

#endif /* RESOURCE_MANAGER_H */