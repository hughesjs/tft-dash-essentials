/*
 * menu.c - Menu system data tables for TFT Dash
 *
 * All data extracted from draw_menu() in main.c.
 */

#include "menu.h"
#include <stddef.h>

/* ---------- Screen routing ---------- */

typedef struct {
    int lo, hi;           /* inclusive range [lo, hi) */
    menu_screen screen;
} screen_range;

static const screen_range screen_ranges[] = {
    {   1,   20, MENU_SCREEN_MAIN },
    {  20,   90, MENU_SCREEN_SET_TIME },
    { 100,  170, MENU_SCREEN_SPEED_CORRECTION },
    { 200,  290, MENU_SCREEN_THEME },
    { 300,  390, MENU_SCREEN_SET_ODOMETER },
    { 400,  490, MENU_SCREEN_SPROCKET },
    { 500,  590, MENU_SCREEN_COOLANT_FAN },
    { 600,  690, MENU_SCREEN_SET_UNITS },
    { 700,  790, MENU_SCREEN_TPMS },
    { 800,  830, MENU_SCREEN_CONTROL },
    { 900,  950, MENU_SCREEN_LIGHT },
};

#define SCREEN_RANGE_COUNT (sizeof(screen_ranges) / sizeof(screen_ranges[0]))

menu_screen menu_screen_for_state(int state) {
    for (size_t i = 0; i < SCREEN_RANGE_COUNT; i++) {
        if (state >= screen_ranges[i].lo && state < screen_ranges[i].hi)
            return screen_ranges[i].screen;
    }
    return MENU_SCREEN_NONE;
}

/* ---------- Background textures ---------- */

static const menu_bg backgrounds[] = {
    [MENU_SCREEN_NONE]             = { NULL, false },
    [MENU_SCREEN_MAIN]             = { "Menuoptionsex.png",    true  },
    [MENU_SCREEN_SET_TIME]         = { "Settime.png",          true  },
    [MENU_SCREEN_SPEED_CORRECTION] = { "Speedcorrection.png",  true  },
    [MENU_SCREEN_THEME]            = { "Themeoptions.png",     false },
    [MENU_SCREEN_SET_ODOMETER]     = { "Setodometer.png",      false },
    [MENU_SCREEN_SPROCKET]         = { "Sprocketsetup.png",    true  },
    [MENU_SCREEN_COOLANT_FAN]      = { "Coolantfantemp.png",   true  },
    [MENU_SCREEN_SET_UNITS]        = { "Setunits.png",         true  },
    [MENU_SCREEN_TPMS]             = { "TPMSsetup.png",        true  },
    [MENU_SCREEN_CONTROL]          = { "Controloptions.png",   true  },
    [MENU_SCREEN_LIGHT]            = { "Lightoptions.png",     true  },
};

#define BG_COUNT (sizeof(backgrounds) / sizeof(backgrounds[0]))

const menu_bg *menu_screen_background(menu_screen screen) {
    if (screen <= MENU_SCREEN_NONE || (size_t)screen >= BG_COUNT)
        return NULL;
    return &backgrounds[screen];
}

/* ---------- Cursor positions ---------- */

typedef struct {
    int state;
    const char *texture;
    int x, y, w, h;
} cursor_entry;

static const cursor_entry cursor_table[] = {
    /* Coolant fan (500-570) */
    { 500, "Uparrowsmall.png",     115, 185,  60,  62 },
    { 510, "Uparrowsmall.png",     341, 173,  60,  62 },
    { 520, "Uparrowsmall.png",     625, 173,  60,  62 },
    { 530, "Leftmenuarrowex.png",  667, 300,  56,  50 },
    { 540, "Leftmenuarrowex.png",  667, 378,  56,  50 },
    { 550, "Leftmenuarrowex.png",  667, 454,  56,  50 },
    { 560, "Leftmenuarrowex.png",  667, 532,  56,  50 },
    { 570, "Uparrowsmall.png",     884, 534,  60,  62 },

    /* Sprocket (400-470) */
    { 400, "Uparrowsmall.png",     113, 296,  60,  62 },
    { 410, "Uparrowsmall.png",     286, 300,  60,  62 },
    { 420, "Uparrowsmall.png",     373, 300,  60,  62 },
    { 430, "Uparrowsmall.png",     590, 300,  60,  62 },
    { 440, "Uparrowsmall.png",     678, 300,  60,  62 },
    { 450, "Uparrowsmall.png",     594, 485,  60,  62 },
    { 460, "Uparrowsmall.png",     861, 485,  60,  62 },
    { 470, "Uparrowsmall.png",     848, 300,  60,  62 },

    /* Set odometer (300-360) */
    { 300, "Uparrow.png",          175, 279, 104, 107 },
    { 305, "Uparrow.png",          286, 279, 104, 107 },
    { 310, "Uparrow.png",          399, 279, 104, 107 },
    { 315, "Uparrow.png",          510, 279, 104, 107 },
    { 320, "Uparrow.png",          620, 279, 104, 107 },
    { 325, "Uparrow.png",          733, 279, 104, 107 },
    { 330, "Uparrow.png",          175, 493, 104, 107 },
    { 335, "Uparrow.png",          286, 493, 104, 107 },
    { 340, "Uparrow.png",          399, 493, 104, 107 },
    { 345, "Uparrow.png",          510, 493, 104, 107 },
    { 350, "Uparrow.png",          620, 493, 104, 107 },
    { 355, "Uparrow.png",          733, 493, 104, 107 },
    { 360, "Uparrow.png",          877, 493, 104, 107 },

    /* Speed correction (100-150) */
    { 100, "Uparrow.png",           83, 352, 104, 107 },
    { 110, "Uparrow.png",          262, 352, 104, 107 },
    { 120, "Uparrow.png",          372, 352, 104, 107 },
    { 130, "Uparrow.png",          477, 352, 104, 107 },
    { 140, "Uparrow.png",          657, 352, 104, 107 },
    { 150, "Uparrow.png",          824, 352, 104, 107 },

    /* TPMS (700-745) */
    { 700, "Uparrowsmall.png",      65, 182,  60,  62 },
    { 705, "Uparrowsmall.png",     420, 220,  60,  62 },
    { 710, "Uparrowsmall.png",     610, 220,  60,  62 },
    { 715, "Uparrowsmall.png",     726, 220,  60,  62 },
    { 720, "Uparrowsmall.png",     917, 220,  60,  62 },
    { 725, "Uparrowsmall.png",     727, 337,  60,  62 },
    { 730, "Uparrowsmall.png",     918, 337,  60,  62 },
    { 735, "Uparrowsmall.png",     728, 456,  60,  62 },
    { 740, "Uparrowsmall.png",     918, 456,  60,  62 },
    { 745, "Uparrowsmall.png",      65, 528,  60,  62 },

    /* Control (800-815) */
    { 800, "Uparrowsmall.png",      69, 363,  60,  62 },
    { 805, "Uparrowsmall.png",     316, 394,  60,  62 },
    { 810, "Uparrowsmall.png",     648, 394,  60,  62 },
    { 815, "Uparrowsmall.png",     890, 363,  60,  62 },

    /* Light (900-935) */
    { 900, "Uparrowsmall.png",      66, 207,  60,  62 },
    { 905, "Uparrowsmall.png",     708, 183,  60,  62 },
    { 910, "Uparrowsmall.png",     899, 183,  60,  62 },
    { 915, "Uparrowsmall.png",     708, 302,  60,  62 },
    { 920, "Uparrowsmall.png",     898, 302,  60,  62 },
    { 925, "Uparrowsmall.png",     531, 529,  60,  62 },
    { 930, "Uparrowsmall.png",     722, 529,  60,  62 },
    { 935, "Uparrowsmall.png",     890, 529,  60,  62 },

    /* Set units (600-635) */
    { 600, "Uparrowsmall.png",      34, 197,  60,  62 },
    { 605, "Uparrowsmall.png",     780, 154,  60,  62 },
    { 610, "Uparrowsmall.png",     891, 154,  60,  62 },
    { 615, "Uparrowsmall.png",     780, 314,  60,  62 },
    { 620, "Uparrowsmall.png",     891, 314,  60,  62 },
    { 625, "Uparrowsmall.png",     780, 475,  60,  62 },
    { 630, "Uparrowsmall.png",     891, 475,  60,  62 },
    { 635, "Uparrowsmall.png",      35, 539,  60,  62 },

    /* Set time (20-70) */
    {  20, "Uparrow.png",           92, 355, 104, 107 },
    {  30, "Uparrow.png",          262, 355, 104, 107 },
    {  40, "Uparrow.png",          371, 355, 104, 107 },
    {  50, "Uparrow.png",          545, 355, 104, 107 },
    {  60, "Uparrow.png",          657, 355, 104, 107 },
    {  70, "Uparrow.png",          824, 355, 104, 107 },

    /* Theme (200-280) — left/right arrow pairs handled separately */
};

#define CURSOR_COUNT (sizeof(cursor_table) / sizeof(cursor_table[0]))

bool menu_cursor_for_state(int state, menu_cursor *out) {
    if (!out) return false;
    for (size_t i = 0; i < CURSOR_COUNT; i++) {
        if (cursor_table[i].state == state) {
            out->texture = cursor_table[i].texture;
            out->x = cursor_table[i].x;
            out->y = cursor_table[i].y;
            out->w = cursor_table[i].w;
            out->h = cursor_table[i].h;
            return true;
        }
    }

    /* Theme cursors: pairs of Arrowlefttheme + Arrowrighttheme (handled by caller via theme screen check) */
    return false;
}

/* ---------- Main menu arrows (state 1-11) ---------- */

typedef struct {
    int state;
    int lx, ly, rx, ry;
} main_arrow_entry;

/* yarrowoffset = 51; arrow dimensions: 56x50 for both Leftmenuarrowex and Rightmenuarrowex */
static const main_arrow_entry main_arrows[] = {
    {  1,  77,  94, 465,  94 },  /* Reset trip 1 */
    {  2, 505,  94, 897,  94 },  /* Reset trip 2 */
    {  3,  77, 145, 465, 145 },  /* Control setup        (94 + 51*1) */
    {  4, 505, 145, 897, 145 },  /* Set units            (94 + 51*1) */
    {  5,  77, 196, 897, 196 },  /* Set time             (94 + 51*2) — full width */
    {  6,  77, 247, 897, 247 },  /* Ambient light setup  (94 + 51*3) */
    {  7,  77, 298, 897, 298 },  /* Speed correction     (94 + 51*4) */
    {  8,  77, 349, 897, 349 },  /* Set theme            (94 + 51*5) */
    {  9,  77, 400, 897, 400 },  /* Gear indicator setup (94 + 51*6) */
    { 10,  77, 451, 897, 451 },  /* Coolant fan setup    (94 + 51*7) */
    { 11,  77, 502, 897, 502 },  /* TPMS Setup           (94 + 51*8) */
};

#define MAIN_ARROW_COUNT (sizeof(main_arrows) / sizeof(main_arrows[0]))

bool menu_main_item_arrows(int state, menu_cursor *left, menu_cursor *right) {
    if (!left || !right) return false;
    for (size_t i = 0; i < MAIN_ARROW_COUNT; i++) {
        if (main_arrows[i].state == state) {
            left->texture  = "Leftmenuarrowex.png";
            left->x  = main_arrows[i].lx;
            left->y  = main_arrows[i].ly;
            left->w  = 56;
            left->h  = 50;
            right->texture = "Rightmenuarrowex.png";
            right->x = main_arrows[i].rx;
            right->y = main_arrows[i].ry;
            right->w = 56;
            right->h = 50;
            return true;
        }
    }
    return false;
}

/* ---------- Theme menu arrows (state 200-280) ---------- */

static const main_arrow_entry theme_arrows[] = {
    { 200,   8,  28, 330,  29 },  /* Default theme 0 */
    { 210, 330,  28, 652,  29 },  /* Theme 1 */
    { 220, 660,  28, 982,  29 },  /* Theme 2 */
    { 230,   3, 221, 325, 221 },  /* Theme 3 */
    { 240, 331, 221, 653, 221 },  /* Theme 4 */
    { 250, 661, 221, 983, 221 },  /* Theme 5 */
    { 260,   8, 423, 330, 423 },  /* Theme 6 */
    { 270, 328, 423, 650, 423 },  /* Theme 7 */
    { 280, 653, 423, 982, 423 },  /* Theme 8 */
};

#define THEME_ARROW_COUNT (sizeof(theme_arrows) / sizeof(theme_arrows[0]))

bool menu_theme_arrows(int state, menu_cursor *left, menu_cursor *right) {
    if (!left || !right) return false;
    for (size_t i = 0; i < THEME_ARROW_COUNT; i++) {
        if (theme_arrows[i].state == state) {
            left->texture  = "Arrowlefttheme.png";
            left->x  = theme_arrows[i].lx;
            left->y  = theme_arrows[i].ly;
            left->w  = 48;
            left->h  = 159;
            right->texture = "Arrowrighttheme.png";
            right->x = theme_arrows[i].rx;
            right->y = theme_arrows[i].ry;
            right->w = 48;
            right->h = 159;
            return true;
        }
    }
    return false;
}

/* ---------- Theme thumbnails ---------- */

static const theme_thumb theme_thumbs[] = {
    { "default", "whitethumb.png"  },
    { "green",   "greenthumb.png"  },
    { "red",     "redthumb.png"    },
    { "blue",    "bluethumb.png"   },
    { "orange",  "orangethumb.png" },
    { "yellow",  "yellowthumb.png" },
    { "night",   "nightthumb.png"  },
    { "bright",  "brightthumb.png" },
    { "dark",    "darkthumb.png"   },
};

#define THEME_THUMB_COUNT (sizeof(theme_thumbs) / sizeof(theme_thumbs[0]))

const theme_thumb *menu_theme_thumb(int theme_id) {
    if (theme_id < 0 || (size_t)theme_id >= THEME_THUMB_COUNT)
        return NULL;
    return &theme_thumbs[theme_id];
}
