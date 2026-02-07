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
    FIELD_SKIP(),                               /* 0: message start marker */
    FIELD_INT(DashboardState, currentSpeed),    /* 1 */
    FIELD_INT(DashboardState, rpm),             /* 2 */
    FIELD_INT(DashboardState, coolanttemp),     /* 3 */
    FIELD_FLOAT(DashboardState, batt),          /* 4 */
    FIELD_INT(DashboardState, currenthour),     /* 5 */
    FIELD_INT(DashboardState, currentminute),   /* 6 */
    FIELD_INT(DashboardState, fuelfloat),       /* 7 */
    FIELD_BOOL(DashboardState, neutral),        /* 8 */
    FIELD_BOOL(DashboardState, oilwarning),     /* 9 */
    FIELD_BOOL(DashboardState, highbeam),       /* 10 */
    FIELD_BOOL(DashboardState, indicateleft),   /* 11 */
    FIELD_BOOL(DashboardState, indicateright),  /* 12 */
    FIELD_INT(DashboardState, choicestate),     /* 13 */
    FIELD_INT(DashboardState, infomode),        /* 14 */
    FIELD_FLOAT(DashboardState, trip1),         /* 15 */
    FIELD_FLOAT(DashboardState, trip2),         /* 16 */
    FIELD_FLOAT(DashboardState, odo),           /* 17 */
    FIELD_INT(DashboardState, usingkm),         /* 18 */
    FIELD_FLOAT(DashboardState, spdcorrect),    /* 19 */
    FIELD_INT(DashboardState, theme),           /* 20 */
    FIELD_INT(DashboardState, ambientTemp),     /* 21 */
    FIELD_INT(DashboardState, currentgear),     /* 22 */
    FIELD_INT(DashboardState, mpg),             /* 23 */
    FIELD_INT(DashboardState, range),           /* 24 */
    FIELD_INT(DashboardState, maxspeed),        /* 25 */
    FIELD_INT(DashboardState, triptimehour),    /* 26 */
    FIELD_INT(DashboardState, triptimemin),     /* 27 */
    FIELD_BOOL(DashboardState, oilpressureavailable), /* 28 */
    FIELD_INT(DashboardState, oilpressureohms), /* 29 */
    FIELD_INT(DashboardState, oiltempohms),     /* 30 */
    FIELD_INT(DashboardState, usingfh),         /* 31 */
    FIELD_INT(DashboardState, usingbar),        /* 32 */
    FIELD_INT(DashboardState, frontsensorid),   /* 33 */
    FIELD_INT(DashboardState, rearsensorid),    /* 34 */
    FIELD_INT(DashboardState, frontpressurelow), /* 35 */
    FIELD_INT(DashboardState, rearpressurelow), /* 36 */
    FIELD_STRING(DashboardState, strNav),       /* 37 */
};

/*
 * Menu message field layout
 * Format: [,menustate,odo1,odo2,...,settings,...,]
 */
static const FieldDescriptor menu_fields[] = {
    FIELD_SKIP(),                               /* 0: message start marker */
    FIELD_INT(MenuState, choicestate),          /* 1 */
    FIELD_INT(MenuState, ododigit1),            /* 2 */
    FIELD_INT(MenuState, ododigit2),            /* 3 */
    FIELD_INT(MenuState, ododigit3),            /* 4 */
    FIELD_INT(MenuState, ododigit4),            /* 5 */
    FIELD_INT(MenuState, ododigit5),            /* 6 */
    FIELD_INT(MenuState, ododigit6),            /* 7 */
    FIELD_INT(MenuState, odo2digit1),           /* 8 */
    FIELD_INT(MenuState, odo2digit2),           /* 9 */
    FIELD_INT(MenuState, odo2digit3),           /* 10 */
    FIELD_INT(MenuState, odo2digit4),           /* 11 */
    FIELD_INT(MenuState, odo2digit5),           /* 12 */
    FIELD_INT(MenuState, odo2digit6),           /* 13 */
    FIELD_INT(MenuState, odoerror),             /* 14 */
    FIELD_INT(MenuState, settimedigit0),        /* 15 */
    FIELD_INT(MenuState, settimedigit1),        /* 16 */
    FIELD_INT(MenuState, settimedigit2),        /* 17 */
    FIELD_INT(MenuState, settimedigit3),        /* 18 */
    FIELD_INT(MenuState, spcdigit0),            /* 19 */
    FIELD_INT(MenuState, spcdigit1),            /* 20 */
    FIELD_INT(MenuState, spcdigit2),            /* 21 */
    FIELD_INT(MenuState, spcdigit3),            /* 22 */
    FIELD_INT(MenuState, frontsprocket),        /* 23 */
    FIELD_INT(MenuState, rearsprocket),         /* 24 */
    FIELD_INT(MenuState, coolantfantemp),       /* 25 */
    FIELD_INT(MenuState, usingkm),              /* 26 */
    FIELD_INT(MenuState, usingfh),              /* 27 */
    FIELD_INT(MenuState, usingbar),             /* 28 */
    FIELD_INT(MenuState, frontsensorid),        /* 29 */
    FIELD_INT(MenuState, rearsensorid),         /* 30 */
    FIELD_INT(MenuState, frontpressurelow),     /* 31 */
    FIELD_INT(MenuState, rearpressurelow),      /* 32 */
    FIELD_INT(MenuState, controllayout),        /* 33 */
    FIELD_INT(MenuState, daytheme),             /* 34 */
    FIELD_INT(MenuState, nighttheme),           /* 35 */
    FIELD_INT(MenuState, currentlightlevel),    /* 36 */
    FIELD_INT(MenuState, lightswitchvalue),     /* 37 */
    FIELD_INT(MenuState, fanneutraloption),     /* 38 */
    FIELD_INT(MenuState, gearratiointerval),    /* 39 */
};

/*
 * Nav message field layout
 * Format: %symbol%roadname%towards%exit%distance%units%
 */
static const FieldDescriptor nav_fields[] = {
    FIELD_SKIP(),                               /* 0: message start marker */
    FIELD_STRING(NavState, strNavSymbol),       /* 1 */
    FIELD_STRING(NavState, strNavRoad),         /* 2 */
    FIELD_STRING(NavState, strNavTowards),      /* 3 */
    FIELD_STRING(NavState, strNavExit),         /* 4 */
    FIELD_STRING(NavState, strNavDistance),     /* 5 */
    FIELD_STRING(NavState, strNavDistanceUnits), /* 6 */
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

bool parse_live_message(const char* msg, DashboardState* state) {
    return parse_delimited_message(
        msg,
        ',',
        state,
        live_fields,
        sizeof(live_fields) / sizeof(live_fields[0])
    );
}

bool parse_menu_message(const char* msg, MenuState* state) {
    return parse_delimited_message(
        msg,
        ',',
        state,
        menu_fields,
        sizeof(menu_fields) / sizeof(menu_fields[0])
    );
}

bool parse_nav_message(const char* msg, NavState* state) {
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
    if (strcmp(state->strNavDistanceUnits, "MILE") == 0) {
        state->navMiles = atof(state->strNavDistance);
    } else if (strcmp(state->strNavDistanceUnits, "YARD") == 0) {
        state->navYards = atoi(state->strNavDistance);
    } else if (strcmp(state->strNavDistanceUnits, "KM") == 0) {
        state->navKM = atoi(state->strNavDistance);
    } else if (strcmp(state->strNavDistanceUnits, "METRE") == 0) {
        state->navMetres = atoi(state->strNavDistance);
    }

    /* Nav is active if we have a symbol */
    state->navActive = (strlen(state->strNavSymbol) > 0);

    return true;
}
