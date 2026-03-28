/*
 * draw_dashboard.h — Dashboard rendering module for TFT Dash
 *
 * Owns the main dashboard drawing, startup animation sequence,
 * and all helper functions/state used exclusively by dashboard rendering.
 */

#ifndef DRAW_DASHBOARD_H
#define DRAW_DASHBOARD_H

#include <stdbool.h>

void draw_dashboard_init_rects(void);
void dashboard_startup(void);
void draw_dashboard(void);

/* Warning state — set from the main loop threshold checks */
extern bool fuelwarning;
extern bool overheatwarning;
extern bool enginerunning;
extern bool comms_stale;

/* Fuel bar count — needed by main loop for threshold checks */
int get_num_fuel_bars(int value);

/* Display format buffers — initialised in main(), written each frame in draw_dashboard */
extern char str_trip_time[16];
extern char str_time[16];

#endif /* DRAW_DASHBOARD_H */
