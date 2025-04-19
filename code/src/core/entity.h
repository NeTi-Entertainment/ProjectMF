/*
 * entity.h
 * Definit le systeme d'entites et de composants
*/

#ifndef ENTITY_H
# define ENTITY_H

# include <stdint.h>
# include <stdbool.h>

//ID d'entite unique
typedef uint32_t	EntityID;

//ID d'entite invalide
# define INVALID_ENTITY_ID 0

//Types de composants disponibles
typedef enum
{
	COMPONENT_TRANSFORM,	//Position, rotation, echelle
	COMPONENT_SPRITE,		//Representation visuelle
	COMPONENT_COLLIDER,		//Collisions
	COMPONENT_PLAYER,		//Donnees specifiques au joueur
	COMPONENT_NPC,			//Donnees specifiques aux NPC
	COMPONENT_ITEM,			//Objets pouvant etre ramasses
	COMPONENT_FARMING,		//Cultures, terres agricoles
	COMPONENT_INTERACTABLE,	//Objets interactifs
	COMPONENT_TYPE_COUNT	//Nombre total de types de composants

	//Toujours ajouter de nouveaux types avant cette ligne COMPONENT_TYPE_COUNT
}	ComponentType;

//Masque de bits pour les composants (utilise pour les requetes)
typedef uint32_t	ComponentMask;

//Macro pour manipuler les masques de composants
# define COMPONENT_BIT(type) (1u << (type))
# define HAS_COMPONENT(mask, type) (((mask) & COMPONENT_BIT(type)) != 0)
# define ADD_COMPONENT(mask, type) ((mask) | COMPONENT_BIT(type))
# define REMOVE_COMPONENT(mask, type) ((mask) & ~COMPONENT_BIT(type))

//Structure de base pour un composant
typedef struct
{
	ComponentType	type;	//type du composant
	EntityID		entity;	//Entite proprietaire
}	Component;

//Composant Transform (position, rotation, echelle)
typedef struct
{
	Component		base;
	float			x, y; 				//Position
	float			rotation;			//Rotation en degres
	float			scale_x, scale_y;	//Echelle
}	TransformComponent;

//Composant Sprite (representation visuelle)
typedef struct
{
	Component		base;
	uint32_t		texture_id;			//ID de la texture
	int				width, height;		//Dimensions
	int				sprite_sheet_x;		//Position X dans feuille de sprite
	int				sprite_sheet_y;		//Position Y dans feuille de sprite
	int				z_order;			//Ordre de dessin (profondeur)
	bool			visible;			//Visibilite
}	SpriteComponent;

//Composant Collider (Collisions)
typedef struct
{
	Component		base;
	float			width, height;		//Dimensions du collider
	float			offset_x, offset_y;	//Decalage par rapport au transform
	bool			is_trigger;			//Trigger ou collider solide
}	ColliderComponent;

//Composant Player (joueur)
typedef struct
{
	Component		base;
	float			move_speed;			//Vitesse de deplacement
	int				mana;
	int				max_mana;
	int				health;
	int				max_health;
}	PlayerComponent;

//Composant Farming (cultures, terres agricoles)
typedef struct
{
	Component		base;
	int				growth_stage;		//Etape de croissance
	int				max_growth_stage;	//Etape de croissance maximale
	float			growth_timer;		//Temps avant la prochaine etape
	float			water_level;		//Niveau d'hydratation
	bool			is_watered;			//Arrose aujourd'hui
	int				crop_type;			//Type de culture
	bool			is_harvestable		//Pret a etre recolte
}	FarmingComponent;

//Composant Item (objets pouvant etre ramasses)
typedef struct
{
	Component		base;
	int				item_id;			//ID de l'objet
	int				stack_size;			//Taille de la pile
	int				max_stack_size;		//Taille maximale de la pile
	bool			is_tool;			//Est un outil ou non
	int				tool_type;			//Type d'outil
	int				tool_level;			//Niveau d'outil
}	ItemComponent;

//Composant Interactable (objets interactifs)
typedef struct
{
	Component		base;
	bool			is_active;			//Est actuellement interactif
	float			interaction_radius;	//Rayon d'interaction
	int				interaction_type;	//Type d'interaction
}	InteractableComponent;

//Fonctions utilitaires pour les composants

/*
 * Cree un composant Transform
 * @param entity_id ID de l'entite
 * @param x Position X
 * @param y Position Y
 * @return Composant Transform initialise
*/
TransformComponent*	create_transform_component(EntityID entity_id, float x, float y);

/*
 * Cree un composant Sprite
 * @param entity_id ID de l'entite
 * @param texture_id ID de la texture
 * @param width Largeur
 * @param height Hauteur
 * @return Composant Sprite initialise
*/
SpriteComponent*	create_sprite_component(EntityID entity_id, uint32_t texture_id, int width, int height);

/*
 * Cree un composant Collider
 * @param entity_id	ID de l'entite
 * @param width Largeur
 * @param height Hauteur
 * @param is_trigger Est un trigger ou non
 * @return Composant Collider initialise
*/
ColliderComponent*	create_collider_component(EntityID entity_id, float width, float height, bool is_trigger);

/*
 * Cree un composant Player
 * @param entity_id ID de l'entite
 * @param move_speed Vitesse de deplacement
 * @return Composant Player initialise
*/
PlayerComponent*	create_player_component(EntityID entity_id, float move_speed);

/*
 * Cree un composant Farming
 * @param entity_id ID de l'entite
 * @param crop_type Type de culture
 * @return Composant Farming initialise
*/
FarmingComponent*	create_farming_component(EntityID entity_id, int crop_type);

/*
 * Cree un composant Item
 * @param entity_id ID de l'entite
 * @param item_id ID de l'objet
 * @param stack_size Taille de la pile
 * @return Composant Item initialise
*/
ItemComponent*		create_item_component(EntityID entity_id, int item_id, int stack_size);

/*
 * Cree un composant Interactable
 * @param entity_id ID de l'entite
 * @param interaction_type Type d'interaction
 * @param interaction_radius Rayon d'interaction
 * @return Composant Interactable initialise
*/
InteractableComponent*	create_interactable_component(EntityID entity_id, int interaction_type, float interaction_radius);

#endif
