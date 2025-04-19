/**
 * render.h
 * Système de rendu graphique
 */

#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

// Structure du système de rendu
typedef struct {
    SDL_Renderer* renderer;    // Renderer SDL
    SDL_Texture* render_target; // Texture cible pour le rendu interne
    
    // Tableau de textures chargées
    SDL_Texture** textures;
    char** texture_paths;      // Chemins des textures (pour le rechargement et le debug)
    int texture_count;
    int texture_capacity;
    
    // Paramètres de caméra
    float camera_x;
    float camera_y;
    float camera_zoom;
    
    // Paramètres de rendu
    int screen_width;          // Largeur de la fenêtre d'affichage
    int screen_height;         // Hauteur de la fenêtre d'affichage
    int internal_width;        // Largeur de la résolution interne (fixe)
    int internal_height;       // Hauteur de la résolution interne (fixe)
    float scale_factor_x;      // Facteur d'échelle horizontal (screen/internal)
    float scale_factor_y;      // Facteur d'échelle vertical (screen/internal)
    int tile_size;             // Taille d'une tuile en pixels
    bool debug_render;         // Affichage des informations de debug
} RenderSystem;

/**
 * Initialise le système de rendu
 * @param renderer Renderer SDL initialisé
 * @return Pointeur vers le système de rendu ou NULL en cas d'erreur
 */
RenderSystem* render_system_init(SDL_Renderer* renderer);

/**
 * Libère les ressources du système de rendu
 * @param system Système de rendu à libérer
 */
void render_system_shutdown(RenderSystem* system);

/**
 * Commence une nouvelle frame de rendu
 * @param system Système de rendu
 */
void render_system_begin_frame(RenderSystem* system);

/**
 * Termine la frame de rendu et l'affiche à l'écran
 * @param system Système de rendu
 */
void render_system_end_frame(RenderSystem* system);

/**
 * Gère le redimensionnement de la fenêtre
 * @param system Système de rendu
 * @param width Nouvelle largeur
 * @param height Nouvelle hauteur
 */
void render_system_handle_resize(RenderSystem* system, int width, int height);

/**
 * Charge une texture depuis un fichier
 * @param system Système de rendu
 * @param path Chemin vers le fichier
 * @return ID de la texture ou -1 en cas d'erreur
 */
int render_system_load_texture(RenderSystem* system, const char* path);

/**
 * Récupère une texture par son ID
 * @param system Système de rendu
 * @param texture_id ID de la texture
 * @return Pointeur vers la texture ou NULL si non trouvée
 */
SDL_Texture* render_system_get_texture(RenderSystem* system, int texture_id);

/**
 * Centre la caméra sur une position
 * @param system Système de rendu
 * @param x Position X
 * @param y Position Y
 */
void render_system_center_camera(RenderSystem* system, float x, float y);

/**
 * Convertit des coordonnées monde en coordonnées écran internes
 * @param system Système de rendu
 * @param world_x Position X monde
 * @param world_y Position Y monde
 * @param screen_x Pointeur pour stocker la position X écran
 * @param screen_y Pointeur pour stocker la position Y écran
 */
void render_system_world_to_screen(RenderSystem* system, float world_x, float world_y, 
                                  int* screen_x, int* screen_y);

/**
 * Convertit des coordonnées écran en coordonnées monde
 * @param system Système de rendu
 * @param screen_x Position X écran
 * @param screen_y Position Y écran
 * @param world_x Pointeur pour stocker la position X monde
 * @param world_y Pointeur pour stocker la position Y monde
 */
void render_system_screen_to_world(RenderSystem* system, int screen_x, int screen_y,
                                  float* world_x, float* world_y);

/**
 * Définit le niveau de zoom de la caméra
 * @param system Système de rendu
 * @param zoom Niveau de zoom (1.0 = normal)
 */
void render_system_set_zoom(RenderSystem* system, float zoom);

/**
 * Active ou désactive l'affichage des informations de debug
 * @param system Système de rendu
 * @param debug_render true pour activer, false pour désactiver
 */
void render_system_set_debug(RenderSystem* system, bool debug_render);

/**
 * Dessine un sprite
 * @param system Système de rendu
 * @param texture_id ID de la texture
 * @param x Position X monde
 * @param y Position Y monde
 * @param width Largeur
 * @param height Hauteur
 * @param src_x Position X source dans la texture
 * @param src_y Position Y source dans la texture
 * @param src_width Largeur source
 * @param src_height Hauteur source
 * @param rotation Rotation en degrés
 * @param scale_x Échelle X
 * @param scale_y Échelle Y
 */
void render_system_draw_sprite(RenderSystem* system, int texture_id, 
                              float x, float y, 
                              int width, int height,
                              int src_x, int src_y,
                              int src_width, int src_height,
                              float rotation,
                              float scale_x, float scale_y);

/**
 * Dessine un rectangle
 * @param system Système de rendu
 * @param x Position X monde
 * @param y Position Y monde
 * @param width Largeur
 * @param height Hauteur
 * @param r Composante rouge (0-255)
 * @param g Composante verte (0-255)
 * @param b Composante bleue (0-255)
 * @param a Composante alpha (0-255)
 * @param filled true pour un rectangle plein, false pour un contour
 */
void render_system_draw_rect(RenderSystem* system, 
                            float x, float y, 
                            float width, float height,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a,
                            bool filled);

/**
 * Dessine une ligne
 * @param system Système de rendu
 * @param x1 Position X début
 * @param y1 Position Y début
 * @param x2 Position X fin
 * @param y2 Position Y fin
 * @param r Composante rouge (0-255)
 * @param g Composante verte (0-255)
 * @param b Composante bleue (0-255)
 * @param a Composante alpha (0-255)
 */
void render_system_draw_line(RenderSystem* system,
                            float x1, float y1,
                            float x2, float y2,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * Dessine du texte (à implémenter avec SDL_ttf)
 * @param system Système de rendu
 * @param text Texte à afficher
 * @param x Position X monde
 * @param y Position Y monde
 * @param r Composante rouge (0-255)
 * @param g Composante verte (0-255)
 * @param b Composante bleue (0-255)
 * @param a Composante alpha (0-255)
 */
void render_system_draw_text(RenderSystem* system,
                            const char* text,
                            float x, float y,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#endif /* RENDER_H */