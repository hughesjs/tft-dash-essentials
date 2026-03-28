/*
 * test_animation.c — Tests for the animation module
 */

#include "animation.h"
#include "test_helpers.h"

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { \
    test_count++; \
    printf("  %d. %-50s", test_count, #name); \
    anim_registry_clear(); \
    test_##name(); \
    pass_count++; \
    printf("PASS\n"); \
} while (0)

/* --- Helper to tick via registry for single-animation tests --- */

static void tick_all_once(animation *a) {
    anim_registry_clear();
    anim_register(a);
    anim_tick_all();
}

/* --- Oneshot tests --- */

void test_oneshot_basics(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 10, .limit = 100 };
    anim_start(&a);

    ASSERT_TRUE(anim_is_active(&a));
    ASSERT_FALSE(anim_is_done(&a));
    ASSERT_EQ(anim_frame(&a), 0);

    /* Tick 5 times: 0 → 10 → 20 → 30 → 40 → 50 */
    for (int i = 0; i < 5; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 50);
    ASSERT_FALSE(anim_is_done(&a));

    /* Tick 5 more: 60 → 70 → 80 → 90 → 100 (done) */
    for (int i = 0; i < 5; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 100);
    ASSERT_TRUE(anim_is_done(&a));
}

void test_oneshot_no_overshoot(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 30, .limit = 100 };
    anim_start(&a);

    /* speed=30, limit=100: after 4 ticks should be clamped to 100 */
    for (int i = 0; i < 10; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 100);
    ASSERT_TRUE(anim_is_done(&a));
}

void test_oneshot_progress(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 25, .limit = 100 };
    anim_start(&a);

    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.0f);

    tick_all_once(&a); /* frame=25 */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.25f);

    tick_all_once(&a); /* frame=50 */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.50f);

    tick_all_once(&a); /* frame=75 */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.75f);

    tick_all_once(&a); /* frame=100, done */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 1.0f);
}

/* --- Ping-pong tests --- */

void test_ping_pong_basics(void) {
    animation a = { .mode = ANIM_PING_PONG, .speed = 10, .limit = 30 };
    anim_start(&a);

    /* Ascent: 0 → 10 → 20 → 30 */
    for (int i = 0; i < 3; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 30);
    ASSERT_TRUE(anim_is_reversing(&a));
    ASSERT_FALSE(anim_is_done(&a));

    /* Descent: 30 → 20 → 10 → 0 (done) */
    for (int i = 0; i < 3; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 0);
    ASSERT_TRUE(anim_is_done(&a));
}

void test_ping_pong_reversing_flag(void) {
    animation a = { .mode = ANIM_PING_PONG, .speed = 50, .limit = 100 };
    anim_start(&a);

    ASSERT_FALSE(anim_is_reversing(&a));
    tick_all_once(&a); /* frame=50 */
    ASSERT_FALSE(anim_is_reversing(&a));
    tick_all_once(&a); /* frame=100, reverses */
    ASSERT_TRUE(anim_is_reversing(&a));
}

void test_ping_pong_progress(void) {
    animation a = { .mode = ANIM_PING_PONG, .speed = 50, .limit = 100 };
    anim_start(&a);

    tick_all_once(&a); /* frame=50 ascending */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.5f);

    tick_all_once(&a); /* frame=100, just reversed */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 1.0f);

    tick_all_once(&a); /* frame=50 descending */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.5f);

    tick_all_once(&a); /* frame=0, done */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.0f);
}

/* --- Loop tests --- */

void test_loop_basics(void) {
    animation a = { .mode = ANIM_LOOP, .speed = 10, .limit = 20 };
    anim_start(&a);

    /* Up: 0 → 10 → 20 */
    tick_all_once(&a);
    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 20);
    ASSERT_TRUE(anim_is_reversing(&a));

    /* Down: 20 → 10 → 0 */
    tick_all_once(&a);
    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 0);
    ASSERT_FALSE(anim_is_reversing(&a));

    /* Up again */
    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 10);
    ASSERT_FALSE(anim_is_reversing(&a));
    ASSERT_FALSE(anim_is_done(&a));
}

void test_loop_never_done(void) {
    animation a = { .mode = ANIM_LOOP, .speed = 5, .limit = 10 };
    anim_start(&a);

    for (int i = 0; i < 1000; i++) tick_all_once(&a);
    ASSERT_TRUE(anim_is_active(&a));
    ASSERT_FALSE(anim_is_done(&a));
}

/* --- Chase tests --- */

void test_chase_basics(void) {
    animation a = { .mode = ANIM_CHASE, .speed = 1, .limit = 100 };
    anim_start(&a);
    anim_set_target(&a, 5);

    for (int i = 0; i < 5; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 5);
}

void test_chase_tracks_moving_target(void) {
    animation a = { .mode = ANIM_CHASE, .speed = 1, .limit = 100 };
    anim_start(&a);

    anim_set_target(&a, 3);
    for (int i = 0; i < 3; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 3);

    /* Target moves down */
    anim_set_target(&a, 1);
    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 2);
    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 1);
}

void test_chase_at_target(void) {
    animation a = { .mode = ANIM_CHASE, .speed = 1, .limit = 100 };
    anim_start(&a);
    anim_set_target(&a, 0);

    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 0); /* already there, no movement */
}

void test_chase_no_overshoot(void) {
    animation a = { .mode = ANIM_CHASE, .speed = 10, .limit = 100 };
    anim_start(&a);
    anim_set_target(&a, 3);

    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 3); /* clamped to target, not 10 */
}

/* --- Lifecycle tests --- */

void test_stop_and_start(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 10, .limit = 100 };
    anim_start(&a);

    for (int i = 0; i < 3; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 30);

    anim_stop(&a);
    ASSERT_FALSE(anim_is_active(&a));

    /* Tick does nothing when stopped */
    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 30);

    /* Start resets */
    anim_start(&a);
    ASSERT_EQ(anim_frame(&a), 0);
    ASSERT_TRUE(anim_is_active(&a));
    ASSERT_FALSE(anim_is_done(&a));
}

void test_inactive_not_ticked(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 10, .limit = 100 };
    /* Not started — active is false */

    tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 0);
}

/* --- Registry tests --- */

void test_registry_ticks_all(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 1, .limit = 100 };
    animation b = { .mode = ANIM_ONESHOT, .speed = 2, .limit = 100 };
    anim_start(&a);
    anim_start(&b);
    anim_register(&a);
    anim_register(&b);

    anim_tick_all();
    ASSERT_EQ(anim_frame(&a), 1);
    ASSERT_EQ(anim_frame(&b), 2);

    anim_tick_all();
    ASSERT_EQ(anim_frame(&a), 2);
    ASSERT_EQ(anim_frame(&b), 4);
}

void test_registry_skips_inactive(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 1, .limit = 100 };
    animation b = { .mode = ANIM_ONESHOT, .speed = 1, .limit = 100 };
    anim_start(&a);
    /* b not started */
    anim_register(&a);
    anim_register(&b);

    anim_tick_all();
    ASSERT_EQ(anim_frame(&a), 1);
    ASSERT_EQ(anim_frame(&b), 0);
}

/* --- Edge case tests --- */

void test_chase_target_above_limit(void) {
    animation a = { .mode = ANIM_CHASE, .speed = 1, .limit = 100 };
    anim_start(&a);
    anim_set_target(&a, 200);

    /* Chase should move towards 200, even though limit is 100 */
    for (int i = 0; i < 200; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 200);
}

void test_oneshot_speed_zero(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 0, .limit = 100 };
    anim_start(&a);

    /* speed=0, frame should stay at 0 forever — no infinite loop */
    for (int i = 0; i < 100; i++) tick_all_once(&a);
    ASSERT_EQ(anim_frame(&a), 0);
    ASSERT_FALSE(anim_is_done(&a));
}

void test_progress_limit_zero(void) {
    animation a = { .mode = ANIM_ONESHOT, .speed = 1, .limit = 0 };
    anim_start(&a);

    /* limit=0: progress should return 0.0, not NaN or crash */
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.0f);

    tick_all_once(&a);
    ASSERT_FLOAT_NEAR(anim_progress(&a), 0.0f);
}

/* --- Null safety --- */

void test_null_safety(void) {
    anim_start(NULL);
    anim_stop(NULL);
    anim_set_target(NULL, 5);
    anim_register(NULL);

    ASSERT_FALSE(anim_is_active(NULL));
    ASSERT_FALSE(anim_is_done(NULL));
    ASSERT_FALSE(anim_is_reversing(NULL));
    ASSERT_EQ(anim_frame(NULL), 0);
    ASSERT_FLOAT_NEAR(anim_progress(NULL), 0.0f);
}

/* --- Main --- */

int main(void) {
    printf("=== Animation Tests ===\n\n");

    TEST(oneshot_basics);
    TEST(oneshot_no_overshoot);
    TEST(oneshot_progress);
    TEST(ping_pong_basics);
    TEST(ping_pong_reversing_flag);
    TEST(ping_pong_progress);
    TEST(loop_basics);
    TEST(loop_never_done);
    TEST(chase_basics);
    TEST(chase_tracks_moving_target);
    TEST(chase_at_target);
    TEST(chase_no_overshoot);
    TEST(stop_and_start);
    TEST(inactive_not_ticked);
    TEST(registry_ticks_all);
    TEST(registry_skips_inactive);
    TEST(chase_target_above_limit);
    TEST(oneshot_speed_zero);
    TEST(progress_limit_zero);
    TEST(null_safety);

    printf("\n  Results: %d/%d passed\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
