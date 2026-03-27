/*
 * menu.h - Menu system data tables for TFT Dash
 *
 * Pure data lookup tables extracted from draw_menu() in testsdl.cpp.
 * No SDL dependency — suitable for unit testing.
 */

#ifndef MENU_H
#define MENU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef enum {
    MENU_SCREEN_NONE,
    MENU_SCREEN_MAIN,             /* state 1-11 */
    MENU_SCREEN_SET_TIME,         /* state 20-70 */
    MENU_SCREEN_SPEED_CORRECTION, /* state 100-150 */
    MENU_SCREEN_THEME,            /* state 200-280 */
    MENU_SCREEN_SET_ODOMETER,     /* state 300-360 */
    MENU_SCREEN_SPROCKET,         /* state 400-470 */
    MENU_SCREEN_COOLANT_FAN,      /* state 500-570 */
    MENU_SCREEN_SET_UNITS,        /* state 600-635 */
    MENU_SCREEN_TPMS,             /* state 700-745 */
    MENU_SCREEN_CONTROL,          /* state 800-815 */
    MENU_SCREEN_LIGHT,            /* state 900-935 */
} menu_screen;

/* Map a raw menu state integer to a screen enum. */
menu_screen menu_screen_for_state(int state);

/* Background texture for a menu screen. */
typedef struct { const char *texture; bool theme_specific; } menu_bg;
const menu_bg *menu_screen_background(menu_screen screen);

/* Cursor/arrow overlay for a specific state. */
typedef struct { const char *texture; int x, y, w, h; } menu_cursor;
bool menu_cursor_for_state(int state, menu_cursor *out);

/* Main menu items (state 1-11) have left+right arrows. */
bool menu_main_item_arrows(int state, menu_cursor *left, menu_cursor *right);

/* Theme menu items (state 200-280) have left+right arrows (default theme only). */
bool menu_theme_arrows(int state, menu_cursor *left, menu_cursor *right);

/* Theme thumbnail lookup by theme ID (0-8). Returns NULL for out-of-range. */
typedef struct { const char *theme_name; const char *thumb_file; } theme_thumb;
const theme_thumb *menu_theme_thumb(int theme_id);

#ifdef __cplusplus
}
#endif

#endif /* MENU_H */
