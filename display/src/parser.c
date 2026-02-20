/*
 * parser.c - Message parsing implementation
 *
 * Uses data-driven lookup tables to avoid if-ladders and redundant parsing.
 */

#define _GNU_SOURCE  /* for strsep */

#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

/* Field types for parsing */
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_SKIP
} FieldType;

/* Field descriptor - maps field number to struct location and type */
typedef struct {
    FieldType type;
    size_t offset;      /* offset into target struct */
    size_t max_len;     /* for strings only */
} FieldDescriptor;

/* Helper macro to define field descriptors */
#define FIELD_INT(struct_type, field) \
    { TYPE_INT, offsetof(struct_type, field), 0 }

#define FIELD_FLOAT(struct_type, field) \
    { TYPE_FLOAT, offsetof(struct_type, field), 0 }

#define FIELD_BOOL(struct_type, field) \
    { TYPE_BOOL, offsetof(struct_type, field), 0 }

#define FIELD_STRING(struct_type, field) \
    { TYPE_STRING, offsetof(struct_type, field), sizeof(((struct_type*)0)->field) }

#define FIELD_SKIP() \
    { TYPE_SKIP, 0, 0 }

/*
 * Live message field layout
 * Format: {,speed,rpm,coolant,battery,hour,minute,fuel,neutral,oil,...,}
 */
static const FieldDescriptor live_fields[] = {
    FIELD_SKIP(),                                    /* 0: message start marker */
    FIELD_INT(dashboard_state, current_speed),       /* 1 */
    FIELD_INT(dashboard_state, rpm),                 /* 2 */
    FIELD_INT(dashboard_state, coolant_temp),        /* 3 */
    FIELD_FLOAT(dashboard_state, batt),              /* 4 */
    FIELD_INT(dashboard_state, current_hour),        /* 5 */
    FIELD_INT(dashboard_state, current_minute),      /* 6 */
    FIELD_INT(dashboard_state, fuel_float),          /* 7 */
    FIELD_BOOL(dashboard_state, neutral),            /* 8 */
    FIELD_BOOL(dashboard_state, oil_warning),        /* 9 */
    FIELD_BOOL(dashboard_state, high_beam),          /* 10 */
    FIELD_BOOL(dashboard_state, indicate_left),      /* 11 */
    FIELD_BOOL(dashboard_state, indicate_right),     /* 12 */
    FIELD_INT(dashboard_state, choice_state),        /* 13 */
    FIELD_INT(dashboard_state, info_mode),           /* 14 */
    FIELD_FLOAT(dashboard_state, trip1),             /* 15 */
    FIELD_FLOAT(dashboard_state, trip2),             /* 16 */
    FIELD_FLOAT(dashboard_state, odo),               /* 17 */
    FIELD_INT(dashboard_state, using_km),            /* 18 */
    FIELD_FLOAT(dashboard_state, spd_correct),       /* 19 */
    FIELD_INT(dashboard_state, theme),               /* 20 */
    FIELD_INT(dashboard_state, ambient_temp),        /* 21 */
    FIELD_INT(dashboard_state, current_gear),        /* 22 */
    FIELD_INT(dashboard_state, mpg),                 /* 23 */
    FIELD_INT(dashboard_state, range),               /* 24 */
    FIELD_INT(dashboard_state, max_speed),           /* 25 */
    FIELD_INT(dashboard_state, trip_time_hour),      /* 26 */
    FIELD_INT(dashboard_state, trip_time_min),       /* 27 */
    FIELD_BOOL(dashboard_state, oil_pressure_available), /* 28 */
    FIELD_INT(dashboard_state, oil_pressure_ohms),   /* 29 */
    FIELD_INT(dashboard_state, oil_temp_ohms),       /* 30 */
    FIELD_INT(dashboard_state, using_fh),            /* 31 */
    FIELD_INT(dashboard_state, using_bar),           /* 32 */
    FIELD_INT(dashboard_state, front_sensor_id),     /* 33 */
    FIELD_INT(dashboard_state, rear_sensor_id),      /* 34 */
    FIELD_INT(dashboard_state, front_pressure_low),  /* 35 */
    FIELD_INT(dashboard_state, rear_pressure_low),   /* 36 */
    FIELD_STRING(dashboard_state, nav_string),       /* 37 */
};

/*
 * Menu message field layout
 * Format: [,menustate,odo1,odo2,...,settings,...,]
 */
static const FieldDescriptor menu_fields[] = {
    FIELD_SKIP(),                                 /* 0: message start marker */
    FIELD_INT(menu_state, choice_state),          /* 1 */
    FIELD_INT(menu_state, odo_digit1),            /* 2 */
    FIELD_INT(menu_state, odo_digit2),            /* 3 */
    FIELD_INT(menu_state, odo_digit3),            /* 4 */
    FIELD_INT(menu_state, odo_digit4),            /* 5 */
    FIELD_INT(menu_state, odo_digit5),            /* 6 */
    FIELD_INT(menu_state, odo_digit6),            /* 7 */
    FIELD_INT(menu_state, odo2_digit1),           /* 8 */
    FIELD_INT(menu_state, odo2_digit2),           /* 9 */
    FIELD_INT(menu_state, odo2_digit3),           /* 10 */
    FIELD_INT(menu_state, odo2_digit4),           /* 11 */
    FIELD_INT(menu_state, odo2_digit5),           /* 12 */
    FIELD_INT(menu_state, odo2_digit6),           /* 13 */
    FIELD_INT(menu_state, odo_error),             /* 14 */
    FIELD_INT(menu_state, set_time_digit0),       /* 15 */
    FIELD_INT(menu_state, set_time_digit1),       /* 16 */
    FIELD_INT(menu_state, set_time_digit2),       /* 17 */
    FIELD_INT(menu_state, set_time_digit3),       /* 18 */
    FIELD_INT(menu_state, spc_digit0),            /* 19 */
    FIELD_INT(menu_state, spc_digit1),            /* 20 */
    FIELD_INT(menu_state, spc_digit2),            /* 21 */
    FIELD_INT(menu_state, spc_digit3),            /* 22 */
    FIELD_INT(menu_state, front_sprocket),        /* 23 */
    FIELD_INT(menu_state, rear_sprocket),         /* 24 */
    FIELD_INT(menu_state, coolant_fan_temp),      /* 25 */
    FIELD_INT(menu_state, using_km),              /* 26 */
    FIELD_INT(menu_state, using_fh),              /* 27 */
    FIELD_INT(menu_state, using_bar),             /* 28 */
    FIELD_INT(menu_state, front_sensor_id),       /* 29 */
    FIELD_INT(menu_state, rear_sensor_id),        /* 30 */
    FIELD_INT(menu_state, front_pressure_low),    /* 31 */
    FIELD_INT(menu_state, rear_pressure_low),     /* 32 */
    FIELD_INT(menu_state, control_layout),        /* 33 */
    FIELD_INT(menu_state, day_theme),             /* 34 */
    FIELD_INT(menu_state, night_theme),           /* 35 */
    FIELD_INT(menu_state, current_light_level),   /* 36 */
    FIELD_INT(menu_state, light_switch_value),    /* 37 */
    FIELD_INT(menu_state, fan_neutral_option),    /* 38 */
    FIELD_INT(menu_state, gear_ratio_interval),   /* 39 */
};

/*
 * Nav message field layout
 * Format: %symbol%roadname%towards%exit%distance%units%
 */
static const FieldDescriptor nav_fields[] = {
    FIELD_SKIP(),                                  /* 0: message start marker */
    FIELD_STRING(nav_state, nav_symbol),           /* 1 */
    FIELD_STRING(nav_state, nav_road),             /* 2 */
    FIELD_STRING(nav_state, nav_towards),          /* 3 */
    FIELD_STRING(nav_state, nav_exit),             /* 4 */
    FIELD_STRING(nav_state, nav_distance),         /* 5 */
    FIELD_STRING(nav_state, nav_distance_units),   /* 6 */
};

/*
 * Generic field parser - parses one field based on descriptor
 */
static void parse_field(const char* value, void* base_ptr, const FieldDescriptor* desc) {
    char* target = (char*)base_ptr + desc->offset;

    switch (desc->type) {
        case TYPE_INT:
            *(int*)target = atoi(value);
            break;

        case TYPE_FLOAT:
            *(float*)target = atof(value);
            break;

        case TYPE_BOOL:
            *(bool*)target = (atoi(value) == 1);
            break;

        case TYPE_STRING:
            strncpy(target, value, desc->max_len - 1);
            ((char*)target)[desc->max_len - 1] = '\0';  /* ensure null termination */
            break;

        case TYPE_SKIP:
            /* Do nothing */
            break;
    }
}

/*
 * Generic message parser - splits on delimiter and applies field descriptors
 * Uses strsep which correctly handles empty fields (consecutive delimiters)
 */
static bool parse_delimited_message(
    const char* msg,
    char delimiter,
    void* state,
    const FieldDescriptor* fields,
    size_t num_fields)
{
    if (!msg || !state || !fields) {
        return false;
    }

    /* strsep modifies the string, so we need a copy */
    char* copy = strdup(msg);
    if (!copy) {
        return false;
    }

    char* ptr = copy;
    char* token;
    char delim_str[2] = { delimiter, '\0' };
    size_t field_num = 0;

    while ((token = strsep(&ptr, delim_str)) != NULL && field_num < num_fields) {
        parse_field(token, state, &fields[field_num]);
        field_num++;
    }

    free(copy);
    return true;
}

bool parse_live_message(const char* msg, dashboard_state* state) {
    return parse_delimited_message(
        msg,
        ',',
        state,
        live_fields,
        sizeof(live_fields) / sizeof(live_fields[0])
    );
}

bool parse_menu_message(const char* msg, menu_state* state) {
    return parse_delimited_message(
        msg,
        ',',
        state,
        menu_fields,
        sizeof(menu_fields) / sizeof(menu_fields[0])
    );
}

bool parse_nav_message(const char* msg, nav_state* state) {
    bool result = parse_delimited_message(
        msg,
        '%',
        state,
        nav_fields,
        sizeof(nav_fields) / sizeof(nav_fields[0])
    );

    if (!result) {
        return false;
    }

    /* Post-processing: convert distance based on units */
    if (strcmp(state->nav_distance_units, "MILE") == 0) {
        state->nav_miles = atof(state->nav_distance);
    } else if (strcmp(state->nav_distance_units, "YARD") == 0) {
        state->nav_yards = atoi(state->nav_distance);
    } else if (strcmp(state->nav_distance_units, "KM") == 0) {
        state->nav_km = atoi(state->nav_distance);
    } else if (strcmp(state->nav_distance_units, "METRE") == 0) {
        state->nav_metres = atoi(state->nav_distance);
    }

    /* Nav is active if we have a symbol */
    state->nav_active = (strlen(state->nav_symbol) > 0);

    return true;
}
