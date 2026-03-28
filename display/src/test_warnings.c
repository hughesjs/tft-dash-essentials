/*
 * test_warnings.c — Tests for the declarative warnings module
 */

#include "warnings.h"
#include "parser.h"
#include "tpms_feed.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

/* Stub for get_num_fuel_bars — the real one lives in draw_dashboard.c
 * which has SDL dependencies. This reproduces the low-fuel threshold. */
int get_num_fuel_bars(int value) {
    if (value >= 481) return 2;
    if (value >= 426) return 3;
    if (value >= 386) return 4;
    if (value >= 351) return 5;
    if (value >= 321) return 6;
    if (value >= 296) return 7;
    if (value >= 271) return 8;
    if (value >= 251) return 9;
    if (value >= 231) return 10;
    if (value >= 211) return 11;
    if (value >= 196) return 12;
    if (value >= 176) return 13;
    if (value >= 161) return 14;
    if (value >= 141) return 15;
    return 16;
}

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { \
    test_count++; \
    printf("  %d. %-50s", test_count, #name); \
    test_##name(); \
    pass_count++; \
    printf("PASS\n"); \
} while (0)

#define ASSERT_TRUE(cond)  assert(cond)
#define ASSERT_FALSE(cond) assert(!(cond))
#define ASSERT_NULL(p)     assert((p) == NULL)
#define ASSERT_NOT_NULL(p) assert((p) != NULL)
#define ASSERT_STR_EQ(a, b) assert(strcmp((a), (b)) == 0)

/* Helper: create a zeroed dashboard state */
static dashboard_state make_dash(void) {
    dashboard_state d = {0};
    return d;
}

/* Helper: create a zeroed tpms state */
static tpms_state make_tpms(void) {
    tpms_state t = {0};
    return t;
}

/* --- No warnings --- */

void test_no_warnings(void) {
    dashboard_state d = make_dash();
    tpms_state t = make_tpms();
    d.rpm = 500;

    warnings_update(&d, &t, 0);
    ASSERT_NULL(warnings_active());
}

/* --- Comms staleness (highest priority) --- */

void test_comms_stale_timeout(void) {
    dashboard_state d = make_dash();
    tpms_state t = make_tpms();

    warnings_update(&d, &t, 1500);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Commsbadge.png");
}

void test_comms_stale_no_data(void) {
    dashboard_state d = make_dash();
    tpms_state t = make_tpms();

    warnings_update(&d, &t, -1);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Commsbadge.png");
}

void test_comms_ok(void) {
    dashboard_state d = make_dash();
    tpms_state t = make_tpms();

    warnings_update(&d, &t, 100);
    ASSERT_NULL(warnings_active());
}

/* --- Tyre pressure --- */

void test_tyre_front_low(void) {
    dashboard_state d = make_dash();
    d.front_pressure_low = 30;
    tpms_state t = make_tpms();
    t.connected = true;
    t.front.received = true;
    t.front.psi = 25;

    warnings_update(&d, &t, 100);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Lowtyrebadge.png");
    ASSERT_STR_EQ(w->flash(&d, &t), "Fronttyrelow.png");
}

void test_tyre_rear_low(void) {
    dashboard_state d = make_dash();
    d.rear_pressure_low = 30;
    tpms_state t = make_tpms();
    t.connected = true;
    t.rear.received = true;
    t.rear.psi = 25;

    warnings_update(&d, &t, 100);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->flash(&d, &t), "Reartyrelow.png");
}

void test_tyre_both_low(void) {
    dashboard_state d = make_dash();
    d.front_pressure_low = 30;
    d.rear_pressure_low = 30;
    tpms_state t = make_tpms();
    t.connected = true;
    t.front.received = true;
    t.front.psi = 25;
    t.rear.received = true;
    t.rear.psi = 25;

    warnings_update(&d, &t, 100);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->flash(&d, &t), "Frontrearlow.png");
}

void test_tyre_not_connected(void) {
    dashboard_state d = make_dash();
    d.front_pressure_low = 30;
    tpms_state t = make_tpms();
    t.connected = false;
    t.front.received = true;
    t.front.psi = 25;

    warnings_update(&d, &t, 100);
    ASSERT_NULL(warnings_active());
}

/* --- Oil warning --- */

void test_oil_warning_engine_running(void) {
    dashboard_state d = make_dash();
    d.oil_warning = true;
    d.rpm = 3000;
    tpms_state t = make_tpms();

    /* Need two updates to get engine_running past hysteresis */
    warnings_update(&d, &t, 100);
    warnings_update(&d, &t, 100);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Lowoilbadge.png");
}

void test_oil_warning_engine_off(void) {
    dashboard_state d = make_dash();
    d.oil_warning = true;
    d.rpm = 500;
    tpms_state t = make_tpms();

    /* Engine not running — oil warning suppressed */
    warnings_update(&d, &t, 100);
    ASSERT_NULL(warnings_active());
}

/* --- Overheat warning --- */

void test_overheat_engine_running(void) {
    dashboard_state d = make_dash();
    d.coolant_temp = 125;
    d.rpm = 3000;
    tpms_state t = make_tpms();

    warnings_update(&d, &t, 100);
    warnings_update(&d, &t, 100);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Overheatbadge.png");
}

void test_overheat_below_threshold(void) {
    dashboard_state d = make_dash();
    d.coolant_temp = 119;
    d.rpm = 3000;
    tpms_state t = make_tpms();

    warnings_update(&d, &t, 100);
    warnings_update(&d, &t, 100);
    ASSERT_NULL(warnings_active());
}

/* --- Fuel warning --- */

void test_fuel_warning(void) {
    dashboard_state d = make_dash();
    d.fuel_float = 950;  /* High resistance = low fuel */
    d.rpm = 3000;
    tpms_state t = make_tpms();

    warnings_update(&d, &t, 100);
    warnings_update(&d, &t, 100);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Fuelwarningbadge.png");
}

void test_fuel_warning_suppressed_in_nav(void) {
    dashboard_state d = make_dash();
    d.fuel_float = 950;
    d.rpm = 3000;
    d.info_mode = 3;  /* Nav mode — fuel warning suppressed */
    tpms_state t = make_tpms();

    warnings_update(&d, &t, 100);
    warnings_update(&d, &t, 100);
    ASSERT_NULL(warnings_active());
}

/* --- Priority ordering --- */

void test_comms_beats_tyre(void) {
    dashboard_state d = make_dash();
    d.front_pressure_low = 30;
    tpms_state t = make_tpms();
    t.connected = true;
    t.front.received = true;
    t.front.psi = 25;

    /* Both comms stale and tyre low — comms should win */
    warnings_update(&d, &t, 1500);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Commsbadge.png");
}

void test_tyre_beats_oil(void) {
    dashboard_state d = make_dash();
    d.oil_warning = true;
    d.rpm = 3000;
    d.front_pressure_low = 30;
    tpms_state t = make_tpms();
    t.connected = true;
    t.front.received = true;
    t.front.psi = 25;

    warnings_update(&d, &t, 100);
    warnings_update(&d, &t, 100);
    const warning_def *w = warnings_active();
    ASSERT_NOT_NULL(w);
    ASSERT_STR_EQ(w->badge, "Lowtyrebadge.png");
}

/* --- Engine running hysteresis --- */

void test_engine_hysteresis(void) {
    dashboard_state d = make_dash();
    tpms_state t = make_tpms();

    d.rpm = 500;
    warnings_update(&d, &t, 100);
    ASSERT_FALSE(warnings_engine_running());

    d.rpm = 2500;
    warnings_update(&d, &t, 100);
    ASSERT_TRUE(warnings_engine_running());

    /* Drop below 2000 but above 1000 — still running (hysteresis) */
    d.rpm = 1500;
    warnings_update(&d, &t, 100);
    ASSERT_TRUE(warnings_engine_running());

    /* Drop below 1000 — now off */
    d.rpm = 800;
    warnings_update(&d, &t, 100);
    ASSERT_FALSE(warnings_engine_running());
}

/* --- Cancellation --- */

void test_cancel(void) {
    dashboard_state d = make_dash();
    tpms_state t = make_tpms();

    warnings_update(&d, &t, 1500);
    ASSERT_NOT_NULL(warnings_active());

    warnings_cancel();
    ASSERT_NULL(warnings_active());
    ASSERT_TRUE(warnings_is_cancelled());
}

/* --- Main --- */

int main(void) {
    printf("=== Warnings Tests ===\n\n");

    TEST(no_warnings);
    TEST(comms_stale_timeout);
    TEST(comms_stale_no_data);
    TEST(comms_ok);
    TEST(tyre_front_low);
    TEST(tyre_rear_low);
    TEST(tyre_both_low);
    TEST(tyre_not_connected);
    TEST(oil_warning_engine_running);
    TEST(oil_warning_engine_off);
    TEST(overheat_engine_running);
    TEST(overheat_below_threshold);
    TEST(fuel_warning);
    TEST(fuel_warning_suppressed_in_nav);
    TEST(comms_beats_tyre);
    TEST(tyre_beats_oil);
    TEST(engine_hysteresis);
    TEST(cancel);

    printf("\n  Results: %d/%d passed\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
