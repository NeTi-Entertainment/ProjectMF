/*
 * Projet Stardew Valley-like en C
 * Point d'entree du programme principal
*/

# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>
# include "core/game.h"
# include "utils/error_handler.h"

int	main(int argc, char **argv)
{
	GameContext *game = game_init();
	if (!game)
	{
		log_error("Echec de l'initialisation du jeu");
		return (EXIT_FAILURE);
	}
	while (game_is_running(game))
	{
		game_handle_events(game);
		game_update(game);
		game_render(game);
	}
	game_shutdown(game);
	return (EXIT_SUCCESS);
}
