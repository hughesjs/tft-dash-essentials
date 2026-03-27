
#include "menu.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Test helpers — same pattern as test_parser.c */
#define TEST(name) \
    printf("Running test: %s...", #name); \
    test_##name(); \
    printf(" PASSED\n");

#define ASSERT_EQ(actual, expected) \
    assert((actual) == (expected))

#define ASSERT_STR_EQ(actual, expected) \
    assert(strcmp((actual), (expected)) == 0)

#define ASSERT_TRUE(condition) \
    assert(condition)

#define ASSERT_FALSE(condition) \
    assert(!(condition))

#define ASSERT_NULL(ptr) \
    assert((ptr) == NULL)

#define ASSERT_NOT_NULL(ptr) \
    assert((ptr) != NULL)

/* Test: screen routing — representative states from each range */
void test_screen_routing() {
    ASSERT_EQ(menu_screen_for_state(1),   MENU_SCREEN_MAIN);
    ASSERT_EQ(menu_screen_for_state(11),  MENU_SCREEN_MAIN);
    ASSERT_EQ(menu_screen_for_state(20),  MENU_SCREEN_SET_TIME);
    ASSERT_EQ(menu_screen_for_state(70),  MENU_SCREEN_SET_TIME);
    ASSERT_EQ(menu_screen_for_state(100), MENU_SCREEN_SPEED_CORRECTION);
    ASSERT_EQ(menu_screen_for_state(150), MENU_SCREEN_SPEED_CORRECTION);
    ASSERT_EQ(menu_screen_for_state(200), MENU_SCREEN_THEME);
    ASSERT_EQ(menu_screen_for_state(280), MENU_SCREEN_THEME);
    ASSERT_EQ(menu_screen_for_state(300), MENU_SCREEN_SET_ODOMETER);
    ASSERT_EQ(menu_screen_for_state(360), MENU_SCREEN_SET_ODOMETER);
    ASSERT_EQ(menu_screen_for_state(400), MENU_SCREEN_SPROCKET);
    ASSERT_EQ(menu_screen_for_state(470), MENU_SCREEN_SPROCKET);
    ASSERT_EQ(menu_screen_for_state(500), MENU_SCREEN_COOLANT_FAN);
    ASSERT_EQ(menu_screen_for_state(570), MENU_SCREEN_COOLANT_FAN);
    ASSERT_EQ(menu_screen_for_state(600), MENU_SCREEN_SET_UNITS);
    ASSERT_EQ(menu_screen_for_state(635), MENU_SCREEN_SET_UNITS);
    ASSERT_EQ(menu_screen_for_state(700), MENU_SCREEN_TPMS);
    ASSERT_EQ(menu_screen_for_state(745), MENU_SCREEN_TPMS);
    ASSERT_EQ(menu_screen_for_state(800), MENU_SCREEN_CONTROL);
    ASSERT_EQ(menu_screen_for_state(815), MENU_SCREEN_CONTROL);
    ASSERT_EQ(menu_screen_for_state(900), MENU_SCREEN_LIGHT);
    ASSERT_EQ(menu_screen_for_state(935), MENU_SCREEN_LIGHT);
}

/* Test: screen routing edge cases — states that should map to NONE */
void test_screen_routing_edges() {
    ASSERT_EQ(menu_screen_for_state(0),   MENU_SCREEN_NONE);
    ASSERT_EQ(menu_screen_for_state(-1),  MENU_SCREEN_NONE);
    ASSERT_EQ(menu_screen_for_state(19),  MENU_SCREEN_MAIN);  /* 19 is still in [1,20) */
    ASSERT_EQ(menu_screen_for_state(90),  MENU_SCREEN_NONE);  /* gap between set_time and speed_correction */
    ASSERT_EQ(menu_screen_for_state(99),  MENU_SCREEN_NONE);
    ASSERT_EQ(menu_screen_for_state(170), MENU_SCREEN_NONE);  /* just past speed correction */
    ASSERT_EQ(menu_screen_for_state(590), MENU_SCREEN_NONE);  /* just past coolant fan */
    ASSERT_EQ(menu_screen_for_state(999), MENU_SCREEN_NONE);
}

/* Test: background textures for each screen */
void test_screen_backgrounds() {
    const menu_bg *bg;

    ASSERT_NULL(menu_screen_background(MENU_SCREEN_NONE));

    bg = menu_screen_background(MENU_SCREEN_MAIN);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Menuoptionsex.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_SET_TIME);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Settime.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_SPEED_CORRECTION);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Speedcorrection.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_THEME);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Themeoptions.bmp");
    ASSERT_FALSE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_SET_ODOMETER);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Setodometer.bmp");
    ASSERT_FALSE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_SPROCKET);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Sprocketsetup.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_COOLANT_FAN);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Coolantfantemp.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_SET_UNITS);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Setunits.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_TPMS);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "TPMSsetup.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_CONTROL);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Controloptions.bmp");
    ASSERT_TRUE(bg->theme_specific);

    bg = menu_screen_background(MENU_SCREEN_LIGHT);
    ASSERT_NOT_NULL(bg);
    ASSERT_STR_EQ(bg->texture, "Lightoptions.bmp");
    ASSERT_TRUE(bg->theme_specific);
}

/* Test: cursor positions — spot-check states from different screens */
void test_cursor_positions() {
    menu_cursor c;

    /* Coolant fan */
    ASSERT_TRUE(menu_cursor_for_state(500, &c));
    ASSERT_STR_EQ(c.texture, "Uparrowsmall.bmp");
    ASSERT_EQ(c.x, 115); ASSERT_EQ(c.y, 185); ASSERT_EQ(c.w, 60); ASSERT_EQ(c.h, 62);

    ASSERT_TRUE(menu_cursor_for_state(530, &c));
    ASSERT_STR_EQ(c.texture, "Leftmenuarrowex.bmp");
    ASSERT_EQ(c.x, 667); ASSERT_EQ(c.y, 300);

    /* Sprocket */
    ASSERT_TRUE(menu_cursor_for_state(400, &c));
    ASSERT_EQ(c.x, 113); ASSERT_EQ(c.y, 296);

    /* Set odometer */
    ASSERT_TRUE(menu_cursor_for_state(300, &c));
    ASSERT_STR_EQ(c.texture, "Uparrow.bmp");
    ASSERT_EQ(c.x, 175); ASSERT_EQ(c.y, 279); ASSERT_EQ(c.w, 104); ASSERT_EQ(c.h, 107);

    ASSERT_TRUE(menu_cursor_for_state(360, &c));
    ASSERT_EQ(c.x, 877); ASSERT_EQ(c.y, 493);

    /* Speed correction */
    ASSERT_TRUE(menu_cursor_for_state(100, &c));
    ASSERT_EQ(c.x, 83); ASSERT_EQ(c.y, 352);

    ASSERT_TRUE(menu_cursor_for_state(150, &c));
    ASSERT_EQ(c.x, 824); ASSERT_EQ(c.y, 352);

    /* TPMS */
    ASSERT_TRUE(menu_cursor_for_state(700, &c));
    ASSERT_EQ(c.x, 65); ASSERT_EQ(c.y, 182);

    ASSERT_TRUE(menu_cursor_for_state(745, &c));
    ASSERT_EQ(c.x, 65); ASSERT_EQ(c.y, 528);

    /* Control */
    ASSERT_TRUE(menu_cursor_for_state(805, &c));
    ASSERT_EQ(c.x, 316); ASSERT_EQ(c.y, 394);

    /* Light */
    ASSERT_TRUE(menu_cursor_for_state(935, &c));
    ASSERT_EQ(c.x, 890); ASSERT_EQ(c.y, 529);

    /* Set units */
    ASSERT_TRUE(menu_cursor_for_state(600, &c));
    ASSERT_EQ(c.x, 34); ASSERT_EQ(c.y, 197);

    /* Set time */
    ASSERT_TRUE(menu_cursor_for_state(20, &c));
    ASSERT_EQ(c.x, 92); ASSERT_EQ(c.y, 355);

    ASSERT_TRUE(menu_cursor_for_state(70, &c));
    ASSERT_EQ(c.x, 824); ASSERT_EQ(c.y, 355);

    /* Unknown state returns false */
    ASSERT_FALSE(menu_cursor_for_state(999, &c));
    ASSERT_FALSE(menu_cursor_for_state(0, &c));
}

/* Test: main menu arrows — all 11 items */
void test_main_menu_arrows() {
    menu_cursor left, right;

    /* State 1: Reset trip 1 */
    ASSERT_TRUE(menu_main_item_arrows(1, &left, &right));
    ASSERT_STR_EQ(left.texture, "Leftmenuarrowex.bmp");
    ASSERT_STR_EQ(right.texture, "Rightmenuarrowex.bmp");
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 94);
    ASSERT_EQ(right.x, 465); ASSERT_EQ(right.y, 94);
    ASSERT_EQ(left.w, 56);   ASSERT_EQ(left.h, 50);
    ASSERT_EQ(right.w, 56);  ASSERT_EQ(right.h, 50);

    /* State 2: Reset trip 2 */
    ASSERT_TRUE(menu_main_item_arrows(2, &left, &right));
    ASSERT_EQ(left.x, 505);  ASSERT_EQ(left.y, 94);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 94);

    /* State 3: Control setup — yarrowoffset*1 = 51 → y=145 */
    ASSERT_TRUE(menu_main_item_arrows(3, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 145);
    ASSERT_EQ(right.x, 465); ASSERT_EQ(right.y, 145);

    /* State 4: Set units */
    ASSERT_TRUE(menu_main_item_arrows(4, &left, &right));
    ASSERT_EQ(left.x, 505);  ASSERT_EQ(left.y, 145);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 145);

    /* State 5: Set time — full width */
    ASSERT_TRUE(menu_main_item_arrows(5, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 196);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 196);

    /* State 6: Ambient light — yarrowoffset*3 → y=247 */
    ASSERT_TRUE(menu_main_item_arrows(6, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 247);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 247);

    /* State 7: Speed correction — yarrowoffset*4 → y=298 */
    ASSERT_TRUE(menu_main_item_arrows(7, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 298);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 298);

    /* State 8: Set theme — yarrowoffset*5 → y=349 */
    ASSERT_TRUE(menu_main_item_arrows(8, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 349);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 349);

    /* State 9: Gear indicator — yarrowoffset*6 → y=400 */
    ASSERT_TRUE(menu_main_item_arrows(9, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 400);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 400);

    /* State 10: Coolant fan — yarrowoffset*7 → y=451 */
    ASSERT_TRUE(menu_main_item_arrows(10, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 451);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 451);

    /* State 11: TPMS — yarrowoffset*8 → y=502 */
    ASSERT_TRUE(menu_main_item_arrows(11, &left, &right));
    ASSERT_EQ(left.x, 77);   ASSERT_EQ(left.y, 502);
    ASSERT_EQ(right.x, 897); ASSERT_EQ(right.y, 502);

    /* Out of range */
    ASSERT_FALSE(menu_main_item_arrows(0, &left, &right));
    ASSERT_FALSE(menu_main_item_arrows(12, &left, &right));
}

/* Test: theme thumbnails — all 9 IDs */
void test_theme_thumbnails() {
    const theme_thumb *t;

    t = menu_theme_thumb(0);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "default");
    ASSERT_STR_EQ(t->thumb_file, "whitethumb.bmp");

    t = menu_theme_thumb(1);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "green");
    ASSERT_STR_EQ(t->thumb_file, "greenthumb.bmp");

    t = menu_theme_thumb(2);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "red");
    ASSERT_STR_EQ(t->thumb_file, "redthumb.bmp");

    t = menu_theme_thumb(3);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "blue");
    ASSERT_STR_EQ(t->thumb_file, "bluethumb.bmp");

    t = menu_theme_thumb(4);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "orange");
    ASSERT_STR_EQ(t->thumb_file, "orangethumb.bmp");

    t = menu_theme_thumb(5);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "yellow");
    ASSERT_STR_EQ(t->thumb_file, "yellowthumb.bmp");

    t = menu_theme_thumb(6);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "night");
    ASSERT_STR_EQ(t->thumb_file, "nightthumb.bmp");

    t = menu_theme_thumb(7);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "bright");
    ASSERT_STR_EQ(t->thumb_file, "brightthumb.bmp");

    t = menu_theme_thumb(8);
    ASSERT_NOT_NULL(t);
    ASSERT_STR_EQ(t->theme_name, "dark");
    ASSERT_STR_EQ(t->thumb_file, "darkthumb.bmp");
}

/* Test: theme thumbnail bounds — invalid IDs return NULL */
void test_theme_thumb_bounds() {
    ASSERT_NULL(menu_theme_thumb(-1));
    ASSERT_NULL(menu_theme_thumb(9));
    ASSERT_NULL(menu_theme_thumb(100));
}

/* Test: NULL output pointers — should return false, not crash */
void test_null_safety() {
    menu_cursor c;

    ASSERT_FALSE(menu_cursor_for_state(500, NULL));
    ASSERT_FALSE(menu_main_item_arrows(1, NULL, &c));
    ASSERT_FALSE(menu_main_item_arrows(1, &c, NULL));
    ASSERT_FALSE(menu_main_item_arrows(1, NULL, NULL));
}

int main(void) {
    printf("=== TFT Dash Menu Test Suite ===\n\n");

    TEST(screen_routing);
    TEST(screen_routing_edges);
    TEST(screen_backgrounds);
    TEST(cursor_positions);
    TEST(main_menu_arrows);
    TEST(theme_thumbnails);
    TEST(theme_thumb_bounds);
    TEST(null_safety);

    printf("\n=== All tests passed! ===\n");
    return 0;
}
