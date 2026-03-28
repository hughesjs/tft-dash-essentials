
#include "parser.h"
#include "test_helpers.h"

#define TEST(name) \
    printf("Running test: %s...", #name); \
    test_##name(); \
    printf(" PASSED\n");

/* Test: Parse basic live message */
void test_parse_live_basic() {
    dashboard_state state = {0};
    const char* msg = ",068,1234,80,12.5,14,23,477,1,0,1,0,0,200,0,1.2,3.4,23456.7,0,0.5,2,21,3,45,120,85,1,30,0,100,200,0,0,1,2,28,30,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_EQ(state.current_speed, 68);
    ASSERT_EQ(state.rpm, 1234);
    ASSERT_EQ(state.coolant_temp, 80);
    ASSERT_FLOAT_NEAR(state.batt, 12.5);
    ASSERT_EQ(state.current_hour, 14);
    ASSERT_EQ(state.current_minute, 23);
    ASSERT_EQ(state.fuel_float, 477);
}

/* Test: Parse boolean fields correctly */
void test_parse_live_booleans() {
    dashboard_state state = {0};
    const char* msg = ",0,0,0,0.0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_TRUE(state.neutral);
    ASSERT_TRUE(state.oil_warning);
    ASSERT_TRUE(state.high_beam);
    ASSERT_TRUE(state.indicate_left);
    ASSERT_TRUE(state.indicate_right);
    ASSERT_TRUE(state.oil_pressure_available);
}

/* Test: Parse float fields correctly */
void test_parse_live_floats() {
    dashboard_state state = {0};
    const char* msg = ",0,0,0,13.8,0,0,0,0,0,0,0,0,0,0,123.45,678.90,12345.6,0,-2.5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_FLOAT_NEAR(state.batt, 13.8);
    ASSERT_FLOAT_NEAR(state.trip1, 123.45);
    ASSERT_FLOAT_NEAR(state.trip2, 678.90);
    ASSERT_FLOAT_NEAR(state.odo, 12345.6);
    ASSERT_FLOAT_NEAR(state.spd_correct, -2.5);
}

/* Test: Parse navigation string field */
void test_parse_live_nav_string() {
    dashboard_state state = {0};
    const char* msg = ",0,0,0,0.0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,%TNL%A9%Glasgow%J16%0.5%MILE%,";

    ASSERT_TRUE(parse_live_message(msg, &state));
    ASSERT_STR_EQ(state.nav_string, "%TNL%A9%Glasgow%J16%0.5%MILE%");
}

/* Test: Parse menu message */
void test_parse_menu_basic() {
    menu_state state = {0};
    const char* msg = ",300,1,2,3,4,5,6,7,8,9,0,1,2,0,1,2,3,4,1,5,6,7,15,42,85,0,0,0,1,2,28,30,0,2,6,100,50,0,200,";

    ASSERT_TRUE(parse_menu_message(msg, &state));

    ASSERT_EQ(state.choice_state, 300);
    ASSERT_EQ(state.odo_digit1, 1);
    ASSERT_EQ(state.odo_digit2, 2);
    ASSERT_EQ(state.odo_digit3, 3);
    ASSERT_EQ(state.odo_digit4, 4);
    ASSERT_EQ(state.odo_digit5, 5);
    ASSERT_EQ(state.odo_digit6, 6);
    ASSERT_EQ(state.front_sprocket, 15);
    ASSERT_EQ(state.rear_sprocket, 42);
    ASSERT_EQ(state.coolant_fan_temp, 85);
}

/* Test: Parse navigation message */
void test_parse_nav_basic() {
    nav_state state = {0};
    const char* msg = "%TNL%A9%Glasgow%J16%350%0.2%1%12.5%";

    ASSERT_TRUE(parse_nav_message(msg, &state));

    ASSERT_STR_EQ(state.nav_symbol, "TNL");
    ASSERT_STR_EQ(state.nav_road, "A9");
    ASSERT_STR_EQ(state.nav_towards, "Glasgow");
    ASSERT_STR_EQ(state.nav_exit, "J16");
    ASSERT_EQ(state.nav_yards, 350);
    ASSERT_FLOAT_NEAR(state.nav_miles, 0.2);
    ASSERT_EQ(state.driving_left, 1);
    ASSERT_FLOAT_NEAR(state.nav_dest_distance, 12.5);
    ASSERT_TRUE(state.nav_active);
}

/* Test: Parse navigation with different values */
void test_parse_nav_units() {
    nav_state state = {0};

    /* Right-hand drive, large distance */
    const char* msg = "%TNR%M6%Manchester%14%500%1.4%0%25.0%";
    ASSERT_TRUE(parse_nav_message(msg, &state));
    ASSERT_EQ(state.nav_yards, 500);
    ASSERT_FLOAT_NEAR(state.nav_miles, 1.4);
    ASSERT_EQ(state.driving_left, 0);
    ASSERT_FLOAT_NEAR(state.nav_dest_distance, 25.0);
}

/* Test: Empty navigation should not be active */
void test_parse_nav_empty() {
    nav_state state = {0};
    const char* msg = "%%%%%%";

    ASSERT_TRUE(parse_nav_message(msg, &state));
    ASSERT_FALSE(state.nav_active);
}

/* Test: Long nav_string close to 255-char limit */
void test_parse_live_long_nav_string() {
    dashboard_state state = {0};

    /* Build a message with a nav_string of 253 chars (just under the 255-char buffer) */
    char long_nav[254];
    memset(long_nav, 'A', 253);
    long_nav[253] = '\0';

    char msg[512];
    snprintf(msg, sizeof(msg), ",0,0,0,0.0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,%s,", long_nav);

    ASSERT_TRUE(parse_live_message(msg, &state));
    /* nav_string should be truncated to fit within the 255-char buffer */
    ASSERT_TRUE(strlen(state.nav_string) <= 254);
    ASSERT_TRUE(strlen(state.nav_string) > 0);
}

/* Test: Null pointer safety */
void test_null_safety() {
    dashboard_state ds = {0};
    menu_state ms = {0};
    nav_state ns = {0};

    ASSERT_FALSE(parse_live_message(NULL, &ds));
    ASSERT_FALSE(parse_live_message(",0,", NULL));
    ASSERT_FALSE(parse_menu_message(NULL, &ms));
    ASSERT_FALSE(parse_menu_message(",0,", NULL));
    ASSERT_FALSE(parse_nav_message(NULL, &ns));
    ASSERT_FALSE(parse_nav_message("%0%", NULL));
}

/* Test: Real-world message from comments */
void test_real_world_message() {
    dashboard_state state = {0};
    /* From main.c comment: "S,079,12500,100,12.5,14:23,477,0,0,0,0,0,200,E" */
    const char* msg = ",079,12500,100,12.5,14,23,477,0,0,0,0,0,200,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,";

    ASSERT_TRUE(parse_live_message(msg, &state));

    ASSERT_EQ(state.current_speed, 79);
    ASSERT_EQ(state.rpm, 12500);
    ASSERT_EQ(state.coolant_temp, 100);
    ASSERT_FLOAT_NEAR(state.batt, 12.5);
    ASSERT_EQ(state.current_hour, 14);
    ASSERT_EQ(state.current_minute, 23);
    ASSERT_EQ(state.fuel_float, 477);
    ASSERT_FALSE(state.neutral);
    ASSERT_FALSE(state.oil_warning);
    ASSERT_FALSE(state.high_beam);
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
    TEST(parse_live_long_nav_string);
    TEST(null_safety);
    TEST(real_world_message);

    printf("\n=== All tests passed! ===\n");
    return 0;
}
