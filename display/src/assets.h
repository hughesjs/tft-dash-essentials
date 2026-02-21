/*
 * assets.h - Asset management for TFT Dash
 *
 * Two-key lookup system for themed BMP assets.
 * Loads theme directories and retrieves textures by (theme, asset_name) pair.
 * Pure data container — the caller decides which theme to use.
 */

#ifndef ASSETS_H
#define ASSETS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL3/SDL.h>

typedef struct asset_store asset_store;

/*
 * Create a new asset store bound to the given renderer.
 * The renderer is used to create textures from loaded BMP surfaces.
 * Returns NULL on allocation failure.
 */
asset_store* asset_store_create(SDL_Renderer* renderer);

/*
 * Destroy the asset store and free all surfaces, textures, and memory.
 * Safe to call with NULL.
 */
void asset_store_destroy(asset_store* store);

/*
 * Scan dir_path for *.bmp files, load each as a surface, convert to texture.
 * Assets are keyed by (theme, filename) where filename includes the .bmp extension.
 *
 * Returns: number of assets loaded, or -1 on error.
 */
int asset_store_load_theme(asset_store* store, const char* theme, const char* dir_path);

/*
 * Look up a texture by theme and asset name.
 * Returns NULL if the (theme, name) pair is not found.
 */
SDL_Texture* asset_store_get_texture(asset_store* store, const char* theme, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* ASSETS_H */
