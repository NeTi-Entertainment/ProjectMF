/*
 * error_handler.h
 * Systeme de gestion des erreurs et de journalisation
*/

#ifndef ERROR_HANDLER_H
# define ERROR_HANDLER_H

# include <stdio.h>
# include <stdarg.h>
# include <time.h>

//Niveau de journalisation
typedef enum
{
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL
}	LogLevel;

//Niveau minimal de journalisation (peut etre modifie a l'execution)
extern LogLevel	g_current_log_level;

/*
 * Journalise un message avec un niveau de priorite
 * @param level Niveau de priorite du message
 * @param file Nom du fichier source
 * @param line Numero de ligne
 * @param format Format de chaine (style printf)
 * @param ... Arguments variables pour le format
*/
void	log_message(LogLevel level, const char* file, int line, const char* format, ...);

//Macros de journalisation
# define log_debug(fmt, ...) log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
# define log_info(fmt, ...) log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
# define log_warning(fmt, ...) log_message(LOG_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
# define log_error(fmt, ...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
# define log_fatal(fmt, ...) log_message(LOG_LEVEL_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

/*
 * Verifie une condition et journalise une erreur si elle echoue
 * @param condition Condition a verifier
 * @param level Niveau de journalisation en cas d'echec
 * @param message Message d'erreur
 * @return La valeur de la condition
*/
# define check_condition(condition, level, message) \
	((condition) ? (true) : \
		(log_message(level, __FILE__, __LINE__, "%s (Echec de la condtion: %s)", message, #condition), false))

/*
 * Verifie si un pointeur est non NULL, journalise une erreur si NULL
 * @param ptr Pointeur a verifier
 * @param level Niveau de journalisation en cas d'echec
 * @param message Message d'erreur
 * @return true si le pointeur est non NULL, false sinon
*/
# define check_ptr(ptr, level, message) check_condition((ptr) != NULL, level, message)

/*
 * Verifie le resultat d'une fonction SDL et journalise une erreur si necessaire
 * @param sdl_result Resultat de la fonction SDL (0 = succes)
 * @param level Niveau de journalisation en cas d'echec
 * @param message Message d'erreur
 * @return true si la fonction a reussi, false sinon
*/
# define check_sdl(sdl_result, level, message) \
	((sdl_result) == 0 ? (true) : \
	 	(log_message(level, __FILE__, __LINE__, "%s: %s", message, SDL_GetError()), false))

#endif
