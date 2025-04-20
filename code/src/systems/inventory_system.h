/**
 * inventory_system.h
 * Système de gestion de l'inventaire
 */

#ifndef INVENTORY_SYSTEM_H
#define INVENTORY_SYSTEM_H

#include <stdbool.h>
#include "../systems/entity_manager.h"
#include "../systems/render.h"

// Dimensions par défaut de la grille d'inventaire
#define DEFAULT_INVENTORY_WIDTH 8
#define DEFAULT_INVENTORY_HEIGHT 6
#define HOTBAR_SIZE 10  // Taille de la barre de raccourcis

// Types d'objets dans l'inventaire
typedef enum {
    ITEM_TYPE_NONE,       // Emplacement vide
    ITEM_TYPE_TOOL,       // Outil
    ITEM_TYPE_SEED,       // Graine
    ITEM_TYPE_CROP,       // Récolte
    ITEM_TYPE_MATERIAL,   // Matériau (bois, pierre, etc.)
    ITEM_TYPE_CRAFTABLE,  // Objet fabriqué
    ITEM_TYPE_ANIMAL_PRODUCT, // Produit animal (œuf, lait, etc.)
    ITEM_TYPE_FISH,       // Poisson
    ITEM_TYPE_MISC        // Divers
} ItemType;

// Structure pour un objet
typedef struct {
    int id;               // ID unique de l'objet
    char name[32];        // Nom de l'objet
    ItemType type;        // Type d'objet
    int stack_size;       // Taille de pile actuelle
    int max_stack_size;   // Taille de pile maximale
    int buy_price;        // Prix d'achat
    int sell_price;       // Prix de vente
    char description[128]; // Description de l'objet
    int sprite_id;        // ID du sprite de l'objet
    int associated_id;    // ID associé (ex: ID d'outil, ID de plante pour les graines)
} ItemData;

// Structure pour un emplacement d'inventaire
typedef struct {
    int item_id;          // ID de l'objet (-1 si vide)
    int quantity;         // Quantité d'objets dans cet emplacement
} InventorySlot;

// Système d'inventaire
typedef struct {
    EntityManager* entity_manager;      // Référence au gestionnaire d'entités
    RenderSystem* render_system;        // Référence au système de rendu
    
    ItemData* item_database;            // Base de données des objets
    int item_count;                     // Nombre d'objets dans la base
    
    InventorySlot* inventory_slots;     // Emplacements d'inventaire
    int inventory_width;                // Largeur de l'inventaire
    int inventory_height;               // Hauteur de l'inventaire
    
    InventorySlot* hotbar_slots;        // Emplacements de la barre de raccourcis
    int hotbar_size;                    // Taille de la barre de raccourcis
    int selected_hotbar_slot;           // Emplacement sélectionné dans la barre
    
    bool inventory_open;                // L'inventaire est ouvert
    int selected_slot_x;                // Position X du slot sélectionné
    int selected_slot_y;                // Position Y du slot sélectionné
    
    int money;                          // Argent du joueur
} InventorySystem;

/**
 * Initialise le système d'inventaire
 * @param entity_manager Gestionnaire d'entités
 * @param render_system Système de rendu
 * @param width Largeur de l'inventaire (nombre de slots)
 * @param height Hauteur de l'inventaire (nombre de slots)
 * @return Pointeur vers le système d'inventaire ou NULL en cas d'erreur
 */
InventorySystem* inventory_system_init(EntityManager* entity_manager, RenderSystem* render_system,
                                     int width, int height);

/**
 * Libère les ressources du système d'inventaire
 * @param system Système d'inventaire
 */
void inventory_system_shutdown(InventorySystem* system);

/**
 * Met à jour le système d'inventaire
 * @param system Système d'inventaire
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void inventory_system_update(InventorySystem* system, float delta_time);

/**
 * Rend l'interface d'inventaire
 * @param system Système d'inventaire
 */
void inventory_system_render(InventorySystem* system);

/**
 * Ouvre ou ferme l'inventaire
 * @param system Système d'inventaire
 * @param open true pour ouvrir, false pour fermer
 */
void inventory_system_toggle(InventorySystem* system, bool open);

/**
 * Ajoute un objet à l'inventaire
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @param quantity Quantité à ajouter
 * @return true si l'ajout a réussi, false sinon
 */
bool inventory_system_add_item(InventorySystem* system, int item_id, int quantity);

/**
 * Retire un objet de l'inventaire
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @param quantity Quantité à retirer
 * @return true si le retrait a réussi, false sinon
 */
bool inventory_system_remove_item(InventorySystem* system, int item_id, int quantity);

/**
 * Vérifie si l'inventaire contient un objet
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @param quantity Quantité minimale (1 par défaut)
 * @return true si l'objet est présent en quantité suffisante, false sinon
 */
bool inventory_system_has_item(InventorySystem* system, int item_id, int quantity);

/**
 * Compte le nombre d'un objet spécifique dans l'inventaire
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @return Quantité de l'objet dans l'inventaire
 */
int inventory_system_count_item(InventorySystem* system, int item_id);

/**
 * Sélectionne un emplacement dans la barre de raccourcis
 * @param system Système d'inventaire
 * @param slot_index Index de l'emplacement (0-9)
 * @return true si la sélection a réussi, false sinon
 */
bool inventory_system_select_hotbar_slot(InventorySystem* system, int slot_index);

/**
 * Récupère l'objet actuellement sélectionné dans la barre de raccourcis
 * @param system Système d'inventaire
 * @return ID de l'objet ou -1 si aucun
 */
int inventory_system_get_selected_item(InventorySystem* system);

/**
 * Déplace un objet d'un emplacement à un autre
 * @param system Système d'inventaire
 * @param from_x Position X source
 * @param from_y Position Y source
 * @param to_x Position X destination
 * @param to_y Position Y destination
 * @return true si le déplacement a réussi, false sinon
 */
bool inventory_system_move_item(InventorySystem* system, int from_x, int from_y, int to_x, int to_y);

/**
 * Ajoute de l'argent au joueur
 * @param system Système d'inventaire
 * @param amount Montant à ajouter
 */
void inventory_system_add_money(InventorySystem* system, int amount);

/**
 * Retire de l'argent au joueur
 * @param system Système d'inventaire
 * @param amount Montant à retirer
 * @return true si le retrait a réussi, false sinon
 */
bool inventory_system_remove_money(InventorySystem* system, int amount);

/**
 * Récupère le montant d'argent du joueur
 * @param system Système d'inventaire
 * @return Montant d'argent
 */
int inventory_system_get_money(InventorySystem* system);

/**
 * Récupère les données d'un objet par son ID
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @return Pointeur vers les données de l'objet ou NULL si non trouvé
 */
const ItemData* inventory_system_get_item_data(InventorySystem* system, int item_id);

#endif /* INVENTORY_SYSTEM_H */
