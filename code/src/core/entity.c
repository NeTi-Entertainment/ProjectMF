/**
 * entity.c
 * Implémentation des fonctions de création de composants
 */

#include <stdlib.h>
#include <string.h>
#include "../core/entity.h"
#include "../utils/error_handler.h"

// Fonction utilitaire pour initialiser la base d'un composant
static void initialize_component_base(Component* base, ComponentType type, EntityID entity_id) {
    if (base) {
        base->type = type;
        base->entity = entity_id;
    }
}

// Crée un composant Transform
TransformComponent* create_transform_component(EntityID entity_id, float x, float y) {
    TransformComponent* component = (TransformComponent*)calloc(1, sizeof(TransformComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Transform")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_TRANSFORM, entity_id);
    
    component->x = x;
    component->y = y;
    component->rotation = 0.0f;
    component->scale_x = 1.0f;
    component->scale_y = 1.0f;
    
    return component;
}

// Crée un composant Sprite
SpriteComponent* create_sprite_component(EntityID entity_id, uint32_t texture_id, int width, int height) {
    SpriteComponent* component = (SpriteComponent*)calloc(1, sizeof(SpriteComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Sprite")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_SPRITE, entity_id);
    
    component->texture_id = texture_id;
    component->width = width;
    component->height = height;
    component->sprite_sheet_x = 0;
    component->sprite_sheet_y = 0;
    component->z_order = 0;
    component->visible = true;
    
    return component;
}

// Crée un composant Collider
ColliderComponent* create_collider_component(EntityID entity_id, float width, float height, CollisionType collision_type) {
    ColliderComponent* component = (ColliderComponent*)calloc(1, sizeof(ColliderComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Collider")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_COLLIDER, entity_id);
    
    component->width = width;
    component->height = height;
    component->offset_x = 0.0f;
    component->offset_y = 0.0f;
    component->type = collision_type;
    component->collision_mask = 0xFFFFFFFF;  // Par défaut, collisionne avec tout
    component->collision_layer = 1;          // Couche par défaut
    component->is_trigger = (collision_type == COLLISION_TRIGGER);
    
    return component;
}

// Crée un composant Animation
AnimationComponent* create_animation_component(EntityID entity_id) {
    AnimationComponent* component = (AnimationComponent*)calloc(1, sizeof(AnimationComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Animation")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_ANIMATION, entity_id);
    
    // Initialiser les animations à NULL
    for (int i = 0; i < 8; i++) {
        component->animations[i] = NULL;
    }
    
    component->current_animation = 0;
    component->current_time = 0.0f;
    component->current_frame = 0;
    component->is_playing = false;
    component->loop = true;
    
    return component;
}

// Crée un composant Player
PlayerComponent* create_player_component(EntityID entity_id, float move_speed) {
    PlayerComponent* component = (PlayerComponent*)calloc(1, sizeof(PlayerComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Player")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_PLAYER, entity_id);
    
    component->move_speed = move_speed;
    component->stamina = 100;
    component->max_stamina = 100;
    component->health = 100;
    component->max_health = 100;
    
    return component;
}

// Crée un composant Farming
FarmingComponent* create_farming_component(EntityID entity_id, int crop_type) {
    FarmingComponent* component = (FarmingComponent*)calloc(1, sizeof(FarmingComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Farming")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_FARMING, entity_id);
    
    component->crop_type = crop_type;
    component->growth_stage = 0;
    component->max_growth_stage = 5;  // Par défaut 5 étapes de croissance
    component->growth_timer = 0.0f;
    component->water_level = 0.0f;
    component->is_watered = false;
    component->is_harvestable = false;
    
    return component;
}

// Crée un composant Item
ItemComponent* create_item_component(EntityID entity_id, int item_id, int stack_size) {
    ItemComponent* component = (ItemComponent*)calloc(1, sizeof(ItemComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Item")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_ITEM, entity_id);
    
    component->item_id = item_id;
    component->stack_size = stack_size;
    component->max_stack_size = 99;  // Par défaut 99 items maximum par stack
    component->is_tool = false;
    component->tool_type = 0;
    component->tool_level = 0;
    
    return component;
}

// Crée un composant Interactable
InteractableComponent* create_interactable_component(EntityID entity_id, int interaction_type, float interaction_radius) {
    InteractableComponent* component = (InteractableComponent*)calloc(1, sizeof(InteractableComponent));
    if (!check_ptr(component, LOG_LEVEL_ERROR, "Échec d'allocation du composant Interactable")) {
        return NULL;
    }
    
    initialize_component_base(&component->base, COMPONENT_INTERACTABLE, entity_id);
    
    component->interaction_type = interaction_type;
    component->interaction_radius = interaction_radius;
    component->is_active = true;
    
    return component;
}