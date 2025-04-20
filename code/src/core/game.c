/**
 * game.c
 * Implémentation du module principal du jeu
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "../core/game.h"
#include "../utils/error_handler.h"
#include "../systems/render.h"
#include "../systems/world.h"
#include "../systems/entity_manager.h"
#include "../core/phase3_integration.h" // Ajout de l'inclusion pour la phase 3

// Constantes du jeu
#define WINDOW_TITLE "Stardew Valley Clone"
#define DEFAULT_SCREEN_WIDTH 1280
#define DEFAULT_SCREEN_HEIGHT 720
#define INTERNAL_WIDTH 640
#define INTERNAL_HEIGHT 360
#define TARGET_FPS 60
#define MS_PER_FRAME (1000 / TARGET_FPS)

// Initialise le contexte de jeu et tous les sous-systèmes
GameContext* game_init(void) {
    // Allouer et initialiser le contexte de jeu
    GameContext* game = (GameContext*)calloc(1, sizeof(GameContext));
    if (!check_ptr(game, LOG_LEVEL_FATAL, "Échec d'allocation de mémoire pour le contexte de jeu")) {
        return NULL;
    }
    
    // Initialiser les valeurs par défaut
    game->running = true;
    game->screen_width = DEFAULT_SCREEN_WIDTH;
    game->screen_height = DEFAULT_SCREEN_HEIGHT;
    game->last_update_time = 0;
    game->delta_time = 0.0f;
    game->phase3_systems = NULL; // Initialisation du pointeur des systèmes de phase 3
    
    // Initialiser SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        log_fatal("Échec d'initialisation de SDL: %s", SDL_GetError());
        free(game);
        return NULL;
    }
    
    // Configurez SDL pour utiliser le redimensionnement "nearest pixel"
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    
    // Initialiser SDL_image
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        log_fatal("Échec d'initialisation de SDL_image: %s", IMG_GetError());
        SDL_Quit();
        free(game);
        return NULL;
    }
    
    // Créer la fenêtre
    game->window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        game->screen_width, game->screen_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!check_ptr(game->window, LOG_LEVEL_FATAL, "Échec de création de la fenêtre SDL")) {
        IMG_Quit();
        SDL_Quit();
        free(game);
        return NULL;
    }
    
    // Créer le renderer
    game->renderer = SDL_CreateRenderer(
        game->window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    
    if (!check_ptr(game->renderer, LOG_LEVEL_FATAL, "Échec de création du renderer SDL")) {
        SDL_DestroyWindow(game->window);
        IMG_Quit();
        SDL_Quit();
        free(game);
        return NULL;
    }
    
    // Initialiser le système de rendu
    game->render_system = render_system_init(game->renderer);
    if (!check_ptr(game->render_system, LOG_LEVEL_FATAL, "Échec d'initialisation du système de rendu")) {
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(game->window);
        IMG_Quit();
        SDL_Quit();
        free(game);
        return NULL;
    }
    
    // Initialiser le gestionnaire d'entités
    game->entity_manager = entity_manager_init();
    if (!check_ptr(game->entity_manager, LOG_LEVEL_FATAL, "Échec d'initialisation du gestionnaire d'entités")) {
        render_system_shutdown(game->render_system);
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(game->window);
        IMG_Quit();
        SDL_Quit();
        free(game);
        return NULL;
    }
    
    // Initialiser le système de monde
    game->world_system = world_system_init(game->entity_manager);
    if (!check_ptr(game->world_system, LOG_LEVEL_FATAL, "Échec d'initialisation du système de monde")) {
        entity_manager_shutdown(game->entity_manager);
        render_system_shutdown(game->render_system);
        SDL_DestroyRenderer(game->renderer);
        SDL_DestroyWindow(game->window);
        IMG_Quit();
        SDL_Quit();
        free(game);
        return NULL;
    }
    
    // Initialiser une nouvelle partie
    if (!world_system_init_new_game(game->world_system)) {
        log_error("Échec d'initialisation d'une nouvelle partie");
        // Continuer quand même, ce n'est pas une erreur fatale
    }
    
    // Initialiser les systèmes de la phase 3
    game->phase3_systems = phase3_init(game);
    if (!check_ptr(game->phase3_systems, LOG_LEVEL_ERROR, "Échec d'initialisation des systèmes de la phase 3")) {
        // Continuer même en cas d'échec, ce n'est pas fatal
        log_warning("Les fonctionnalités de la phase 3 ne seront pas disponibles");
    }
    
    // Charger les textures de base
    // Note: cette partie serait normalement gérée par un système de ressources,
    // mais pour simplifier, nous le faisons ici pour le prototype.
    
    // Initialiser le temps de la dernière mise à jour
    game->last_update_time = SDL_GetTicks();
    
    log_info("Jeu initialisé avec succès");
    return game;
}

// Vérifie si le jeu est en cours d'exécution
bool game_is_running(GameContext* game) {
    return game && game->running;
}

// Gère les événements d'entrée utilisateur
void game_handle_events(GameContext* game) {
    if (!game) return;
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                game->running = false;
                break;
                
            case SDL_KEYDOWN:
                // Si les systèmes de la phase 3 sont initialisés, leur donner la priorité
                if (game->phase3_systems && phase3_handle_keydown(game->phase3_systems, event.key.keysym.sym)) {
                    // L'événement a été traité par les systèmes de la phase 3
                    break;
                }
                
                // Gérer les touches spéciales
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    game->running = false;
                } else if (event.key.keysym.sym == SDLK_F11) {
                    // Basculer en plein écran
                    static bool is_fullscreen = false;
                    is_fullscreen = !is_fullscreen;
                    
                    if (is_fullscreen) {
                        SDL_SetWindowFullscreen(game->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    } else {
                        SDL_SetWindowFullscreen(game->window, 0);
                    }
                } else if (event.key.keysym.sym == SDLK_F12) {
                    // Basculer le mode debug
                    render_system_set_debug(game->render_system, 
                                          !game->render_system->debug_render);
                }
                
                // Propager l'événement au système de monde
                world_system_handle_keydown(game->world_system, event.key.keysym.sym);
                break;
            
            case SDL_MOUSEBUTTONDOWN:
                // Si les systèmes de la phase 3 sont initialisés, leur donner la priorité
                if (game->phase3_systems && phase3_handle_mousedown(game->phase3_systems, 
                                                                  event.button.button, 
                                                                  event.button.x, 
                                                                  event.button.y)) {
                    // L'événement a été traité par les systèmes de la phase 3
                    break;
                }
                
                // Sinon, traiter normalement (si vous avez un traitement pour les clics de souris)
                break;
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    game->screen_width = event.window.data1;
                    game->screen_height = event.window.data2;
                    render_system_handle_resize(game->render_system, game->screen_width, game->screen_height);
                    
                    log_info("Fenêtre redimensionnée à %dx%d", game->screen_width, game->screen_height);
                }
                break;
        }
    }
}

// Met à jour l'état du jeu
void game_update(GameContext* game) {
    if (!game) return;
    
    // Calculer le delta time
    uint32_t current_time = SDL_GetTicks();
    game->delta_time = (current_time - game->last_update_time) / 1000.0f;
    game->last_update_time = current_time;
    
    // Limiter le delta time à une valeur maximale pour éviter les sauts trop grands
    // pendant le débogage ou les lags
    const float MAX_DELTA_TIME = 0.1f;
    if (game->delta_time > MAX_DELTA_TIME) {
        game->delta_time = MAX_DELTA_TIME;
    }
    
    // Mettre à jour les systèmes
    world_system_update(game->world_system, game->delta_time);
    
    // Mettre à jour les systèmes de la phase 3
    if (game->phase3_systems) {
        phase3_update(game->phase3_systems, game->delta_time);
    }
    
    // Limiter le taux de rafraîchissement
    uint32_t frame_time = SDL_GetTicks() - current_time;
    if (frame_time < MS_PER_FRAME) {
        SDL_Delay(MS_PER_FRAME - frame_time);
    }
}

// Rend le jeu à l'écran
void game_render(GameContext* game) {
    if (!game) return;
    
    // Rendre le monde à travers le système de monde
    // Notre système de rendu avec résolution interne gère maintenant l'effacement et la présentation
    world_system_render(game->world_system, game->render_system);
    
    // Rendre les éléments visuels des systèmes de la phase 3
    if (game->phase3_systems) {
        phase3_render(game->phase3_systems);
    }
}

// Libère toutes les ressources et ferme le jeu
void game_shutdown(GameContext* game) {
    if (!game) return;
    
    log_info("Fermeture du jeu en cours...");
    
    // Libérer les systèmes de la phase 3
    if (game->phase3_systems) {
        phase3_shutdown(game->phase3_systems);
        game->phase3_systems = NULL;
    }
    
    // Libérer les systèmes dans l'ordre inverse d'initialisation
    if (game->world_system) {
        world_system_shutdown(game->world_system);
        game->world_system = NULL;
    }
    
    if (game->entity_manager) {
        entity_manager_shutdown(game->entity_manager);
        game->entity_manager = NULL;
    }
    
    if (game->render_system) {
        render_system_shutdown(game->render_system);
        game->render_system = NULL;
    }
    
    // Nettoyer SDL
    if (game->renderer) {
        SDL_DestroyRenderer(game->renderer);
        game->renderer = NULL;
    }
    
    if (game->window) {
        SDL_DestroyWindow(game->window);
        game->window = NULL;
    }
    
    // Quitter les sous-systèmes
    IMG_Quit();
    SDL_Quit();
    
    // Libérer le contexte
    free(game);
    
    log_info("Jeu fermé avec succès");
}