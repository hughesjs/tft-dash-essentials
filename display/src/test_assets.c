/*
 * test_assets.c - Test suite for asset management
 *
 * Requires SDL3 initialised with a hidden window + renderer.
 * Run from the project root so that assets/themes/ paths resolve.
 */

#include "assets.h"

#include <stdio.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("  %d. %-40s ", tests_run, name); \
    } while (0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while (0)

#define FAIL(msg) \
    do { \
        printf("FAIL - %s\n", msg); \
    } while (0)

int main(void) {
    printf("\n=== Asset Manager Tests ===\n\n");

    /* Initialise SDL with a hidden window for texture creation */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("test", 1, 1, SDL_WINDOW_HIDDEN);
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, "software");
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    /* Test 1: Create/destroy empty store */
    TEST("Create/destroy empty store");
    {
        asset_store* store = asset_store_create(renderer);
        if (store) {
            asset_store_destroy(store);
            PASS();
        } else {
            FAIL("asset_store_create returned NULL");
        }
    }

    /* Test 2: Load default theme */
    TEST("Load default theme");
    {
        asset_store* store = asset_store_create(renderer);
        int loaded = asset_store_load_theme(store, "default", "assets/themes/default");
        if (loaded > 0) {
            SDL_Texture* tex = asset_store_get_texture(store, "default", "Coolant.bmp");
            if (tex) {
                PASS();
            } else {
                FAIL("Coolant.bmp texture is NULL after loading default theme");
            }
        } else {
            FAIL("asset_store_load_theme returned <= 0");
        }
        asset_store_destroy(store);
    }

    /* Test 3: Unknown asset */
    TEST("Unknown asset returns NULL");
    {
        asset_store* store = asset_store_create(renderer);
        asset_store_load_theme(store, "default", "assets/themes/default");
        SDL_Texture* tex = asset_store_get_texture(store, "default", "nonexistent.bmp");
        if (tex == NULL) {
            PASS();
        } else {
            FAIL("expected NULL for nonexistent asset");
        }
        asset_store_destroy(store);
    }

    /* Test 4: Unknown theme */
    TEST("Unknown theme returns NULL");
    {
        asset_store* store = asset_store_create(renderer);
        asset_store_load_theme(store, "default", "assets/themes/default");
        SDL_Texture* tex = asset_store_get_texture(store, "blue", "Coolant.bmp");
        if (tex == NULL) {
            PASS();
        } else {
            FAIL("expected NULL for unloaded theme");
        }
        asset_store_destroy(store);
    }

    /* Test 5: Two themes loaded independently */
    TEST("Two themes return correct textures");
    {
        asset_store* store = asset_store_create(renderer);
        int loaded_default = asset_store_load_theme(store, "default", "assets/themes/default");
        int loaded_blue = asset_store_load_theme(store, "blue", "assets/themes/blue");
        if (loaded_default > 0 && loaded_blue > 0) {
            SDL_Texture* tex_default = asset_store_get_texture(store, "default", "Coolant.bmp");
            SDL_Texture* tex_blue = asset_store_get_texture(store, "blue", "Coolant.bmp");
            if (tex_default && tex_blue && tex_default != tex_blue) {
                PASS();
            } else if (!tex_default || !tex_blue) {
                FAIL("one or both textures are NULL");
            } else {
                FAIL("both themes returned same texture pointer");
            }
        } else {
            FAIL("failed to load one or both themes");
        }
        asset_store_destroy(store);
    }

    /* Test 6: Case sensitivity */
    TEST("Case sensitivity (lowercase name)");
    {
        asset_store* store = asset_store_create(renderer);
        asset_store_load_theme(store, "default", "assets/themes/default");
        SDL_Texture* tex = asset_store_get_texture(store, "default", "coolant.bmp");
        if (tex == NULL) {
            PASS();
        } else {
            FAIL("expected NULL for wrong case");
        }
        asset_store_destroy(store);
    }

    /* Test 7: NULL safety */
    TEST("NULL safety");
    {
        /* None of these should crash */
        asset_store_destroy(NULL);
        SDL_Texture* t1 = asset_store_get_texture(NULL, "default", "Coolant.bmp");
        asset_store* store = asset_store_create(renderer);
        SDL_Texture* t2 = asset_store_get_texture(store, NULL, "Coolant.bmp");
        SDL_Texture* t3 = asset_store_get_texture(store, "default", NULL);
        int r1 = asset_store_load_theme(store, NULL, "assets/themes/default");
        int r2 = asset_store_load_theme(store, "default", NULL);
        int r3 = asset_store_load_theme(NULL, "default", "assets/themes/default");
        if (t1 == NULL && t2 == NULL && t3 == NULL &&
            r1 == -1 && r2 == -1 && r3 == -1) {
            PASS();
        } else {
            FAIL("NULL arguments didn't return expected sentinel values");
        }
        asset_store_destroy(store);
    }

    printf("\n  Results: %d/%d passed\n\n", tests_passed, tests_run);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return (tests_passed == tests_run) ? 0 : 1;
}
