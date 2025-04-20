/**
 * zone_transition.h
 * Système de gestion des transitions entre zones
 */

#ifndef ZONE_TRANSITION_H
#define ZONE_TRANSITION_H

#include <stdbool.h>
#include "../systems/world.h"
#include "../core/resource_manager.h"

// Structure d'un point de transition
typedef struct {
    int id;                  // ID unique de la transition
    ZoneType source_zone;    // Zone source
    ZoneType target_zone;    // Zone cible
    float source_x, source_y; // Position dans la zone source
    float target_x, target_y; // Position dans la zone cible
    char* target_map;        // Chemin du fichier de la carte cible
    bool is_active;          // La transition est-elle active
} ZoneTransition;

// Système de transition entre zones
typedef struct {
    ZoneTransition* transitions;  // Tableau des transitions
    int transition_count;         // Nombre de transitions
    int transition_capacity;      // Capacité du tableau
    ZoneTransition* active_transition; // Transition en cours (NULL si aucune)
    float transition_progress;    // Progression de la transition (0.0-1.0)
    bool is_transitioning;        // Est en cours de transition
} ZoneTransitionSystem;

/**
 * Initialise le système de transition
 * @return Pointeur vers le système de transition ou NULL en cas d'erreur
 */
ZoneTransitionSystem* zone_transition_init(void);

/**
 * Libère les ressources du système de transition
 * @param system Système de transition
 */
void zone_transition_shutdown(ZoneTransitionSystem* system);

/**
 * Ajoute une transition
 * @param system Système de transition
 * @param source_zone Zone source
 * @param target_zone Zone cible
 * @param source_x Position X dans la zone source
 * @param source_y Position Y dans la zone source
 * @param target_x Position X dans la zone cible
 * @param target_y Position Y dans la zone cible
 * @param target_map Chemin du fichier de la carte cible (peut être NULL)
 * @return ID de la transition ou -1 en cas d'erreur
 */
int zone_transition_add(
    ZoneTransitionSystem* system,
    ZoneType source_zone,
    ZoneType target_zone,
    float source_x, float source_y,
    float target_x, float target_y,
    const char* target_map
);

/**
 * Active une transition
 * @param system Système de transition
 * @param transition_id ID de la transition à activer
 * @return true si la transition a été activée, false sinon
 */
bool zone_transition_activate(ZoneTransitionSystem* system, int transition_id);

/**
 * Désactive une transition
 * @param system Système de transition
 * @param transition_id ID de la transition à désactiver
 * @return true si la transition a été désactivée, false sinon
 */
bool zone_transition_deactivate(ZoneTransitionSystem* system, int transition_id);

/**
 * Trouve une transition à partir d'une position
 * @param system Système de transition
 * @param zone Zone actuelle
 * @param x Position X
 * @param y Position Y
 * @param radius Rayon de recherche
 * @return ID de la transition trouvée ou -1 si aucune transition n'est trouvée
 */
int zone_transition_find_at_position(
    ZoneTransitionSystem* system,
    ZoneType zone,
    float x, float y,
    float radius
);

/**
 * Déclenche une transition
 * @param system Système de transition
 * @param world_system Système de monde
 * @param transition_id ID de la transition à déclencher
 * @return true si la transition a été déclenchée, false sinon
 */
bool zone_transition_trigger(
    ZoneTransitionSystem* system,
    WorldSystem* world_system,
    int transition_id
);

/**
 * Met à jour le système de transition
 * @param system Système de transition
 * @param world_system Système de monde
 * @param resource_manager Gestionnaire de ressources
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void zone_transition_update(
    ZoneTransitionSystem* system,
    WorldSystem* world_system,
    ResourceManager* resource_manager,
    float delta_time
);

/**
 * Rend le système de transition
 * @param system Système de transition
 * @param render_system Système de rendu
 */
void zone_transition_render(
    ZoneTransitionSystem* system,
    RenderSystem* render_system
);

/**
 * Charge les transitions depuis une carte Tiled
 * @param system Système de transition
 * @param tiled_map Carte Tiled
 * @param zone_type Type de zone de la carte
 * @return Nombre de transitions chargées ou -1 en cas d'erreur
 */
int zone_transition_load_from_map(
    ZoneTransitionSystem* system,
    struct TiledMap* tiled_map,
    ZoneType zone_type
);

/**
 * Rend le système de transition prêt pour une nouvelle zone
 * @param system Système de transition
 * @param zone_type Type de la nouvelle zone
 */
void zone_transition_prepare_zone(
    ZoneTransitionSystem* system,
    ZoneType zone_type
);

#endif /* ZONE_TRANSITION_H */