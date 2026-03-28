/*
 * warnings.c — Declarative warning system for TFT Dash
 *
 * Warning conditions are defined as a priority-ordered table.
 * The first condition that evaluates true is the active warning.
 */

#include "warnings.h"
#include <stddef.h>

/* --- Internal state --- */

static bool engine_running = false;
static bool comms_is_stale = false;
static bool cancelled = false;

/* --- Condition functions --- */

static bool check_comms(const dashboard_state *dash, const tpms_state *tpms) {
    (void)dash; (void)tpms;
    return comms_is_stale;
}

static bool check_tyre_pressure(const dashboard_state *dash, const tpms_state *tpms) {
    if (!tpms->connected) return false;
    bool front = tpms->front.received && tpms->front.psi <= dash->front_pressure_low;
    bool rear  = tpms->rear.received  && tpms->rear.psi  <= dash->rear_pressure_low;
    return front || rear;
}

static bool check_oil(const dashboard_state *dash, const tpms_state *tpms) {
    (void)tpms;
    return dash->oil_warning && engine_running;
}

static bool check_overheat(const dashboard_state *dash, const tpms_state *tpms) {
    (void)tpms;
    return dash->coolant_temp >= 120 && engine_running;
}

static bool check_fuel(const dashboard_state *dash, const tpms_state *tpms) {
    (void)tpms;
    return dash->fuel_float >= 481 && engine_running && dash->info_mode != 3;
}

/* --- Flash texture functions --- */

static const char *flash_nocomms(const dashboard_state *dash, const tpms_state *tpms) {
    (void)dash; (void)tpms;
    return "Nocomms.png";
}

static const char *flash_tyre(const dashboard_state *dash, const tpms_state *tpms) {
    bool front = tpms->connected && tpms->front.received && tpms->front.psi <= dash->front_pressure_low;
    bool rear  = tpms->connected && tpms->rear.received  && tpms->rear.psi  <= dash->rear_pressure_low;
    if (front && rear) return "Frontrearlow.png";
    return front ? "Fronttyrelow.png" : "Reartyrelow.png";
}

static const char *flash_oil(const dashboard_state *dash, const tpms_state *tpms) {
    (void)dash; (void)tpms;
    return "Lowoil.png";
}

static const char *flash_overheat(const dashboard_state *dash, const tpms_state *tpms) {
    (void)dash; (void)tpms;
    return "Engineoverheat.png";
}

static const char *flash_fuel(const dashboard_state *dash, const tpms_state *tpms) {
    (void)dash; (void)tpms;
    return "Lowfuel.png";
}

/* --- Warning table (priority order — first match wins) --- */

static const warning_def WARNING_TABLE[] = {
    { check_comms,         "Commsbadge.png",       flash_nocomms,   209, 236, 160, 105 },
    { check_tyre_pressure, "Lowtyrebadge.png",     flash_tyre,      168, 244, 257, 76 },
    { check_oil,           "Lowoilbadge.png",      flash_oil,       222, 236, 134, 105 },
    { check_overheat,      "Overheatbadge.png",    flash_overheat,  189, 237, 212, 95 },
    { check_fuel,          "Fuelwarningbadge.png",  flash_fuel,      222, 236, 134, 105 },
};

#define WARNING_COUNT (int)(sizeof(WARNING_TABLE) / sizeof(WARNING_TABLE[0]))

static const warning_def *current_warning = NULL;

/* --- Public API --- */

void warnings_update(const dashboard_state *dash, const tpms_state *tpms, int ms_since_update) {
    /* Engine running hysteresis */
    if (dash->rpm > 2000) engine_running = true;
    if (dash->rpm < 1000) engine_running = false;

    /* Comms staleness */
    comms_is_stale = (ms_since_update > 1000) || (ms_since_update == -1);

    /* Resolve highest-priority warning */
    current_warning = NULL;
    if (!cancelled) {
        for (int i = 0; i < WARNING_COUNT; i++) {
            if (WARNING_TABLE[i].condition(dash, tpms)) {
                current_warning = &WARNING_TABLE[i];
                break;
            }
        }
    }
}

const warning_def *warnings_active(void) {
    return current_warning;
}

void warnings_cancel(void) {
    cancelled = true;
    current_warning = NULL;
}

bool warnings_is_cancelled(void) {
    return cancelled;
}

bool warnings_engine_running(void) {
    return engine_running;
}
