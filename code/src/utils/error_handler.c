/*
 * error_handler.c
 * Implementation du systeme de gestion des erreurs et de journalisation
*/

# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
# include <string.h>
# include <time.h>
# include "../utils/error_handler.h"

//Niveau de journalisation par defaut
LogLevel	g_current_log_level = LOG_LEVEL_INFO;

//Chaines correspondant aux niveaux de journalisation
static const char*	log_level_strings[] = {
	"DEBUG",
	"INFO",
	"WARNING",
	"ERROR",
	"FATAL"
};

//Couleurs ANSI pour les differents niveaux (si le terminal les supporte)
static const char*	log_level_colors[] = {
	"\x1b[36m",//cyan
	"\x1b[32m",//vert
	"\x1b[33m",//jaune
	"\x1b[31m",//rouge
	"\x1b[35m"//magenta
};

//Constantes pour la couleur ANSI
static const char*	COLOR_RESET = "\x1b[0m";

//Fichier de journalisation
static FILE*	log_file = NULL;

//Fonction d'initialisation du systeme de journalisation
__attribute__((constructor)) static void	init_logger(void)
{
	log_file = fopen("game.log", "a");
	if (!log_file)
		fprintf(stderr, "Impossible d'ouvrir le fichier de log\n");
	time_t		current_time = time(NULL);
	struct tm*	time_info = localtime(&current_time);
	char		time_str[26];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
	if (log_file)
	{
		fprintf(log_file, "\n\n=== Nouvelle session de log commencee a %s ===\n\n", time_str);
		fflush(log_file);
	}
}

//Fonction de nettoyage du systeme de journalisation
__attribute__((destructor)) static void	cleanup_logger(void)
{
	if (log_file)
	{
		fclose(log_file);
		log_file = NULL;
	}
}

//Implementation de la fonction de journalisation
void	log_message(LogLevel level, const char* file, int line, const char* format, ...)
{
	if (level < g_current_log_level)
		return ;
	time_t		current_time = time(NULL);
	struct tm*	time_info = localtime(&current_time);
	char		time_str[26];
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", time_info);
	const char*	short_file = strrchr(file, '/');
	if (short_file)
		short_file++;
	else
		short_file = file;
	char		prefix[256];
	snprintf(prefix, sizeof(prefix), "[%s] [%s] %s:%d: ", time_str, log_level_strings[level], short_file, line);
	va_list		args;
	va_start(args, format);
	char		message[1024];
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	if (log_file)
	{
		fprintf(log_file, "%s%s\n", prefix, message);
		fflush(log_file);
	}
	fprintf(stderr, "%s%s%s%s\n", log_level_colors[level], prefix, message, COLOR_RESET);
	if (level == LOG_LEVEL_FATAL)
	{
		if (log_file)
		{
			fprintf(log_file, "Fin du programme suite a une erreur fatale.\n");
			fclose(log_file);
			log_file = NULL;
		}
		exit(EXIT_FAILURE);
	}
}
