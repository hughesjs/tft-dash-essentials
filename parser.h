/*
 * parser.h - Message parsing for TFT Dash
 *
 * Handles deserialisation of comma-delimited serial messages from the firmware.
 * Pure C implementation for testability.
 */

#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>

/* Live dashboard data from firmware */
typedef struct {
    int currentSpeed;
    int rpm;
    int coolanttemp;
    float batt;
    int currenthour;
    int currentminute;
    int fuelfloat;
    bool neutral;
    bool oilwarning;
    bool highbeam;
    bool indicateleft;
    bool indicateright;
    int choicestate;
    int infomode;
    float trip1;
    float trip2;
    float odo;
    int usingkm;
    float spdcorrect;
    int theme;
    int ambientTemp;
    int currentgear;
    int mpg;
    int range;
    int maxspeed;
    int triptimehour;
    int triptimemin;
    bool oilpressureavailable;
    int oilpressureohms;
    int oiltempohms;
    int usingfh;
    int usingbar;
    int frontsensorid;
    int rearsensorid;
    int frontpressurelow;
    int rearpressurelow;
    char strNav[255];
} DashboardState;

/* Menu configuration data from firmware */
typedef struct {
    int choicestate;
    int ododigit1, ododigit2, ododigit3, ododigit4, ododigit5, ododigit6;
    int odo2digit1, odo2digit2, odo2digit3, odo2digit4, odo2digit5, odo2digit6;
    int odoerror;
    int settimedigit0, settimedigit1, settimedigit2, settimedigit3;
    int spcdigit0, spcdigit1, spcdigit2, spcdigit3;
    int frontsprocket;
    int rearsprocket;
    int coolantfantemp;
    int usingkm;
    int usingfh;
    int usingbar;
    int frontsensorid;
    int rearsensorid;
    int frontpressurelow;
    int rearpressurelow;
    int controllayout;
    int daytheme;
    int nighttheme;
    int currentlightlevel;
    int lightswitchvalue;
    int fanneutraloption;
    int gearratiointerval;
} MenuState;

/* Navigation data from phone via BLE */
typedef struct {
    char strNavSymbol[16];
    char strNavRoad[255];
    char strNavTowards[255];
    char strNavExit[16];
    char strNavDistance[16];
    char strNavDistanceUnits[16];
    int navMetres;
    int navYards;
    double navMiles;
    int navKM;
    bool navActive;
    double navDestdistance;
} NavState;

/*
 * Parse comma-delimited live data message from firmware.
 * Format: {,speed,rpm,coolant,battery,hour,minute,fuel,...,}
 *
 * Returns: true on success, false on parse error
 */
bool parse_live_message(const char* msg, DashboardState* state);

/*
 * Parse comma-delimited menu message from firmware.
 * Format: [,menustate,odo1,odo2,...,settings,...,]
 *
 * Returns: true on success, false on parse error
 */
bool parse_menu_message(const char* msg, MenuState* state);

/*
 * Parse percent-delimited navigation message from phone.
 * Format: %symbol%roadname%towards%exit%distance%units%
 *
 * Returns: true on success, false on parse error
 */
bool parse_nav_message(const char* msg, NavState* state);

#endif /* PARSER_H */
