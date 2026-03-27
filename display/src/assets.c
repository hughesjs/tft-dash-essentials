/*
 * assets.c - Asset management for TFT Dash
 *
 * Open-addressing hash map with FNV-1a hashing for (theme, name) lookups.
 * Loads PNG files from theme directories via opendir/readdir.
 */

#include "assets.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stb_image.h>

#define INITIAL_CAPACITY 64
#define LOAD_FACTOR_PERCENT 70

typedef struct {
    char* theme;
    char* name;
    SDL_Surface* surface;
    SDL_Texture* texture;
} asset_entry;

struct asset_store {
    SDL_Renderer* renderer;
    asset_entry* entries;
    size_t capacity;
    size_t count;
};

/* FNV-1a hash */
static uint32_t fnv1a(const char* s) {
    uint32_t h = 2166136261u;
    for (; *s; s++) {
        h ^= (uint8_t)*s;
        h *= 16777619u;
    }
    return h;
}

/* Hash a (theme, name) pair by feeding both strings into FNV-1a sequentially */
static uint32_t hash_key(const char* theme, const char* name) {
    uint32_t h = 2166136261u;
    for (const char* s = theme; *s; s++) {
        h ^= (uint8_t)*s;
        h *= 16777619u;
    }
    /* separator to avoid collisions between e.g. ("ab","cd") and ("a","bcd") */
    h ^= 0xFF;
    h *= 16777619u;
    for (const char* s = name; *s; s++) {
        h ^= (uint8_t)*s;
        h *= 16777619u;
    }
    return h;
}

static bool entry_matches(const asset_entry* e, const char* theme, const char* name) {
    return e->theme && strcmp(e->theme, theme) == 0 && strcmp(e->name, name) == 0;
}

static bool entry_is_empty(const asset_entry* e) {
    return e->theme == NULL;
}

static bool resize_map(asset_store* store);

static bool insert_entry(asset_store* store, char* theme, char* name,
                         SDL_Surface* surface, SDL_Texture* texture) {
    /* Resize if we're at the load factor threshold */
    if ((store->count + 1) * 100 > store->capacity * LOAD_FACTOR_PERCENT) {
        if (!resize_map(store)) {
            return false;
        }
    }

    uint32_t idx = hash_key(theme, name) % store->capacity;
    while (!entry_is_empty(&store->entries[idx])) {
        if (entry_matches(&store->entries[idx], theme, name)) {
            /* Replace existing entry */
            SDL_DestroyTexture(store->entries[idx].texture);
            SDL_DestroySurface(store->entries[idx].surface);
            store->entries[idx].surface = surface;
            store->entries[idx].texture = texture;
            return true;
        }
        idx = (idx + 1) % store->capacity;
    }

    store->entries[idx].theme = theme;
    store->entries[idx].name = name;
    store->entries[idx].surface = surface;
    store->entries[idx].texture = texture;
    store->count++;
    return true;
}

static bool resize_map(asset_store* store) {
    size_t old_capacity = store->capacity;
    asset_entry* old_entries = store->entries;

    size_t new_capacity = old_capacity * 2;
    asset_entry* new_entries = calloc(new_capacity, sizeof(asset_entry));
    if (!new_entries) {
        return false;
    }

    store->entries = new_entries;
    store->capacity = new_capacity;
    store->count = 0;

    for (size_t i = 0; i < old_capacity; i++) {
        if (!entry_is_empty(&old_entries[i])) {
            insert_entry(store, old_entries[i].theme, old_entries[i].name,
                         old_entries[i].surface, old_entries[i].texture);
        }
    }

    free(old_entries);
    return true;
}

static bool has_png_suffix(const char* name) {
    size_t len = strlen(name);
    if (len < 4) return false;
    return strcasecmp(name + len - 4, ".png") == 0;
}

asset_store* asset_store_create(SDL_Renderer* renderer) {
    if (!renderer) return NULL;

    asset_store* store = malloc(sizeof(asset_store));
    if (!store) return NULL;

    store->entries = calloc(INITIAL_CAPACITY, sizeof(asset_entry));
    if (!store->entries) {
        free(store);
        return NULL;
    }

    store->renderer = renderer;
    store->capacity = INITIAL_CAPACITY;
    store->count = 0;
    return store;
}

void asset_store_destroy(asset_store* store) {
    if (!store) return;

    for (size_t i = 0; i < store->capacity; i++) {
        if (!entry_is_empty(&store->entries[i])) {
            SDL_DestroyTexture(store->entries[i].texture);
            SDL_DestroySurface(store->entries[i].surface);
            free(store->entries[i].theme);
            free(store->entries[i].name);
        }
    }

    free(store->entries);
    free(store);
}

int asset_store_load_theme(asset_store* store, const char* theme, const char* dir_path) {
    if (!store || !theme || !dir_path) return -1;

    DIR* dir = opendir(dir_path);
    if (!dir) return -1;

    int loaded = 0;
    struct dirent* ent;

    while ((ent = readdir(dir)) != NULL) {
        if (!has_png_suffix(ent->d_name)) continue;

        /* Build full path */
        size_t path_len = strlen(dir_path) + 1 + strlen(ent->d_name) + 1;
        char* path = malloc(path_len);
        if (!path) continue;
        snprintf(path, path_len, "%s/%s", dir_path, ent->d_name);

        int w, h, channels;
        unsigned char* data = stbi_load(path, &w, &h, &channels, 4);
        free(path);
        if (!data) continue;

        SDL_Surface* surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
        if (!surface) { stbi_image_free(data); continue; }
        memcpy(surface->pixels, data, (size_t)w * h * 4);
        stbi_image_free(data);

        SDL_Texture* texture = SDL_CreateTextureFromSurface(store->renderer, surface);
        if (!texture) {
            SDL_DestroySurface(surface);
            continue;
        }

        char* theme_dup = strdup(theme);
        char* name_dup = strdup(ent->d_name);
        if (!theme_dup || !name_dup) {
            free(theme_dup);
            free(name_dup);
            SDL_DestroyTexture(texture);
            SDL_DestroySurface(surface);
            continue;
        }

        if (insert_entry(store, theme_dup, name_dup, surface, texture)) {
            loaded++;
        } else {
            free(theme_dup);
            free(name_dup);
            SDL_DestroyTexture(texture);
            SDL_DestroySurface(surface);
        }
    }

    closedir(dir);
    return loaded;
}

SDL_Texture* asset_store_get_texture(asset_store* store, const char* theme, const char* name) {
    if (!store || !theme || !name) return NULL;

    uint32_t idx = hash_key(theme, name) % store->capacity;
    for (size_t probes = 0; probes < store->capacity; probes++) {
        if (entry_is_empty(&store->entries[idx])) {
            return NULL;
        }
        if (entry_matches(&store->entries[idx], theme, name)) {
            return store->entries[idx].texture;
        }
        idx = (idx + 1) % store->capacity;
    }

    return NULL;
}
