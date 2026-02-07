/*
 * test_parser.c - Unit tests for message parsing
 *
 * Compile: gcc -o test_parser test_parser.c parser.c -std=c99
 * Run: ./test_parser
 */

#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

/* Test helpers */
#define TEST(name) \
    printf("Running test: %s...", #name); \
    test_##name(); \
    printf(" PASSED\n");

#define ASSERT_EQ(actual, expected) \
    assert((actual) == (expected))

#define ASSERT_FLOAT_EQ(actual, expected) \
    assert(fabs((actual) - (expected)) < 0.01)

#define ASSERT_STR_EQ(actual, expected) \
    assert(strcmp((actual), (expected)) == 0)

#define ASSERT_TRUE(condition) \
    assert(condition)

#define ASSERT_FALSE(condition) \
    assert(!(condition))

/* Test: Parse basic live message */
void test_parse_live_basic() {
    DashboardState state = {0};
    const char* msg = ",068,1234,80,12.5,14,23,477,1,0,1,0,0,200,0,1.2,3.4,23456.7,0,0.5,2,21,3,45,120,85,1,30,0,100,200,0,0,1,2,28,30,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_EQ(state.currentSpeed, 68);
    ASSERT_EQ(state.rpm, 1234);
    ASSERT_EQ(state.coolanttemp, 80);
    ASSERT_FLOAT_EQ(state.batt, 12.5);
    ASSERT_EQ(state.currenthour, 14);
    ASSERT_EQ(state.currentminute, 23);
    ASSERT_EQ(state.fuelfloat, 477);
}

/* Test: Parse boolean fields correctly */
void test_parse_live_booleans() {
    DashboardState state = {0};
    const char* msg = ",0,0,0,0.0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_TRUE(state.neutral);
    ASSERT_TRUE(state.oilwarning);
    ASSERT_TRUE(state.highbeam);
    ASSERT_TRUE(state.indicateleft);
    ASSERT_TRUE(state.indicateright);
    ASSERT_TRUE(state.oilpressureavailable);
}

/* Test: Parse float fields correctly */
void test_parse_live_floats() {
    DashboardState state = {0};
    const char* msg = ",0,0,0,13.8,0,0,0,0,0,0,0,0,0,0,123.45,678.90,12345.6,0,-2.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_FLOAT_EQ(state.batt, 13.8);
    ASSERT_FLOAT_EQ(state.trip1, 123.45);
    ASSERT_FLOAT_EQ(state.trip2, 678.90);
    ASSERT_FLOAT_EQ(state.odo, 12345.6);
    ASSERT_FLOAT_EQ(state.spdcorrect, -2.5);
}

/* Test: Parse navigation string field */
void test_parse_live_nav_string() {
    DashboardState state = {0};
    const char* msg = ",0,0,0,0.0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,%TNL%A9%Glasgow%J16%0.5%MILE%,";

    ASSERT_TRUE(parse_live_message(msg, &state));
    ASSERT_STR_EQ(state.strNav, "%TNL%A9%Glasgow%J16%0.5%MILE%");
}

/* Test: Parse menu message */
void test_parse_menu_basic() {
    MenuState state = {0};
    const char* msg = ",300,1,2,3,4,5,6,7,8,9,0,1,2,0,1,2,3,4,1,5,6,7,15,42,85,0,0,0,1,2,28,30,0,2,6,100,50,0,200,";

    ASSERT_TRUE(parse_menu_message(msg, &state));

    ASSERT_EQ(state.choicestate, 300);
    ASSERT_EQ(state.ododigit1, 1);
    ASSERT_EQ(state.ododigit2, 2);
    ASSERT_EQ(state.ododigit3, 3);
    ASSERT_EQ(state.ododigit4, 4);
    ASSERT_EQ(state.ododigit5, 5);
    ASSERT_EQ(state.ododigit6, 6);
    ASSERT_EQ(state.frontsprocket, 15);
    ASSERT_EQ(state.rearsprocket, 42);
    ASSERT_EQ(state.coolantfantemp, 85);
}

/* Test: Parse navigation message */
void test_parse_nav_basic() {
    NavState state = {0};
    const char* msg = "%TNL%A9%Glasgow%J16%0.5%MILE%";

    ASSERT_TRUE(parse_nav_message(msg, &state));

    ASSERT_STR_EQ(state.strNavSymbol, "TNL");
    ASSERT_STR_EQ(state.strNavRoad, "A9");
    ASSERT_STR_EQ(state.strNavTowards, "Glasgow");
    ASSERT_STR_EQ(state.strNavExit, "J16");
    ASSERT_STR_EQ(state.strNavDistance, "0.5");
    ASSERT_STR_EQ(state.strNavDistanceUnits, "MILE");
    ASSERT_FLOAT_EQ(state.navMiles, 0.5);
    ASSERT_TRUE(state.navActive);
}

/* Test: Parse navigation with different units */
void test_parse_nav_units() {
    NavState state = {0};

    /* Test yards */
    const char* msg_yards = "%TNR%M6%Manchester%14%500%YARD%";
    ASSERT_TRUE(parse_nav_message(msg_yards, &state));
    ASSERT_EQ(state.navYards, 500);

    /* Test kilometres */
    memset(&state, 0, sizeof(state));
    const char* msg_km = "%SLL%N7%Dublin%2%15%KM%";
    ASSERT_TRUE(parse_nav_message(msg_km, &state));
    ASSERT_EQ(state.navKM, 15);

    /* Test metres */
    memset(&state, 0, sizeof(state));
    const char* msg_metres = "%RB2L%A1%Edinburgh%1%250%METRE%";
    ASSERT_TRUE(parse_nav_message(msg_metres, &state));
    ASSERT_EQ(state.navMetres, 250);
}

/* Test: Empty navigation should not be active */
void test_parse_nav_empty() {
    NavState state = {0};
    const char* msg = "%%%%%%";

    ASSERT_TRUE(parse_nav_message(msg, &state));
    ASSERT_FALSE(state.navActive);
}

/* Test: Null pointer safety */
void test_null_safety() {
    DashboardState dash_state = {0};
    MenuState menu_state = {0};
    NavState nav_state = {0};

    ASSERT_FALSE(parse_live_message(NULL, &dash_state));
    ASSERT_FALSE(parse_live_message(",0,", NULL));
    ASSERT_FALSE(parse_menu_message(NULL, &menu_state));
    ASSERT_FALSE(parse_menu_message(",0,", NULL));
    ASSERT_FALSE(parse_nav_message(NULL, &nav_state));
    ASSERT_FALSE(parse_nav_message("%0%", NULL));
}

/* Test: Real-world message from comments */
void test_real_world_message() {
    DashboardState state = {0};
    /* From testsdl.cpp comment: "S,079,12500,100,12.5,14:23,477,0,0,0,0,0,200,E" */
    const char* msg = ",079,12500,100,12.5,14,23,477,0,0,0,0,0,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_EQ(state.currentSpeed, 79);
    ASSERT_EQ(state.rpm, 12500);
    ASSERT_EQ(state.coolanttemp, 100);
    ASSERT_FLOAT_EQ(state.batt, 12.5);
    ASSERT_EQ(state.currenthour, 14);
    ASSERT_EQ(state.currentminute, 23);
    ASSERT_EQ(state.fuelfloat, 477);
    ASSERT_FALSE(state.neutral);
    ASSERT_FALSE(state.oilwarning);
    ASSERT_FALSE(state.highbeam);
}

int main(void) {
    printf("=== TFT Dash Parser Test Suite ===\n\n");

    TEST(parse_live_basic);
    TEST(parse_live_booleans);
    TEST(parse_live_floats);
    TEST(parse_live_nav_string);
    TEST(parse_menu_basic);
    TEST(parse_nav_basic);
    TEST(parse_nav_units);
    TEST(parse_nav_empty);
    TEST(null_safety);
    TEST(real_world_message);

    printf("\n=== All tests passed! ===\n");
    return 0;
}
