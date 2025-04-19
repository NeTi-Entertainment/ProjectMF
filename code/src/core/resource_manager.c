/**
 * resource_manager.c
 * Implémentation du système de gestion des ressources
 */

#include <stdlib.h>
#include <string.h>
#include "../core/resource_manager.h"
#include "../utils/error_handler.h"

#define INITIAL_RESOURCE_CAPACITY 32

// Initialise le gestionnaire de ressources
ResourceManager* resource_manager_init(SDL_Renderer* renderer) {
    if (!check_ptr(renderer, LOG_LEVEL_ERROR, "Renderer NULL passé à resource_manager_init")) {
        return NULL;
    }

    ResourceManager* manager = (ResourceManager*)calloc(1, sizeof(ResourceManager));
    if (!check_ptr(manager, LOG_LEVEL_ERROR, "Échec d'allocation du gestionnaire de ressources")) {
        return NULL;
    }

    manager->renderer = renderer;
    
    // Initialiser le tableau de textures
    manager->texture_capacity = INITIAL_RESOURCE_CAPACITY;
    manager->textures = (TextureResource*)calloc(manager->texture_capacity, sizeof(TextureResource));
    if (!check_ptr(manager->textures, LOG_LEVEL_ERROR, "Échec d'allocation du tableau de textures")) {
        free(manager);
        return NULL;
    }
    
    // Initialiser le tableau d'animations
    manager->animation_capacity = INITIAL_RESOURCE_CAPACITY;
    manager->animations = (AnimationResource*)calloc(manager->animation_capacity, sizeof(AnimationResource));
    if (!check_ptr(manager->animations, LOG_LEVEL_ERROR, "Échec d'allocation du tableau d'animations")) {
        free(manager->textures);
        free(manager);
        return NULL;
    }
    
    // Initialiser le cache (valeurs -1 indiquent "non utilisé")
    for (int i = 0; i < 256; i++) {
        manager->texture_lookup_cache[i] = -1;
    }
    
    log_info("Gestionnaire de ressources initialisé avec succès");
    return manager;
}

// Libère le gestionnaire de ressources
void resource_manager_shutdown(ResourceManager* manager) {
    if (!manager) return;
    
    // Libérer les textures
    if (manager->textures) {
        for (int i = 0; i < manager->texture_count; i++) {
            if (manager->textures[i].texture) {
                SDL_DestroyTexture(manager->textures[i].texture);
                manager->textures[i].texture = NULL;
            }
            
            if (manager->textures[i].path) {
                free(manager->textures[i].path);
                manager->textures[i].path = NULL;
            }
        }
        free(manager->textures);
        manager->textures = NULL;
    }
    
    // Libérer les animations
    if (manager->animations) {
        for (int i = 0; i < manager->animation_count; i++) {
            if (manager->animations[i].name) {
                free(manager->animations[i].name);
                manager->animations[i].name = NULL;
            }
        }
        free(manager->animations);
        manager->animations = NULL;
    }
    
    // Libérer le gestionnaire lui-même
    free(manager);
    
    log_info("Gestionnaire de ressources libéré");
}

/**
 * Fonction utilitaire pour calculer un hachage simple
 * @param str Chaîne à hacher
 * @return Valeur de hachage entre 0 et 255
 */
static unsigned char simple_hash(const char* str) {
    unsigned char hash = 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        hash = hash * 31 + str[i];
    }
    
    return hash;
}

// Charge une texture
int resource_load_texture(ResourceManager* manager, const char* path) {
    if (!manager || !path) return -1;
    
    // Vérifier le cache pour un accès rapide
    unsigned char hash = simple_hash(path);
    if (manager->texture_lookup_cache[hash] >= 0) {
        int cached_id = manager->texture_lookup_cache[hash];
        if (strcmp(manager->textures[cached_id].path, path) == 0) {
            return cached_id; // Cache hit
        }
    }
    
    // Vérifier si la texture est déjà chargée
    for (int i = 0; i < manager->texture_count; i++) {
        if (manager->textures[i].path && strcmp(manager->textures[i].path, path) == 0) {
            // Mettre à jour le cache
            manager->texture_lookup_cache[hash] = i;
            return i; // Retourner l'ID de la texture existante
        }
    }
    
    // Vérifier si nous avons besoin d'agrandir le tableau
    if (manager->texture_count >= manager->texture_capacity) {
        int new_capacity = manager->texture_capacity * 2;
        
        // Réallouer le tableau de textures
        TextureResource* new_textures = (TextureResource*)realloc(
            manager->textures, new_capacity * sizeof(TextureResource)
        );
        if (!check_ptr(new_textures, LOG_LEVEL_ERROR, "Échec de réallocation du tableau de textures")) {
            return -1;
        }
        manager->textures = new_textures;
        
        // Initialiser les nouveaux éléments
        for (int i = manager->texture_capacity; i < new_capacity; i++) {
            manager->textures[i].texture = NULL;
            manager->textures[i].path = NULL;
            manager->textures[i].width = 0;
            manager->textures[i].height = 0;
            manager->textures[i].is_loaded = false;
        }
        
        manager->texture_capacity = new_capacity;
    }
    
    // Charger la nouvelle texture
    SDL_Texture* texture = IMG_LoadTexture(manager->renderer, path);
    if (!check_ptr(texture, LOG_LEVEL_ERROR, "Échec du chargement de la texture %s: %s", 
                 path, IMG_GetError())) {
        return -1;
    }
    
    // Obtenir les dimensions de la texture
    int width, height;
    SDL_QueryTexture(texture, NULL, NULL, &width, &height);
    
    // Stocker la texture
    int texture_id = manager->texture_count;
    manager->textures[texture_id].texture = texture;
    manager->textures[texture_id].width = width;
    manager->textures[texture_id].height = height;
    manager->textures[texture_id].is_loaded = true;
    
    // Stocker le chemin
    manager->textures[texture_id].path = (char*)malloc(strlen(path) + 1);
    if (!check_ptr(manager->textures[texture_id].path, LOG_LEVEL_ERROR, "Échec d'allocation du chemin de texture")) {
        SDL_DestroyTexture(texture);
        return -1;
    }
    strcpy(manager->textures[texture_id].path, path);
    
    // Mettre à jour le cache
    manager->texture_lookup_cache[hash] = texture_id;
    
    // Incrémenter le compteur
    manager->texture_count++;
    
    log_debug("Texture chargée: %s (ID: %d, %dx%d)", path, texture_id, width, height);
    return texture_id;
}

// Récupère une texture par ID
SDL_Texture* resource_get_texture(ResourceManager* manager, int id) {
    if (!manager || id < 0 || id >= manager->texture_count) {
        log_warning("Tentative d'accès à une texture invalide (ID: %d)", id);
        return NULL;
    }
    
    return manager->textures[id].texture;
}

// Récupère les dimensions d'une texture
bool resource_get_texture_size(ResourceManager* manager, int id, int* width, int* height) {
    if (!manager || id < 0 || id >= manager->texture_count) {
        log_warning("Tentative d'accès à une texture invalide (ID: %d)", id);
        return false;
    }
    
    if (width) *width = manager->textures[id].width;
    if (height) *height = manager->textures[id].height;
    
    return true;
}

// Crée une animation à partir d'une texture
int resource_create_animation(
    ResourceManager* manager,
    const char* name,
    int texture_id,
    int frame_width,
    int frame_height,
    int frames_per_row,
    int frame_count,
    float frame_duration
) {
    if (!manager || !name || texture_id < 0 || texture_id >= manager->texture_count) {
        return -1;
    }
    
    // Vérifier si l'animation existe déjà
    for (int i = 0; i < manager->animation_count; i++) {
        if (manager->animations[i].name && strcmp(manager->animations[i].name, name) == 0) {
            return i; // Animation déjà créée
        }
    }
    
    // Vérifier si nous devons agrandir le tableau
    if (manager->animation_count >= manager->animation_capacity) {
        int new_capacity = manager->animation_capacity * 2;
        
        // Réallouer le tableau d'animations
        AnimationResource* new_animations = (AnimationResource*)realloc(
            manager->animations, new_capacity * sizeof(AnimationResource)
        );
        if (!check_ptr(new_animations, LOG_LEVEL_ERROR, "Échec de réallocation du tableau d'animations")) {
            return -1;
        }
        manager->animations = new_animations;
        
        // Initialiser les nouveaux éléments
        for (int i = manager->animation_capacity; i < new_capacity; i++) {
            manager->animations[i].name = NULL;
            manager->animations[i].is_loaded = false;
        }
        
        manager->animation_capacity = new_capacity;
    }
    
    // Créer la nouvelle animation
    int animation_id = manager->animation_count;
    
    // Initialiser les propriétés de l'animation
    manager->animations[animation_id].animation.texture_id = texture_id;
    manager->animations[animation_id].animation.frame_width = frame_width;
    manager->animations[animation_id].animation.frame_height = frame_height;
    manager->animations[animation_id].animation.frames_per_row = frames_per_row;
    manager->animations[animation_id].animation.frame_count = frame_count;
    manager->animations[animation_id].animation.frame_duration = frame_duration;
    
    // Stocker le nom
    manager->animations[animation_id].name = (char*)malloc(strlen(name) + 1);
    if (!check_ptr(manager->animations[animation_id].name, LOG_LEVEL_ERROR, "Échec d'allocation du nom d'animation")) {
        return -1;
    }
    strcpy(manager->animations[animation_id].name, name);
    
    manager->animations[animation_id].is_loaded = true;
    manager->animation_count++;
    
    log_debug("Animation créée: %s (ID: %d, frames: %d)", name, animation_id, frame_count);
    return animation_id;
}

// Récupère une animation par ID
Animation* resource_get_animation(ResourceManager* manager, int id) {
    if (!manager || id < 0 || id >= manager->animation_count) {
        log_warning("Tentative d'accès à une animation invalide (ID: %d)", id);
        return NULL;
    }
    
    return &manager->animations[id].animation;
}

// Trouve une animation par nom
int resource_find_animation(ResourceManager* manager, const char* name) {
    if (!manager || !name) return -1;
    
    for (int i = 0; i < manager->animation_count; i++) {
        if (manager->animations[i].name && strcmp(manager->animations[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;
}