/*
 * assets.h - Asset management for TFT Dash
 *
 * Two-key lookup system for themed PNG assets.
 * Loads theme directories and retrieves textures by (theme, asset_name) pair.
 * Pure data container — the caller decides which theme to use.
 */

#ifndef ASSETS_H
#define ASSETS_H

#include <SDL3/SDL.h>

typedef struct asset_store asset_store;

/*
 * Create a new asset store bound to the given renderer.
 * The renderer is used to create textures from loaded PNG images.
 * Returns NULL on allocation failure.
 */
asset_store* asset_store_create(SDL_Renderer* renderer);

/*
 * Destroy the asset store and free all surfaces, textures, and memory.
 * Safe to call with NULL.
 */
void asset_store_destroy(asset_store* store);

/*
 * Scan dir_path for *.png files, load each via stb_image, convert to texture.
 * Assets are keyed by (theme, filename) where filename includes the .png extension.
 *
 * Returns: number of assets loaded, or -1 on error.
 */
int asset_store_load_theme(asset_store* store, const char* theme, const char* dir_path);

/*
 * Look up a texture by theme and asset name.
 * Returns NULL if the (theme, name) pair is not found.
 */
SDL_Texture* asset_store_get_texture(asset_store* store, const char* theme, const char* name);

#endif /* ASSETS_H */
