/**
 * inventory_system.c
 * Implémentation du système de gestion de l'inventaire
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../systems/inventory_system.h"
#include "../utils/error_handler.h"

// Nombre maximal d'objets dans la base de données
#define MAX_ITEMS 1000

// Données des objets de base (semences, produits de base, etc.)
// Ces données devront être enrichies en fonction des besoins du jeu
static ItemData default_items[] = {
    // Semences
    {100, "Semence de Pomme de terre", ITEM_TYPE_SEED, 1, 99, 60, 30, "Semence pour planter une pomme de terre", 0, 1},
    {101, "Semence de Blé", ITEM_TYPE_SEED, 1, 99, 40, 20, "Semence pour planter du blé", 0, 2},
    {102, "Semence d'Oignon", ITEM_TYPE_SEED, 1, 99, 60, 30, "Semence pour planter un oignon", 0, 3},
    {103, "Semence de Navet", ITEM_TYPE_SEED, 1, 99, 50, 25, "Semence pour planter un navet", 0, 4},
    {104, "Semence de Chou-fleur", ITEM_TYPE_SEED, 1, 99, 70, 35, "Semence pour planter un chou-fleur", 0, 5},
    {105, "Semence de Laitue", ITEM_TYPE_SEED, 1, 99, 80, 40, "Semence pour planter de la laitue", 0, 6},
    
    // Produits de récolte
    {200, "Pomme de terre", ITEM_TYPE_CROP, 1, 99, 0, 70, "Une délicieuse pomme de terre", 0, 0},
    {201, "Blé", ITEM_TYPE_CROP, 1, 99, 0, 45, "Du blé frais", 0, 0},
    {202, "Oignon", ITEM_TYPE_CROP, 1, 99, 0, 80, "Un oignon juteux", 0, 0},
    {203, "Navet", ITEM_TYPE_CROP, 1, 99, 0, 65, "Un navet croquant", 0, 0},
    {204, "Chou-fleur", ITEM_TYPE_CROP, 1, 99, 0, 100, "Un gros chou-fleur", 0, 0},
    {205, "Laitue", ITEM_TYPE_CROP, 1, 99, 0, 105, "Une laitue fraîche", 0, 0},
    
    // Matériaux
    {300, "Bois", ITEM_TYPE_MATERIAL, 1, 99, 0, 10, "Du bois brut", 0, 0},
    {301, "Pierre", ITEM_TYPE_MATERIAL, 1, 99, 0, 15, "De la pierre brute", 0, 0},
    {302, "Charbon", ITEM_TYPE_MATERIAL, 1, 99, 0, 25, "Du charbon pour le feu", 0, 0},
    {303, "Minerai de cuivre", ITEM_TYPE_MATERIAL, 1, 99, 0, 30, "Du minerai de cuivre brut", 0, 0},
    {304, "Barre de cuivre", ITEM_TYPE_MATERIAL, 1, 99, 0, 60, "Une barre de cuivre raffinée", 0, 0},
    {305, "Minerai de fer", ITEM_TYPE_MATERIAL, 1, 99, 0, 50, "Du minerai de fer brut", 0, 0},
    {306, "Barre de fer", ITEM_TYPE_MATERIAL, 1, 99, 0, 100, "Une barre de fer raffinée", 0, 0},
    
    // Poissons
    {400, "Carpe", ITEM_TYPE_FISH, 1, 99, 0, 40, "Une carpe fraîche", 0, 0},
    {401, "Truite", ITEM_TYPE_FISH, 1, 99, 0, 65, "Une truite fraîche", 0, 0},
    {402, "Sardine", ITEM_TYPE_FISH, 1, 99, 0, 30, "Une sardine fraîche", 0, 0},
    {403, "Thon", ITEM_TYPE_FISH, 1, 99, 0, 100, "Un thon frais", 0, 0},
    
    // Objets divers
    {500, "Ficelle", ITEM_TYPE_MISC, 1, 99, 20, 10, "De la ficelle standard", 0, 0},
    {501, "Sève", ITEM_TYPE_MISC, 1, 99, 0, 15, "De la sève d'arbre", 0, 0},
    {502, "Herbe sèche", ITEM_TYPE_MISC, 1, 99, 0, 5, "De l'herbe séchée", 0, 0}
};

/**
 * Initialise le système d'inventaire
 * @param entity_manager Gestionnaire d'entités
 * @param render_system Système de rendu
 * @param width Largeur de l'inventaire (nombre de slots)
 * @param height Hauteur de l'inventaire (nombre de slots)
 * @return Pointeur vers le système d'inventaire ou NULL en cas d'erreur
 */
InventorySystem* inventory_system_init(EntityManager* entity_manager, RenderSystem* render_system,
                                     int width, int height) {
    if (!entity_manager || !render_system) {
        log_error("Paramètres invalides pour inventory_system_init");
        return NULL;
    }
    
    if (width <= 0) width = DEFAULT_INVENTORY_WIDTH;
    if (height <= 0) height = DEFAULT_INVENTORY_HEIGHT;
    
    InventorySystem* system = (InventorySystem*)calloc(1, sizeof(InventorySystem));
    if (!check_ptr(system, LOG_LEVEL_ERROR, "Échec d'allocation du système d'inventaire")) {
        return NULL;
    }
    
    system->entity_manager = entity_manager;
    system->render_system = render_system;
    
    // Initialiser la base de données des objets
    system->item_count = sizeof(default_items) / sizeof(ItemData);
    system->item_database = (ItemData*)calloc(MAX_ITEMS, sizeof(ItemData));
    
    if (!check_ptr(system->item_database, LOG_LEVEL_ERROR, "Échec d'allocation de la base de données d'objets")) {
        free(system);
        return NULL;
    }
    
    // Copier les objets par défaut
    memcpy(system->item_database, default_items, sizeof(default_items));
    
    // Initialiser les emplacements d'inventaire
    system->inventory_width = width;
    system->inventory_height = height;
    system->inventory_slots = (InventorySlot*)calloc(width * height, sizeof(InventorySlot));
    
    if (!check_ptr(system->inventory_slots, LOG_LEVEL_ERROR, "Échec d'allocation des emplacements d'inventaire")) {
        free(system->item_database);
        free(system);
        return NULL;
    }
    
    // Initialiser tous les emplacements comme vides
    for (int i = 0; i < width * height; i++) {
        system->inventory_slots[i].item_id = -1;
        system->inventory_slots[i].quantity = 0;
    }
    
    // Initialiser la barre de raccourcis
    system->hotbar_size = HOTBAR_SIZE;
    system->hotbar_slots = (InventorySlot*)calloc(HOTBAR_SIZE, sizeof(InventorySlot));
    
    if (!check_ptr(system->hotbar_slots, LOG_LEVEL_ERROR, "Échec d'allocation des emplacements de barre de raccourcis")) {
        free(system->inventory_slots);
        free(system->item_database);
        free(system);
        return NULL;
    }
    
    // Initialiser tous les emplacements de la barre comme vides
    for (int i = 0; i < HOTBAR_SIZE; i++) {
        system->hotbar_slots[i].item_id = -1;
        system->hotbar_slots[i].quantity = 0;
    }
    
    // Initialiser les autres variables
    system->selected_hotbar_slot = 0;
    system->inventory_open = false;
    system->selected_slot_x = 0;
    system->selected_slot_y = 0;
    system->money = 500; // Argent de départ
    
    log_info("Système d'inventaire initialisé avec %d objets, %dx%d emplacements, %d raccourcis",
           system->item_count, width, height, HOTBAR_SIZE);
    
    return system;
}

/**
 * Libère les ressources du système d'inventaire
 * @param system Système d'inventaire
 */
void inventory_system_shutdown(InventorySystem* system) {
    if (!system) return;
    
    if (system->item_database) {
        free(system->item_database);
        system->item_database = NULL;
    }
    
    if (system->inventory_slots) {
        free(system->inventory_slots);
        system->inventory_slots = NULL;
    }
    
    if (system->hotbar_slots) {
        free(system->hotbar_slots);
        system->hotbar_slots = NULL;
    }
    
    free(system);
    log_info("Système d'inventaire libéré");
}

/**
 * Met à jour le système d'inventaire
 * @param system Système d'inventaire
 * @param delta_time Temps écoulé depuis la dernière mise à jour en secondes
 */
void inventory_system_update(InventorySystem* system, float delta_time) {
    if (!system) return;
    
    // Pour le moment, rien à mettre à jour périodiquement
}

/**
 * Rend l'interface d'inventaire
 * @param system Système d'inventaire
 */
void inventory_system_render(InventorySystem* system) {
    if (!system || !system->render_system) return;
    
    // Rendre toujours la barre de raccourcis en bas de l'écran
    int bar_width = system->hotbar_size * 32 + (system->hotbar_size - 1) * 4; // 32px par slot, 4px de marge
    int bar_height = 32;
    
    int bar_x = system->render_system->internal_width / 2;
    int bar_y = system->render_system->internal_height - bar_height / 2 - 10; // 10px du bas
    
    // Dessiner le fond de la barre
    render_system_draw_rect(
        system->render_system,
        bar_x, bar_y,
        bar_width + 10, bar_height + 10,
        40, 40, 40, 200,
        true
    );
    
    // Dessiner le cadre
    render_system_draw_rect(
        system->render_system,
        bar_x, bar_y,
        bar_width + 10, bar_height + 10,
        80, 80, 80, 255,
        false
    );
    
    // Dessiner chaque emplacement
    int slot_width = 32;
    int slot_height = 32;
    int start_x = bar_x - bar_width / 2 + slot_width / 2 + 5;
    
    for (int i = 0; i < system->hotbar_size; i++) {
        int slot_x = start_x + i * (slot_width + 4);
        
        // Dessiner le fond du slot
        render_system_draw_rect(
            system->render_system,
            slot_x, bar_y,
            slot_width, slot_height,
            60, 60, 60, 200,
            true
        );
        
        // Dessiner un cadre pour le slot sélectionné
        if (i == system->selected_hotbar_slot) {
            render_system_draw_rect(
                system->render_system,
                slot_x, bar_y,
                slot_width, slot_height,
                255, 255, 0, 255,
                false
            );
        } else {
            render_system_draw_rect(
                system->render_system,
                slot_x, bar_y,
                slot_width, slot_height,
                120, 120, 120, 255,
                false
            );
        }
        
        // Dessiner l'objet si présent
        if (system->hotbar_slots[i].item_id >= 0) {
            const ItemData* item_data = inventory_system_get_item_data(system, system->hotbar_slots[i].item_id);
            if (item_data && item_data->sprite_id >= 0) {
                // Dessiner le sprite de l'objet (à adapter selon votre système de rendu)
                // render_system_draw_sprite(system->render_system, item_data->sprite_id, ...);
                
                // En attendant, dessiner un rectangle de couleur différente selon le type
                uint8_t r = 255, g = 255, b = 255;
                switch (item_data->type) {
                    case ITEM_TYPE_TOOL: r = 200; g = 100; b = 100; break;
                    case ITEM_TYPE_SEED: r = 100; g = 200; b = 100; break;
                    case ITEM_TYPE_CROP: r = 100; g = 100; b = 200; break;
                    case ITEM_TYPE_MATERIAL: r = 200; g = 200; b = 100; break;
                    case ITEM_TYPE_FISH: r = 100; g = 200; b = 200; break;
                    default: break;
                }
                
                render_system_draw_rect(
                    system->render_system,
                    slot_x, bar_y,
                    slot_width - 4, slot_height - 4,
                    r, g, b, 255,
                    true
                );
                
                // Dessiner la quantité si > 1
                if (system->hotbar_slots[i].quantity > 1) {
                    char quantity_str[8];
                    snprintf(quantity_str, sizeof(quantity_str), "%d", system->hotbar_slots[i].quantity);
                    render_system_draw_text(
                        system->render_system,
                        quantity_str,
                        slot_x + slot_width / 2 - 4,
                        bar_y + slot_height / 2 - 4,
                        255, 255, 255, 255
                    );
                }
            }
        }
    }
    
    // Si l'inventaire est ouvert, le dessiner
    if (system->inventory_open) {
        int inv_width = system->inventory_width * 32 + (system->inventory_width - 1) * 4;
        int inv_height = system->inventory_height * 32 + (system->inventory_height - 1) * 4;
        
        int inv_x = system->render_system->internal_width / 2;
        int inv_y = system->render_system->internal_height / 2 - 20; // Légèrement au-dessus du centre
        
        // Dessiner le fond de l'inventaire
        render_system_draw_rect(
            system->render_system,
            inv_x, inv_y,
            inv_width + 20, inv_height + 20,
            30, 30, 30, 220,
            true
        );
        
        // Dessiner le cadre
        render_system_draw_rect(
            system->render_system,
            inv_x, inv_y,
            inv_width + 20, inv_height + 20,
            80, 80, 80, 255,
            false
        );
        
        // Dessiner le titre
        render_system_draw_text(
            system->render_system,
            "Inventaire",
            inv_x,
            inv_y - inv_height / 2 - 15,
            255, 255, 255, 255
        );
        
        // Dessiner l'argent
        char money_str[32];
        snprintf(money_str, sizeof(money_str), "Or: %d", system->money);
        render_system_draw_text(
            system->render_system,
            money_str,
            inv_x,
            inv_y - inv_height / 2 - 5,
            255, 215, 0, 255
        );
        
        // Dessiner chaque emplacement
        int start_inv_x = inv_x - inv_width / 2 + slot_width / 2 + 10;
        int start_inv_y = inv_y - inv_height / 2 + slot_height / 2 + 10;
        
        for (int y = 0; y < system->inventory_height; y++) {
            for (int x = 0; x < system->inventory_width; x++) {
                int slot_x = start_inv_x + x * (slot_width + 4);
                int slot_y = start_inv_y + y * (slot_height + 4);
                int slot_index = y * system->inventory_width + x;
                
                // Dessiner le fond du slot
                render_system_draw_rect(
                    system->render_system,
                    slot_x, slot_y,
                    slot_width, slot_height,
                    60, 60, 60, 200,
                    true
                );
                
                // Dessiner un cadre pour le slot sélectionné
                if (x == system->selected_slot_x && y == system->selected_slot_y) {
                    render_system_draw_rect(
                        system->render_system,
                        slot_x, slot_y,
                        slot_width, slot_height,
                        255, 255, 0, 255,
                        false
                    );
                } else {
                    render_system_draw_rect(
                        system->render_system,
                        slot_x, slot_y,
                        slot_width, slot_height,
                        120, 120, 120, 255,
                        false
                    );
                }
                
                // Dessiner l'objet si présent
                if (system->inventory_slots[slot_index].item_id >= 0) {
                    const ItemData* item_data = inventory_system_get_item_data(
                        system, system->inventory_slots[slot_index].item_id
                    );
                    
                    if (item_data && item_data->sprite_id >= 0) {
                        // Dessiner le sprite de l'objet (à adapter selon votre système de rendu)
                        // render_system_draw_sprite(system->render_system, item_data->sprite_id, ...);
                        
                        // En attendant, dessiner un rectangle de couleur différente selon le type
                        uint8_t r = 255, g = 255, b = 255;
                        switch (item_data->type) {
                            case ITEM_TYPE_TOOL: r = 200; g = 100; b = 100; break;
                            case ITEM_TYPE_SEED: r = 100; g = 200; b = 100; break;
                            case ITEM_TYPE_CROP: r = 100; g = 100; b = 200; break;
                            case ITEM_TYPE_MATERIAL: r = 200; g = 200; b = 100; break;
                            case ITEM_TYPE_FISH: r = 100; g = 200; b = 200; break;
                            default: break;
                        }
                        
                        render_system_draw_rect(
                            system->render_system,
                            slot_x, slot_y,
                            slot_width - 4, slot_height - 4,
                            r, g, b, 255,
                            true
                        );
                        
                        // Dessiner la quantité si > 1
                        if (system->inventory_slots[slot_index].quantity > 1) {
                            char quantity_str[8];
                            snprintf(quantity_str, sizeof(quantity_str), "%d", system->inventory_slots[slot_index].quantity);
                            render_system_draw_text(
                                system->render_system,
                                quantity_str,
                                slot_x + slot_width / 2 - 4,
                                slot_y + slot_height / 2 - 4,
                                255, 255, 255, 255
                            );
                        }
                    }
                }
            }
        }
        
        // Si un slot est sélectionné et contient un objet, afficher sa description
        if (system->selected_slot_x >= 0 && system->selected_slot_y >= 0) {
            int slot_index = system->selected_slot_y * system->inventory_width + system->selected_slot_x;
            if (slot_index < system->inventory_width * system->inventory_height) {
                int item_id = system->inventory_slots[slot_index].item_id;
                if (item_id >= 0) {
                    const ItemData* item_data = inventory_system_get_item_data(system, item_id);
                    if (item_data) {
                        // Dessiner un fond pour la description
                        render_system_draw_rect(
                            system->render_system,
                            inv_x,
                            inv_y + inv_height / 2 + 30,
                            inv_width,
                            40,
                            20, 20, 20, 200,
                            true
                        );
                        
                        // Dessiner le nom de l'objet
                        render_system_draw_text(
                            system->render_system,
                            item_data->name,
                            inv_x,
                            inv_y + inv_height / 2 + 20,
                            255, 255, 255, 255
                        );
                        
                        // Dessiner la description
                        render_system_draw_text(
                            system->render_system,
                            item_data->description,
                            inv_x,
                            inv_y + inv_height / 2 + 35,
                            200, 200, 200, 255
                        );
                    }
                }
            }
        }
    }
}

/**
 * Ouvre ou ferme l'inventaire
 * @param system Système d'inventaire
 * @param open true pour ouvrir, false pour fermer
 */
void inventory_system_toggle(InventorySystem* system, bool open) {
    if (!system) return;
    
    system->inventory_open = open;
    
    if (open) {
        log_info("Inventaire ouvert");
    } else {
        log_info("Inventaire fermé");
    }
}

/**
 * Trouve le premier emplacement vide ou contenant l'objet spécifié
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @param required_space Espace requis
 * @return Index de l'emplacement ou -1 si non trouvé
 */
static int find_free_or_stackable_slot(InventorySystem* system, int item_id, int required_space) {
    if (!system) return -1;
    
    // D'abord, chercher un emplacement contenant déjà cet objet avec de l'espace
    const ItemData* item_data = inventory_system_get_item_data(system, item_id);
    if (!item_data) return -1;
    
    // Parcourir les emplacements d'inventaire
    for (int i = 0; i < system->inventory_width * system->inventory_height; i++) {
        if (system->inventory_slots[i].item_id == item_id) {
            // Vérifier s'il y a assez d'espace dans cet emplacement
            if (system->inventory_slots[i].quantity + required_space <= item_data->max_stack_size) {
                return i;
            }
        }
    }
    
    // Ensuite, chercher un emplacement vide
    for (int i = 0; i < system->inventory_width * system->inventory_height; i++) {
        if (system->inventory_slots[i].item_id == -1) {
            return i;
        }
    }
    
    return -1; // Aucun emplacement trouvé
}

/**
 * Ajoute un objet à l'inventaire
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @param quantity Quantité à ajouter
 * @return true si l'ajout a réussi, false sinon
 */
bool inventory_system_add_item(InventorySystem* system, int item_id, int quantity) {
    if (!system || item_id < 0 || quantity <= 0) return false;
    
    // Vérifier si l'objet existe
    const ItemData* item_data = inventory_system_get_item_data(system, item_id);
    if (!item_data) {
        log_warning("Tentative d'ajout d'un objet inconnu (ID: %d)", item_id);
        return false;
    }
    
    // Compteur d'objets restants à ajouter
    int remaining = quantity;
    
    // Tant qu'il reste des objets à ajouter, chercher un emplacement approprié
    while (remaining > 0) {
        int slot_index = find_free_or_stackable_slot(system, item_id, 1);
        if (slot_index == -1) {
            log_warning("Inventaire plein, impossible d'ajouter %d %s", remaining, item_data->name);
            break;
        }
        
        // Calculer combien d'objets peuvent être ajoutés à cet emplacement
        int can_add = item_data->max_stack_size;
        if (system->inventory_slots[slot_index].item_id == item_id) {
            can_add -= system->inventory_slots[slot_index].quantity;
        }
        
        // Limiter à la quantité restante
        if (can_add > remaining) {
            can_add = remaining;
        }
        
        // Ajouter les objets
        if (system->inventory_slots[slot_index].item_id == -1) {
            // Nouvel emplacement
            system->inventory_slots[slot_index].item_id = item_id;
            system->inventory_slots[slot_index].quantity = can_add;
        } else {
            // Emplacement existant
            system->inventory_slots[slot_index].quantity += can_add;
        }
        
        remaining -= can_add;
    }
    
    // Retourner vrai si tous les objets ont été ajoutés
    bool success = (remaining == 0);
    
    if (success) {
        log_info("%d %s ajouté(s) à l'inventaire", quantity, item_data->name);
    } else {
        log_warning("Seulement %d/%d %s ajouté(s) à l'inventaire", quantity - remaining, quantity, item_data->name);
    }
    
    return success;
}

/**
 * Retire un objet de l'inventaire
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @param quantity Quantité à retirer
 * @return true si le retrait a réussi, false sinon
 */
bool inventory_system_remove_item(InventorySystem* system, int item_id, int quantity) {
    if (!system || item_id < 0 || quantity <= 0) return false;
    
    // Vérifier si l'objet existe
    const ItemData* item_data = inventory_system_get_item_data(system, item_id);
    if (!item_data) {
        log_warning("Tentative de retrait d'un objet inconnu (ID: %d)", item_id);
        return false;
    }
    
    // Vérifier la quantité totale disponible
    int total_available = inventory_system_count_item(system, item_id);
    if (total_available < quantity) {
        log_warning("Pas assez de %s dans l'inventaire (%d/%d)", item_data->name, total_available, quantity);
        return false;
    }
    
    // Compteur d'objets restants à retirer
    int remaining = quantity;
    
    // Parcourir tous les emplacements pour retirer les objets
    for (int i = 0; i < system->inventory_width * system->inventory_height && remaining > 0; i++) {
        if (system->inventory_slots[i].item_id == item_id) {
            // Calculer combien d'objets peuvent être retirés de cet emplacement
            int can_remove = system->inventory_slots[i].quantity;
            if (can_remove > remaining) {
                can_remove = remaining;
            }
            
            // Retirer les objets
            system->inventory_slots[i].quantity -= can_remove;
            
            // Si l'emplacement est vide, le marquer comme tel
            if (system->inventory_slots[i].quantity <= 0) {
                system->inventory_slots[i].item_id = -1;
                system->inventory_slots[i].quantity = 0;
            }
            
            remaining -= can_remove;
        }
    }
    
    // Faire de même pour la barre de raccourcis
    for (int i = 0; i < system->hotbar_size && remaining > 0; i++) {
        if (system->hotbar_slots[i].item_id == item_id) {
            // Calculer combien d'objets peuvent être retirés de cet emplacement
            int can_remove = system->hotbar_slots[i].quantity;
            if (can_remove > remaining) {
                can_remove = remaining;
            }
            
            // Retirer les objets
            system->hotbar_slots[i].quantity -= can_remove;
            
            // Si l'emplacement est vide, le marquer comme tel
            if (system->hotbar_slots[i].quantity <= 0) {
                system->hotbar_slots[i].item_id = -1;
                system->hotbar_slots[i].quantity = 0;
            }
            
            remaining -= can_remove;
        }
    }
    
    // À ce stade, remaining devrait être 0
    log_info("%d %s retiré(s) de l'inventaire", quantity, item_data->name);
    
    return true;
}

/**
 * Vérifie si l'inventaire contient un objet
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @param quantity Quantité minimale (1 par défaut)
 * @return true si l'objet est présent en quantité suffisante, false sinon
 */
bool inventory_system_has_item(InventorySystem* system, int item_id, int quantity) {
    if (!system || item_id < 0) return false;
    if (quantity <= 0) quantity = 1;
    
    int count = inventory_system_count_item(system, item_id);
    return count >= quantity;
}

/**
 * Compte le nombre d'un objet spécifique dans l'inventaire
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @return Quantité de l'objet dans l'inventaire
 */
int inventory_system_count_item(InventorySystem* system, int item_id) {
    if (!system || item_id < 0) return 0;
    
    int count = 0;
    
    // Compter dans l'inventaire principal
    for (int i = 0; i < system->inventory_width * system->inventory_height; i++) {
        if (system->inventory_slots[i].item_id == item_id) {
            count += system->inventory_slots[i].quantity;
        }
    }
    
    // Compter dans la barre de raccourcis
    for (int i = 0; i < system->hotbar_size; i++) {
        if (system->hotbar_slots[i].item_id == item_id) {
            count += system->hotbar_slots[i].quantity;
        }
    }
    
    return count;
}

/**
 * Sélectionne un emplacement dans la barre de raccourcis
 * @param system Système d'inventaire
 * @param slot_index Index de l'emplacement (0-9)
 * @return true si la sélection a réussi, false sinon
 */
bool inventory_system_select_hotbar_slot(InventorySystem* system, int slot_index) {
    if (!system || slot_index < 0 || slot_index >= system->hotbar_size) return false;
    
    system->selected_hotbar_slot = slot_index;
    log_debug("Emplacement %d sélectionné dans la barre de raccourcis", slot_index);
    
    return true;
}

/**
 * Récupère l'objet actuellement sélectionné dans la barre de raccourcis
 * @param system Système d'inventaire
 * @return ID de l'objet ou -1 si aucun
 */
int inventory_system_get_selected_item(InventorySystem* system) {
    if (!system || system->selected_hotbar_slot < 0 || system->selected_hotbar_slot >= system->hotbar_size) {
        return -1;
    }
    
    return system->hotbar_slots[system->selected_hotbar_slot].item_id;
}

/**
 * Déplace un objet d'un emplacement à un autre
 * @param system Système d'inventaire
 * @param from_x Position X source
 * @param from_y Position Y source
 * @param to_x Position X destination
 * @param to_y Position Y destination
 * @return true si le déplacement a réussi, false sinon
 */
bool inventory_system_move_item(InventorySystem* system, int from_x, int from_y, int to_x, int to_y) {
    if (!system) return false;
    
    // Vérifier les limites
    if (from_x < 0 || from_x >= system->inventory_width ||
        from_y < 0 || from_y >= system->inventory_height ||
        to_x < 0 || to_x >= system->inventory_width ||
        to_y < 0 || to_y >= system->inventory_height) {
        return false;
    }
    
    // Calculer les indices
    int from_index = from_y * system->inventory_width + from_x;
    int to_index = to_y * system->inventory_width + to_x;
    
    // Vérifier si les indices sont valides
    if (from_index < 0 || from_index >= system->inventory_width * system->inventory_height ||
        to_index < 0 || to_index >= system->inventory_width * system->inventory_height ||
        from_index == to_index) {
        return false;
    }
    
    // Vérifier si l'emplacement source contient un objet
    if (system->inventory_slots[from_index].item_id == -1) {
        return false;
    }
    
    // Si l'emplacement destination est vide, simplement déplacer l'objet
    if (system->inventory_slots[to_index].item_id == -1) {
        system->inventory_slots[to_index] = system->inventory_slots[from_index];
        system->inventory_slots[from_index].item_id = -1;
        system->inventory_slots[from_index].quantity = 0;
        return true;
    }
    
    // Si les deux emplacements contiennent le même objet, essayer de les empiler
    if (system->inventory_slots[from_index].item_id == system->inventory_slots[to_index].item_id) {
        const ItemData* item_data = inventory_system_get_item_data(system, system->inventory_slots[from_index].item_id);
        if (!item_data) return false;
        
        // Calculer combien d'objets peuvent être déplacés
        int max_quantity = item_data->max_stack_size - system->inventory_slots[to_index].quantity;
        if (max_quantity <= 0) return false; // La pile destination est pleine
        
        int move_quantity = system->inventory_slots[from_index].quantity;
        if (move_quantity > max_quantity) move_quantity = max_quantity;
        
        // Déplacer les objets
        system->inventory_slots[to_index].quantity += move_quantity;
        system->inventory_slots[from_index].quantity -= move_quantity;
        
        // Si l'emplacement source est vide, le marquer comme tel
        if (system->inventory_slots[from_index].quantity <= 0) {
            system->inventory_slots[from_index].item_id = -1;
            system->inventory_slots[from_index].quantity = 0;
        }
        
        return true;
    }
    
    // Si les objets sont différents, échanger les emplacements
    InventorySlot temp = system->inventory_slots[from_index];
    system->inventory_slots[from_index] = system->inventory_slots[to_index];
    system->inventory_slots[to_index] = temp;
    
    return true;
}

/**
 * Ajoute de l'argent au joueur
 * @param system Système d'inventaire
 * @param amount Montant à ajouter
 */
void inventory_system_add_money(InventorySystem* system, int amount) {
    if (!system || amount <= 0) return;
    
    system->money += amount;
    log_info("%d or ajouté (total: %d)", amount, system->money);
}

/**
 * Retire de l'argent au joueur
 * @param system Système d'inventaire
 * @param amount Montant à retirer
 * @return true si le retrait a réussi, false sinon
 */
bool inventory_system_remove_money(InventorySystem* system, int amount) {
    if (!system || amount <= 0) return false;
    
    if (system->money < amount) {
        log_warning("Pas assez d'or (%d/%d)", system->money, amount);
        return false;
    }
    
    system->money -= amount;
    log_info("%d or retiré (reste: %d)", amount, system->money);
    
    return true;
}

/**
 * Récupère le montant d'argent du joueur
 * @param system Système d'inventaire
 * @return Montant d'argent
 */
int inventory_system_get_money(InventorySystem* system) {
    if (!system) return 0;
    
    return system->money;
}

/**
 * Récupère les données d'un objet par son ID
 * @param system Système d'inventaire
 * @param item_id ID de l'objet
 * @return Pointeur vers les données de l'objet ou NULL si non trouvé
 */
const ItemData* inventory_system_get_item_data(InventorySystem* system, int item_id) {
    if (!system || item_id < 0) return NULL;
    
    for (int i = 0; i < system->item_count; i++) {
        if (system->item_database[i].id == item_id) {
            return &system->item_database[i];
        }
    }
    
    return NULL;
}