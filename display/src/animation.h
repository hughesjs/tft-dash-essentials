/*
 * animation.h — Data-driven animation system for TFT Dash
 *
 * Manages frame counters with four modes: oneshot, ping-pong, loop, and chase.
 * Animations are registered once and ticked together each frame via anim_tick_all().
 * No SDL dependency — pure state management.
 */

#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdbool.h>

typedef enum {
    ANIM_ONESHOT,       /* 0 → limit, then done */
    ANIM_PING_PONG,     /* 0 → limit → 0, then done */
    ANIM_LOOP,          /* 0 → limit → 0 → limit → ... forever */
    ANIM_CHASE,         /* move towards target by speed per tick */
} anim_mode;

typedef struct {
    anim_mode mode;
    int frame;
    int speed;
    int limit;
    int target;         /* ANIM_CHASE only */
    bool active;
    bool reversing;
    bool done;
} animation;

/* Registry — tick all active animations at once */
void anim_register(animation *a);
void anim_tick_all(void);
void anim_registry_clear(void);

/* Lifecycle */
void anim_start(animation *a);
void anim_stop(animation *a);

/* ANIM_CHASE only */
void anim_set_target(animation *a, int target);

/* Queries */
bool anim_is_active(const animation *a);
bool anim_is_done(const animation *a);
bool anim_is_reversing(const animation *a);
int anim_frame(const animation *a);
float anim_progress(const animation *a);

#endif /* ANIMATION_H */
