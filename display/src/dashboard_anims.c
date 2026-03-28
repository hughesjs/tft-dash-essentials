/*
 * dashboard_anims.c — Animation instances and data tables for the dashboard
 */

#include "dashboard_anims.h"

/* --- Animation instances --- */

animation anim_startup = { .mode = ANIM_ONESHOT,    .speed = 8,  .limit = 1400 };
animation anim_info    = { .mode = ANIM_PING_PONG,  .speed = 15, .limit = 600 };
animation anim_flash   = { .mode = ANIM_LOOP,       .speed = 1,  .limit = 50 };
animation anim_rpm     = { .mode = ANIM_CHASE,      .speed = 1,  .limit = 100 };

/* --- Info mode screen definitions --- */

/*
 * Each info mode has a set of textures that slide on/off during transitions.
 * Mode 0 has a km variant (InfotopKM.png vs Infotop.png).
 * Mode 3 (nav) uses a single full-height background.
 */
const info_screen INFO_SCREENS[INFO_MODE_COUNT] = {
    [0] = {
        .textures    = { "Infotop.png",      "Infobottom.png" },
        .textures_km = { "InfotopKM.png",    "Infobottom.png" },
        .rects       = { { 0, 176, 510, 97 }, { 0, 273, 454, 125 } },
    },
    [1] = {
        .textures    = { "Infotopdiag.png",  "Infobottomdiag.png" },
        .rects       = { { 0, 176, 510, 97 }, { 0, 273, 454, 125 } },
    },
    [2] = {
        .textures    = { "tyretop.png",      "tyrebottom.png" },
        .rects       = { { 0, 176, 510, 97 }, { 0, 273, 454, 125 } },
    },
    [3] = {
        .textures    = { "Navbg.png",        NULL },
        .rects       = { { 0, 158, 434, 268 }, { 0 } },
    },
};

int info_current_mode = 0;

/* --- Startup rev counter reveal table --- */

/*
 * Rev counter numbers appear one by one during the startup animation.
 * Thresholds match the original: first at frame 110, then every 100 frames.
 * rect pointers are set during dashboard_anims_init() since the rects
 * are defined and initialised in main.c's init_rects().
 */
const rev_reveal_entry REV_REVEAL[REV_REVEAL_COUNT] = {
    { 110,  "R0.png",      { 471, 518, 88,  56 } },
    { 210,  "R1.png",      { 475, 458, 83,  44 } },
    { 310,  "R2.png",      { 487, 401, 89,  48 } },
    { 410,  "R3.png",      { 507, 348, 87,  54 } },
    { 510,  "R4.png",      { 533, 300, 82,  58 } },
    { 610,  "R5.png",      { 564, 258, 79,  62 } },
    { 710,  "R6.png",      { 601, 219, 72,  68 } },
    { 810,  "R7.png",      { 641, 179, 64,  80 } },
    { 910,  "R8.png",      { 688, 145, 58,  86 } },
    { 1010, "R9.png",      { 739, 117, 52,  89 } },
    { 1110, "R10.png",     { 796, 95,  45,  96 } },
    { 1210, "R11.png",     { 846, 78,  48,  97 } },
    { 1310, "R12-13.png",  { 898, 61,  108, 102 } },
};

/* --- Init --- */

void dashboard_anims_init(void) {
    anim_register(&anim_startup);
    anim_register(&anim_info);
    anim_register(&anim_flash);
    anim_register(&anim_rpm);

    anim_start(&anim_startup);
    anim_start(&anim_flash);
    anim_start(&anim_rpm);
}
