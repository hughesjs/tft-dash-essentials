/*
 * sensor_feed.h — Bike sensor data feed for TFT Dash
 *
 * Manages the serial/pipe connection to the firmware, reads and frames
 * messages on a background thread, and exposes parsed state through
 * read-only pointers. The consumer (render loop) gets const access;
 * only the background thread writes.
 */

#ifndef SENSOR_FEED_H
#define SENSOR_FEED_H

#include <stdbool.h>
#include "parser.h"

/* Opaque handle — internals hidden from consumers */
typedef struct sensor_feed sensor_feed;

/*
 * Create a sensor feed.
 *   path = specific pipe/device path to connect to.
 *   NULL = use default search order: simulator pipe, macOS serial, Linux serial.
 */
sensor_feed *sensor_feed_create(const char *path);

/* Destroy and free all resources. Safe to call with NULL. */
void sensor_feed_destroy(sensor_feed *feed);

/* Start the background polling thread. Connects and begins reading. */
void sensor_feed_start(sensor_feed *feed);

/* Stop the background thread and close the connection. */
void sensor_feed_stop(sensor_feed *feed);

/* Read-only access to current state (updated continuously by background thread) */
const dashboard_state *sensor_feed_dashboard(const sensor_feed *feed);
const menu_state *sensor_feed_menu(const sensor_feed *feed);
const nav_state *sensor_feed_nav(const sensor_feed *feed);

/* True if currently connected to a data source */
bool sensor_feed_connected(const sensor_feed *feed);

#endif /* SENSOR_FEED_H */
