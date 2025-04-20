/**
 * phase3_integration.h
 * Fichier d'intégration pour les systèmes de la phase 3
 */

#ifndef PHASE3_INTEGRATION_H
#define PHASE3_INTEGRATION_H

#include "../systems/farming_system.h"
#include "../systems/tools_system.h"
#include "../systems/inventory_system.h"
#include "../core/game.h"

// Structure pour regrouper les systèmes de la phase 3
typedef struct {
    FarmingSystem* farming_system;     // Système de farming
    ToolsSystem* tools_system;         // Système d'outils
    InventorySystem* inventory_system; // Système d'inventaire
    bool initialized;                  // Indique si les systèmes sont initialisés
} Phase3Systems;

/**
 * Initialise les systèmes de la phase 3
 * @param game Contexte de jeu
 * @return Structure contenant les systèmes initialisés ou NULL en cas d'erreur
 */
Phase3Systems* phase3_init(GameContext* game);

/**
 * Libère les ressources utilisées par les systèmes de la phase 3
 * @param systems Systèmes à libérer
 */
void phase3_shutdown(Phase3Systems* systems);

/**
 * Met à jour les systèmes de la phase 3
 * @param systems Systèmes à mettre à jour
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void phase3_update(Phase3Systems* systems, float delta_time);

/**
 * Rend les éléments visuels des systèmes de la phase 3
 * @param systems Systèmes à rendre
 */
void phase3_render(Phase3Systems* systems);

/**
 * Gère les événements clavier pour les systèmes de la phase 3
 * @param systems Systèmes à mettre à jour
 * @param key Touche appuyée
 * @return true si l'événement a été traité, false sinon
 */
bool phase3_handle_keydown(Phase3Systems* systems, SDL_Keycode key);

/**
 * Gère les événements de clic pour les systèmes de la phase 3
 * @param systems Systèmes à mettre à jour
 * @param button Bouton de la souris
 * @param x Position X du clic
 * @param y Position Y du clic
 * @return true si l'événement a été traité, false sinon
 */
bool phase3_handle_mousedown(Phase3Systems* systems, int button, int x, int y);

/**
 * Ajoute un outil de base au joueur (pour débuter le jeu)
 * @param systems Systèmes de la phase 3
 */
void phase3_give_starting_tools(Phase3Systems* systems);

/**
 * Ajoute des semences de base au joueur (pour débuter le jeu)
 * @param systems Systèmes de la phase 3
 */
void phase3_give_starting_seeds(Phase3Systems* systems);

#endif /* PHASE3_INTEGRATION_H */