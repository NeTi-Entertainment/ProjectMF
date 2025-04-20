/**
 * tools_system.h
 * Système de gestion des outils
 */

#ifndef TOOLS_SYSTEM_H
#define TOOLS_SYSTEM_H

#include <stdbool.h>
#include "../core/entity.h"
#include "../systems/entity_manager.h"
#include "../systems/world.h"

// Types d'outils
typedef enum {
    TOOL_TYPE_PICKAXE,   // Pioche, pour miner les pierres
    TOOL_TYPE_AXE,       // Hache, pour couper du bois
    TOOL_TYPE_HOE,       // Houe, pour labourer la terre
    TOOL_TYPE_WATERING,  // Arrosoir, pour arroser les plantes
    TOOL_TYPE_SCYTHE,    // Faux, pour couper l'herbe
    TOOL_TYPE_FISHING,   // Canne à pêche
    
    TOOL_TYPE_COUNT      // Nombre total de types d'outils
} ToolType;

// Matériaux des outils
typedef enum {
    TOOL_MATERIAL_RUSTY,     // Rouillé (base)
    TOOL_MATERIAL_COPPER,    // Cuivre
    TOOL_MATERIAL_IRON,      // Fer
    TOOL_MATERIAL_GOLD,      // Or
    TOOL_MATERIAL_MITHRIL,   // Mithril
    TOOL_MATERIAL_ENCHANTIUM,// Enchantium
    TOOL_MATERIAL_ORICHALQUE,// Orichalque
    TOOL_MATERIAL_THAMIUM,   // Thamium
    TOOL_MATERIAL_ADAMANTITE,// Adamantite
    TOOL_MATERIAL_EBONITE,   // Ebonite
    TOOL_MATERIAL_HERODIUM,  // Herodium
    
    TOOL_MATERIAL_COUNT      // Nombre total de matériaux
} ToolMaterial;

// Forme d'une zone d'effet
typedef enum {
    EFFECT_SHAPE_SQUARE,     // Zone carrée (ex: 1x1, 2x2, 3x3)
    EFFECT_SHAPE_RECTANGLE,  // Zone rectangulaire (ex: 1x3, 2x5)
    EFFECT_SHAPE_CIRCLE,     // Zone circulaire (rayon)
    EFFECT_SHAPE_CONE,       // Zone en cône (angle)
    
    EFFECT_SHAPE_COUNT       // Nombre total de formes
} EffectShape;

// Données de zone d'effet
typedef struct {
    EffectShape shape;           // Forme de la zone
    int width;                   // Largeur ou diamètre
    int height;                  // Hauteur (si applicable)
    int radius;                  // Rayon (si applicable)
    int angle;                   // Angle (si applicable)
} EffectZone;

// Structure pour un outil
typedef struct {
    ToolType type;               // Type d'outil
    ToolMaterial material;       // Matériau de l'outil
    char name[32];               // Nom de l'outil
    int damage;                  // Dégâts infligés (si applicable)
    float use_speed;             // Vitesse d'utilisation (secondes)
    EffectZone effect_zone;      // Zone d'effet
    char limitation[32];         // Limitation d'usage (textuelle)
    int reservoir_capacity;      // Capacité du réservoir (pour arrosoir)
    float mini_game_difficulty;  // Difficulté du mini-jeu (pour pêche)
    int mastery_required;        // Niveau de maîtrise requis
    int mastery_max;             // Niveau de maîtrise maximal
    int upgrade_cost;            // Coût d'amélioration
    int upgrade_material_id;     // ID du matériau d'amélioration
    int sprite_id;               // ID du sprite pour l'outil
} ToolData;

// État d'un outil que le joueur possède
typedef struct {
    int tool_id;                 // ID de l'outil
    int mastery_level;           // Niveau de maîtrise actuel
    int mastery_points;          // Points de maîtrise accumulés
    int current_reservoir;       // Niveau du réservoir actuel (pour arrosoir)
    bool is_equipped;            // Est équipé actuellement
} ToolState;

// Système d'outils
typedef struct {
    EntityManager* entity_manager;  // Référence au gestionnaire d'entités
    WorldSystem* world_system;      // Référence au système de monde
    
    ToolData* tool_database;        // Base de données des outils
    int tool_count;                 // Nombre d'outils dans la base
    
    ToolState* player_tools;        // Outils possédés par le joueur
    int player_tool_count;          // Nombre d'outils du joueur
    
    float tool_cooldown;            // Temps de recharge avant utilisation suivante
    bool is_tool_in_use;            // Un outil est en cours d'utilisation
    ToolState* active_tool;         // Outil actuellement équipé
} ToolsSystem;

/**
 * Initialise le système d'outils
 * @param entity_manager Gestionnaire d'entités
 * @param world_system Système de monde
 * @return Pointeur vers le système d'outils ou NULL en cas d'erreur
 */
ToolsSystem* tools_system_init(EntityManager* entity_manager, WorldSystem* world_system);

/**
 * Libère les ressources du système d'outils
 * @param system Système d'outils
 */
void tools_system_shutdown(ToolsSystem* system);

/**
 * Met à jour le système d'outils
 * @param system Système d'outils
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void tools_system_update(ToolsSystem* system, float delta_time);

/**
 * Équipe un outil spécifique
 * @param system Système d'outils
 * @param tool_id ID de l'outil à équiper
 * @return true si l'équipement a réussi, false sinon
 */
bool tools_system_equip_tool(ToolsSystem* system, int tool_id);

/**
 * Utilise l'outil actuellement équipé
 * @param system Système d'outils
 * @param x Position X cible
 * @param y Position Y cible
 * @return true si l'utilisation a réussi, false sinon
 */
bool tools_system_use_tool(ToolsSystem* system, int x, int y);

/**
 * Recharge l'arrosoir à une source d'eau
 * @param system Système d'outils
 * @param tool_id ID de l'arrosoir
 * @return true si le rechargement a réussi, false sinon
 */
bool tools_system_refill_watering_can(ToolsSystem* system, int tool_id);

/**
 * Améliore un outil au niveau supérieur
 * @param system Système d'outils
 * @param tool_id ID de l'outil à améliorer
 * @return true si l'amélioration a réussi, false sinon
 */
bool tools_system_upgrade_tool(ToolsSystem* system, int tool_id);

/**
 * Ajoute des points de maîtrise à un outil
 * @param system Système d'outils
 * @param tool_id ID de l'outil
 * @param points Points de maîtrise à ajouter
 * @return true si les points ont été ajoutés, false sinon
 */
bool tools_system_add_mastery_points(ToolsSystem* system, int tool_id, int points);

/**
 * Donne un nouvel outil au joueur
 * @param system Système d'outils
 * @param type Type d'outil
 * @param material Matériau de l'outil
 * @return ID de l'outil ou -1 en cas d'erreur
 */
int tools_system_give_tool(ToolsSystem* system, ToolType type, ToolMaterial material);

/**
 * Récupère les données d'un outil par son ID
 * @param system Système d'outils
 * @param tool_id ID de l'outil
 * @return Pointeur vers les données de l'outil ou NULL si non trouvé
 */
const ToolData* tools_system_get_tool_data(ToolsSystem* system, int tool_id);

/**
 * Récupère l'état d'un outil du joueur par son ID
 * @param system Système d'outils
 * @param tool_id ID de l'outil
 * @return Pointeur vers l'état de l'outil ou NULL si non trouvé
 */
ToolState* tools_system_get_tool_state(ToolsSystem* system, int tool_id);

/**
 * Récupère l'outil actuellement équipé
 * @param system Système d'outils
 * @return ID de l'outil équipé ou -1 si aucun
 */
int tools_system_get_equipped_tool(ToolsSystem* system);

#endif /* TOOLS_SYSTEM_H */
