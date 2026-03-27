/*
 * tpms_feed.h - TPMS (Tyre Pressure Monitoring System) data feed
 *
 * Opaque module that owns the TPMS serial connection and background
 * polling thread.  Supports two hardware models (Standard and eBay).
 * Consumers get a const pointer to the current state.
 */

#ifndef TPMS_FEED_H
#define TPMS_FEED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* ── Hardware model ── */

typedef enum {
    TPMS_MODEL_STANDARD = 100,  /* 9600 bps, 0xAA 0xA1 frame header */
    TPMS_MODEL_EBAY     = 200,  /* 19200 bps, 0x55 0xAA frame header */
} tpms_model;

/* ── Sensor state flags ── */

typedef enum {
    TPMS_STATE_NORMAL        = 0, /* No issues */
    TPMS_STATE_FAST_LEAK     = 1, /* Rapid pressure drop detected */
    TPMS_STATE_HIGH_PRESSURE = 2, /* Over-inflated */
    TPMS_STATE_LOW_PRESSURE  = 3, /* Under-inflated */
    TPMS_STATE_HIGH_TEMP     = 4, /* Tyre overheating */
    TPMS_STATE_LOW_BATTERY   = 5, /* Sensor battery low */
} tpms_sensor_state;

/* ── Per-sensor reading ── */

typedef struct {
    double bar;
    double psi;
    int temp_celsius;
    tpms_sensor_state state;
    bool received;              /* true once at least one reading arrived */
} tpms_sensor;

/* ── Aggregate state — returned via const pointer ── */

typedef struct {
    tpms_sensor front;
    tpms_sensor rear;
    bool connected;
    bool signal_active;         /* true while recent data is arriving */
} tpms_state;

/* ── Opaque feed handle ── */

typedef struct tpms_feed tpms_feed;

/*
 * Create a TPMS feed.
 *   model           - hardware model (STANDARD or EBAY)
 *   front_sensor_id - physical sensor number for front tyre
 *   rear_sensor_id  - physical sensor number for rear tyre
 *   path            - explicit serial device path, or NULL to search defaults
 */
tpms_feed *tpms_feed_create(tpms_model model, int front_sensor_id, int rear_sensor_id, const char *path);
void tpms_feed_destroy(tpms_feed *feed);

void tpms_feed_start(tpms_feed *feed);
void tpms_feed_stop(tpms_feed *feed);

const tpms_state *tpms_feed_state(const tpms_feed *feed);

/* ── Frame decoding (exposed for unit testing) ── */

bool tpms_decode_standard_frame(const uint8_t *buf, int len,
                                int *sensor_id, double *bar, double *psi,
                                int *temp_celsius, tpms_sensor_state *state);

bool tpms_decode_ebay_frame(const uint8_t *buf, int len,
                            int *sensor_id, double *psi, double *bar,
                            int *temp_celsius);

#ifdef __cplusplus
}
#endif

#endif /* TPMS_FEED_H */
