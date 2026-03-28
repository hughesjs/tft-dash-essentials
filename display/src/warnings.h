/*
 * warnings.h — Declarative warning system for TFT Dash
 *
 * Evaluates sensor data against threshold conditions and resolves
 * which warning (if any) should be displayed, in priority order.
 * The renderer queries the active warning to decide what to draw.
 * No SDL dependency — pure logic, fully testable.
 */

#ifndef WARNINGS_H
#define WARNINGS_H

#include <stdbool.h>
#include "parser.h"
#include "tpms_feed.h"

typedef struct {
    bool (*condition)(const dashboard_state *dash, const tpms_state *tpms);
    const char *badge;
    const char *(*flash)(const dashboard_state *dash, const tpms_state *tpms);
    int flash_x, flash_y, flash_w, flash_h;
} warning_def;

/* Update internal state (engine running hysteresis, comms staleness).
 * Call once per frame before querying. */
void warnings_update(const dashboard_state *dash, const tpms_state *tpms, int ms_since_update);

/* Returns the highest-priority active warning, or NULL if none. */
const warning_def *warnings_active(void);

/* Cancel the current warning (e.g. when info mode changes during a warning). */
void warnings_cancel(void);

/* True if a warning was cancelled and hasn't been cleared yet. */
bool warnings_is_cancelled(void);

/* True if the engine is considered running (RPM hysteresis). */
bool warnings_engine_running(void);

#endif /* WARNINGS_H */
