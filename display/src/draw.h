/*
 * draw.h — Rendering system for TFT Dash
 *
 * Owns SDL initialisation, asset loading, event pumping, and all
 * drawing primitives. No SDL types leak into the public interface
 * except through the sub-module headers (draw_dashboard.h, draw_menu.h).
 */

#ifndef DRAW_H
#define DRAW_H

#include <stdbool.h>
#include "parser.h"
#include "tpms_feed.h"

/* --- Lifecycle (called from main) --- */

bool draw_init(const dashboard_state *dash, const menu_state *menu, const nav_state *nav, const tpms_state *tpms);
void draw_cleanup(void);

/* Pump events, render the current frame (dashboard or menu), delay. */
void draw_frame(void);

/* --- Internal interface (used by draw_dashboard.c and draw_menu.c only) ---
 * Code outside the draw module should not access these directly. */

#include <SDL3/SDL.h>
#include "assets.h"

/* Screen dimensions */
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600

/* Shared rendering state — internal to the draw modules */
extern SDL_Renderer *renderer;
extern asset_store *g_assets;
extern const char *g_current_theme;

extern const dashboard_state *dash;
extern const menu_state *menu;
extern const nav_state *nav;
extern const tpms_state *tpms;

/* Theme lookup */
#define THEME_COUNT 9
extern const char *THEME_NAMES[];
const char *theme_name_from_id(int id);

/* Texture lookup helpers */
SDL_Texture *tex(const char *name);
SDL_Texture *tex_from(const char *theme, const char *name);

/* Render a texture at integer coordinates */
bool render_texture(SDL_Texture *t, int x, int y, int w, int h);
bool render_top_icon_grey_texture(int x, int y, int w, int h);
bool render_oil_light_texture(int x, int y, int w, int h);

/* Glyph-based text drawing */
void draw_small_grey_string(const char *digits, int xpos, int ypos);
void draw_small_blue_string(const char *digits, int xpos, int ypos);
void draw_medium_string(const char *digits, int xpos, int ypos);
void draw_large_string(const char *digits, int xpos, int ypos);
void draw_nav_large_string(const char *digits, int xpos, int ypos);
void draw_nav_small_string(const char *digits, int xpos, int ypos);
void draw_nav_digits(const char *digits, int xpos, int ypos);
void draw_medium_char(char digit, int xpos, int ypos);
void draw_medium_num(int num, int xpos, int ypos);
void draw_large_num(int num, int xpos, int ypos);
void draw_nav_numbers(int num, int xpos, int ypos);

/* Navigation icon rendering */
typedef enum {
    NAV_ICON_MUT = 0, NAV_ICON_EXITL, NAV_ICON_EXITR, NAV_ICON_TNR, NAV_ICON_TNL,
    NAV_ICON_SLL, NAV_ICON_SLR, NAV_ICON_RB2L, NAV_ICON_RB3L, NAV_ICON_RB4L,
    NAV_ICON_RB5L, NAV_ICON_RB1R, NAV_ICON_RB3R, NAV_ICON_RB4R, NAV_ICON_RB5R,
    NAV_ICON_KPL, NAV_ICON_KPR, NAV_ICON_CON, NAV_ICON_ARV,
    NAV_ICON_RB1L, NAV_ICON_RB2R, NAV_ICON_MUTR,
    NAV_ICON_MILE, NAV_ICON_YARD, NAV_ICON_KM, NAV_ICON_METRE,
    NAV_ICON_ONTO, NAV_ICON_TWRDS,
    NAV_ICON_COUNT,
    NAV_ICON_NONE = -1
} nav_icon;

void draw_nav_symbol(nav_icon sym, int xpos, int ypos);

typedef struct {
    const char *code;
    nav_icon icon_lhd;
    nav_icon icon_rhd;
    int x, y;
} nav_symbol_entry;

const nav_symbol_entry *lookup_nav_symbol(const char *code);

/* RPM lookup table */
extern const int g_rpm_lookup[53][2];

/* Speed digit rendering */
void draw_speed_digit(char digit, int position);
void draw_speed(const char *speed);

#endif /* DRAW_H */
