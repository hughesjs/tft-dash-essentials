/*
 * test_tpms_feed.c - TPMS feed module test suite
 */

#include "tpms_feed.h"
#include "test_helpers.h"

#define TEST(name) \
    printf("Running test: %s...", #name); \
    test_##name(); \
    printf(" PASSED\n");

/* ── Test: create and destroy without crash ── */

void test_create_destroy() {
    tpms_feed *feed = tpms_feed_create(TPMS_MODEL_STANDARD, 2, 4, NULL);
    ASSERT_TRUE(feed != NULL);
    tpms_feed_destroy(feed);

    feed = tpms_feed_create(TPMS_MODEL_EBAY, 1, 3, NULL);
    ASSERT_TRUE(feed != NULL);
    tpms_feed_destroy(feed);
}

/* ── Test: initial state is zeroed ── */

void test_initial_state() {
    tpms_feed *feed = tpms_feed_create(TPMS_MODEL_STANDARD, 2, 4, NULL);
    const tpms_state *st = tpms_feed_state(feed);
    ASSERT_TRUE(st != NULL);
    ASSERT_FALSE(st->connected);
    ASSERT_FALSE(st->signal_active);
    ASSERT_FALSE(st->front.received);
    ASSERT_FALSE(st->rear.received);
    ASSERT_FLOAT_NEAR(st->front.psi, 0.0);
    ASSERT_FLOAT_NEAR(st->rear.psi, 0.0);
    ASSERT_EQ(st->front.temp_celsius, 0);
    ASSERT_EQ(st->rear.temp_celsius, 0);
    ASSERT_EQ(st->front.state, TPMS_STATE_NORMAL);
    ASSERT_EQ(st->rear.state, TPMS_STATE_NORMAL);
    tpms_feed_destroy(feed);
}

/* ── Test: NULL safety ── */

void test_null_safety() {
    tpms_feed_destroy(NULL);
    tpms_feed_start(NULL);
    tpms_feed_stop(NULL);
    ASSERT_TRUE(tpms_feed_state(NULL) == NULL);

    /* Decode functions with NULL pointers */
    ASSERT_FALSE(tpms_decode_standard_frame(NULL, 14, NULL, NULL, NULL, NULL, NULL));
    ASSERT_FALSE(tpms_decode_ebay_frame(NULL, 7, NULL, NULL, NULL, NULL));
}

/* ── Test: Standard TPMS frame decoding ── */

void test_decode_standard_frame() {
    /*
     * Build a 14-byte Standard TPMS frame:
     *   [0]  0xAA  header byte 0
     *   [1]  0xA1  header byte 1
     *   [2-4] padding
     *   [5]  sensor_id = 2
     *   [6-8] padding
     *   [9]  pressure byte 1: high bits (& 3) = 1  → (1 * 256) = 256
     *   [10] pressure byte 2: 200                   → 256 + 200 = 456
     *        bar = 0.025 * 456 = 11.4
     *        psi = 11.4 * 14.5 = 165.3
     *   [11] temp: 70 + 50 = 120 raw → celsius = 120 - 50 = 70
     *   [12] state: 0 = NORMAL
     *   [13] padding
     */
    uint8_t frame[14] = {0};
    frame[0] = 0xAA;
    frame[1] = 0xA1;
    frame[5] = 2;       /* sensor ID */
    frame[9] = 0x01;    /* pressure high: (1 & 3) * 256 = 256 */
    frame[10] = 200;    /* pressure low: 200 */
    frame[11] = 120;    /* temp raw: 120 - 50 = 70°C */
    frame[12] = 0;      /* state: NORMAL */

    int sensor_id;
    double bar, psi;
    int temp;
    tpms_sensor_state state;

    ASSERT_TRUE(tpms_decode_standard_frame(frame, 14, &sensor_id, &bar, &psi, &temp, &state));
    ASSERT_EQ(sensor_id, 2);
    ASSERT_FLOAT_NEAR(bar, 11.4);
    ASSERT_FLOAT_NEAR(psi, 165.3);
    ASSERT_EQ(temp, 70);
    ASSERT_EQ(state, TPMS_STATE_NORMAL);
}

/* ── Test: Standard TPMS state flag decoding ── */

void test_decode_standard_states() {
    uint8_t frame[14] = {0};
    frame[0] = 0xAA;
    frame[1] = 0xA1;
    frame[5] = 1;

    int sid; double bar, psi; int temp; tpms_sensor_state state;

    /* FAST_LEAK: (state & 3) == 1 */
    frame[12] = 0x01;
    ASSERT_TRUE(tpms_decode_standard_frame(frame, 14, &sid, &bar, &psi, &temp, &state));
    ASSERT_EQ(state, TPMS_STATE_FAST_LEAK);

    /* HIGH_PRESSURE: (state & 16) == 16 */
    frame[12] = 0x10;
    ASSERT_TRUE(tpms_decode_standard_frame(frame, 14, &sid, &bar, &psi, &temp, &state));
    ASSERT_EQ(state, TPMS_STATE_HIGH_PRESSURE);

    /* LOW_PRESSURE: (state & 8) == 8 */
    frame[12] = 0x08;
    ASSERT_TRUE(tpms_decode_standard_frame(frame, 14, &sid, &bar, &psi, &temp, &state));
    ASSERT_EQ(state, TPMS_STATE_LOW_PRESSURE);

    /* HIGH_TEMP: (state & 4) == 4 */
    frame[12] = 0x04;
    ASSERT_TRUE(tpms_decode_standard_frame(frame, 14, &sid, &bar, &psi, &temp, &state));
    ASSERT_EQ(state, TPMS_STATE_HIGH_TEMP);

    /* LOW_BATTERY: (state & 0x80) == 0x80 */
    frame[12] = 0x80;
    ASSERT_TRUE(tpms_decode_standard_frame(frame, 14, &sid, &bar, &psi, &temp, &state));
    ASSERT_EQ(state, TPMS_STATE_LOW_BATTERY);
}

/* ── Test: eBay TPMS frame decoding ── */

void test_decode_ebay_frame() {
    /*
     * Build a 7-byte eBay TPMS frame:
     *   [0]  0x55  header byte 0
     *   [1]  0xAA  header byte 1
     *   [2]  padding
     *   [3]  sensor position: 0 → sensor_id 1
     *   [4]  pressure byte: 10 → psi = 10 * 3.44 = 34.4, bar = 34.4 / 14.5 = 2.372
     *   [5]  temp: 75 → celsius = 75 - 50 = 25
     *   [6]  state (unused by eBay)
     */
    uint8_t frame[7] = {0};
    frame[0] = 0x55;
    frame[1] = 0xAA;
    frame[3] = 0;     /* sensor position 0 → ID 1 */
    frame[4] = 10;    /* pressure */
    frame[5] = 75;    /* temp raw */

    int sensor_id;
    double psi, bar;
    int temp;

    ASSERT_TRUE(tpms_decode_ebay_frame(frame, 7, &sensor_id, &psi, &bar, &temp));
    ASSERT_EQ(sensor_id, 1);
    ASSERT_FLOAT_NEAR(psi, 34.4);
    ASSERT_FLOAT_NEAR(bar, 2.37);
    ASSERT_EQ(temp, 25);
}

/* ── Test: eBay sensor ID mapping ── */

void test_decode_ebay_sensor_mapping() {
    uint8_t frame[7] = { 0x55, 0xAA, 0, 0, 10, 75, 0 };
    int sensor_id; double psi, bar; int temp;

    frame[3] = 0;
    ASSERT_TRUE(tpms_decode_ebay_frame(frame, 7, &sensor_id, &psi, &bar, &temp));
    ASSERT_EQ(sensor_id, 1);

    frame[3] = 1;
    ASSERT_TRUE(tpms_decode_ebay_frame(frame, 7, &sensor_id, &psi, &bar, &temp));
    ASSERT_EQ(sensor_id, 2);

    frame[3] = 16;
    ASSERT_TRUE(tpms_decode_ebay_frame(frame, 7, &sensor_id, &psi, &bar, &temp));
    ASSERT_EQ(sensor_id, 3);

    frame[3] = 17;
    ASSERT_TRUE(tpms_decode_ebay_frame(frame, 7, &sensor_id, &psi, &bar, &temp));
    ASSERT_EQ(sensor_id, 4);

    /* Unknown position → fail */
    frame[3] = 5;
    ASSERT_FALSE(tpms_decode_ebay_frame(frame, 7, &sensor_id, &psi, &bar, &temp));
}

/* ── Test: short frames rejected ── */

void test_decode_short_frame() {
    uint8_t frame[4] = { 0xAA, 0xA1, 0, 0 };
    int sid; double bar, psi; int temp; tpms_sensor_state state;

    /* Standard needs 13+ bytes */
    ASSERT_FALSE(tpms_decode_standard_frame(frame, 4, &sid, &bar, &psi, &temp, &state));

    /* eBay needs 6+ bytes */
    uint8_t ebay[4] = { 0x55, 0xAA, 0, 0 };
    ASSERT_FALSE(tpms_decode_ebay_frame(ebay, 4, &sid, &psi, &bar, &temp));
}

/* ── Test: wrong header rejected ── */

void test_decode_wrong_header() {
    uint8_t frame[14] = {0};
    int sid; double bar, psi; int temp; tpms_sensor_state state;

    /* Wrong standard header */
    frame[0] = 0xBB; frame[1] = 0xA1;
    ASSERT_FALSE(tpms_decode_standard_frame(frame, 14, &sid, &bar, &psi, &temp, &state));

    /* Wrong eBay header */
    uint8_t ebay[7] = {0};
    ebay[0] = 0x55; ebay[1] = 0xBB;
    ASSERT_FALSE(tpms_decode_ebay_frame(ebay, 7, &sid, &psi, &bar, &temp));
}

int main(void) {
    printf("=== TPMS Feed Test Suite ===\n\n");

    TEST(create_destroy);
    TEST(initial_state);
    TEST(null_safety);
    TEST(decode_standard_frame);
    TEST(decode_standard_states);
    TEST(decode_ebay_frame);
    TEST(decode_ebay_sensor_mapping);
    TEST(decode_short_frame);
    TEST(decode_wrong_header);

    printf("\n=== All tests passed! ===\n");
    return 0;
}
