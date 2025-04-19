/**
 * entity.h
 * Définit le système d'entités et de composants
 */

#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>

// ID d'entité unique
typedef uint32_t EntityID;

// ID d'entité invalide
#define INVALID_ENTITY_ID 0

// Types de composants disponibles
typedef enum {
    COMPONENT_TRANSFORM,     // Position, rotation, échelle
    COMPONENT_SPRITE,        // Représentation visuelle
    COMPONENT_COLLIDER,      // Collisions
    COMPONENT_PLAYER,        // Données spécifiques au joueur
    COMPONENT_NPC,           // Données spécifiques aux PNJ
    COMPONENT_ITEM,          // Objets pouvant être ramassés
    COMPONENT_FARMING,       // Cultures, terres agricoles
    COMPONENT_INTERACTABLE,  // Objets interactifs
    COMPONENT_ANIMATION,     // Animation
    
    // Toujours ajouter de nouveaux types avant cette ligne
    COMPONENT_TYPE_COUNT     // Nombre total de types de composants
} ComponentType;

// Types de collision
typedef enum {
    COLLISION_NONE = 0,      // Aucune collision
    COLLISION_STATIC = 1,    // Objets immobiles (murs, rochers)
    COLLISION_DYNAMIC = 2,   // Objets mobiles (PNJ, animaux)
    COLLISION_TRIGGER = 3    // Zones de déclenchement (ne bloquent pas)
} CollisionType;

// Masque de bits pour les composants (utilisé pour les requêtes)
typedef uint32_t ComponentMask;

// Macros pour manipuler les masques de composants
#define COMPONENT_BIT(type) (1u << (type))
#define HAS_COMPONENT(mask, type) (((mask) & COMPONENT_BIT(type)) != 0)
#define ADD_COMPONENT(mask, type) ((mask) | COMPONENT_BIT(type))
#define REMOVE_COMPONENT(mask, type) ((mask) & ~COMPONENT_BIT(type))

// Structure de base pour un composant
typedef struct {
    ComponentType type;  // Type du composant
    EntityID entity;     // Entité propriétaire
} Component;

// Composant Transform (position, rotation, échelle)
typedef struct {
    Component base;
    float x, y;           // Position
    float rotation;       // Rotation en degrés
    float scale_x, scale_y; // Échelle
} TransformComponent;

// Composant Sprite (représentation visuelle)
typedef struct {
    Component base;
    uint32_t texture_id;   // ID de la texture
    int width, height;     // Dimensions
    int sprite_sheet_x;    // Position X dans la feuille de sprites
    int sprite_sheet_y;    // Position Y dans la feuille de sprites
    int z_order;           // Ordre de dessin (profondeur)
    bool visible;          // Visibilité
} SpriteComponent;

// Composant Collider (collisions)
typedef struct {
    Component base;
    float width, height;      // Dimensions du collider
    float offset_x, offset_y; // Décalage par rapport au transform
    CollisionType type;       // Type de collision
    uint32_t collision_mask;  // Masque pour filtrer les collisions
    uint32_t collision_layer; // Couche de collision
    bool is_trigger;          // Trigger ou collider solide (gardé pour compatibilité)
} ColliderComponent;

// Structure pour une animation
typedef struct {
    int texture_id;        // ID de la texture contenant l'animation
    int frame_width;       // Largeur d'une frame
    int frame_height;      // Hauteur d'une frame
    int frames_per_row;    // Nombre de frames par ligne dans la texture
    int frame_count;       // Nombre total de frames
    float frame_duration;  // Durée d'une frame en secondes
} Animation;

// Composant Animation
typedef struct {
    Component base;
    Animation* animations[8]; // 4 directions × 2 états (idle/moving)
    int current_animation;    // Animation en cours
    float current_time;       // Temps écoulé dans l'animation
    int current_frame;        // Frame actuelle
    bool is_playing;          // Animation en cours de lecture
    bool loop;                // L'animation doit-elle boucler
} AnimationComponent;

// Composant Player (joueur)
typedef struct {
    Component base;
    float move_speed;      // Vitesse de déplacement
    int stamina;           // Endurance
    int max_stamina;       // Endurance maximale
    int health;            // Santé
    int max_health;        // Santé maximale
} PlayerComponent;

// Composant Farming (cultures, terres agricoles)
typedef struct {
    Component base;
    int growth_stage;       // Étape de croissance
    int max_growth_stage;   // Étape de croissance maximale
    float growth_timer;     // Temps avant la prochaine étape
    float water_level;      // Niveau d'hydratation
    bool is_watered;        // Arrosé aujourd'hui
    int crop_type;          // Type de culture
    bool is_harvestable;    // Prêt à être récolté
} FarmingComponent;

// Composant Item (objets pouvant être ramassés)
typedef struct {
    Component base;
    int item_id;           // ID de l'objet
    int stack_size;        // Taille de la pile
    int max_stack_size;    // Taille maximale de la pile
    bool is_tool;          // Est un outil ou non
    int tool_type;         // Type d'outil
    int tool_level;        // Niveau d'outil
} ItemComponent;

// Composant Interactable (objets interactifs)
typedef struct {
    Component base;
    bool is_active;         // Est actuellement interactif
    float interaction_radius; // Rayon d'interaction
    int interaction_type;   // Type d'interaction
} InteractableComponent;

// Fonctions utilitaires pour les composants

/**
 * Crée un composant Transform
 * @param entity_id ID de l'entité
 * @param x Position X
 * @param y Position Y
 * @return Composant Transform initialisé
 */
TransformComponent* create_transform_component(EntityID entity_id, float x, float y);

/**
 * Crée un composant Sprite
 * @param entity_id ID de l'entité
 * @param texture_id ID de la texture
 * @param width Largeur
 * @param height Hauteur
 * @return Composant Sprite initialisé
 */
SpriteComponent* create_sprite_component(EntityID entity_id, uint32_t texture_id, int width, int height);

/**
 * Crée un composant Collider
 * @param entity_id ID de l'entité
 * @param width Largeur
 * @param height Hauteur
 * @param collision_type Type de collision
 * @return Composant Collider initialisé
 */
ColliderComponent* create_collider_component(EntityID entity_id, float width, float height, CollisionType collision_type);

/**
 * Crée un composant Animation
 * @param entity_id ID de l'entité
 * @return Composant Animation initialisé
 */
AnimationComponent* create_animation_component(EntityID entity_id);

/**
 * Crée un composant Player
 * @param entity_id ID de l'entité
 * @param move_speed Vitesse de déplacement
 * @return Composant Player initialisé
 */
PlayerComponent* create_player_component(EntityID entity_id, float move_speed);

/**
 * Crée un composant Farming
 * @param entity_id ID de l'entité
 * @param crop_type Type de culture
 * @return Composant Farming initialisé
 */
FarmingComponent* create_farming_component(EntityID entity_id, int crop_type);

/**
 * Crée un composant Item
 * @param entity_id ID de l'entité
 * @param item_id ID de l'objet
 * @param stack_size Taille de la pile
 * @return Composant Item initialisé
 */
ItemComponent* create_item_component(EntityID entity_id, int item_id, int stack_size);

/**
 * Crée un composant Interactable
 * @param entity_id ID de l'entité
 * @param interaction_type Type d'interaction
 * @param interaction_radius Rayon d'interaction
 * @return Composant Interactable initialisé
 */
InteractableComponent* create_interactable_component(EntityID entity_id, int interaction_type, float interaction_radius);

#endif /* ENTITY_H */