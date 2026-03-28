/*
 * draw_dashboard.h — Dashboard rendering module for TFT Dash
 *
 * Owns the main dashboard drawing, startup animation sequence,
 * and all helper functions/state used exclusively by dashboard rendering.
 */

#ifndef DRAW_DASHBOARD_H
#define DRAW_DASHBOARD_H

void dashboard_startup(void);
void draw_dashboard(void);

/* Fuel bar count — used by the warnings module for threshold checks */
int get_num_fuel_bars(int value);

#endif /* DRAW_DASHBOARD_H */
