/*
 * animation.c — Data-driven animation system for TFT Dash
 */

#include "animation.h"
#include <stddef.h>

#define MAX_ANIMATIONS 16

static animation *registry[MAX_ANIMATIONS];
static int registry_count = 0;

/* --- Registry --- */

void anim_register(animation *a) {
    if (!a || registry_count >= MAX_ANIMATIONS) return;
    registry[registry_count++] = a;
}

static void tick_one(animation *a) {
    if (!a || !a->active || a->done) return;

    switch (a->mode) {
    case ANIM_ONESHOT:
        a->frame += a->speed;
        if (a->frame >= a->limit) {
            a->frame = a->limit;
            a->done = true;
        }
        break;

    case ANIM_PING_PONG:
        if (!a->reversing) {
            a->frame += a->speed;
            if (a->frame >= a->limit) {
                a->frame = a->limit;
                a->reversing = true;
            }
        } else {
            a->frame -= a->speed;
            if (a->frame <= 0) {
                a->frame = 0;
                a->done = true;
            }
        }
        break;

    case ANIM_LOOP:
        if (!a->reversing) {
            a->frame += a->speed;
            if (a->frame >= a->limit) {
                a->frame = a->limit;
                a->reversing = true;
            }
        } else {
            a->frame -= a->speed;
            if (a->frame <= 0) {
                a->frame = 0;
                a->reversing = false;
            }
        }
        break;

    case ANIM_CHASE:
        if (a->frame < a->target) {
            a->frame += a->speed;
            if (a->frame > a->target) a->frame = a->target;
        } else if (a->frame > a->target) {
            a->frame -= a->speed;
            if (a->frame < a->target) a->frame = a->target;
        }
        break;
    }
}

void anim_tick_all(void) {
    for (int i = 0; i < registry_count; i++) {
        tick_one(registry[i]);
    }
}

void anim_registry_clear(void) {
    registry_count = 0;
}

/* --- Lifecycle --- */

void anim_start(animation *a) {
    if (!a) return;
    a->frame = 0;
    a->active = true;
    a->reversing = false;
    a->done = false;
}

void anim_stop(animation *a) {
    if (!a) return;
    a->active = false;
}

/* --- Chase --- */

void anim_set_target(animation *a, int target) {
    if (!a) return;
    a->target = target;
}

/* --- Queries --- */

bool anim_is_active(const animation *a) {
    return a ? a->active : false;
}

bool anim_is_done(const animation *a) {
    return a ? a->done : false;
}

bool anim_is_reversing(const animation *a) {
    return a ? a->reversing : false;
}

int anim_frame(const animation *a) {
    return a ? a->frame : 0;
}

float anim_progress(const animation *a) {
    if (!a || a->limit == 0) return 0.0f;
    float p = (float)a->frame / (float)a->limit;
    if (p < 0.0f) return 0.0f;
    if (p > 1.0f) return 1.0f;
    return p;
}
