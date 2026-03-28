/*
 * dashboard_anims.h — Animation instances and data tables for the dashboard
 *
 * Defines the startup, info transition, flash, and RPM animations plus
 * the lookup tables for info mode screen transitions and startup rev counter reveal.
 */

#ifndef DASHBOARD_ANIMS_H
#define DASHBOARD_ANIMS_H

#include "animation.h"
#include <stddef.h>

/* --- Animation instances --- */

extern animation anim_startup;
extern animation anim_info;
extern animation anim_flash;
extern animation anim_rpm;

/* --- Info mode transition table --- */

#define INFO_MODE_COUNT 4
#define INFO_SCREEN_MAX_TEXTURES 2

typedef struct {
    const char *textures[INFO_SCREEN_MAX_TEXTURES];     /* texture names (NULL-terminated) */
    const char *textures_km[INFO_SCREEN_MAX_TEXTURES];  /* km variant (NULL = same as textures) */
    float x[INFO_SCREEN_MAX_TEXTURES], y[INFO_SCREEN_MAX_TEXTURES], w[INFO_SCREEN_MAX_TEXTURES], h[INFO_SCREEN_MAX_TEXTURES];
} info_screen;

/* Indexed by info mode (0-3). Describes the textures shown when that mode is active. */
extern const info_screen INFO_SCREENS[INFO_MODE_COUNT];

/* Current info mode — updated when info transition completes */
int dashboard_anims_info_mode(void);
void dashboard_anims_set_info_mode(int mode);

/* --- Startup rev counter reveal table --- */

typedef struct {
    int threshold;          /* frame count at which this number appears */
    const char *texture;    /* texture name */
    float x, y, w, h;      /* destination rect */
} rev_reveal_entry;

#define REV_REVEAL_COUNT 13

extern const rev_reveal_entry REV_REVEAL[REV_REVEAL_COUNT];

/* --- Init — register all animations --- */

void dashboard_anims_init(void);

#endif /* DASHBOARD_ANIMS_H */
