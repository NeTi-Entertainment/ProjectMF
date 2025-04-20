/*
 * game.h
 * Definit la structure principale du jeu et ses fonctions associees
*/

#ifndef GAME_H
# define GAME_H

# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>
# include <SDL2/SDL.h>
# include "../systems/render.h"
# include "../systems/world.h"
# include "../systems/entity_manager.h"

// Forward declaration pour éviter les inclusions circulaires
typedef struct Phase3Systems Phase3Systems;

//Structure de contexte du jeu contenant tous les sous-systemes
typedef struct GameContext
{
	bool			running;		//etat d'execution du jeu
	uint32_t		last_update_time;//tmp last update pour delta time
	float			delta_time;		//tmp ecoule depuis la derniere frame en sec
	SDL_Window*		window;			//fentre SDL
	SDL_Renderer*	renderer;		//renderer SDL
	int				screen_width;	//largeur de l'ecran
	int				screen_height;	//hauteur de l'ecran
	RenderSystem*	render_system;	//systeme de rendu
	WorldSystem*	world_system;	//systeme de gestion du monde
	EntityManager*	entity_manager;	//gestionnaire d'entites
	Phase3Systems*  phase3_systems; //systemes de la phase 3
	//ajouter d'autres sous-systemes au fur et a mesure, quete, inventaire, etc
}	GameContext;

/*
 * Initialise le contexte de jeu et tous les sous-systemes.
 * @return pointeur vers le contexte de jeu ou NULL en cas d'erreur.
*/
GameContext*	game_init(void);

/*
 * Verifie si le jeu est en cours d'execution.
 * @param game Contexte du jeu
 * @return true si le jeu est en cours d'execution, false sinon
*/
bool			game_is_running(GameContext* game);

/*
 * Gere les evenements d'entree utilisateur
 * @param game Contexte du jeu
*/
void			game_handle_events(GameContext* game);

/*
 * Met a jour l'etat du jeu
 * @param game Contexte du jeu
*/
void			game_update(GameContext* game);

/*
 * Rend le jeu a l'ecran
 * @param game Contexte du jeu
*/
void			game_render(GameContext* game);

/*
 * Libere toutes les ressources et ferme le jeu
 * @param game Contexte du jeu
*/
void			game_shutdown(GameContext* game);

#endif
