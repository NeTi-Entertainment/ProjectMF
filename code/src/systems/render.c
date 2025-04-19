/**
 * render.c
 * Implémentation du système de rendu
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "../systems/render.h"
#include "../utils/error_handler.h"

// Capacité initiale du tableau de textures
#define INITIAL_TEXTURE_CAPACITY 32
#define DEFAULT_TILE_SIZE 32

// Résolution interne fixe
#define INTERNAL_WIDTH 640
#define INTERNAL_HEIGHT 360

// Initialise le système de rendu
RenderSystem* render_system_init(SDL_Renderer* renderer) {
    if (!check_ptr(renderer, LOG_LEVEL_ERROR, "Renderer NULL passé à render_system_init")) {
        return NULL;
    }

    // Configurer SDL pour utiliser un redimensionnement sans filtrage
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");  // 0 = nearest pixel sampling

    RenderSystem* system = (RenderSystem*)calloc(1, sizeof(RenderSystem));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système de rendu")) {
        return NULL;
    }

    system->renderer = renderer;
    
    // Initialiser les paramètres de résolution
    system->internal_width = INTERNAL_WIDTH;
    system->internal_height = INTERNAL_HEIGHT;
    
    // Récupérer les dimensions actuelles de la fenêtre
    SDL_GetRendererOutputSize(renderer, &system->screen_width, &system->screen_height);
    
    // Calculer les facteurs d'échelle
    system->scale_factor_x = (float)system->screen_width / (float)system->internal_width;
    system->scale_factor_y = (float)system->screen_height / (float)system->internal_height;
    
    // Créer la texture cible pour le rendu interne
    system->render_target = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        system->internal_width,
        system->internal_height
    );
    
    if (!check_ptr(system->render_target, LOG_LEVEL_ERROR, 
                 "Échec de création de la texture cible pour le rendu interne: %s", SDL_GetError())) {
        free(system);
        return NULL;
    }
    
    // Autoriser le blending alpha sur la texture
    SDL_SetTextureBlendMode(system->render_target, SDL_BLENDMODE_BLEND);
    
    // Allouer les tableaux de textures
    system->texture_capacity = INITIAL_TEXTURE_CAPACITY;
    system->textures = (SDL_Texture**)calloc(system->texture_capacity, sizeof(SDL_Texture*));
    if (!check_ptr(system->textures, LOG_LEVEL_ERROR, "Échec d'allocation du tableau de textures")) {
        SDL_DestroyTexture(system->render_target);
        free(system);
        return NULL;
    }
    
    system->texture_paths = (char**)calloc(system->texture_capacity, sizeof(char*));
    if (!check_ptr(system->texture_paths, LOG_LEVEL_ERROR, "Échec d'allocation du tableau de chemins de textures")) {
        SDL_DestroyTexture(system->render_target);
        free(system->textures);
        free(system);
        return NULL;
    }
    
    system->texture_count = 0;
    
    // Initialiser les paramètres de caméra
    system->camera_x = 0.0f;
    system->camera_y = 0.0f;
    system->camera_zoom = 1.0f;
    
    // Initialiser la taille des tuiles
    system->tile_size = DEFAULT_TILE_SIZE;
    
    // Désactiver le mode debug par défaut
    system->debug_render = false;
    
    log_info("Système de rendu initialisé avec succès (résolution interne: %dx%d, fenêtre: %dx%d)",
           system->internal_width, system->internal_height, 
           system->screen_width, system->screen_height);
    return system;
}

// Libère les ressources du système de rendu
void render_system_shutdown(RenderSystem* system) {
    if (!system) return;
    
    // Libérer la texture cible
    if (system->render_target) {
        SDL_DestroyTexture(system->render_target);
        system->render_target = NULL;
    }
    
    // Libérer les textures
    if (system->textures) {
        for (int i = 0; i < system->texture_count; i++) {
            if (system->textures[i]) {
                SDL_DestroyTexture(system->textures[i]);
                system->textures[i] = NULL;
            }
        }
        free(system->textures);
        system->textures = NULL;
    }
    
    // Libérer les chemins des textures
    if (system->texture_paths) {
        for (int i = 0; i < system->texture_count; i++) {
            if (system->texture_paths[i]) {
                free(system->texture_paths[i]);
                system->texture_paths[i] = NULL;
            }
        }
        free(system->texture_paths);
        system->texture_paths = NULL;
    }
    
    // Libérer le système lui-même
    free(system);
    
    log_info("Système de rendu libéré");
}

// Commence une nouvelle frame de rendu
void render_system_begin_frame(RenderSystem* system) {
    if (!system) return;
    
    // Rediriger le rendu vers notre texture cible interne
    SDL_SetRenderTarget(system->renderer, system->render_target);
    
    // Effacer la texture cible
    SDL_SetRenderDrawColor(system->renderer, 0, 0, 0, 255);
    SDL_RenderClear(system->renderer);
}

// Termine la frame de rendu et l'affiche à l'écran
void render_system_end_frame(RenderSystem* system) {
    if (!system) return;
    
    // Rétablir le rendu vers l'écran
    SDL_SetRenderTarget(system->renderer, NULL);
    
    // Effacer l'écran
    SDL_SetRenderDrawColor(system->renderer, 0, 0, 0, 255);
    SDL_RenderClear(system->renderer);
    
    // Définir la destination à la taille de l'écran
    SDL_Rect dest = {
        0, 0, 
        system->screen_width, 
        system->screen_height
    };
    
    // Copier notre texture cible vers l'écran avec mise à l'échelle
    SDL_RenderCopy(system->renderer, system->render_target, NULL, &dest);
    
    // Afficher le résultat
    SDL_RenderPresent(system->renderer);
}

// Gère le redimensionnement de la fenêtre
void render_system_handle_resize(RenderSystem* system, int width, int height) {
    if (!system) return;
    
    system->screen_width = width;
    system->screen_height = height;
    
    // Recalculer les facteurs d'échelle
    system->scale_factor_x = (float)system->screen_width / (float)system->internal_width;
    system->scale_factor_y = (float)system->screen_height / (float)system->internal_height;
    
    log_debug("Fenêtre redimensionnée à %dx%d (facteurs d'échelle: %.2f, %.2f)", 
            width, height, system->scale_factor_x, system->scale_factor_y);
}

// Charge une texture depuis un fichier
int render_system_load_texture(RenderSystem* system, const char* path) {
    if (!system || !path) return -1;
    
    // Vérifier si la texture est déjà chargée
    for (int i = 0; i < system->texture_count; i++) {
        if (system->texture_paths[i] && strcmp(system->texture_paths[i], path) == 0) {
            return i; // Retourner l'ID de la texture existante
        }
    }
    
    // Vérifier si nous avons besoin d'agrandir le tableau
    if (system->texture_count >= system->texture_capacity) {
        int new_capacity = system->texture_capacity * 2;
        
        // Réallouer le tableau de textures
        SDL_Texture** new_textures = (SDL_Texture**)realloc(
            system->textures, new_capacity * sizeof(SDL_Texture*)
        );
        if (!check_ptr(new_textures, LOG_LEVEL_ERROR, "Échec de réallocation du tableau de textures")) {
            return -1;
        }
        system->textures = new_textures;
        
        // Réallouer le tableau de chemins
        char** new_paths = (char**)realloc(
            system->texture_paths, new_capacity * sizeof(char*)
        );
        if (!check_ptr(new_paths, LOG_LEVEL_ERROR, "Échec de réallocation du tableau de chemins")) {
            return -1;
        }
        system->texture_paths = new_paths;
        
        // Initialiser les nouveaux éléments
        for (int i = system->texture_capacity; i < new_capacity; i++) {
            system->textures[i] = NULL;
            system->texture_paths[i] = NULL;
        }
        
        system->texture_capacity = new_capacity;
    }
    
    // Charger la nouvelle texture
    SDL_Texture* texture = IMG_LoadTexture(system->renderer, path);
    if (!check_ptr(texture, LOG_LEVEL_ERROR, "Échec du chargement de la texture %s: %s", 
                 path, IMG_GetError())) {
        return -1;
    }
    
    // Stocker la texture et son chemin
    int texture_id = system->texture_count;
    system->textures[texture_id] = texture;
    
    system->texture_paths[texture_id] = (char*)malloc(strlen(path) + 1);
    if (!check_ptr(system->texture_paths[texture_id], LOG_LEVEL_ERROR, "Échec d'allocation du chemin de texture")) {
        SDL_DestroyTexture(texture);
        return -1;
    }
    
    strcpy(system->texture_paths[texture_id], path);
    system->texture_count++;
    
    log_debug("Texture chargée: %s (ID: %d)", path, texture_id);
    return texture_id;
}

// Récupère une texture par son ID
SDL_Texture* render_system_get_texture(RenderSystem* system, int texture_id) {
    if (!system || texture_id < 0 || texture_id >= system->texture_count) {
        log_warning("Tentative d'accès à une texture invalide (ID: %d)", texture_id);
        return NULL;
    }
    
    return system->textures[texture_id];
}

// Centre la caméra sur une position
void render_system_center_camera(RenderSystem* system, float x, float y) {
    if (!system) return;
    
    system->camera_x = x;
    system->camera_y = y;
}

// Convertit des coordonnées monde en coordonnées écran internes
void render_system_world_to_screen(RenderSystem* system, float world_x, float world_y, 
                                  int* screen_x, int* screen_y) {
    if (!system || !screen_x || !screen_y) return;
    
    // Appliquer le décalage de la caméra
    float cam_rel_x = (world_x - system->camera_x) * system->camera_zoom;
    float cam_rel_y = (world_y - system->camera_y) * system->camera_zoom;
    
    // Convertir en coordonnées écran internes (origine au centre)
    *screen_x = (int)(system->internal_width / 2 + cam_rel_x);
    *screen_y = (int)(system->internal_height / 2 + cam_rel_y);
}

// Convertit des coordonnées écran en coordonnées monde
void render_system_screen_to_world(RenderSystem* system, int screen_x, int screen_y,
                                  float* world_x, float* world_y) {
    if (!system || !world_x || !world_y) return;
    
    // Convertir les coordonnées écran en coordonnées internes
    float internal_x = screen_x / system->scale_factor_x;
    float internal_y = screen_y / system->scale_factor_y;
    
    // Convertir en coordonnées relatives à la caméra
    float cam_rel_x = (internal_x - system->internal_width / 2) / system->camera_zoom;
    float cam_rel_y = (internal_y - system->internal_height / 2) / system->camera_zoom;
    
    // Appliquer le décalage de la caméra
    *world_x = cam_rel_x + system->camera_x;
    *world_y = cam_rel_y + system->camera_y;
}

// Définit le niveau de zoom de la caméra
void render_system_set_zoom(RenderSystem* system, float zoom) {
    if (!system) return;
    
    // Limiter le zoom à des valeurs raisonnables
    const float MIN_ZOOM = 0.1f;
    const float MAX_ZOOM = 5.0f;
    
    if (zoom < MIN_ZOOM) zoom = MIN_ZOOM;
    if (zoom > MAX_ZOOM) zoom = MAX_ZOOM;
    
    system->camera_zoom = zoom;
}

// Active ou désactive l'affichage des informations de debug
void render_system_set_debug(RenderSystem* system, bool debug_render) {
    if (!system) return;
    
    system->debug_render = debug_render;
}

// Dessine un sprite
void render_system_draw_sprite(RenderSystem* system, int texture_id, 
                              float x, float y, 
                              int width, int height,
                              int src_x, int src_y,
                              int src_width, int src_height,
                              float rotation,
                              float scale_x, float scale_y) {
    if (!system) return;
    
    SDL_Texture* texture = render_system_get_texture(system, texture_id);
    if (!texture) return;
    
    // Convertir les coordonnées monde en coordonnées écran internes
    int screen_x, screen_y;
    render_system_world_to_screen(system, x, y, &screen_x, &screen_y);
    
    // Appliquer l'échelle et le zoom de la caméra aux dimensions
    int scaled_width = (int)(width * scale_x * system->camera_zoom);
    int scaled_height = (int)(height * scale_y * system->camera_zoom);
    
    // Définir les rectangles source et destination
    SDL_Rect src_rect = {
        src_x, src_y,
        src_width, src_height
    };
    
    SDL_Rect dest_rect = {
        screen_x - scaled_width / 2, // Centrer sur la position
        screen_y - scaled_height / 2,
        scaled_width,
        scaled_height
    };
    
    // Dessiner la texture avec rotation
    SDL_RenderCopyEx(
        system->renderer,
        texture,
        &src_rect,
        &dest_rect,
        rotation,        // Angle en degrés
        NULL,            // Point de rotation (centre par défaut)
        SDL_FLIP_NONE    // Pas de retournement
    );
    
    // Dessiner le cadre de debug si nécessaire
    if (system->debug_render) {
        SDL_SetRenderDrawColor(system->renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(system->renderer, &dest_rect);
        
        // Dessiner un point au centre
        SDL_Rect center_point = {
            screen_x - 1, screen_y - 1,
            3, 3
        };
        SDL_RenderFillRect(system->renderer, &center_point);
    }
}

// Dessine un rectangle
void render_system_draw_rect(RenderSystem* system, 
                           float x, float y, 
                           float width, float height,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                           bool filled) {
    if (!system) return;
    
    // Convertir les coordonnées monde en coordonnées écran internes
    int screen_x, screen_y;
    render_system_world_to_screen(system, x, y, &screen_x, &screen_y);
    
    // Appliquer le zoom de la caméra aux dimensions
    int scaled_width = (int)(width * system->camera_zoom);
    int scaled_height = (int)(height * system->camera_zoom);
    
    // Définir le rectangle destination
    SDL_Rect dest_rect = {
        screen_x - scaled_width / 2, // Centrer sur la position
        screen_y - scaled_height / 2,
        scaled_width,
        scaled_height
    };
    
    // Définir la couleur
    SDL_SetRenderDrawColor(system->renderer, r, g, b, a);
    
    // Dessiner le rectangle
    if (filled) {
        SDL_RenderFillRect(system->renderer, &dest_rect);
    } else {
        SDL_RenderDrawRect(system->renderer, &dest_rect);
    }
}

// Dessine une ligne
void render_system_draw_line(RenderSystem* system,
                           float x1, float y1,
                           float x2, float y2,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!system) return;
    
    // Convertir les coordonnées monde en coordonnées écran internes
    int screen_x1, screen_y1, screen_x2, screen_y2;
    render_system_world_to_screen(system, x1, y1, &screen_x1, &screen_y1);
    render_system_world_to_screen(system, x2, y2, &screen_x2, &screen_y2);
    
    // Définir la couleur
    SDL_SetRenderDrawColor(system->renderer, r, g, b, a);
    
    // Dessiner la ligne
    SDL_RenderDrawLine(system->renderer, screen_x1, screen_y1, screen_x2, screen_y2);
}

// Dessine du texte (version basique sans SDL_ttf, à améliorer plus tard)
void render_system_draw_text(RenderSystem* system,
                           const char* text,
                           float x, float y,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!system || !text) return;
    
    // Cette fonction est une ébauche qui devrait être implémentée avec SDL_ttf
    // Pour l'instant, on peut afficher un rectangle à la place du texte
    
    int text_length = strlen(text);
    float text_width = text_length * 8.0f; // Approximation simple
    float text_height = 16.0f;
    
    // Afficher un rectangle représentant le texte
    if (system->debug_render) {
        render_system_draw_rect(system, x, y, text_width, text_height, r, g, b, a, false);
        
        log_debug("Texte non rendu (SDL_ttf non implémenté): %s", text);
    }
    
    // Note: Implémenter cette fonction correctement avec SDL_ttf plus tard
}
