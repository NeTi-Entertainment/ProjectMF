/**
 * phase3_integration.c
 * Implémentation du fichier d'intégration pour les systèmes de la phase 3
 */

#include <stdlib.h>
#include "../core/phase3_integration.h"
#include "../utils/error_handler.h"

/**
 * Initialise les systèmes de la phase 3
 * @param game Contexte de jeu
 * @return Structure contenant les systèmes initialisés ou NULL en cas d'erreur
 */
Phase3Systems* phase3_init(GameContext* game) {
    if (!game) {
        log_error("Contexte de jeu NULL passé à phase3_init");
        return NULL;
    }
    
    Phase3Systems* systems = (Phase3Systems*)calloc(1, sizeof(Phase3Systems));
    if (!check_ptr(systems, LOG_LEVEL_ERROR, "Échec d'allocation des systèmes de la phase 3")) {
        return NULL;
    }
    
    // Initialiser le système de farming
    systems->farming_system = farming_system_init(game->entity_manager, game->world_system);
    if (!check_ptr(systems->farming_system, LOG_LEVEL_ERROR, "Échec d'initialisation du système de farming")) {
        free(systems);
        return NULL;
    }
    
    // Initialiser le système d'outils
    systems->tools_system = tools_system_init(game->entity_manager, game->world_system);
    if (!check_ptr(systems->tools_system, LOG_LEVEL_ERROR, "Échec d'initialisation du système d'outils")) {
        farming_system_shutdown(systems->farming_system);
        free(systems);
        return NULL;
    }
    
    // Initialiser le système d'inventaire
    systems->inventory_system = inventory_system_init(game->entity_manager, game->render_system, 0, 0);
    if (!check_ptr(systems->inventory_system, LOG_LEVEL_ERROR, "Échec d'initialisation du système d'inventaire")) {
        tools_system_shutdown(systems->tools_system);
        farming_system_shutdown(systems->farming_system);
        free(systems);
        return NULL;
    }
    
    systems->initialized = true;
    
    // Donner les outils et semences de départ au joueur
    phase3_give_starting_tools(systems);
    phase3_give_starting_seeds(systems);
    
    log_info("Systèmes de la phase 3 initialisés avec succès");
    return systems;
}

/**
 * Libère les ressources utilisées par les systèmes de la phase 3
 * @param systems Systèmes à libérer
 */
void phase3_shutdown(Phase3Systems* systems) {
    if (!systems) return;
    
    if (systems->inventory_system) {
        inventory_system_shutdown(systems->inventory_system);
        systems->inventory_system = NULL;
    }
    
    if (systems->tools_system) {
        tools_system_shutdown(systems->tools_system);
        systems->tools_system = NULL;
    }
    
    if (systems->farming_system) {
        farming_system_shutdown(systems->farming_system);
        systems->farming_system = NULL;
    }
    
    systems->initialized = false;
    free(systems);
    
    log_info("Systèmes de la phase 3 libérés");
}

/**
 * Met à jour les systèmes de la phase 3
 * @param systems Systèmes à mettre à jour
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void phase3_update(Phase3Systems* systems, float delta_time) {
    if (!systems || !systems->initialized) return;
    
    // Mettre à jour le système d'inventaire
    inventory_system_update(systems->inventory_system, delta_time);
    
    // Mettre à jour le système d'outils
    tools_system_update(systems->tools_system, delta_time);
    
    // Mettre à jour le système de farming
    // Pour le système de farming, le delta_time est converti en jours
    // en supposant qu'une journée de jeu dure environ 20 minutes réelles
    float day_fraction = delta_time / (20.0f * 60.0f);
    farming_system_update(systems->farming_system, day_fraction);
}

/**
 * Rend les éléments visuels des systèmes de la phase 3
 * @param systems Systèmes à rendre
 */
void phase3_render(Phase3Systems* systems) {
    if (!systems || !systems->initialized) return;
    
    // Seul le système d'inventaire a besoin d'être rendu
    inventory_system_render(systems->inventory_system);
}

/**
 * Gère les événements clavier pour les systèmes de la phase 3
 * @param systems Systèmes à mettre à jour
 * @param key Touche appuyée
 * @return true si l'événement a été traité, false sinon
 */
bool phase3_handle_keydown(Phase3Systems* systems, SDL_Keycode key) {
    if (!systems || !systems->initialized) return false;
    
    // Gestion de l'ouverture/fermeture de l'inventaire
    if (key == SDLK_e || key == SDLK_i) {
        inventory_system_toggle(systems->inventory_system, !systems->inventory_system->inventory_open);
        return true;
    }
    
    // Gestion des touches numériques pour sélectionner les emplacements de la barre d'outils
    if (key >= SDLK_1 && key <= SDLK_9) {
        int slot = key - SDLK_1;
        inventory_system_select_hotbar_slot(systems->inventory_system, slot);
        
        // Équiper l'outil si l'emplacement contient un outil
        int item_id = systems->inventory_system->hotbar_slots[slot].item_id;
        if (item_id >= 0) {
            const ItemData* item_data = inventory_system_get_item_data(systems->inventory_system, item_id);
            if (item_data && item_data->type == ITEM_TYPE_TOOL) {
                tools_system_equip_tool(systems->tools_system, item_data->associated_id);
            }
        }
        
        return true;
    }
    
    // Touche 0 pour le dernier emplacement de la barre d'outils
    if (key == SDLK_0) {
        inventory_system_select_hotbar_slot(systems->inventory_system, 9);
        
        // Équiper l'outil si l'emplacement contient un outil
        int item_id = systems->inventory_system->hotbar_slots[9].item_id;
        if (item_id >= 0) {
            const ItemData* item_data = inventory_system_get_item_data(systems->inventory_system, item_id);
            if (item_data && item_data->type == ITEM_TYPE_TOOL) {
                tools_system_equip_tool(systems->tools_system, item_data->associated_id);
            }
        }
        
        return true;
    }
    
    return false;
}

/**
 * Gère les événements de clic pour les systèmes de la phase 3
 * @param systems Systèmes à mettre à jour
 * @param button Bouton de la souris
 * @param x Position X du clic
 * @param y Position Y du clic
 * @return true si l'événement a été traité, false sinon
 */
bool phase3_handle_mousedown(Phase3Systems* systems, int button, int x, int y) {
    if (!systems || !systems->initialized) return false;
    
    // Si l'inventaire est ouvert, gérer les clics sur les emplacements
    if (systems->inventory_system->inventory_open) {
        // TODO: Implémenter la logique de clic sur les emplacements d'inventaire
        return true;
    }
    
    // Si l'inventaire est fermé, utiliser l'outil équipé
    if (button == SDL_BUTTON_LEFT) {
        // Convertir les coordonnées écran en coordonnées monde
        float world_x, world_y;
        render_system_screen_to_world(systems->tools_system->world_system->render_system, x, y, &world_x, &world_y);
        
        // Utiliser l'outil à cette position
        int grid_x = (int)floorf(world_x);
        int grid_y = (int)floorf(world_y);
        
        return tools_system_use_tool(systems->tools_system, grid_x, grid_y);
    }
    
    return false;
}

/**
 * Ajoute un outil de base au joueur (pour débuter le jeu)
 * @param systems Systèmes de la phase 3
 */
void phase3_give_starting_tools(Phase3Systems* systems) {
    if (!systems || !systems->initialized) return;
    
    // Ajouter les outils de base
    int hoe_id = tools_system_give_tool(systems->tools_system, TOOL_TYPE_HOE, TOOL_MATERIAL_RUSTY);
    int watering_can_id = tools_system_give_tool(systems->tools_system, TOOL_TYPE_WATERING, TOOL_MATERIAL_RUSTY);
    int axe_id = tools_system_give_tool(systems->tools_system, TOOL_TYPE_AXE, TOOL_MATERIAL_RUSTY);
    int pickaxe_id = tools_system_give_tool(systems->tools_system, TOOL_TYPE_PICKAXE, TOOL_MATERIAL_RUSTY);
    
    // Équiper la houe par défaut
    tools_system_equip_tool(systems->tools_system, hoe_id);
    
    // Ajouter les outils à l'inventaire du joueur
    // Les IDs des objets d'inventaire correspondant aux outils
    // Cela nécessiterait normalement un système plus sophistiqué
    inventory_system_add_item(systems->inventory_system, 1001, 1); // Houe rouillée
    inventory_system_add_item(systems->inventory_system, 1002, 1); // Arrosoir rouillé
    inventory_system_add_item(systems->inventory_system, 1003, 1); // Hache rouillée
    inventory_system_add_item(systems->inventory_system, 1004, 1); // Pioche rouillée
    
    // Ajouter les outils à la barre de raccourcis
    systems->inventory_system->hotbar_slots[0].item_id = 1001; // Houe
    systems->inventory_system->hotbar_slots[0].quantity = 1;
    systems->inventory_system->hotbar_slots[1].item_id = 1002; // Arrosoir
    systems->inventory_system->hotbar_slots[1].quantity = 1;
    systems->inventory_system->hotbar_slots[2].item_id = 1003; // Hache
    systems->inventory_system->hotbar_slots[2].quantity = 1;
    systems->inventory_system->hotbar_slots[3].item_id = 1004; // Pioche
    systems->inventory_system->hotbar_slots[3].quantity = 1;
    
    log_info("Outils de départ ajoutés à l'inventaire du joueur");
}

/**
 * Ajoute des semences de base au joueur (pour débuter le jeu)
 * @param systems Systèmes de la phase 3
 */
void phase3_give_starting_seeds(Phase3Systems* systems) {
    if (!systems || !systems->initialized) return;
    
    // Ajouter des semences de base
    inventory_system_add_item(systems->inventory_system, 100, 5); // Semence de Pomme de terre
    inventory_system_add_item(systems->inventory_system, 101, 5); // Semence de Blé
    inventory_system_add_item(systems->inventory_system, 102, 5); // Semence d'Oignon
    
    // Ajouter une semence à la barre de raccourcis
    systems->inventory_system->hotbar_slots[4].item_id = 100; // Semence de Pomme de terre
    systems->inventory_system->hotbar_slots[4].quantity = 5;
    
    log_info("Semences de départ ajoutées à l'inventaire du joueur");
}
