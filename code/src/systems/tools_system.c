/**
 * tools_system.c
 * Implémentation du système de gestion des outils
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "../systems/tools_system.h"
#include "../utils/error_handler.h"
#include "../systems/farming_system.h"

// Nombre maximal d'outils dans la base de données
#define MAX_TOOLS 100

// Nombre maximal d'outils que le joueur peut posséder
#define MAX_PLAYER_TOOLS 20

// Points d'expérience nécessaires pour monter de niveau
#define XP_PER_LEVEL 100

// Données des outils (à partir du fichier tools_data.xlsx)
static ToolData default_tools[] = {
    // Pioches
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_RUSTY, "Pioche rouillée", 3, 1.5f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Cuivre", 0, 0.0f, 0, 10, 500, 1, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_COPPER, "Pioche en cuivre", 6, 1.4f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Fer", 0, 0.0f, 10, 20, 1000, 2, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_IRON, "Pioche en fer", 9, 1.3f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Or", 0, 0.0f, 20, 30, 2000, 3, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_GOLD, "Pioche en or", 12, 1.2f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Mithril", 0, 0.0f, 30, 40, 3000, 4, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_MITHRIL, "Pioche en mithril", 15, 1.1f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Enchantium", 0, 0.0f, 40, 50, 5000, 5, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_ENCHANTIUM, "Pioche en enchantium", 18, 1.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Orichalque", 0, 0.0f, 50, 60, 8000, 6, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_ORICHALQUE, "Pioche en orichalque", 21, 0.9f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Thamium", 0, 0.0f, 60, 70, 12000, 7, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_THAMIUM, "Pioche en thamium", 24, 0.8f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Adamantite", 0, 0.0f, 70, 80, 18000, 8, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_ADAMANTITE, "Pioche en adamantite", 27, 0.7f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Ebonite", 0, 0.0f, 80, 90, 25000, 9, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_EBONITE, "Pioche en ebonite", 30, 0.6f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "Herodium", 0, 0.0f, 90, 200, 35000, 10, 0},
    {TOOL_TYPE_PICKAXE, TOOL_MATERIAL_HERODIUM, "Pioche en herodium", 45, 0.5f, {EFFECT_SHAPE_SQUARE, 3, 3, 0, 0}, "", 0, 0.0f, 200, 200, 50000, 11, 0},
    
    // Haches
    {TOOL_TYPE_AXE, TOOL_MATERIAL_RUSTY, "Hache rouillée", 3, 1.5f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Branche", 0, 0.0f, 0, 10, 500, 1, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_COPPER, "Hache en cuivre", 6, 1.4f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Bûche", 0, 0.0f, 10, 20, 1000, 2, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_IRON, "Hache en fer", 9, 1.3f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Souche", 0, 0.0f, 20, 30, 2000, 3, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_GOLD, "Hache en or", 12, 1.2f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Arbre", 0, 0.0f, 30, 40, 3000, 4, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_MITHRIL, "Hache en mithril", 15, 1.1f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "Grand Arbre", 0, 0.0f, 40, 50, 5000, 5, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_ENCHANTIUM, "Hache en enchantium", 18, 1.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 50, 60, 8000, 6, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_ORICHALQUE, "Hache en orichalque", 21, 0.9f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 60, 70, 12000, 7, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_THAMIUM, "Hache en thamium", 24, 0.8f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 70, 80, 18000, 8, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_ADAMANTITE, "Hache en adamantite", 27, 0.7f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 80, 90, 25000, 9, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_EBONITE, "Hache en ebonite", 30, 0.6f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "", 0, 0.0f, 90, 200, 35000, 10, 0},
    {TOOL_TYPE_AXE, TOOL_MATERIAL_HERODIUM, "Hache en herodium", 45, 0.5f, {EFFECT_SHAPE_SQUARE, 3, 3, 0, 0}, "", 0, 0.0f, 200, 200, 50000, 11, 0},
    
    // Houes
    {TOOL_TYPE_HOE, TOOL_MATERIAL_RUSTY, "Houe rouillée", 0, 1.5f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 0, 10, 500, 1, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_COPPER, "Houe en cuivre", 0, 1.4f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 10, 20, 1000, 2, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_IRON, "Houe en fer", 0, 1.3f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 20, 30, 2000, 3, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_GOLD, "Houe en or", 0, 1.2f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 30, 40, 3000, 4, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_MITHRIL, "Houe en mithril", 0, 1.1f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 40, 50, 5000, 5, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_ENCHANTIUM, "Houe en enchantium", 0, 1.0f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "", 0, 0.0f, 50, 60, 8000, 6, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_ORICHALQUE, "Houe en orichalque", 0, 0.9f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "", 0, 0.0f, 60, 70, 12000, 7, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_THAMIUM, "Houe en thamium", 0, 0.8f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "", 0, 0.0f, 70, 80, 18000, 8, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_ADAMANTITE, "Houe en adamantite", 0, 0.7f, {EFFECT_SHAPE_SQUARE, 3, 3, 0, 0}, "", 0, 0.0f, 80, 90, 25000, 9, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_EBONITE, "Houe en ebonite", 0, 0.6f, {EFFECT_SHAPE_SQUARE, 3, 3, 0, 0}, "", 0, 0.0f, 90, 200, 35000, 10, 0},
    {TOOL_TYPE_HOE, TOOL_MATERIAL_HERODIUM, "Houe en herodium", 0, 0.5f, {EFFECT_SHAPE_SQUARE, 4, 4, 0, 0}, "", 0, 0.0f, 200, 200, 50000, 11, 0},
    
    // Arrosoirs
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_RUSTY, "Arrosoir rouillé", 0, 1.5f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 10, 0.0f, 0, 10, 500, 1, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_COPPER, "Arrosoir en cuivre", 0, 1.4f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 15, 0.0f, 10, 20, 1000, 2, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_IRON, "Arrosoir en fer", 0, 1.3f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 20, 0.0f, 20, 30, 2000, 3, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_GOLD, "Arrosoir en or", 0, 1.2f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 25, 0.0f, 30, 40, 3000, 4, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_MITHRIL, "Arrosoir en mithril", 0, 1.1f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 30, 0.0f, 40, 50, 5000, 5, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_ENCHANTIUM, "Arrosoir en enchantium", 0, 1.0f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "", 35, 0.0f, 50, 60, 8000, 6, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_ORICHALQUE, "Arrosoir en orichalque", 0, 0.9f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "", 40, 0.0f, 60, 70, 12000, 7, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_THAMIUM, "Arrosoir en thamium", 0, 0.8f, {EFFECT_SHAPE_SQUARE, 2, 2, 0, 0}, "", 45, 0.0f, 70, 80, 18000, 8, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_ADAMANTITE, "Arrosoir en adamantite", 0, 0.7f, {EFFECT_SHAPE_SQUARE, 3, 3, 0, 0}, "", 50, 0.0f, 80, 90, 25000, 9, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_EBONITE, "Arrosoir en ebonite", 0, 0.6f, {EFFECT_SHAPE_SQUARE, 3, 3, 0, 0}, "", 55, 0.0f, 90, 200, 35000, 10, 0},
    {TOOL_TYPE_WATERING, TOOL_MATERIAL_HERODIUM, "Arrosoir en herodium", 0, 0.5f, {EFFECT_SHAPE_SQUARE, 4, 4, 0, 0}, "", -1, 0.0f, 200, 200, 50000, 11, 0},
    
    // Faux
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_RUSTY, "Faux rouillée", 3, 1.5f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 0, 10, 500, 1, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_COPPER, "Faux en cuivre", 6, 1.4f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 10, 20, 1000, 2, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_IRON, "Faux en fer", 9, 1.3f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 20, 30, 2000, 3, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_GOLD, "Faux en or", 12, 1.2f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 30, 40, 3000, 4, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_MITHRIL, "Faux en mithril", 15, 1.1f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 0.0f, 40, 50, 5000, 5, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_ENCHANTIUM, "Faux en enchantium", 18, 1.0f, {EFFECT_SHAPE_RECTANGLE, 1, 3, 0, 0}, "", 0, 0.0f, 50, 60, 8000, 6, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_ORICHALQUE, "Faux en orichalque", 21, 0.9f, {EFFECT_SHAPE_RECTANGLE, 1, 3, 0, 0}, "", 0, 0.0f, 60, 70, 12000, 7, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_THAMIUM, "Faux en thamium", 24, 0.8f, {EFFECT_SHAPE_RECTANGLE, 2, 5, 0, 0}, "", 0, 0.0f, 70, 80, 18000, 8, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_ADAMANTITE, "Faux en adamantite", 27, 0.7f, {EFFECT_SHAPE_RECTANGLE, 2, 5, 0, 0}, "", 0, 0.0f, 80, 90, 25000, 9, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_EBONITE, "Faux en ebonite", 30, 0.6f, {EFFECT_SHAPE_RECTANGLE, 3, 5, 0, 0}, "", 0, 0.0f, 90, 200, 35000, 10, 0},
    {TOOL_TYPE_SCYTHE, TOOL_MATERIAL_HERODIUM, "Faux en herodium", 45, 0.5f, {EFFECT_SHAPE_RECTANGLE, 5, 5, 0, 0}, "", 0, 0.0f, 200, 200, 50000, 11, 0},
    
    // Cannes à pêche
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_RUSTY, "Canne à pêche rouillée", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 200.0f, 0, 10, 500, 1, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_COPPER, "Canne à pêche en cuivre", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 190.0f, 10, 20, 1000, 2, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_IRON, "Canne à pêche en fer", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 180.0f, 20, 30, 2000, 3, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_GOLD, "Canne à pêche en or", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 170.0f, 30, 40, 3000, 4, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_MITHRIL, "Canne à pêche en mithril", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 160.0f, 40, 50, 5000, 5, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_ENCHANTIUM, "Canne à pêche en enchantium", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 150.0f, 50, 60, 8000, 6, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_ORICHALQUE, "Canne à pêche en orichalque", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 140.0f, 60, 70, 12000, 7, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_THAMIUM, "Canne à pêche en thamium", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 130.0f, 70, 80, 18000, 8, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_ADAMANTITE, "Canne à pêche en adamantite", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 120.0f, 80, 90, 25000, 9, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_EBONITE, "Canne à pêche en ebonite", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 110.0f, 90, 200, 35000, 10, 0},
    {TOOL_TYPE_FISHING, TOOL_MATERIAL_HERODIUM, "Canne à pêche en herodium", 0, 0.0f, {EFFECT_SHAPE_SQUARE, 1, 1, 0, 0}, "", 0, 50.0f, 200, 200, 50000, 11, 0}
};

/**
 * Initialise le système d'outils
 * @param entity_manager Gestionnaire d'entités
 * @param world_system Système de monde
 * @return Pointeur vers le système d'outils ou NULL en cas d'erreur
 */
ToolsSystem* tools_system_init(EntityManager* entity_manager, WorldSystem* world_system) {
    if (!entity_manager || !world_system) {
        log_error("Paramètres invalides pour tools_system_init");
        return NULL;
    }
    
    ToolsSystem* system = (ToolsSystem*)calloc(1, sizeof(ToolsSystem));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système d'outils")) {
        return NULL;
    }
    
    system->entity_manager = entity_manager;
    system->world_system = world_system;
    
    // Initialiser la base de données des outils
    system->tool_count = sizeof(default_tools) / sizeof(ToolData);
    system->tool_database = (ToolData*)calloc(MAX_TOOLS, sizeof(ToolData));
    
    if (!check_ptr(system->tool_database, LOG_LEVEL_ERROR, "Échec d'allocation de la base de données d'outils")) {
        free(system);
        return NULL;
    }
    
    // Copier les outils par défaut
    memcpy(system->tool_database, default_tools, sizeof(default_tools));
    
    // Initialiser les outils du joueur
    system->player_tool_count = 0;
    system->player_tools = (ToolState*)calloc(MAX_PLAYER_TOOLS, sizeof(ToolState));
    
    if (!check_ptr(system->player_tools, LOG_LEVEL_ERROR, "Échec d'allocation des outils du joueur")) {
        free(system->tool_database);
        free(system);
        return NULL;
    }
    
    // Initialiser les autres variables
    system->tool_cooldown = 0.0f;
    system->is_tool_in_use = false;
    system->active_tool = NULL;
    
    log_info("Système d'outils initialisé avec %d outils", system->tool_count);
    
    return system;
}

/**
 * Libère les ressources du système d'outils
 * @param system Système d'outils
 */
void tools_system_shutdown(ToolsSystem* system) {
    if (!system) return;
    
    if (system->tool_database) {
        free(system->tool_database);
        system->tool_database = NULL;
    }
    
    if (system->player_tools) {
        free(system->player_tools);
        system->player_tools = NULL;
    }
    
    free(system);
    log_info("Système d'outils libéré");
}

/**
 * Met à jour le système d'outils
 * @param system Système d'outils
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void tools_system_update(ToolsSystem* system, float delta_time) {
    if (!system) return;
    
    // Mettre à jour le cooldown de l'outil
    if (system->tool_cooldown > 0.0f) {
        system->tool_cooldown -= delta_time;
        
        if (system->tool_cooldown <= 0.0f) {
            system->tool_cooldown = 0.0f;
            system->is_tool_in_use = false;
        }
    }
}

/**
 * Équipe un outil spécifique
 * @param system Système d'outils
 * @param tool_id ID de l'outil à équiper
 * @return true si l'équipement a réussi, false sinon
 */
bool tools_system_equip_tool(ToolsSystem* system, int tool_id) {
    if (!system) return false;
    
    // Déséquiper l'outil actuel
    if (system->active_tool) {
        system->active_tool->is_equipped = false;
        system->active_tool = NULL;
    }
    
    // Rechercher l'outil dans l'inventaire du joueur
    for (int i = 0; i < system->player_tool_count; i++) {
        if (system->player_tools[i].tool_id == tool_id) {
            // Vérifier le niveau de maîtrise requis
            const ToolData* tool_data = tools_system_get_tool_data(system, tool_id);
            if (!tool_data) return false;
            
            if (system->player_tools[i].mastery_level < tool_data->mastery_required) {
                log_warning("Niveau de maîtrise insuffisant pour équiper l'outil %s", tool_data->name);
                return false;
            }
            
            // Équiper l'outil
            system->player_tools[i].is_equipped = true;
            system->active_tool = &system->player_tools[i];
            
            log_info("Outil %s équipé", tool_data->name);
            return true;
        }
    }
    
    log_warning("Outil %d non trouvé dans l'inventaire du joueur", tool_id);
    return false;
}

/**
 * Utilise l'outil actuellement équipé
 * @param system Système d'outils
 * @param x Position X cible
 * @param y Position Y cible
 * @return true si l'utilisation a réussi, false sinon
 */
bool tools_system_use_tool(ToolsSystem* system, int x, int y) {
    if (!system || !system->active_tool || system->is_tool_in_use) return false;
    
    // Récupérer les données de l'outil
    const ToolData* tool_data = tools_system_get_tool_data(system, system->active_tool->tool_id);
    if (!tool_data) return false;
    
    // Vérifier le cooldown
    if (system->tool_cooldown > 0.0f) {
        log_debug("Outil en cooldown: %.2f secondes restantes", system->tool_cooldown);
        return false;
    }
    
    // Initialiser le cooldown
    system->tool_cooldown = tool_data->use_speed;
    system->is_tool_in_use = true;
    
    // Calculer la zone d'effet
    int effect_width = tool_data->effect_zone.width;
    int effect_height = tool_data->effect_zone.height;
    
    int start_x = x - effect_width / 2;
    int start_y = y - effect_height / 2;
    
    // Ajouter des points de maîtrise
    tools_system_add_mastery_points(system, system->active_tool->tool_id, 1);
    
    // Effectuer l'action en fonction du type d'outil
    bool success = false;
    
    switch (tool_data->type) {
        case TOOL_TYPE_PICKAXE:
            // Miner des pierres
            // TODO: Implémenter la logique de minage
            log_info("Utilisation de la pioche en (%d, %d)", x, y);
            success = true;
            break;
            
        case TOOL_TYPE_AXE:
            // Couper du bois
            // TODO: Implémenter la logique de coupe
            log_info("Utilisation de la hache en (%d, %d)", x, y);
            success = true;
            break;
            
        case TOOL_TYPE_HOE:
            // Labourer la terre
            for (int i = 0; i < effect_width; i++) {
                for (int j = 0; j < effect_height; j++) {
                    int target_x = start_x + i;
                    int target_y = start_y + j;
                    
                    // Obtenir le système de farming (à implémenter selon votre architecture)
                    FarmingSystem* farming_system = NULL; // À remplacer par votre logique
                    
                    if (farming_system) {
                        success |= farming_system_till_soil(farming_system, target_x, target_y);
                    }
                }
            }
            
            log_info("Utilisation de la houe en (%d, %d), %s", x, y, success ? "réussi" : "échoué");
            break;
            
        case TOOL_TYPE_WATERING:
            // Arroser les plantes
            // Vérifier si l'arrosoir a de l'eau
            if (system->active_tool->current_reservoir <= 0) {
                log_warning("L'arrosoir est vide");
                system->tool_cooldown = 0.0f;
                system->is_tool_in_use = false;
                return false;
            }
            
            for (int i = 0; i < effect_width; i++) {
                for (int j = 0; j < effect_height; j++) {
                    int target_x = start_x + i;
                    int target_y = start_y + j;
                    
                    // Obtenir le système de farming (à implémenter selon votre architecture)
                    FarmingSystem* farming_system = NULL; // À remplacer par votre logique
                    
                    if (farming_system) {
                        bool watered = farming_system_water_soil(farming_system, target_x, target_y);
                        if (watered) {
                            // Décrémenter le réservoir
                            system->active_tool->current_reservoir--;
                            success = true;
                            
                            // Arrêter si le réservoir est vide
                            if (system->active_tool->current_reservoir <= 0) {
                                break;
                            }
                        }
                    }
                }
                
                if (system->active_tool->current_reservoir <= 0) {
                    break;
                }
            }
            
            log_info("Utilisation de l'arrosoir en (%d, %d), %s, réservoir: %d", 
                   x, y, success ? "réussi" : "échoué", system->active_tool->current_reservoir);
            break;
            
        case TOOL_TYPE_SCYTHE:
            // Couper l'herbe, les plantes mortes, etc.
            // TODO: Implémenter la logique de fauchage
            log_info("Utilisation de la faux en (%d, %d)", x, y);
            success = true;
            break;
            
        case TOOL_TYPE_FISHING:
            // Pêcher (mini-jeu)
            // TODO: Implémenter le mini-jeu de pêche
            log_info("Début de la pêche en (%d, %d)", x, y);
            success = true;
            break;
            
        default:
            log_error("Type d'outil inconnu: %d", tool_data->type);
            system->tool_cooldown = 0.0f;
            system->is_tool_in_use = false;
            return false;
    }
    
    return success;
}

/**
 * Recharge l'arrosoir à une source d'eau
 * @param system Système d'outils
 * @param tool_id ID de l'arrosoir
 * @return true si le rechargement a réussi, false sinon
 */
bool tools_system_refill_watering_can(ToolsSystem* system, int tool_id) {
    if (!system) return false;
    
    // Récupérer l'outil
    ToolState* tool_state = tools_system_get_tool_state(system, tool_id);
    if (!tool_state) return false;
    
    // Vérifier s'il s'agit d'un arrosoir
    const ToolData* tool_data = tools_system_get_tool_data(system, tool_id);
    if (!tool_data || tool_data->type != TOOL_TYPE_WATERING) {
        log_warning("L'outil %d n'est pas un arrosoir", tool_id);
        return false;
    }
    
    // Vérifier si l'arrosoir est déjà plein
    if (tool_data->reservoir_capacity > 0 && tool_state->current_reservoir >= tool_data->reservoir_capacity) {
        log_info("L'arrosoir est déjà plein");
        return true;
    }
    
    // Remplir le réservoir
    tool_state->current_reservoir = tool_data->reservoir_capacity;
    
    log_info("Arrosoir %s rechargé à %d", tool_data->name, tool_state->current_reservoir);
    return true;
}

/**
 * Améliore un outil au niveau supérieur
 * @param system Système d'outils
 * @param tool_id ID de l'outil à améliorer
 * @return true si l'amélioration a réussi, false sinon
 */
bool tools_system_upgrade_tool(ToolsSystem* system, int tool_id) {
    if (!system) return false;
    
    // Récupérer l'outil actuel
    ToolState* tool_state = tools_system_get_tool_state(system, tool_id);
    if (!tool_state) return false;
    
    // Récupérer les données de l'outil
    const ToolData* tool_data = tools_system_get_tool_data(system, tool_id);
    if (!tool_data) return false;
    
    // Vérifier si l'outil peut être amélioré
    if (tool_data->material >= TOOL_MATERIAL_HERODIUM) {
        log_warning("L'outil %s est déjà au niveau maximum", tool_data->name);
        return false;
    }
    
    // Vérifier le niveau de maîtrise
    if (tool_state->mastery_level < tool_data->mastery_max) {
        log_warning("Niveau de maîtrise insuffisant pour améliorer l'outil %s", tool_data->name);
        return false;
    }
    
    // Rechercher l'outil de niveau supérieur
    int next_tool_id = -1;
    for (int i = 0; i < system->tool_count; i++) {
        if (system->tool_database[i].type == tool_data->type &&
            system->tool_database[i].material == tool_data->material + 1) {
            next_tool_id = i;
            break;
        }
    }
    
    if (next_tool_id == -1) {
        log_error("Impossible de trouver l'outil de niveau supérieur pour %s", tool_data->name);
        return false;
    }
    
    // Vérifier si le joueur possède déjà cet outil
    for (int i = 0; i < system->player_tool_count; i++) {
        if (system->player_tools[i].tool_id == next_tool_id) {
            log_warning("Le joueur possède déjà l'outil %s", system->tool_database[next_tool_id].name);
            return false;
        }
    }
    
    // Mettre à jour l'outil
    tool_state->tool_id = next_tool_id;
    tool_state->mastery_level = system->tool_database[next_tool_id].mastery_required;
    tool_state->mastery_points = 0;
    
    // Si c'était l'outil actif, mettre à jour la référence
    if (system->active_tool == tool_state) {
        system->active_tool = tool_state;
    }
    
    log_info("Outil amélioré en %s", system->tool_database[next_tool_id].name);
    return true;
}

/**
 * Ajoute des points de maîtrise à un outil
 * @param system Système d'outils
 * @param tool_id ID de l'outil
 * @param points Points de maîtrise à ajouter
 * @return true si les points ont été ajoutés, false sinon
 */
bool tools_system_add_mastery_points(ToolsSystem* system, int tool_id, int points) {
    if (!system || points <= 0) return false;
    
    // Récupérer l'outil
    ToolState* tool_state = tools_system_get_tool_state(system, tool_id);
    if (!tool_state) return false;
    
    // Récupérer les données de l'outil
    const ToolData* tool_data = tools_system_get_tool_data(system, tool_id);
    if (!tool_data) return false;
    
    // Vérifier si l'outil a atteint son niveau maximum
    if (tool_state->mastery_level >= tool_data->mastery_max) {
        return false;
    }
    
    // Ajouter les points
    tool_state->mastery_points += points;
    
    // Vérifier si le niveau a augmenté
    if (tool_state->mastery_points >= XP_PER_LEVEL) {
        int levels_gained = tool_state->mastery_points / XP_PER_LEVEL;
        tool_state->mastery_level += levels_gained;
        tool_state->mastery_points %= XP_PER_LEVEL;
        
        // Limiter au niveau maximum de l'outil
        if (tool_state->mastery_level > tool_data->mastery_max) {
            tool_state->mastery_level = tool_data->mastery_max;
            tool_state->mastery_points = 0;
        }
        
        log_info("L'outil %s a gagné %d niveau(x) de maîtrise (niveau actuel: %d)",
               tool_data->name, levels_gained, tool_state->mastery_level);
    }
    
    return true;
}

/**
 * Donne un nouvel outil au joueur
 * @param system Système d'outils
 * @param type Type d'outil
 * @param material Matériau de l'outil
 * @return ID de l'outil ou -1 en cas d'erreur
 */
int tools_system_give_tool(ToolsSystem* system, ToolType type, ToolMaterial material) {
    if (!system) return -1;
    
    // Vérifier s'il y a de la place dans l'inventaire du joueur
    if (system->player_tool_count >= MAX_PLAYER_TOOLS) {
        log_warning("Inventaire d'outils plein");
        return -1;
    }
    
    // Rechercher l'outil correspondant
    int tool_id = -1;
    for (int i = 0; i < system->tool_count; i++) {
        if (system->tool_database[i].type == type && 
            system->tool_database[i].material == material) {
            tool_id = i;
            break;
        }
    }
    
    if (tool_id == -1) {
        log_error("Outil de type %d et matériau %d non trouvé", type, material);
        return -1;
    }
    
    // Vérifier si le joueur possède déjà cet outil
    for (int i = 0; i < system->player_tool_count; i++) {
        if (system->player_tools[i].tool_id == tool_id) {
            log_warning("Le joueur possède déjà l'outil %s", system->tool_database[tool_id].name);
            return tool_id;
        }
    }
    
    // Ajouter l'outil à l'inventaire du joueur
    ToolState* tool_state = &system->player_tools[system->player_tool_count++];
    tool_state->tool_id = tool_id;
    tool_state->mastery_level = system->tool_database[tool_id].mastery_required;
    tool_state->mastery_points = 0;
    tool_state->is_equipped = false;
    
    // Initialiser le réservoir pour l'arrosoir
    if (system->tool_database[tool_id].type == TOOL_TYPE_WATERING) {
        tool_state->current_reservoir = system->tool_database[tool_id].reservoir_capacity;
    } else {
        tool_state->current_reservoir = 0;
    }
    
    log_info("Outil %s ajouté à l'inventaire du joueur", system->tool_database[tool_id].name);
    return tool_id;
}

/**
 * Récupère les données d'un outil par son ID
 * @param system Système d'outils
 * @param tool_id ID de l'outil
 * @return Pointeur vers les données de l'outil ou NULL si non trouvé
 */
const ToolData* tools_system_get_tool_data(ToolsSystem* system, int tool_id) {
    if (!system || tool_id < 0 || tool_id >= system->tool_count) {
        return NULL;
    }
    
    return &system->tool_database[tool_id];
}

/**
 * Récupère l'état d'un outil du joueur par son ID
 * @param system Système d'outils
 * @param tool_id ID de l'outil
 * @return Pointeur vers l'état de l'outil ou NULL si non trouvé
 */
ToolState* tools_system_get_tool_state(ToolsSystem* system, int tool_id) {
    if (!system) return NULL;
    
    for (int i = 0; i < system->player_tool_count; i++) {
        if (system->player_tools[i].tool_id == tool_id) {
            return &system->player_tools[i];
        }
    }
    
    return NULL;
}

/**
 * Récupère l'outil actuellement équipé
 * @param system Système d'outils
 * @return ID de l'outil équipé ou -1 si aucun
 */
int tools_system_get_equipped_tool(ToolsSystem* system) {
    if (!system || !system->active_tool) {
        return -1;
    }
    
    return system->active_tool->tool_id;
}