/**
 * tiled_parser.c
 * Implémentation du système de chargement et parsage des cartes créées avec Tiled
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../systems/tiled_parser.h"
#include "../utils/error_handler.h"
#include "../thirdparty/cJSON.h" // Il faudra ajouter cJSON à votre projet

// Fonction utilitaire pour lire un fichier entier
static char* read_entire_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        log_error("Impossible d'ouvrir le fichier %s", filename);
        return NULL;
    }
    
    // Déterminer la taille du fichier
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allouer la mémoire pour le contenu
    char* content = (char*)malloc(file_size + 1);
    if (!content) {
        log_error("Échec d'allocation mémoire pour le fichier %s", filename);
        fclose(file);
        return NULL;
    }
    
    // Lire le contenu
    size_t read_size = fread(content, 1, file_size, file);
    fclose(file);
    
    if (read_size != file_size) {
        log_error("Échec de lecture du fichier %s", filename);
        free(content);
        return NULL;
    }
    
    content[file_size] = '\0'; // Ajouter le caractère de fin de chaîne
    return content;
}

// Fonction utilitaire pour extraire le chemin du dossier d'un fichier
static char* get_directory_path(const char* filename) {
    // Trouver le dernier séparateur de chemin
    const char* last_slash = strrchr(filename, '/');
    const char* last_backslash = strrchr(filename, '\\');
    
    const char* last_separator = last_slash > last_backslash ? last_slash : last_backslash;
    
    if (!last_separator) {
        // Aucun séparateur trouvé, on retourne "./"
        char* result = (char*)malloc(3);
        strcpy(result, "./");
        return result;
    }
    
    // Allouer et copier le chemin du dossier
    size_t length = last_separator - filename + 1;
    char* result = (char*)malloc(length + 1);
    if (!result) {
        log_error("Échec d'allocation mémoire pour le chemin du dossier");
        return NULL;
    }
    
    strncpy(result, filename, length);
    result[length] = '\0';
    
    return result;
}

// Fonction pour charger une propriété
static TiledProperty* load_property(cJSON* property_json) {
    if (!property_json) return NULL;
    
    TiledProperty* property = (TiledProperty*)calloc(1, sizeof(TiledProperty));
    if (!property) {
        log_error("Échec d'allocation mémoire pour une propriété");
        return NULL;
    }
    
    // Charger le nom
    cJSON* name_json = cJSON_GetObjectItem(property_json, "name");
    if (name_json && name_json->type == cJSON_String) {
        property->name = strdup(name_json->valuestring);
    } else {
        property->name = strdup("unknown");
    }
    
    // Charger le type
    cJSON* type_json = cJSON_GetObjectItem(property_json, "type");
    if (type_json && type_json->type == cJSON_String) {
        property->type = strdup(type_json->valuestring);
    } else {
        property->type = strdup("string");
    }
    
    // Charger la valeur (selon le type)
    if (strcmp(property->type, "string") == 0) {
        cJSON* value_json = cJSON_GetObjectItem(property_json, "value");
        if (value_json && value_json->type == cJSON_String) {
            property->value = strdup(value_json->valuestring);
        } else {
            property->value = strdup("");
        }
    } else if (strcmp(property->type, "int") == 0 || strcmp(property->type, "float") == 0) {
        cJSON* value_json = cJSON_GetObjectItem(property_json, "value");
        if (value_json) {
            char buffer[32];
            sprintf(buffer, "%f", value_json->valuedouble);
            property->value = strdup(buffer);
        } else {
            property->value = strdup("0");
        }
    } else if (strcmp(property->type, "bool") == 0) {
        cJSON* value_json = cJSON_GetObjectItem(property_json, "value");
        if (value_json) {
            property->value = strdup(value_json->valueint ? "true" : "false");
        } else {
            property->value = strdup("false");
        }
    } else {
        property->value = strdup("");
    }
    
    return property;
}

// Fonction pour charger les propriétés
static void load_properties(cJSON* properties_json, TiledProperty*** properties, int* property_count) {
    if (!properties_json || properties_json->type != cJSON_Array) {
        *properties = NULL;
        *property_count = 0;
        return;
    }
    
    // Compter le nombre de propriétés
    int count = cJSON_GetArraySize(properties_json);
    if (count <= 0) {
        *properties = NULL;
        *property_count = 0;
        return;
    }
    
    // Allouer le tableau de propriétés
    *properties = (TiledProperty**)calloc(count, sizeof(TiledProperty*));
    if (!*properties) {
        log_error("Échec d'allocation mémoire pour les propriétés");
        *property_count = 0;
        return;
    }
    
    // Charger chaque propriété
    int valid_count = 0;
    for (int i = 0; i < count; i++) {
        cJSON* property_json = cJSON_GetArrayItem(properties_json, i);
        TiledProperty* property = load_property(property_json);
        
        if (property) {
            (*properties)[valid_count++] = property;
        }
    }
    
    *property_count = valid_count;
}

// Fonction pour charger un tileset
static TiledTileset* load_tileset(cJSON* tileset_json) {
    if (!tileset_json) return NULL;
    
    TiledTileset* tileset = (TiledTileset*)calloc(1, sizeof(TiledTileset));
    if (!tileset) {
        log_error("Échec d'allocation mémoire pour un tileset");
        return NULL;
    }
    
    // Extraire les informations de base
    cJSON* firstgid_json = cJSON_GetObjectItem(tileset_json, "firstgid");
    cJSON* name_json = cJSON_GetObjectItem(tileset_json, "name");
    cJSON* source_json = cJSON_GetObjectItem(tileset_json, "source");
    cJSON* tile_width_json = cJSON_GetObjectItem(tileset_json, "tilewidth");
    cJSON* tile_height_json = cJSON_GetObjectItem(tileset_json, "tileheight");
    cJSON* spacing_json = cJSON_GetObjectItem(tileset_json, "spacing");
    cJSON* margin_json = cJSON_GetObjectItem(tileset_json, "margin");
    cJSON* columns_json = cJSON_GetObjectItem(tileset_json, "columns");
    cJSON* image_json = cJSON_GetObjectItem(tileset_json, "image");
    cJSON* image_width_json = cJSON_GetObjectItem(tileset_json, "imagewidth");
    cJSON* image_height_json = cJSON_GetObjectItem(tileset_json, "imageheight");
    cJSON* tile_count_json = cJSON_GetObjectItem(tileset_json, "tilecount");
    
    // Remplir la structure
    tileset->firstgid = firstgid_json ? firstgid_json->valueint : 1;
    tileset->name = name_json && name_json->type == cJSON_String ? strdup(name_json->valuestring) : strdup("unknown");
    tileset->source = source_json && source_json->type == cJSON_String ? strdup(source_json->valuestring) : NULL;
    
    tileset->tile_width = tile_width_json ? tile_width_json->valueint : 0;
    tileset->tile_height = tile_height_json ? tile_height_json->valueint : 0;
    tileset->spacing = spacing_json ? spacing_json->valueint : 0;
    tileset->margin = margin_json ? margin_json->valueint : 0;
    tileset->columns = columns_json ? columns_json->valueint : 0;
    
    tileset->image_source = image_json && image_json->type == cJSON_String ? strdup(image_json->valuestring) : NULL;
    tileset->image_width = image_width_json ? image_width_json->valueint : 0;
    tileset->image_height = image_height_json ? image_height_json->valueint : 0;
    tileset->tile_count = tile_count_json ? tile_count_json->valueint : 0;
    
    // Charger les propriétés si présentes
    cJSON* properties_json = cJSON_GetObjectItem(tileset_json, "properties");
    load_properties(properties_json, &tileset->properties, &tileset->property_count);
    
    tileset->texture_id = -1; // Texture pas encore chargée
    
    return tileset;
}

// Fonction pour charger une couche de tuiles
static TiledLayer* load_layer(cJSON* layer_json) {
    if (!layer_json) return NULL;
    
    // Vérifier que c'est bien une couche de tuiles
    cJSON* type_json = cJSON_GetObjectItem(layer_json, "type");
    if (!type_json || type_json->type != cJSON_String || strcmp(type_json->valuestring, "tilelayer") != 0) {
        return NULL;
    }
    
    TiledLayer* layer = (TiledLayer*)calloc(1, sizeof(TiledLayer));
    if (!layer) {
        log_error("Échec d'allocation mémoire pour une couche");
        return NULL;
    }
    
    // Extraire les informations de base
    cJSON* name_json = cJSON_GetObjectItem(layer_json, "name");
    cJSON* width_json = cJSON_GetObjectItem(layer_json, "width");
    cJSON* height_json = cJSON_GetObjectItem(layer_json, "height");
    cJSON* opacity_json = cJSON_GetObjectItem(layer_json, "opacity");
    cJSON* visible_json = cJSON_GetObjectItem(layer_json, "visible");
    cJSON* data_json = cJSON_GetObjectItem(layer_json, "data");
    
    // Remplir la structure
    layer->name = name_json && name_json->type == cJSON_String ? strdup(name_json->valuestring) : strdup("unknown");
    layer->width = width_json ? width_json->valueint : 0;
    layer->height = height_json ? height_json->valueint : 0;
    layer->opacity = opacity_json ? (float)opacity_json->valuedouble : 1.0f;
    layer->visible = visible_json ? (visible_json->valueint != 0) : true;
    
    // Charger les données des tuiles
    if (data_json && data_json->type == cJSON_Array) {
        int data_size = cJSON_GetArraySize(data_json);
        layer->data = (int*)calloc(data_size, sizeof(int));
        
        if (layer->data) {
            for (int i = 0; i < data_size; i++) {
                cJSON* tile_json = cJSON_GetArrayItem(data_json, i);
                if (tile_json) {
                    layer->data[i] = tile_json->valueint;
                } else {
                    layer->data[i] = 0;
                }
            }
        }
    } else {
        layer->data = NULL;
    }
    
    // Charger les propriétés si présentes
    cJSON* properties_json = cJSON_GetObjectItem(layer_json, "properties");
    load_properties(properties_json, &layer->properties, &layer->property_count);
    
    return layer;
}

// Fonction pour charger un objet
static TiledObject* load_object(cJSON* object_json) {
    if (!object_json) return NULL;
    
    TiledObject* object = (TiledObject*)calloc(1, sizeof(TiledObject));
    if (!object) {
        log_error("Échec d'allocation mémoire pour un objet");
        return NULL;
    }
    
    // Extraire les informations de base
    cJSON* id_json = cJSON_GetObjectItem(object_json, "id");
    cJSON* name_json = cJSON_GetObjectItem(object_json, "name");
    cJSON* type_json = cJSON_GetObjectItem(object_json, "type");
    cJSON* x_json = cJSON_GetObjectItem(object_json, "x");
    cJSON* y_json = cJSON_GetObjectItem(object_json, "y");
    cJSON* width_json = cJSON_GetObjectItem(object_json, "width");
    cJSON* height_json = cJSON_GetObjectItem(object_json, "height");
    cJSON* rotation_json = cJSON_GetObjectItem(object_json, "rotation");
    cJSON* visible_json = cJSON_GetObjectItem(object_json, "visible");
    cJSON* gid_json = cJSON_GetObjectItem(object_json, "gid");
    
    // Remplir la structure
    object->id = id_json ? id_json->valueint : 0;
    object->name = name_json && name_json->type == cJSON_String ? strdup(name_json->valuestring) : strdup("");
    object->type = type_json && type_json->type == cJSON_String ? strdup(type_json->valuestring) : strdup("");
    object->x = x_json ? (float)x_json->valuedouble : 0.0f;
    object->y = y_json ? (float)y_json->valuedouble : 0.0f;
    object->width = width_json ? (float)width_json->valuedouble : 0.0f;
    object->height = height_json ? (float)height_json->valuedouble : 0.0f;
    object->rotation = rotation_json ? (float)rotation_json->valuedouble : 0.0f;
    object->visible = visible_json ? (visible_json->valueint != 0) : true;
    object->gid = gid_json ? gid_json->valueint : 0;
    
    // Charger les propriétés si présentes
    cJSON* properties_json = cJSON_GetObjectItem(object_json, "properties");
    load_properties(properties_json, &object->properties, &object->property_count);
    
    return object;
}

// Fonction pour charger une couche d'objets
static TiledObjectGroup* load_object_group(cJSON* group_json) {
    if (!group_json) return NULL;
    
    // Vérifier que c'est bien une couche d'objets
    cJSON* type_json = cJSON_GetObjectItem(group_json, "type");
    if (!type_json || type_json->type != cJSON_String || strcmp(type_json->valuestring, "objectgroup") != 0) {
        return NULL;
    }
    
    TiledObjectGroup* group = (TiledObjectGroup*)calloc(1, sizeof(TiledObjectGroup));
    if (!group) {
        log_error("Échec d'allocation mémoire pour une couche d'objets");
        return NULL;
    }
    
    // Extraire les informations de base
    cJSON* name_json = cJSON_GetObjectItem(group_json, "name");
    cJSON* opacity_json = cJSON_GetObjectItem(group_json, "opacity");
    cJSON* visible_json = cJSON_GetObjectItem(group_json, "visible");
    cJSON* objects_json = cJSON_GetObjectItem(group_json, "objects");
    
    // Remplir la structure
    group->name = name_json && name_json->type == cJSON_String ? strdup(name_json->valuestring) : strdup("unknown");
    group->opacity = opacity_json ? (float)opacity_json->valuedouble : 1.0f;
    group->visible = visible_json ? (visible_json->valueint != 0) : true;
    
    // Charger les objets
    if (objects_json && objects_json->type == cJSON_Array) {
        int object_count = cJSON_GetArraySize(objects_json);
        if (object_count > 0) {
            group->objects = (TiledObject**)calloc(object_count, sizeof(TiledObject*));
            if (group->objects) {
                int valid_count = 0;
                for (int i = 0; i < object_count; i++) {
                    cJSON* object_json = cJSON_GetArrayItem(objects_json, i);
                    TiledObject* object = load_object(object_json);
                    if (object) {
                        group->objects[valid_count++] = object;
                    }
                }
                group->object_count = valid_count;
            }
        }
    }
    
    // Charger les propriétés si présentes
    cJSON* properties_json = cJSON_GetObjectItem(group_json, "properties");
    load_properties(properties_json, &group->properties, &group->property_count);
    
    return group;
}

// Fonction principale pour charger une carte Tiled
TiledMap* tiled_load_map(const char* filename) {
    if (!filename) return NULL;
    
    // Lire le fichier
    char* json_content = read_entire_file(filename);
    if (!json_content) {
        return NULL;
    }
    
    // Parser le JSON
    cJSON* json = cJSON_Parse(json_content);
    free(json_content);
    
    if (!json) {
        log_error("Échec de parsing du fichier JSON %s", filename);
        return NULL;
    }
    
    // Créer la structure de carte
    TiledMap* map = (TiledMap*)calloc(1, sizeof(TiledMap));
    if (!map) {
        log_error("Échec d'allocation mémoire pour la carte");
        cJSON_Delete(json);
        return NULL;
    }
    
    // Extraire les informations de base
    cJSON* width_json = cJSON_GetObjectItem(json, "width");
    cJSON* height_json = cJSON_GetObjectItem(json, "height");
    cJSON* tile_width_json = cJSON_GetObjectItem(json, "tilewidth");
    cJSON* tile_height_json = cJSON_GetObjectItem(json, "tileheight");
    
    map->width = width_json ? width_json->valueint : 0;
    map->height = height_json ? height_json->valueint : 0;
    map->tile_width = tile_width_json ? tile_width_json->valueint : 0;
    map->tile_height = tile_height_json ? tile_height_json->valueint : 0;
    
    // Charger les tilesets
    cJSON* tilesets_json = cJSON_GetObjectItem(json, "tilesets");
    if (tilesets_json && tilesets_json->type == cJSON_Array) {
        int tileset_count = cJSON_GetArraySize(tilesets_json);
        if (tileset_count > 0) {
            map->tilesets = (TiledTileset**)calloc(tileset_count, sizeof(TiledTileset*));
            if (map->tilesets) {
                int valid_count = 0;
                for (int i = 0; i < tileset_count; i++) {
                    cJSON* tileset_json = cJSON_GetArrayItem(tilesets_json, i);
                    TiledTileset* tileset = load_tileset(tileset_json);
                    if (tileset) {
                        map->tilesets[valid_count++] = tileset;
                    }
                }
                map->tileset_count = valid_count;
            }
        }
    }
    
    // Charger les couches
    cJSON* layers_json = cJSON_GetObjectItem(json, "layers");
    if (layers_json && layers_json->type == cJSON_Array) {
        int layer_count = cJSON_GetArraySize(layers_json);
        if (layer_count > 0) {
            // Pré-allouer les tableaux (nous ne savons pas encore combien de chaque type)
            map->layers = (TiledLayer**)calloc(layer_count, sizeof(TiledLayer*));
            map->object_groups = (TiledObjectGroup**)calloc(layer_count, sizeof(TiledObjectGroup*));
            
            int valid_layer_count = 0;
            int valid_object_group_count = 0;
            
            for (int i = 0; i < layer_count; i++) {
                cJSON* layer_json = cJSON_GetArrayItem(layers_json, i);
                if (!layer_json) continue;
                
                cJSON* type_json = cJSON_GetObjectItem(layer_json, "type");
                if (!type_json || type_json->type != cJSON_String) continue;
                
                if (strcmp(type_json->valuestring, "tilelayer") == 0) {
                    TiledLayer* layer = load_layer(layer_json);
                    if (layer) {
                        map->layers[valid_layer_count++] = layer;
                    }
                } else if (strcmp(type_json->valuestring, "objectgroup") == 0) {
                    TiledObjectGroup* group = load_object_group(layer_json);
                    if (group) {
                        map->object_groups[valid_object_group_count++] = group;
                    }
                }
            }
            
            map->layer_count = valid_layer_count;
            map->object_group_count = valid_object_group_count;
        }
    }
    
    // Charger les propriétés de la carte
    cJSON* properties_json = cJSON_GetObjectItem(json, "properties");
    load_properties(properties_json, &map->properties, &map->property_count);
    
    // Libérer le JSON
    cJSON_Delete(json);
    
    // Charger les tilesets externes si nécessaire
    char* base_path = get_directory_path(filename);
    if (base_path) {
        tiled_load_external_tilesets(map, base_path);
        free(base_path);
    }
    
    log_info("Carte Tiled \"%s\" chargée avec succès (%dx%d)", filename, map->width, map->height);
    return map;
}

// Fonction pour charger des tilesets externes
bool tiled_load_external_tilesets(TiledMap* map, const char* base_path) {
    if (!map || !base_path) return false;
    
    bool success = true;
    
    for (int i = 0; i < map->tileset_count; i++) {
        TiledTileset* tileset = map->tilesets[i];
        
        // Si le tileset a une source externe
        if (tileset && tileset->source) {
            // Construire le chemin complet
            char* full_path = (char*)malloc(strlen(base_path) + strlen(tileset->source) + 1);
            if (!full_path) {
                log_error("Échec d'allocation mémoire pour le chemin du tileset");
                continue;
            }
            
            sprintf(full_path, "%s%s", base_path, tileset->source);
            
            // Lire le fichier TSX/JSON
            char* json_content = read_entire_file(full_path);
            free(full_path);
            
            if (!json_content) {
                success = false;
                continue;
            }
            
            // Parser le JSON
            cJSON* json = cJSON_Parse(json_content);
            free(json_content);
            
            if (!json) {
                log_error("Échec de parsing du fichier de tileset %s", tileset->source);
                success = false;
                continue;
            }
            
            // Mettre à jour les informations du tileset
            cJSON* name_json = cJSON_GetObjectItem(json, "name");
            cJSON* tile_width_json = cJSON_GetObjectItem(json, "tilewidth");
            cJSON* tile_height_json = cJSON_GetObjectItem(json, "tileheight");
            cJSON* spacing_json = cJSON_GetObjectItem(json, "spacing");
            cJSON* margin_json = cJSON_GetObjectItem(json, "margin");
            cJSON* columns_json = cJSON_GetObjectItem(json, "columns");
            cJSON* image_json = cJSON_GetObjectItem(json, "image");
            cJSON* image_width_json = cJSON_GetObjectItem(json, "imagewidth");
            cJSON* image_height_json = cJSON_GetObjectItem(json, "imageheight");
            cJSON* tile_count_json = cJSON_GetObjectItem(json, "tilecount");
            
            // Mettre à jour les champs
            if (name_json && name_json->type == cJSON_String) {
                free(tileset->name);
                tileset->name = strdup(name_json->valuestring);
            }
            
            tileset->tile_width = tile_width_json ? tile_width_json->valueint : 0;
            tileset->tile_height = tile_height_json ? tile_height_json->valueint : 0;
            tileset->spacing = spacing_json ? spacing_json->valueint : 0;
            tileset->margin = margin_json ? margin_json->valueint : 0;
            tileset->columns = columns_json ? columns_json->valueint : 0;
            
            if (image_json && image_json->type == cJSON_String) {
                // Pour l'image, on garde seulement le nom de fichier, pas le chemin relatif
                const char* image_path = image_json->valuestring;
                const char* image_file = strrchr(image_path, '/');
                if (image_file) {
                    image_file++; // Sauter le '/'
                } else {
                    image_file = image_path; // Pas de '/' trouvé
                }
                
                if (tileset->image_source) {
                    free(tileset->image_source);
                }
                tileset->image_source = strdup(image_file);
            }
            
            tileset->image_width = image_width_json ? image_width_json->valueint : 0;
            tileset->image_height = image_height_json ? image_height_json->valueint : 0;
            tileset->tile_count = tile_count_json ? tile_count_json->valueint : 0;
            
            // Charger les propriétés si présentes
            cJSON* properties_json = cJSON_GetObjectItem(json, "properties");
            load_properties(properties_json, &tileset->properties, &tileset->property_count);
            
            // Libérer le JSON
            cJSON_Delete(json);
        }
    }
    
    return success;
}

// Fonction pour libérer une propriété
static void free_property(TiledProperty* property) {
    if (!property) return;
    
    if (property->name) free(property->name);
    if (property->type) free(property->type);
    if (property->value) free(property->value);
    free(property);
}

// Fonction pour libérer un tileset
static void free_tileset(TiledTileset* tileset) {
    if (!tileset) return;
    
    if (tileset->name) free(tileset->name);
    if (tileset->source) free(tileset->source);
    if (tileset->image_source) free(tileset->image_source);
    
    // Libérer les propriétés
    for (int i = 0; i < tileset->property_count; i++) {
        free_property(tileset->properties[i]);
    }
    if (tileset->properties) free(tileset->properties);
    
    free(tileset);
}

// Fonction pour libérer une couche
static void free_layer(TiledLayer* layer) {
    if (!layer) return;
    
    if (layer->name) free(layer->name);
    if (layer->data) free(layer->data);
    
    // Libérer les propriétés
    for (int i = 0; i < layer->property_count; i++) {
        free_property(layer->properties[i]);
    }
    if (layer->properties) free(layer->properties);
    
    free(layer);
}

// Fonction pour libérer un objet
static void free_object(TiledObject* object) {
    if (!object) return;
    
    if (object->name) free(object->name);
    if (object->type) free(object->type);
    
    // Libérer les propriétés
    for (int i = 0; i < object->property_count; i++) {
        free_property(object->properties[i]);
    }
    if (object->properties) free(object->properties);
    
    free(object);
}

// Fonction pour libérer une couche d'objets
static void free_object_group(TiledObjectGroup* group) {
    if (!group) return;
    
    if (group->name) free(group->name);
    
    // Libérer les objets
    for (int i = 0; i < group->object_count; i++) {
        free_object(group->objects[i]);
    }
    if (group->objects) free(group->objects);
    
    // Libérer les propriétés
    for (int i = 0; i < group->property_count; i++) {
        free_property(group->properties[i]);
    }
    if (group->properties) free(group->properties);
    
    free(group);
}

// Fonction pour libérer une carte Tiled
void tiled_free_map(TiledMap* map) {
    if (!map) return;
    
    // Libérer les tilesets
    for (int i = 0; i < map->tileset_count; i++) {
        free_tileset(map->tilesets[i]);
    }
    if (map->tilesets) free(map->tilesets);
    
    // Libérer les couches
    for (int i = 0; i < map->layer_count; i++) {
        free_layer(map->layers[i]);
    }
    if (map->layers) free(map->layers);
    
    // Libérer les couches d'objets
    for (int i = 0; i < map->object_group_count; i++) {
        free_object_group(map->object_groups[i]);
    }
    if (map->object_groups) free(map->object_groups);
    
    // Libérer les propriétés
    for (int i = 0; i < map->property_count; i++) {
        free_property(map->properties[i]);
    }
    if (map->properties) free(map->properties);
    
    free(map);
}

// Fonction pour obtenir une propriété par son nom
TiledProperty* tiled_get_property(TiledProperty** properties, int property_count, const char* name) {
    if (!properties || !name) return NULL;
    
    for (int i = 0; i < property_count; i++) {
        if (properties[i] && properties[i]->name && strcmp(properties[i]->name, name) == 0) {
            return properties[i];
        }
    }
    
    return NULL;
}

// Fonction pour convertir une carte Tiled en carte du jeu
Map* tiled_convert_to_game_map(TiledMap* tiled_map, ResourceManager* resource_manager) {
    if (!tiled_map || !resource_manager) return NULL;
    
    // Créer la carte
    Map* game_map = (Map*)calloc(1, sizeof(Map));
    if (!game_map) {
        log_error("Échec d'allocation mémoire pour la carte du jeu");
        return NULL;
    }
    
    // Calculer le nombre de chunks nécessaires
    int chunks_x = (tiled_map->width + DEFAULT_CHUNK_SIZE - 1) / DEFAULT_CHUNK_SIZE;
    int chunks_y = (tiled_map->height + DEFAULT_CHUNK_SIZE - 1) / DEFAULT_CHUNK_SIZE;
    
    game_map->chunks_x = chunks_x;
    game_map->chunks_y = chunks_y;
    game_map->chunk_size = DEFAULT_CHUNK_SIZE;
    game_map->tile_size = tiled_map->tile_width; // On suppose que width = height
    
    // Déterminer la zone actuelle
    TiledProperty* zone_property = tiled_get_property(tiled_map->properties, tiled_map->property_count, "zone");
    if (zone_property && zone_property->value) {
        if (strcmp(zone_property->value, "farm") == 0) {
            game_map->current_zone = ZONE_FARM;
        } else if (strcmp(zone_property->value, "village") == 0) {
            game_map->current_zone = ZONE_VILLAGE;
        } else if (strcmp(zone_property->value, "forest") == 0) {
            game_map->current_zone = ZONE_FOREST;
        } else if (strcmp(zone_property->value, "mine") == 0) {
            game_map->current_zone = ZONE_MINE;
        } else if (strcmp(zone_property->value, "beach") == 0) {
            game_map->current_zone = ZONE_BEACH;
        } else {
            game_map->current_zone = ZONE_FARM; // Par défaut
        }
    } else {
        game_map->current_zone = ZONE_FARM; // Par défaut
    }
    
    // Allouer les chunks
    game_map->chunks = (Chunk**)calloc(chunks_x * chunks_y, sizeof(Chunk*));
    if (!game_map->chunks) {
        log_error("Échec d'allocation des chunks");
        free(game_map);
        return NULL;
    }
    
    // Initialiser chaque chunk
    for (int cy = 0; cy < chunks_y; cy++) {
        for (int cx = 0; cx < chunks_x; cx++) {
            int chunk_index = cy * chunks_x + cx;
            
            Chunk* chunk = (Chunk*)calloc(1, sizeof(Chunk));
            if (!chunk) {
                log_error("Échec d'allocation du chunk (%d, %d)", cx, cy);
                // Nettoyer les chunks déjà alloués
                for (int j = 0; j < chunk_index; j++) {
                    free(game_map->chunks[j]);
                }
                free(game_map->chunks);
                free(game_map);
                return NULL;
            }
            
            chunk->chunk_x = cx;
            chunk->chunk_y = cy;
            chunk->is_loaded = true;
            chunk->is_dirty = true;
            
            // Initialiser toutes les tuiles comme vides
            for (int x = 0; x < DEFAULT_CHUNK_SIZE; x++) {
                for (int y = 0; y < DEFAULT_CHUNK_SIZE; y++) {
                    for (int layer = 0; layer < LAYER_COUNT; layer++) {
                        chunk->tiles[x][y][layer] = create_default_tile(TILE_NONE);
                    }
                }
            }
            
            game_map->chunks[chunk_index] = chunk;
        }
    }
    
    // Charger les tileset dans le gestionnaire de ressources
    int* texture_ids = (int*)calloc(tiled_map->tileset_count, sizeof(int));
    if (!texture_ids) {
        log_error("Échec d'allocation mémoire pour les IDs de texture");
        // Continuer quand même, on peut avoir une carte sans textures
    } else {
        for (int i = 0; i < tiled_map->tileset_count; i++) {
            TiledTileset* tileset = tiled_map->tilesets[i];
            if (tileset && tileset->image_source) {
                // Charger la texture
                texture_ids[i] = resource_load_texture(resource_manager, tileset->image_source);
                tileset->texture_id = texture_ids[i];
            } else {
                texture_ids[i] = -1;
            }
        }
    }
    
    // Remplir la carte avec les tuiles de chaque couche
    for (int layer_index = 0; layer_index < tiled_map->layer_count; layer_index++) {
        TiledLayer* tiled_layer = tiled_map->layers[layer_index];
        if (!tiled_layer || !tiled_layer->data) continue;
        
        // Déterminer la couche du jeu correspondante
        MapLayer game_layer = LAYER_GROUND; // Par défaut
        
        if (strstr(tiled_layer->name, "ground") || strstr(tiled_layer->name, "Ground")) {
            game_layer = LAYER_GROUND;
        } else if (strstr(tiled_layer->name, "object") || strstr(tiled_layer->name, "Object")) {
            game_layer = LAYER_OBJECTS;
        } else if (strstr(tiled_layer->name, "item") || strstr(tiled_layer->name, "Item")) {
            game_layer = LAYER_ITEMS;
        } else if (strstr(tiled_layer->name, "building") || strstr(tiled_layer->name, "Building")) {
            game_layer = LAYER_BUILDINGS;
        }
        
        // Parcourir toutes les tuiles de la couche
        for (int y = 0; y < tiled_map->height; y++) {
            for (int x = 0; x < tiled_map->width; x++) {
                int tile_index = y * tiled_map->width + x;
                int gid = tiled_layer->data[tile_index];
                
                // Si GID = 0, c'est une tuile vide
                if (gid == 0) continue;
                
                // Trouver le tileset correspondant
                int tileset_index = -1;
                int local_tile_id = -1;
                
                for (int i = tiled_map->tileset_count - 1; i >= 0; i--) {
                    if (gid >= tiled_map->tilesets[i]->firstgid) {
                        tileset_index = i;
                        local_tile_id = gid - tiled_map->tilesets[i]->firstgid;
                        break;
                    }
                }
                
                if (tileset_index < 0 || local_tile_id < 0) continue;
                
                // Déterminer le type de tuile en fonction de son ID ou de propriétés
                TileType tile_type = TILE_GRASS; // Par défaut
                
                // Logique pour déterminer le type (à adapter selon vos besoins)
                if (game_layer == LAYER_GROUND) {
                    if (local_tile_id % 10 == 0) tile_type = TILE_GRASS;
                    else if (local_tile_id % 10 == 1) tile_type = TILE_DIRT;
                    else if (local_tile_id % 10 == 2) tile_type = TILE_WATER;
                    else if (local_tile_id % 10 == 3) tile_type = TILE_STONE;
                    else if (local_tile_id % 10 == 4) tile_type = TILE_SAND;
                    else tile_type = TILE_GRASS;
                } else if (game_layer == LAYER_OBJECTS) {
                    tile_type = TILE_STONE;
                } else if (game_layer == LAYER_BUILDINGS) {
                    tile_type = TILE_BUILDING;
                }
                
                // Créer la tuile
                Tile tile = create_default_tile(tile_type);
                tile.variant = local_tile_id;
                
                // Calculer les coordonnées du chunk et de la tuile dans le chunk
                int chunk_x = x / DEFAULT_CHUNK_SIZE;
                int chunk_y = y / DEFAULT_CHUNK_SIZE;
                int local_x = x % DEFAULT_CHUNK_SIZE;
                int local_y = y % DEFAULT_CHUNK_SIZE;
                
                // Vérifier les limites de la carte
                if (chunk_x < 0 || chunk_x >= chunks_x || chunk_y < 0 || chunk_y >= chunks_y) {
                    continue;
                }
                
                // Récupérer le chunk
                int chunk_index = chunk_y * chunks_x + chunk_x;
                Chunk* chunk = game_map->chunks[chunk_index];
                
                if (!chunk) continue;
                
                // Définir la tuile
                chunk->tiles[local_x][local_y][game_layer] = tile;
            }
        }
    }
    
    // Libérer les IDs de texture
    if (texture_ids) free(texture_ids);
    
    log_info("Carte convertie avec succès (%dx%d chunks)", chunks_x, chunks_y);
    return game_map;
}

// Fonction pour créer des entités à partir des objets d'une carte Tiled
int tiled_create_entities(TiledMap* tiled_map, EntityManager* entity_manager, float origin_x, float origin_y) {
    if (!tiled_map || !entity_manager) return -1;
    
    int created_entities = 0;
    
    // Parcourir chaque couche d'objets
    for (int group_index = 0; group_index < tiled_map->object_group_count; group_index++) {
        TiledObjectGroup* group = tiled_map->object_groups[group_index];
        if (!group) continue;
        
        // Parcourir chaque objet
        for (int obj_index = 0; obj_index < group->object_count; obj_index++) {
            TiledObject* object = group->objects[obj_index];
            if (!object) continue;
            
            // Créer une nouvelle entité
            EntityID entity_id = entity_create(entity_manager);
            if (entity_id == INVALID_ENTITY_ID) {
                log_error("Échec de création d'une entité pour l'objet %s", object->name);
                continue;
            }
            
            // Ajouter un composant Transform
            TransformComponent* transform = create_transform_component(
                entity_id, 
                origin_x + object->x + object->width / 2, 
                origin_y + object->y + object->height / 2
            );
            
            if (transform && entity_add_component(entity_manager, entity_id, transform)) {
                // Ok
            } else {
                log_error("Échec d'ajout du composant Transform à l'entité %u", entity_id);
                entity_destroy(entity_manager, entity_id);
                free(transform);
                continue;
            }
            
            // Ajouter un composant Collider si ce n'est pas un déclencheur
            TiledProperty* trigger_prop = tiled_get_property(object->properties, object->property_count, "is_trigger");
            bool is_trigger = trigger_prop && strcmp(trigger_prop->value, "true") == 0;
            
            CollisionType collision_type = COLLISION_STATIC;
            if (is_trigger) {
                collision_type = COLLISION_TRIGGER;
            } else {
                TiledProperty* collision_prop = tiled_get_property(object->properties, object->property_count, "collision_type");
                if (collision_prop) {
                    if (strcmp(collision_prop->value, "dynamic") == 0) {
                        collision_type = COLLISION_DYNAMIC;
                    } else if (strcmp(collision_prop->value, "trigger") == 0) {
                        collision_type = COLLISION_TRIGGER;
                    }
                }
            }
            
            ColliderComponent* collider = create_collider_component(
                entity_id, 
                object->width, 
                object->height,
                collision_type
            );
            
            if (collider && entity_add_component(entity_manager, entity_id, collider)) {
                // Ok
            } else {
                log_error("Échec d'ajout du composant Collider à l'entité %u", entity_id);
                free(collider);
                // Ne pas détruire l'entité, elle peut être utile même sans collider
            }
            
            // Ajouter d'autres composants en fonction du type d'objet
            if (object->type && strlen(object->type) > 0) {
                if (strcmp(object->type, "player_spawn") == 0) {
                    // Point de spawn du joueur, pas besoin d'ajouter d'autres composants
                } else if (strcmp(object->type, "npc") == 0) {
                    // Si c'est un NPC, ajouter le composant NPC
                    // Pas encore implémenté
                } else if (strcmp(object->type, "item") == 0) {
                    // Si c'est un item, ajouter le composant Item
                    TiledProperty* item_id_prop = tiled_get_property(object->properties, object->property_count, "item_id");
                    int item_id = item_id_prop ? atoi(item_id_prop->value) : 0;
                    
                    TiledProperty* stack_size_prop = tiled_get_property(object->properties, object->property_count, "stack_size");
                    int stack_size = stack_size_prop ? atoi(stack_size_prop->value) : 1;
                    
                    ItemComponent* item = create_item_component(entity_id, item_id, stack_size);
                    if (item && entity_add_component(entity_manager, entity_id, item)) {
                        // Ok
                    } else {
                        log_error("Échec d'ajout du composant Item à l'entité %u", entity_id);
                        free(item);
                    }
                } else if (strcmp(object->type, "interactable") == 0) {
                    // Si c'est un objet interactif, ajouter le composant Interactable
                    TiledProperty* interaction_type_prop = tiled_get_property(object->properties, object->property_count, "interaction_type");
                    int interaction_type = interaction_type_prop ? atoi(interaction_type_prop->value) : 0;
                    
                    TiledProperty* interaction_radius_prop = tiled_get_property(object->properties, object->property_count, "interaction_radius");
                    float interaction_radius = interaction_radius_prop ? atof(interaction_radius_prop->value) : 32.0f;
                    
                    InteractableComponent* interactable = create_interactable_component(
                        entity_id, 
                        interaction_type, 
                        interaction_radius
                    );
                    
                    if (interactable && entity_add_component(entity_manager, entity_id, interactable)) {
                        // Ok
                    } else {
                        log_error("Échec d'ajout du composant Interactable à l'entité %u", entity_id);
                        free(interactable);
                    }
                }
                // D'autres types peuvent être ajoutés ici
            }
            
            created_entities++;
        }
    }
    
    log_info("Créé %d entités à partir de la carte Tiled", created_entities);
    return created_entities;
}