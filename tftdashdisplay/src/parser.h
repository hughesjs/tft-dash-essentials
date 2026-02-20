/*
 * parser.h - Message parsing for TFT Dash
 *
 * Handles deserialisation of comma-delimited serial messages from the firmware.
 * Pure C implementation for testability.
 */

#ifndef PARSER_H
#define PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Live dashboard data from firmware */
typedef struct {
    int current_speed;
    int rpm;
    int coolant_temp;
    float batt;
    int current_hour;
    int current_minute;
    int fuel_float;
    bool neutral;
    bool oil_warning;
    bool high_beam;
    bool indicate_left;
    bool indicate_right;
    int choice_state;
    int info_mode;
    float trip1;
    float trip2;
    float odo;
    int using_km;
    float spd_correct;
    int theme;
    int ambient_temp;
    int current_gear;
    int mpg;
    int range;
    int max_speed;
    int trip_time_hour;
    int trip_time_min;
    bool oil_pressure_available;
    int oil_pressure_ohms;
    int oil_temp_ohms;
    int using_fh;
    int using_bar;
    int front_sensor_id;
    int rear_sensor_id;
    int front_pressure_low;
    int rear_pressure_low;
    char nav_string[255];
} dashboard_state;

/* Menu configuration data from firmware */
typedef struct {
    int choice_state;
    int odo_digit1, odo_digit2, odo_digit3, odo_digit4, odo_digit5, odo_digit6;
    int odo2_digit1, odo2_digit2, odo2_digit3, odo2_digit4, odo2_digit5, odo2_digit6;
    int odo_error;
    int set_time_digit0, set_time_digit1, set_time_digit2, set_time_digit3;
    int spc_digit0, spc_digit1, spc_digit2, spc_digit3;
    int front_sprocket;
    int rear_sprocket;
    int coolant_fan_temp;
    int using_km;
    int using_fh;
    int using_bar;
    int front_sensor_id;
    int rear_sensor_id;
    int front_pressure_low;
    int rear_pressure_low;
    int control_layout;
    int day_theme;
    int night_theme;
    int current_light_level;
    int light_switch_value;
    int fan_neutral_option;
    int gear_ratio_interval;
} menu_state;

/* Navigation data from phone via BLE */
typedef struct {
    char nav_symbol[16];
    char nav_road[255];
    char nav_towards[255];
    char nav_exit[16];
    char nav_distance[16];
    char nav_distance_units[16];
    int nav_metres;
    int nav_yards;
    double nav_miles;
    int nav_km;
    bool nav_active;
    double nav_dest_distance;
} nav_state;

/*
 * Parse comma-delimited live data message from firmware.
 * Format: {,speed,rpm,coolant,battery,hour,minute,fuel,...,}
 *
 * Returns: true on success, false on parse error
 */
bool parse_live_message(const char* msg, dashboard_state* state);

/*
 * Parse comma-delimited menu message from firmware.
 * Format: [,menustate,odo1,odo2,...,settings,...,]
 *
 * Returns: true on success, false on parse error
 */
bool parse_menu_message(const char* msg, menu_state* state);

/*
 * Parse percent-delimited navigation message from phone.
 * Format: %symbol%roadname%towards%exit%distance%units%
 *
 * Returns: true on success, false on parse error
 */
bool parse_nav_message(const char* msg, nav_state* state);

#ifdef __cplusplus
}
#endif

#endif /* PARSER_H */
