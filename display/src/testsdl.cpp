/*
NOTES:
15-07-2019:
	It seems the Renderer is the way to do things. Apparently uses hardware acceleration instead of software rendering
	Modified the code to load a bitmap, convert it to a texture and move it across the screen. Experimenting with drawing lines.
	Line drawing is only possible with the renderer.

23-09-2019
	Fixed an issue whereby a disconnect on the pi would not reconnect because num bytes comes back as 0 and not -1


Menu options:
	
	Reset Trip 1
	Reset Trip 2
	Toggle KM/H / MPH
	Set Speed Correction
	Set Time

	default
	blue
	red
	green
	yellow
	orange

Remaining tasks:
- Gui startup - DONE
- Odometer setting with error - DONE
- Flip between Info options & Diag using Select button - DONE
- Gear indicator & option to set sprocket sizes
- info panel displaying diagnostics - DONE
- warning badges working, low fuel, low oil, engine overheat - DONE
- Error warnings if comms not working

- Message format and deserialization - DONE


Theme contrast settings
Night - 232, 67
Blue - 178, 100 from Night
Green - 108, 95 from Night
Orange - 27, 95 from Night
Yellow - 72, 95 from Night
Red - 360, 96 from Night


*/

//FIXES:
//29/10/2019 - Fixed an issue with hazard lights not showing properly
//29/10/2019 - Introduced Far hazard left & right for additional indicators on far left & right of screen (Suggested by George)
//29/10/2019 - Added a degrees C to ambient temp readout
//28/02/2020 - Includes changes to include a fix for the odometer reading
//28/02/2020 - Poll interface no longer checks for an Ending E for the message, now just checks the length is within a good dash->range
//28/02/2020 - Changed Get Litres remaining to not show .. as much and account for a value greater than 5 volts.
//28/02/2020 - Changed the trip time to display 0:00 when under 10 minutes trip time as it looked wrong
//30/04/2020 - Custom changes made to read Oil Pressure and Oil Temperature for a customer with Oil gauges installed

/*
NAV TO DO
- Account for Accent characters - done - On iPhone
- Straighten up the fonts in the graphics - done
- Nav graphics in different themes - done
- Sort out compiler warnings - done
- Account for units in Km and Miles - done
- Account for Driving on Left or Right - done
- Turn off Navigation mode if no destination set - done
- Sort out left indicator icon overlapping Nav display - done
*/

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <termios.h>
#include <errno.h>
#include <unistd.h> // write(), read(), close()

// Parser module
#include "parser.h"

// Asset management
#include "assets.h"
#include "sensor_feed.h"
#include "menu.h"

// Read-only pointers to current state — refreshed each frame from sensor_feed
static const dashboard_state *dash = nullptr;
static const menu_state *menu = nullptr;
static const nav_state *nav = nullptr;

asset_store* g_assets = nullptr;
const char*  g_current_theme = "default";

static const char* THEME_NAMES[] = {
	"default", "green", "red", "blue", "orange", "yellow", "night", "bright", "dark"
};
#define THEME_COUNT 9

static const char* theme_name_from_id(int id) {
	if (id >= 0 && id < THEME_COUNT) return THEME_NAMES[id];
	return "default";
}

static inline SDL_Texture* tex(const char* name) {
	return asset_store_get_texture(g_assets, g_current_theme, name);
}

static inline SDL_Texture* tex_from(const char* theme, const char* name) {
	return asset_store_get_texture(g_assets, theme, name);
}

#define STANDARDTPMS 100
#define EBAYTPMS 200

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600

#define NORMAL			0
#define FASTLEAK 		1
#define HIGHPRESSURE 	2
#define LOWPRESSURE 	3
#define HIGHTEMP 		4
#define LOWBATTERY 		5

typedef enum {
	NAV_ICON_MUT = 0,    // U-turn (left-hand drive)
	NAV_ICON_EXITL,      // Motorway exit left
	NAV_ICON_EXITR,      // Motorway exit right
	NAV_ICON_TNR,        // Turn right
	NAV_ICON_TNL,        // Turn left
	NAV_ICON_SLL,        // Slight left
	NAV_ICON_SLR,        // Slight right
	NAV_ICON_RB2L,       // Roundabout 2nd exit (left-hand drive)
	NAV_ICON_RB3L,       // Roundabout 3rd exit (left-hand drive)
	NAV_ICON_RB4L,       // Roundabout 4th exit (left-hand drive)
	NAV_ICON_RB5L,       // Roundabout 5th exit (left-hand drive)
	NAV_ICON_RB1R,       // Roundabout 1st exit (right-hand drive)
	NAV_ICON_RB3R,       // Roundabout 3rd exit (right-hand drive)
	NAV_ICON_RB4R,       // Roundabout 4th exit (right-hand drive)
	NAV_ICON_RB5R,       // Roundabout 5th exit (right-hand drive)
	NAV_ICON_KPL,        // Keep left
	NAV_ICON_KPR,        // Keep right
	NAV_ICON_CON,        // Continue straight
	NAV_ICON_ARV,        // Arrive at destination
	NAV_ICON_RB1L,       // Roundabout 1st exit (left-hand drive)
	NAV_ICON_RB2R,       // Roundabout 2nd exit (right-hand drive)
	NAV_ICON_MUTR,       // U-turn (right-hand drive)
	NAV_ICON_MILE,       // Distance unit: miles
	NAV_ICON_YARD,       // Distance unit: yards
	NAV_ICON_KM,         // Distance unit: kilometres
	NAV_ICON_METRE,      // Distance unit: metres
	NAV_ICON_ONTO,       // "onto" label
	NAV_ICON_TWRDS,      // "towards" label
	NAV_ICON_COUNT,
	NAV_ICON_NONE = -1   // Sentinel for text-only entries (PRK, FRY)
} nav_icon;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Surface Rects
// Rev counter
SDL_FRect rect_g1213;
SDL_FRect rect_g11;
SDL_FRect rect_g10;
SDL_FRect rect_g9;
SDL_FRect rect_g8;
SDL_FRect rect_g7;
SDL_FRect rect_g6;
SDL_FRect rect_g5;
SDL_FRect rect_g4;
SDL_FRect rectg3;
SDL_FRect rectg2;
SDL_FRect rectg1;
SDL_FRect rectg0;
SDL_FRect grevline;
SDL_FRect grrevwhite;
SDL_FPoint gwhitepoint;

// Navigation icon sprite atlas — indexed by nav_icon enum
int g_nav_icons_src_tex_loc[4][NAV_ICON_COUNT] = {
	{21,   169,    317,   449,  628, 809, 1002,17,   163,  312,  460, 598, 771, 937, 1092,6,  174,333,491,642, 823, 983, 11,  106,215,280,  404, 456  }, // X pos
	{12,   11,     12,    20,   19,  22,  22,  184,  216,  217,  217, 214, 217, 216, 216, 383,385,384,394,387, 364, 385, 549, 550,550,549,  556, 557  }, // Y pos
	{112,  112,    112,   157,  147, 158, 176, 115,  134,  114,  110, 131, 131, 131, 131, 141,150,131,112,138, 116, 108, 81,  87,  42,105,  45,  76   }, // Width
	{150,  150,    150,   146,  150, 144, 146, 167,  134,  135,  135, 137, 134, 134, 134, 140,138,138,129,135, 167, 145, 24,  21,  22,22,   16,  14   }  // Height
};

// Navigation symbol lookup table — maps BLE symbol codes to icon(s) and render position
struct nav_symbol_entry {
	const char *code;
	nav_icon icon_lhd;     // Left-hand drive icon (or sole icon). NAV_ICON_NONE = text-only
	nav_icon icon_rhd;     // Right-hand drive icon. NAV_ICON_NONE = same as lhd
	int x, y;
};

static const nav_symbol_entry NAV_SYMBOLS[] = {
	{ "MUT", NAV_ICON_MUT,   NAV_ICON_MUTR,  249, 180 },
	{ "MEX", NAV_ICON_EXITL, NAV_ICON_EXITR,  249, 180 },
	{ "TNR", NAV_ICON_TNR,   NAV_ICON_NONE,   249, 180 },
	{ "TNL", NAV_ICON_TNL,   NAV_ICON_NONE,   249, 180 },
	{ "SLL", NAV_ICON_SLL,   NAV_ICON_NONE,   249, 180 },
	{ "SLR", NAV_ICON_SLR,   NAV_ICON_NONE,   249, 180 },
	{ "RB1", NAV_ICON_RB1L,  NAV_ICON_RB1R,   249, 180 },
	{ "RB2", NAV_ICON_RB2L,  NAV_ICON_RB2R,   255, 170 },
	{ "RB3", NAV_ICON_RB3L,  NAV_ICON_RB3R,   249, 180 },
	{ "RB4", NAV_ICON_RB4L,  NAV_ICON_RB4R,   249, 180 },
	{ "RB5", NAV_ICON_RB5L,  NAV_ICON_RB5R,   249, 180 },
	{ "LNR", NAV_ICON_KPR,   NAV_ICON_NONE,   249, 180 },
	{ "LNL", NAV_ICON_KPL,   NAV_ICON_NONE,   249, 180 },
	{ "CON", NAV_ICON_CON,   NAV_ICON_NONE,   249, 180 },
	{ "ARV", NAV_ICON_ARV,   NAV_ICON_NONE,   249, 180 },
	{ "PRK", NAV_ICON_NONE,  NAV_ICON_NONE,   0,   0   },
	{ "FRY", NAV_ICON_NONE,  NAV_ICON_NONE,   0,   0   },
};

static const nav_symbol_entry *lookup_nav_symbol(const char *code) {
	for (int i = 0; i < (int)(sizeof(NAV_SYMBOLS) / sizeof(NAV_SYMBOLS[0])); i++) {
		if (strcmp(code, NAV_SYMBOLS[i].code) == 0) return &NAV_SYMBOLS[i];
	}
	return nullptr;
}

char g_nav_large_upper_letter_ref[40] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9',',','.','(',')'};
char g_nav_large_lower_letter_ref[40] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9',',','.','(',')'};


int g_nav_letters_large_src_tex_loc[4][40] = {
// 	A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  COM, STP,  (,  ) 
   {15, 46, 75, 106,137,165,190,222,253,268,295,325,351,386,416,449,476,509,537,565,593,622,649,686,714,743,771,797,821,846,871,896,921,946,970,996,1022,1039,1055,1079 },
   {602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602, 602, 602, 602  },
   {22, 19, 20, 20, 17, 16, 22, 19, 5,  15, 20, 16, 24, 19, 22, 18, 22, 18, 19, 18, 19, 19, 29, 20, 21, 19, 16, 11, 16, 16, 16, 16, 16, 15, 17, 16, 5,   5,   9,   8,   },
   {22, 22, 23, 21, 21, 21, 23, 21, 21, 22, 21, 21, 21, 21, 23, 22, 24, 22, 23, 21, 22, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 21, 21, 21, 21, 22, 27,  22,  27,  27,  }
};

int g_nav_letters_small_src_tex_loc[4][40] = {
	
// 	A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  COM, STP,  (,  ) 
   {17, 42, 65, 90, 114,137,157,182,207,219,241,265,286,314,337,364,386,412,435,457,479,502,524,553,576,599,622,643,662,681,701,721,742,762,781,802,822, 836, 849, 861 },
   {778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778, 778, 778, 778 },
   {18, 15, 17, 16, 14, 13, 17,16,  4,  13, 16, 13, 19, 16,	18,	14, 18, 15, 15, 15, 16, 16, 24, 18, 17, 15, 13, 8,  13, 14, 14, 14, 13, 12,	14, 13,	5,	 5,	  7,   7   },
   {17, 17, 17, 17, 17, 17, 17,17,  17, 17, 17, 17, 17, 17,	17,	17, 19, 17, 18, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18,	18, 21,	 18,  22,  22  }

};

int g_nav_numbers_ref[11] = {'0','1','2','3','4','5','6','7','8','9','.'};
int g_nav_numbers_src_tex_loc[4][11] = {
	
//   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,     .
	{10, 115,209,310,409,509,611,711,809,910, 447},
	{660,661,660,661,661,661,660,660,660,660, 734},
	{68, 46, 69, 66, 68, 67, 66, 63, 68, 66,  17},
	{93, 93, 93, 93, 90, 91, 92, 91, 92, 92, 15 }
};


// Large speedometer numbers - texture mapping
int g_speed_src_tex_loc[4][10] = {
//    0    1    2    3    4    5    6    7    8     9		// The digit in the bitmap
	{ 0  , 143, 255, 396, 530, 669, 808, 939, 1080, 1211 }, // X position from Top Left
	{ 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1   , 1 },    // Y position from Top Left
	{ 135, 107, 142, 134, 139, 138, 131, 141, 131 , 137 },  // Width of digit
	{ 172, 172, 166, 172, 172, 172, 172, 172, 172 , 172 }   // Height of digit
};


// Medium sized numbers - clock & temp displays
int g_med_numbers_src_tex_loc[4][16] = {
//    0    1    2    3    4    5    6    7    8     9	  :   +   -     .   oC.   oF	// The digit in the bitmap
	{ 314, 2,  32,  68,  104, 139, 175, 209, 243, 279,   5,   32,  73, 102, 138, 183},	// X position
	{ 6,   6,   6,   6,  6,   6,   6,   6,   6,    6,    50,  52,  51, 47,  50,  50},  // Y position
	{ 26, 20,  29,  28,  28,  27,  25,  27, 26 ,  26,    16,  35,  34, 21,  40,  40},  // Width of digit
	{ 36, 39,  36,  36,  36,  36,  36,  36, 36 ,  35,    31,  31,  31, 39,  33,  33}   // Height of digit
};

// Small blue numbers - mileage and trip displays
int g_small_blue_src_tex_loc[4][11] = {
//    0    1    2    3    4    5    6    7    8     9     .	  // The digit in the bitmap
	{ 546, 355, 372, 394, 416, 438, 460, 483, 503, 525, 354 }, // X position
	{ 19,  19,  19,  19,  19,  19,  19,  19,  19,  19, 37 },  // Y position
	{ 18,  12,  18,  18,  18,  18,  17,  17,  18,  18, 11 },  // Width of digit
	{ 23,  23,  23,  22,  23,  23,  23,  22,  23,  23, 4 }   // Height of digit
};

// Small grey numbers - odometer
int g_small_grey_src_tex_loc[4][11] = {
	//    0    1    2    3    4    5    6    7    8     9    .	  // The digit in the bitmap
	{ 546, 355, 372, 394, 416, 438, 460, 483, 503, 525, 354 }, // X position
	{ 52,  52,  52,  52,  52,  52,  52,  52,  52,  52, 37 },  // Y position
	{ 18,  12,  18,  18,  18,  18,  17,  17,  18,  18, 11 },  // Width of digit
	{ 23,  23,  23,  22,  23,  23,  23,  22,  23,  23, 4 }   // Height of digit
};

int g_large_numbers_src_tex_loc[4][10] = { // NEW FOR LARGE GEAR INDICATOR
	// 1    2    3    4    5    6    7    8    9     0    	  // The digit in the bitmap
	{ 0,   66, 166, 264, 367, 468, 571, 667, 766, 865 }, // X position
	{ 91,  91,  91,  91,  91,  91,  91,  91,  91,  91 },  // Y position
	{ 63,  68,  68,  68,  68,  68,  68,  68,  68,  68 },  // Width of digit
	{ 94,  94,  94,  94,  94,  94,  94,  94,  94,  94 }   // Height of digit
};

// RPM lookup table - Converting RPM values into rotation value to reveal rev line
int g_rpm_lookup[53][2] = {
	{ 0,     6 },
	{ 250,   8 },
	{ 500,   10 },
	{ 750,   12 },
	{ 1000,  14 },
	{ 1250,  16 },
	{ 1500,  18 },
	{ 1750,  20 },
	{ 2000,  22 },
	{ 2250,  23 },
	{ 2500,  25 },
	{ 2750,  27 },
	{ 3000,  29 },
	{ 3250,  31 },
	{ 3500,  32 },
	{ 3750,  34 },
	{ 4000,  36 },
	{ 4250,  38 },
	{ 4500,  39 },
	{ 4750,  41 },
	{ 5000,  42 },
	{ 5250,  44 },
	{ 5500,  45 },
	{ 5750,  47 },
	{ 6000,  49 },
	{ 6250,  51 },
	{ 6500,  52 },
	{ 6750,  54 },
	{ 7000,  55 },
	{ 7250,  57 },
	{ 7500,  58 },
	{ 7750,  60 },
	{ 8000,  62 },
	{ 8250,  64 },
	{ 8500,  65 },
	{ 8750,  67 },
	{ 9000,  68 },
	{ 9250,  70 },
	{ 9500,  71 },
	{ 9750,  73 },
	{ 10000, 75 },
	{ 10250, 77 },
	{ 10500, 78 },
	{ 10750, 80 },
	{ 11000, 82 },
	{ 11250, 84 },
	{ 11500, 86 },
	{ 11750, 88 },
	{ 12000, 90 },
	{ 12250, 92 },
	{ 12500, 93 },
	{ 12750, 95 },
	{ 13000, 98 }
};

char g_num_ref[16] = { '0','1','2','3','4','5','6','7','8','9',':','+','-','.','c','f'};
char g_small_num_ref[11] = { '0','1','2','3','4','5','6','7','8','9','.'};
char g_large_num_ref[10] = { '1','2','3','4','5','6','7','8','9','0'};


// Destination rect for speedo numbers
SDL_FRect spd_digit_one;
SDL_FRect spd_digit_two;
SDL_FRect spd_digit_three;


// Bike specific values
int spin_angle = 0; // revline reveal
int test_counter = 0; // Number to use for test scenarios

int tpms_model = EBAYTPMS; // Standard version I recommend or EBAYTPMS

// Display-side derived state
double coolant_temp_f = 0;
int fuel_percent = 47;
double litres_remaining = 0;
double tempf = 0;


bool front_tyre_warning_triggered = false;
bool rear_tyre_warning_triggered = false;
bool front_sensor_read = false;
bool rear_sensor_read = false;

bool tpms_connected = false;
bool tpms_signal = false;
int tpms_signal_count = 0;

int tpms_serial_port;
pthread_t tpms_tid[2];

double g_sensor1_bar = 0.0;
double g_sensor2_bar = 0.0;
double g_sensor3_bar = 0.0;
double g_sensor4_bar = 0.0;

double g_sensor1_psi = 0.0;
double g_sensor2_psi = -99;
double g_sensor3_psi = 0.0;
double g_sensor4_psi = -99;

int g_sensor1_temp = 0;
int g_sensor2_temp = -99;
int g_sensor3_temp = 0;
int g_sensor4_temp = -99;
double g_sensor2_temp_f = 0;
double g_sensor4_temp_f = 0;

int g_sensor1_state = 0;
int g_sensor2_state = 0;
int g_sensor3_state = 0;
int g_sensor4_state = 0;

// TPMS display strings (formatted from sensor_feed state)
char str_front_sensor_id[16];
char str_rear_sensor_id[16];
char str_front_pressure_low[16];
char str_rear_pressure_low[16];


char str_sensor2_psi[16];
char str_sensor4_psi[16];

char str_sensor2_temp[16];
char str_sensor4_temp[16];

// Display format buffers (sprintf'd each frame from struct data)
char str_coolant_temp[16];
char str_current_speed[16];
char str_trip1[16];
char str_trip2[16];
char str_odo[16];
char str_time[16];
char str_mpg[16];
char str_rpm[16];
char str_fuel[16];
char str_range[16];
char str_max_speed[16];
char str_trip_time[16];
char str_ambient_temp[16];
char str_batt[16];
char str_spd_correct[16];
char str_oil_press[16];
char str_oil_temp[16];
char str_light_switch_value[16];
char str_current_light_level[16];
char str_nav_yards[16];
char str_nav_miles[16];
char str_nav_metres[16];
char str_nav_km[16];
char str_nav_dest_miles[16];
char str_nav_dest_km[16];


bool fuelwarning = false;
bool overheatwarning = false;
bool enginerunning = false;
bool warningbadgeactive = false;
bool warningbadgecancelled = false;

int currenttheme = 0; // Current theme we are on
bool quit = false;
bool reverse = false;
int ncount = 0;

// Animation values
int g_down_arrow_x  = 0;
int g_up_arrow_x  = 0;
int startup_anim_count = 0;
bool startup_done = false;
int flash_count = 0;
bool flash = false;
int target_rpm_rotation = 0;
int current_rpm_rotation = 0;

// Info mode animation
bool info_animation_in_progress = false;
int info_animation_count = 0;
bool infoanimationreverse = false;
int currentinfomode = 0;

// Display format strings (built each frame from struct data)
char str_set_time_digit0[16];
char str_set_time_digit1[16];
char str_set_time_digit2[16];
char str_set_time_digit3[16];
char str_spc_digit0[16];
char str_spc_digit1[16];
char str_spc_digit2[16];
char str_spc_digit3[16];

int barohms[11];
int tempnum[6];
int tempohms[6];

char sz_find_parking[] = "FIND PARKING";
char sz_take_ferry[] = "TAKE THE FERRY";
char sz_no_nav_data[] = "No Navigation Data";
char sz_smartphone_app[] = "launch smartphone app";
char sz_set_destination[] = "and set a destination";
char sz_arrive_in[] = "arv";
char sz_mls[] = "mls";
char sz_km[] = "km";

int cycle_digit (int value, int max) {
	int t = value;
	if (value < max) {
		//fprintf(stderr, "returning value: %d\n", value++);
		t++;
		return t;
	}

	if (value >= max) {
		//fprintf(stderr, "returning 0");
		return 0;
	}

	//fprintf(stderr, "returning default");
	return value;
}

// choice() and select() removed — menu state is now read-only via sensor_feed.
// On real hardware, the firmware handles button presses. On the simulator, the GUI does.

static bool file_exists(const char *path) {
	return access(path, F_OK) == 0;
}

int get_diff (int n1, int n2) {
	if (n1 >= n2) {
		return n1 - n2;
	}

	if (n2 > n1) {
		return n2 - n1;
	}
	return 0;
}

double get_precise_bar (int ohms) {
	int smallest_diff = 1000;
	int closest_bar = 0;
	int test_diff = 0;
	int wanted_diff = 0;
	int value_from_min_bar = 0;
	double tenths_bar = 0.0;

	if (ohms < barohms[0]) {
		return (double)0;
	}

	if (ohms > barohms[10]) {
		return (double)10;
	}

	for (int a=0;a<11;a++) {
		test_diff = get_diff (ohms, barohms[a]);

		//printf ("test_diff: %i\n", test_diff);
		if (test_diff < smallest_diff) {
			smallest_diff = test_diff;
			closest_bar = a;
		}
	}


	if (ohms > barohms[closest_bar]) {
		wanted_diff = barohms[closest_bar+1] - barohms[closest_bar];
		value_from_min_bar = ohms - barohms[closest_bar];
		tenths_bar = (double)value_from_min_bar / (double)wanted_diff;
		return (double)closest_bar + tenths_bar;
	}

	if (ohms < barohms[closest_bar]) {
		wanted_diff = barohms[closest_bar] - barohms[closest_bar-1];
		value_from_min_bar = ohms - barohms[closest_bar-1];
		tenths_bar = (double)value_from_min_bar / (double)wanted_diff;
		return (double)closest_bar-1 + tenths_bar;
	}

	if (ohms == barohms[closest_bar]) {
		return (double)closest_bar;
	}

	return 0.0;
}

double get_precise_temp (int ohms) {
	int test_diff = 0;
	int smallest_diff = 1000;
	int closesttemp = 0;
	int wanted_diff = 0;
	int numdiff = 0;
	int valuefrommintemp = 0;
	double tenthstemp = 0.0;

	if (ohms > tempohms[0]) {
		return (double)tempnum[0];
	}

	if (ohms < tempohms[5]) {
		return (double)tempnum[5];
	}

	for (int a=0;a<6;a++) {
		test_diff = get_diff (ohms, tempohms[a]);

		if (test_diff < smallest_diff) {
			smallest_diff = test_diff;
			closesttemp = a;
		}
	}

	//printf ("closesttemp: %i\n", closesttemp);

	if (ohms > tempohms[closesttemp]) {
		wanted_diff = tempohms[closesttemp-1] - tempohms[closesttemp];
		numdiff = tempnum[closesttemp] - tempnum[closesttemp-1];
		valuefrommintemp = ohms - tempohms[closesttemp];
		tenthstemp = (double)valuefrommintemp / (double)wanted_diff;

		//printf ("MNNumdiff: %i\n", numdiff);
		//printf ("MNAddition: %.2f\n", (tenthstemp * numdiff));

		return (double)tempnum[closesttemp] - (tenthstemp * numdiff);
	}

	if (ohms < tempohms[closesttemp]) {
		wanted_diff = tempohms[closesttemp] - tempohms[closesttemp+1];
		numdiff = tempnum[closesttemp+1] - tempnum[closesttemp];
		valuefrommintemp = ohms - tempohms[closesttemp+1];
		tenthstemp = (double)valuefrommintemp / (double)wanted_diff;

		//printf ("Numdiff: %i\n", numdiff);
		//printf ("Addition: %.2f\n", (tenthstemp * numdiff));

		return (double)tempnum[closesttemp+1] - (tenthstemp * numdiff);
	}

	if (ohms == tempohms[closesttemp]) {
		return (double)tempnum[closesttemp];
	}

	return 0.0;
}




void init_rects()
{
	// Rev Counter
	rect_g1213.x = 898; rect_g1213.y = 61; rect_g1213.w = 108; rect_g1213.h = 102;
	rect_g11.x = 846; rect_g11.y = 78; rect_g11.w = 48; rect_g11.h = 97;
	rect_g10.x = 796; rect_g10.y = 95; rect_g10.w = 45; rect_g10.h = 96;
	rect_g9.x = 739; rect_g9.y = 117; rect_g9.w = 52; rect_g9.h = 89;
	rect_g8.x = 688; rect_g8.y = 145; rect_g8.w = 58; rect_g8.h = 86;
	rect_g7.x = 641; rect_g7.y = 179; rect_g7.w = 64; rect_g7.h = 80;
	rect_g6.x = 601; rect_g6.y = 219; rect_g6.w = 72; rect_g6.h = 68;
	rect_g5.x = 564; rect_g5.y = 258; rect_g5.w = 79; rect_g5.h = 62;
	rect_g4.x = 533; rect_g4.y = 300; rect_g4.w = 82; rect_g4.h = 58;
	rectg3.x = 507; rectg3.y = 348; rectg3.w = 87; rectg3.h = 54;
	rectg2.x = 487; rectg2.y = 401; rectg2.w = 89; rectg2.h = 48;
	rectg1.x = 475; rectg1.y = 458; rectg1.w = 83; rectg1.h = 44;
	rectg0.x = 471; rectg0.y = 518; rectg0.w = 88; rectg0.h = 56;
	// Revline & White covering square
	grevline.x = 430; grevline.y = 23; grevline.w = 594; grevline.h = 577;
	grrevwhite.x = 382; grrevwhite.y = 0; grrevwhite.w = 642; grrevwhite.h = 623;
	// Rotation point for the white square - the bottom right corner
	gwhitepoint.x = 594; gwhitepoint.y = 577;

	// Set the positions of the speedometer numbers
	spd_digit_one.x = 614;
	spd_digit_one.y = 363;
	
	spd_digit_two.x = 735;
	spd_digit_two.y = 363;
	
	spd_digit_three.x = 875;
	spd_digit_three.y = 363;
}



int get_litres_remaining (int fuelintfloat) {

  // Depending on where the fuel float is it will have a different rate of change 
  // as the fuel decreases. These values are based on observations I made
  // Each 'Unit' per litre is how many units make up one litre for the current float reading

  if (fuelintfloat >= 0 && fuelintfloat <= 93) {
    litres_remaining = (21000 - (((double)(1000 / 93)) * ((double)fuelintfloat - 0))) / 1000;
    return 93;
  }

  if (fuelintfloat > 93 && fuelintfloat <= 121) {
    litres_remaining = (20000 - (((double)(1000 / 28)) * ((double)fuelintfloat - 93))) / 1000; // 8.47
    return 28;
  }
  
  if (fuelintfloat > 121 && fuelintfloat <= 148) {
    litres_remaining = (18000 - (((double)(1000 / 27)) * ((double)fuelintfloat - 121))) / 1000; // 30.30
    return 27;
  }

  if (fuelintfloat > 148 && fuelintfloat <= 180) {
    litres_remaining = (17000 - (((double)(1000 / 32)) * ((double)fuelintfloat - 148))) / 1000;
    return 32;
  }

  if (fuelintfloat > 180 && fuelintfloat <= 211) {
    litres_remaining = (15000 - (((double)(1000 / 31)) * ((double)fuelintfloat - 180))) / 1000;
    return 31;
  }

  if (fuelintfloat > 211 && fuelintfloat <= 241) {
    litres_remaining = (14000 - (((double)(1000 / 30)) * ((double)fuelintfloat - 211))) / 1000;
    return 30;
  }

  if (fuelintfloat > 241 && fuelintfloat <= 281) {
    litres_remaining = (12000 - (((double)(1000 / 40)) * ((double)fuelintfloat - 241))) / 1000;
    return 40;
  }

  if (fuelintfloat > 281 && fuelintfloat <= 320) {
    litres_remaining = (10000 - (((double)(1000 / 39)) * ((double)fuelintfloat - 281))) / 1000;
    return 39;
  }

  if (fuelintfloat > 320 && fuelintfloat <= 367) {
    litres_remaining = (9000 - (((double)(1000 / 47)) * ((double)fuelintfloat - 320))) / 1000;
    return 47;
  }

  if (fuelintfloat > 367 && fuelintfloat <= 426) {
    litres_remaining = (7000 - (((double)(1000 / 59)) * ((double)fuelintfloat - 367))) / 1000;
    return 59;
  }

  if (fuelintfloat > 426 && fuelintfloat <= 481) {
    litres_remaining = (6000 - (((double)(1000 / 55)) * ((double)fuelintfloat - 426))) / 1000;
    return 55;
  }

  if (fuelintfloat > 481 && fuelintfloat <= 506) {
    litres_remaining = (5000 - (((double)(1000 / 25)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  if (fuelintfloat > 506) {
    litres_remaining = (5000 - (((double)(1000 / 25)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  return 0;
}

int get_fuel_reveal_from_bars(int bars)
{
    return 17 * bars;
}

int get_num_fuel_bars(int value)
{
    if (value >= 481)
    {
        return 2; // Last 4 litres - fuel warning here
    }

    if (value >= 426 && value < 481)
    {
        return 3; // 5 litres
    }

    if (value >= 367 && value < 426)
    {
        return 5; // 6 litres            
    }

    if (value >= 320 && value < 367)
    {
        return 6; // 7-8 litres
    }

    if (value >= 281 && value < 320)
    {
        return 8; // 9-10 litres
    }

    if (value >= 241 && value < 281)
    {
        return 9; // 11 Litres - half tank
    }

    if (value >= 211 && value < 241)
    {
        return 11; // 12-13 litres
    }

    if (value >= 180 && value < 211)
    {
        return 12; // 14-15 litres
    }

    if (value >= 148 && value < 180)
    {
        return 13; //16 litres;
    }

    if (value >= 121 && value < 148)
    {
        return 14; //17-18 litres;
    }

    if (value >= 93 && value < 121)
    {
        return 15; // 19 litres
    }

    if (value >= 0 && value < 93)
    {
        return 16; // 20 litres
    }

    return 0;
}

int get_fuel_reveal(int percent) {
	if (percent >= 100) {
		return 273;
	}

	if (percent <= 0) {
		return 0;
	}
	
	return (273 * percent) / 100;
}

int	get_rpm_rotation(int rpm) {
	// Function to get the most appropriate rotation angle based on supplied
	// rpm. This will return the closest rotation value that matches the rpm in the lookup table

	if (rpm >= 13000) {
		return g_rpm_lookup[52][1];
	}

	if (rpm <= 0) {
		return g_rpm_lookup[0][1];
	}

	int lowest_diff = 30000;
	int closest = 0;

	for (int r = 0; r < 52; r ++ ) {		
		int lrpm = g_rpm_lookup[r][0];

		if (rpm >= lrpm) {
			if ((rpm - lrpm) < lowest_diff) {
				lowest_diff = (rpm - lrpm);
				closest = r;
			}
		}

		if (rpm < lrpm) {
			if ((lrpm - rpm) < lowest_diff) {
				lowest_diff = (lrpm - rpm);
				closest = r;
			}
		}
	}

	// by the time we get here we should have a lookup value is closest to what we want
	return g_rpm_lookup[closest][1];
}



// --- Glyph font descriptor — parameterises all the draw_*_string() variants ---

struct glyph_font {
	const char *texture_name;
	const int *src_x, *src_y, *src_w, *src_h;
	const char *glyph_chars;       // characters this font can render, in glyph-table order
	const char *upper_chars;       // uppercase equivalents for case-insensitive match (NULL = case-sensitive)
	int glyph_count;
	int spacing;                   // extra pixels between glyphs beyond glyph width
	int space_advance;             // pixels to advance for whitespace (0 = no special handling)
	bool dash_is_space;            // treat '-' as whitespace too
	int clip_x;                    // stop rendering past this x (0 = no clipping)
	char shifted_char;             // glyph that gets a y-offset (0 = none)
	int shifted_y;                 // y-offset for shifted_char
};

static const glyph_font FONT_SMALL_GREY = {
	"Smallnumbers.bmp", g_small_grey_src_tex_loc[0], g_small_grey_src_tex_loc[1], g_small_grey_src_tex_loc[2], g_small_grey_src_tex_loc[3],
	"0123456789.", NULL, 11, 0, 0, false, 0, '.', 18
};
static const glyph_font FONT_SMALL_BLUE = {
	"Smallnumbers.bmp", g_small_blue_src_tex_loc[0], g_small_blue_src_tex_loc[1], g_small_blue_src_tex_loc[2], g_small_blue_src_tex_loc[3],
	"0123456789.", NULL, 11, 0, 0, false, 0, '.', 18
};
static const glyph_font FONT_MEDIUM = {
	"Smallnumbers.bmp", g_med_numbers_src_tex_loc[0], g_med_numbers_src_tex_loc[1], g_med_numbers_src_tex_loc[2], g_med_numbers_src_tex_loc[3],
	"0123456789:+-.cf", NULL, 16, 0, 0, false, 0, 0, 0
};
static const glyph_font FONT_LARGE = {
	"Smallnumbers.bmp", g_large_numbers_src_tex_loc[0], g_large_numbers_src_tex_loc[1], g_large_numbers_src_tex_loc[2], g_large_numbers_src_tex_loc[3],
	"1234567890", NULL, 10, 0, 0, false, 0, 0, 0
};
static const glyph_font FONT_NAV_LARGE = {
	"Navgfx.bmp", g_nav_letters_large_src_tex_loc[0], g_nav_letters_large_src_tex_loc[1], g_nav_letters_large_src_tex_loc[2], g_nav_letters_large_src_tex_loc[3],
	"abcdefghijklmnopqrstuvwxyz0123456789,.()", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.()", 40, 2, 7, true, 412, 0, 0
};
static const glyph_font FONT_NAV_SMALL = {
	"Navgfx.bmp", g_nav_letters_small_src_tex_loc[0], g_nav_letters_small_src_tex_loc[1], g_nav_letters_small_src_tex_loc[2], g_nav_letters_small_src_tex_loc[3],
	"abcdefghijklmnopqrstuvwxyz0123456789,.()", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.()", 40, 2, 7, true, 412, 0, 0
};
static const glyph_font FONT_NAV_DIGITS = {
	"Navgfx.bmp", g_nav_numbers_src_tex_loc[0], g_nav_numbers_src_tex_loc[1], g_nav_numbers_src_tex_loc[2], g_nav_numbers_src_tex_loc[3],
	"0123456789.", NULL, 11, 2, 7, false, 0, '.', 71
};

void draw_glyph_string(const glyph_font *font, const char *text, int xpos, int ypos) {
	int x_offset = 0;
	SDL_Texture *texture = tex(font->texture_name);

	for (int s = 0; text[s]; s++) {
		char ch = text[s];

		if (font->space_advance > 0 && (ch == ' ' || (font->dash_is_space && ch == '-'))) {
			x_offset += font->space_advance;
			continue;
		}

		for (int d = 0; d < font->glyph_count; d++) {
			if (ch == font->glyph_chars[d] || (font->upper_chars && ch == font->upper_chars[d])) {
				if (font->clip_x > 0 && xpos + x_offset >= font->clip_x) break;

				SDL_FRect src = { (float)font->src_x[d], (float)font->src_y[d], (float)font->src_w[d], (float)font->src_h[d] };
				SDL_FRect dst = { (float)(xpos + x_offset), (float)ypos, src.w, src.h };
				if (font->shifted_char && ch == font->shifted_char) dst.y += font->shifted_y;

				SDL_RenderTexture(renderer, texture, &src, &dst);
				x_offset += (int)src.w + font->spacing;
				break;
			}
		}
	}
}

void draw_small_grey_string(const char *digits, int xpos, int ypos) { draw_glyph_string(&FONT_SMALL_GREY, digits, xpos, ypos); }
void draw_small_blue_string(const char *digits, int xpos, int ypos) { draw_glyph_string(&FONT_SMALL_BLUE, digits, xpos, ypos); }
void draw_medium_string(const char *digits, int xpos, int ypos)     { draw_glyph_string(&FONT_MEDIUM, digits, xpos, ypos); }
void draw_large_string(const char *digits, int xpos, int ypos)      { draw_glyph_string(&FONT_LARGE, digits, xpos, ypos); }
void draw_nav_large_string(const char *digits, int xpos, int ypos)  { draw_glyph_string(&FONT_NAV_LARGE, digits, xpos, ypos); }
void draw_nav_small_string(const char *digits, int xpos, int ypos)  { draw_glyph_string(&FONT_NAV_SMALL, digits, xpos, ypos); }
void draw_nav_digits(const char *digits, int xpos, int ypos)        { draw_glyph_string(&FONT_NAV_DIGITS, digits, xpos, ypos); }

void draw_medium_char(char digit, int xpos, int ypos) {
	char s[2] = { digit, 0 };
	draw_glyph_string(&FONT_MEDIUM, s, xpos, ypos);
}

void draw_nav_numbers (int num, int xpos, int ypos) {
	char sz_digit[5];
	memset (sz_digit, 0, 5);
	sprintf (sz_digit, "%d", num);

	draw_nav_digits (sz_digit, xpos, ypos);
}

void draw_nav_symbol (nav_icon sym, int xpos, int ypos)
{
	SDL_FRect g_src_rect;
	g_src_rect.x = g_nav_icons_src_tex_loc[0][sym];
	g_src_rect.y = g_nav_icons_src_tex_loc[1][sym];
	g_src_rect.w = g_nav_icons_src_tex_loc[2][sym];
	g_src_rect.h = g_nav_icons_src_tex_loc[3][sym];

	SDL_FRect g_dst_rect;
	g_dst_rect.x = xpos;
	g_dst_rect.y = ypos;
	g_dst_rect.w = g_src_rect.w;
	g_dst_rect.h = g_src_rect.h;

	SDL_RenderTexture(renderer, tex("Navgfx.bmp"), &g_src_rect, &g_dst_rect);
}

void draw_medium_num (int singledigit, int xpos, int ypos) {
	char sz_digit[4];
	memset (sz_digit, 0, 4);
	sprintf (sz_digit, "%d", singledigit);

	draw_medium_string (sz_digit, xpos, ypos);
}

void draw_large_num (int singledigit, int xpos, int ypos) {
	char sz_digit[4];
	memset (sz_digit, 0, 4);
	sprintf (sz_digit, "%d", singledigit);

	draw_large_string (sz_digit, xpos, ypos);
}

void draw_speed_digit (char digit, int position)
{

	for (int d = 0; d <= 9; d++) {
		if (digit == g_num_ref[d]) {
			SDL_FRect g_src_rect;
			g_src_rect.x = g_speed_src_tex_loc[0][d];
			g_src_rect.y = g_speed_src_tex_loc[1][d];
			g_src_rect.w = g_speed_src_tex_loc[2][d];
			g_src_rect.h = g_speed_src_tex_loc[3][d];

			if (position == 3) {
				spd_digit_three.w = g_speed_src_tex_loc[2][d];
				spd_digit_three.h = g_speed_src_tex_loc[3][d];
				SDL_RenderTexture(renderer, tex("Speednumbers.bmp"), &g_src_rect, &spd_digit_three);
			}

			if (position == 2) {
				spd_digit_two.w = g_speed_src_tex_loc[2][d];
				spd_digit_two.h = g_speed_src_tex_loc[3][d];
				SDL_RenderTexture(renderer, tex("Speednumbers.bmp"), &g_src_rect, &spd_digit_two);
			}

			if (position == 1) {
				spd_digit_one.w = g_speed_src_tex_loc[2][d];
				spd_digit_one.h = g_speed_src_tex_loc[3][d];
				SDL_RenderTexture(renderer, tex("Speednumbers.bmp"), &g_src_rect, &spd_digit_one);
			}
		}
	}	
}

void draw_speed(char *speed) {
	if (strlen(speed) == 1) {
		draw_speed_digit(speed[0], 3);
	}

	if (strlen(speed) == 2) {
		draw_speed_digit(speed[0], 2);
		draw_speed_digit(speed[1], 3);
	}

	if (strlen(speed) == 3) {
		draw_speed_digit(speed[0], 1);
		draw_speed_digit(speed[1], 2);
		draw_speed_digit(speed[2], 3);
	}
}

/*
void thread_worker(char* msg) {
	
	bool x = true;
	int i = 0;
	while (x == true) {
		SDL_Delay(1000);
		i++;
		fprintf(stderr, "Thread is running"); 
		sprintf(g_sz_comms_msg, "Thread count is: %d", i);
	}
}*/

int connect_tpms_interface ()
{
// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)

	if (file_exists("/dev/cu.usbserial-D3077502")) {
		tpms_serial_port = open("/dev/cu.usbserial-D3077502", O_RDWR); // Nano board connected!
		//cu.usbserial-D3077502
		fprintf(stderr, "TPMS interface connected!"); 
		tpms_connected= true;
	} else {
		if (file_exists("/dev/ttyUSB0")) {		
			fprintf(stderr, "TPMS interface connected!"); 
			tpms_serial_port = open("/dev/ttyUSB0", O_RDWR);
			tpms_connected= true;
		} else {

			if (file_exists("/dev/ttyUSB1")) {
				fprintf(stderr, "TPMS interface connected!"); 
				tpms_serial_port = open("/dev/ttyUSB1", O_RDWR);
				tpms_connected= true;
			} else {

				if (file_exists("/dev/cu.usbserial-210")) {
					fprintf(stderr, "Attempt 210 connect....");
					tpms_serial_port = open("/dev/cu.usbserial-210", O_RDWR);
					//tty.usbserial-1420
					fprintf(stderr, "TPMS interface connected!"); 
					tpms_connected= true;
				} else {
					if (!tpms_connected) // Only log on first failure
						fprintf(stderr, "TPMS interface NOT connected\n");
					tpms_connected= false;
					return 0;
				}
			}
		}		
	}

	// Create new termios struc, we call it 'tty' for convention
	struct termios tty;
	memset(&tty, 0, sizeof(tty));

	// Read in existing settings, and handle any error
	if(tcgetattr(tpms_serial_port, &tty) != 0) {
	    //printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
	}

	tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
	tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
	tty.c_cflag |= CS8; // 8 bits per byte (most common)
	tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON;
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	// tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
	// tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

	tty.c_cc[VTIME] = 10;    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	// Set in/out baud rate to be 115200
	if (tpms_model == STANDARDTPMS) {
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
	}

	if (tpms_model == EBAYTPMS) {
		cfsetispeed(&tty, B19200);
		cfsetospeed(&tty, B19200);
	}



	// Save tty settings, also checking for error
	if (tcsetattr(tpms_serial_port, TCSANOW, &tty) != 0) {
	    printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	return 0;
}

void* pollTPMSInterface2 (void *arg) // Poll the interface of the cheaper eBay TPMS Interface
{

	char appendbuf [256];
	
	char read_buf [256];
	bool quit = false;
	int appendpointer = 0;

	connect_tpms_interface();

	while (quit == false) {		
		memset(&read_buf, '\0', sizeof(read_buf));
		int num_bytes = read(tpms_serial_port, &read_buf, sizeof(read_buf));

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		if (num_bytes < 0) {
		    //printf("Error reading: %s\n", strerror(errno));
		    close (tpms_serial_port);
		    sleep(2);
		    connect_tpms_interface();
		} else {
			//fprintf(stderr, "Num bytes: %i\n", num_bytes);
		}


		for (int r=0;r<num_bytes;r++) {

			if (read_buf[r] == 85) {
				if (r < (num_bytes-1)) {
					if ((unsigned char)read_buf[r+1] == 0xAA) {
						//fprintf(stderr, "\n");
						
						appendpointer = 0;
						memset (appendbuf, 0, 256);
						appendbuf[appendpointer] = read_buf[r];
						appendpointer++;				
					}
				}
				
			} else {
				if (appendpointer < 256) {
					appendbuf[appendpointer] = read_buf[r];
					appendpointer++;
				}
			}

			//fprintf(stderr, "%i ", read_buf[r]);
		}



		if (appendbuf[0] == 85 && appendpointer > 5 && (unsigned char)appendbuf[1] == 0xAA) {
			

			int sensor_num = -1;
			//int sensor_num = appendbuf[3];
			// Sensor Number. 0 = Left Front, 1 = Right Front, 16 = Back Left, 17 = Back Right

			if (appendbuf[3] == 0) {sensor_num = 1;}
			if (appendbuf[3] == 1) {sensor_num = 2;}
			if (appendbuf[3] == 16) {sensor_num = 3;}
			if (appendbuf[3] == 17) {sensor_num = 4;}			

			int tp_byte1 = appendbuf[4];			
			int temp_byte = appendbuf[5];
			int state_byte = appendbuf[6];
			int sensor_state = 0;

			double psi = (double) (((tp_byte1 & 255) * 3.44));
			double bar = psi / 14.5;

			//double d = (double) (frame[4] & 255); tires_state.AirPressure = (int) (d * 3.44d);

			//double psi = bar * 14.5;
			
			int celsius = (temp_byte & 255) - 50;
			sensor_state = NORMAL;

			if (sensor_num == 1) {
				g_sensor1_bar = bar;
				g_sensor1_psi = psi;
				g_sensor1_temp = celsius;
				g_sensor1_state = sensor_state;
			}

			if (sensor_num == dash->front_sensor_id) {
				g_sensor2_bar = bar;
				g_sensor2_psi = psi;
				g_sensor2_temp = celsius;
				g_sensor2_state = sensor_state;
				tpms_signal = true;
				tpms_signal_count = 0;
				front_sensor_read = true;
			}

			if (sensor_num == 3) {
				g_sensor3_bar = bar;
				g_sensor3_psi = psi;
				g_sensor3_temp = celsius;
				g_sensor3_state = sensor_state;
			}

			if (sensor_num == dash->rear_sensor_id) {
				g_sensor4_bar = bar;
				g_sensor4_psi = psi;
				g_sensor4_temp = celsius;
				g_sensor4_state = sensor_state;
				tpms_signal = true;
				tpms_signal_count = 0;
				rear_sensor_read = true;
			}
			

			//fprintf (stderr, "\n_psi: S1: %f, S2: %f, S3: %f, S4: %f\n", g_sensor1_psi, g_sensor2_psi, g_sensor3_psi, g_sensor4_psi);
			//fprintf (stderr, "TMP: S1: %i, S2: %i, S3: %i, S4: %i\n", g_sensor1_temp, g_sensor2_temp, g_sensor3_temp, g_sensor4_temp);
			//fprintf (stderr, "STATE: S1: %s, S2: %s, S3: %s, S4: %s\n", string_sensor_state(g_sensor1_state), string_sensor_state(g_sensor2_state), string_sensor_state(g_sensor3_state), string_sensor_state(g_sensor4_state));
			
			appendpointer = 0;
			memset (appendbuf, 0, 256);
		}		
		
	}	

	close(tpms_serial_port);

	return 0;
}

void* pollTPMSInterface(void *arg)
{

	char appendbuf [256];
	
	char read_buf [256];
	bool quit = false;
	int appendpointer = 0;

	connect_tpms_interface();

	while (quit == false) {
		memset(&read_buf, '\0', sizeof(read_buf));
		int num_bytes = read(tpms_serial_port, &read_buf, sizeof(read_buf));

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		if (num_bytes < 0) {
		    printf("Error reading: %s\n", strerror(errno));
		    close (tpms_serial_port);
		    sleep(2);
		    connect_tpms_interface();
		} else {
			//fprintf(stderr, "Num bytes: %i\n", num_bytes);
		}


		for (int r=0;r<num_bytes;r++) {
			
			if ((unsigned char)read_buf[r] == 0xAA) {
				if (r < (num_bytes-1)) {
					if ((unsigned char)read_buf[r+1] == 0xA1) {
						//fprintf(stderr, "\n");
						
						appendpointer = 0;
						memset (appendbuf, 0, 256);
						appendbuf[appendpointer] = read_buf[r];
						appendpointer++;				
					}
				}
				
			} else {
				if (appendpointer < 256) {
					appendbuf[appendpointer] = read_buf[r];
					appendpointer++;
				}
			}

			//fprintf(stderr, "%i ", read_buf[r]);
		}



		if ((unsigned char)appendbuf[0] == 0xAA && appendpointer > 13 && (unsigned char)appendbuf[1] == 0xA1) {
			
			int sensor_num = appendbuf[5];
			int tp_byte1 = appendbuf[9];
			int tpbyte2 = appendbuf[10];
			int temp_byte = appendbuf[11];
			int state_byte = appendbuf[12];
			int sensor_state = 0;

			double bar = 0.025 * ((double) (((tp_byte1 & 3) * 256) + (tpbyte2 & 255)));
			double psi = bar * 14.5;
			int celsius = (temp_byte & 255) - 50;

			if ((state_byte & 3) == 1) {
				sensor_state = FASTLEAK;
			} else if ((state_byte & 16) == 16) {
				sensor_state = HIGHPRESSURE;
			} else if ((state_byte & 8) == 8) {
				sensor_state = LOWPRESSURE;
			} else if ((state_byte & 4) == 4) {
				sensor_state = HIGHTEMP;
			} else if ((state_byte & -128) == 128) {
				sensor_state = LOWBATTERY;
			} else {
				sensor_state = NORMAL;
			}


			if (sensor_num == 1) {
				g_sensor1_bar = bar;
				g_sensor1_psi = psi;
				g_sensor1_temp = celsius;
				g_sensor1_state = sensor_state;
			}

			if (sensor_num == dash->front_sensor_id) {
				g_sensor2_bar = bar;
				g_sensor2_psi = psi;
				g_sensor2_temp = celsius;
				g_sensor2_state = sensor_state;
				tpms_signal = true;
				tpms_signal_count = 0;
				front_sensor_read = true;
			}

			if (sensor_num == 3) {
				g_sensor3_bar = bar;
				g_sensor3_psi = psi;
				g_sensor3_temp = celsius;
				g_sensor3_state = sensor_state;
			}

			if (sensor_num == dash->rear_sensor_id) {
				g_sensor4_bar = bar;
				g_sensor4_psi = psi;
				g_sensor4_temp = celsius;
				g_sensor4_state = sensor_state;
				tpms_signal = true;
				tpms_signal_count = 0;
				rear_sensor_read = true;
			}
			

			//fprintf (stderr, "\n_psi: S1: %f, S2: %f, S3: %f, S4: %f\n", g_sensor1_psi, g_sensor2_psi, g_sensor3_psi, g_sensor4_psi);
			//fprintf (stderr, "TMP: S1: %i, S2: %i, S3: %i, S4: %i\n", g_sensor1_temp, g_sensor2_temp, g_sensor3_temp, g_sensor4_temp);
			//fprintf (stderr, "STATE: S1: %s, S2: %s, S3: %s, S4: %s\n", string_sensor_state(g_sensor1_state), string_sensor_state(g_sensor2_state), string_sensor_state(g_sensor3_state), string_sensor_state(g_sensor4_state));
			
			appendpointer = 0;
			memset (appendbuf, 0, 256);
		}		
		
	}
	

	close(tpms_serial_port);

	return 0;
}

bool render_texture (SDL_Texture* tex, int x, int y, int w, int h)
{
	SDL_FRect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	return SDL_RenderTexture(renderer, tex, nullptr, &dst_rect);
}

bool render_top_icon_grey_texture (int x, int y, int w, int h)
{
	SDL_FRect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	if (dash->oil_pressure_available) {
		return SDL_RenderTexture(renderer, tex("TopiconsgreyOP.bmp"), nullptr, &dst_rect);
	} else {
		return SDL_RenderTexture(renderer, tex("Topiconsgrey.bmp"), nullptr, &dst_rect);
	}
}

bool render_oil_light_texture (int x, int y, int w, int h)
{
	SDL_FRect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	if (dash->oil_pressure_available) {
		return SDL_RenderTexture(renderer, tex("OillightOP.bmp"), nullptr, &dst_rect);
	} else {
		return SDL_RenderTexture(renderer, tex("Oillight.bmp"), nullptr, &dst_rect);
	}
}

void draw_menu (int state) {
	if (dash->theme == 0 || dash->theme == 7) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	}
	SDL_RenderClear(renderer);

	menu_screen screen = menu_screen_for_state(state);

	// Background texture
	const menu_bg *bg = menu_screen_background(screen);
	if (bg && bg->texture) {
		if (bg->theme_specific) render_texture(tex(bg->texture), 0, 0, 1024, 600);
		else                    render_texture(tex_from("default", bg->texture), 0, 0, 1024, 600);
	}

	// Selection cursor (sub-menu arrow)
	menu_cursor cur;
	if (menu_cursor_for_state(state, &cur)) {
		render_texture(tex(cur.texture), cur.x, cur.y, cur.w, cur.h);
	}

	// Main menu left+right bracket arrows
	menu_cursor left, right;
	if (menu_main_item_arrows(state, &left, &right)) {
		render_texture(tex(left.texture), left.x, left.y, left.w, left.h);
		render_texture(tex(right.texture), right.x, right.y, right.w, right.h);
	}

	// Theme menu left+right arrows (default theme only)
	if (menu_theme_arrows(state, &left, &right)) {
		render_texture(tex_from("default", left.texture), left.x, left.y, left.w, left.h);
		render_texture(tex_from("default", right.texture), right.x, right.y, right.w, right.h);
	}

	// Screen-specific dynamic content
	switch (screen) {
	case MENU_SCREEN_COOLANT_FAN: {
		draw_medium_num(menu->coolant_fan_temp, 467, 114);
		static const int fan_option_y[] = { 306, 383, 460, 537 };
		if (menu->fan_neutral_option >= 0 && menu->fan_neutral_option <= 3)
			render_texture(tex("Selecton.bmp"), 741, fan_option_y[menu->fan_neutral_option], 55, 38);
		break;
	}
	case MENU_SCREEN_SPROCKET:
		draw_medium_num(menu->front_sprocket, 332, 127);
		draw_medium_num(menu->rear_sprocket, 638, 127);
		draw_medium_num(menu->gear_ratio_interval, 715, 415);
		break;

	case MENU_SCREEN_SET_ODOMETER: {
		int sp = 111;
		draw_medium_num(menu->odo_digit1, 214, 209);
		draw_medium_num(menu->odo_digit2, 325, 209);
		draw_medium_num(menu->odo_digit3, 325 + sp * 1, 209);
		draw_medium_num(menu->odo_digit4, 325 + sp * 2, 209);
		draw_medium_num(menu->odo_digit5, 325 + sp * 3, 209);
		draw_medium_num(menu->odo_digit6, 325 + sp * 4, 209);
		draw_medium_num(menu->odo2_digit1, 214, 415);
		draw_medium_num(menu->odo2_digit2, 325, 415);
		draw_medium_num(menu->odo2_digit3, 325 + sp * 1, 415);
		draw_medium_num(menu->odo2_digit4, 325 + sp * 2, 415);
		draw_medium_num(menu->odo2_digit5, 325 + sp * 3, 415);
		draw_medium_num(menu->odo2_digit6, 325 + sp * 4, 415);
		if (menu->odo_error == 1) render_texture(tex_from("default", "Odoerror1.bmp"), 130, 524, 758, 45);
		if (menu->odo_error == 2) render_texture(tex_from("default", "Odoerror2.bmp"), 130, 524, 758, 45);
		break;
	}
	case MENU_SCREEN_SPEED_CORRECTION: {
		strcpy(str_spc_digit0, menu->spc_digit0 == 0 ? "-" : "+");
		sprintf(str_spc_digit1, "%d", menu->spc_digit1);
		sprintf(str_spc_digit2, "%d", menu->spc_digit2);
		sprintf(str_spc_digit3, "%d", menu->spc_digit3);
		draw_medium_string(str_spc_digit0, 294, 282);
		draw_medium_string(str_spc_digit1, 408, 277);
		draw_medium_string(str_spc_digit2, 516, 277);
		draw_medium_string(str_spc_digit3, 693, 277);
		break;
	}
	case MENU_SCREEN_TPMS: {
		sprintf(str_front_sensor_id, "%d", dash->front_sensor_id);
		sprintf(str_rear_sensor_id, "%d", dash->rear_sensor_id);
		sprintf(str_front_pressure_low, "%d", dash->front_pressure_low);
		sprintf(str_rear_pressure_low, "%d", dash->rear_pressure_low);
		draw_medium_string(str_front_sensor_id, 527, 170);
		draw_medium_string(str_rear_sensor_id, 832, 170);
		draw_medium_string(str_front_pressure_low, 830, 288);
		draw_medium_string(str_rear_pressure_low, 830, 405);
		break;
	}
	case MENU_SCREEN_CONTROL:
		render_texture(tex("Selectedcontrol.bmp"), menu->control_layout == 0 ? 230 : 555, 195, 236, 30);
		break;

	case MENU_SCREEN_LIGHT: {
		sprintf(str_light_switch_value, "%d", menu->light_switch_value);
		sprintf(str_current_light_level, "%d", menu->current_light_level);
		draw_medium_string(str_light_switch_value, 625, 477);
		draw_medium_string(str_current_light_level, 625, 396);
		const theme_thumb *day = menu_theme_thumb(menu->day_theme);
		if (day) render_texture(tex_from(day->theme_name, day->thumb_file), 772, 118, 126, 74);
		const theme_thumb *night = menu_theme_thumb(menu->night_theme);
		if (night) render_texture(tex_from(night->theme_name, night->thumb_file), 772, 236, 126, 74);
		break;
	}
	case MENU_SCREEN_SET_UNITS:
		render_texture(tex("Selecton.bmp"), dash->using_km  ? 894 : 784, 101, 55, 38);
		render_texture(tex("Selecton.bmp"), dash->using_fh  ? 894 : 784, 262, 55, 38);
		render_texture(tex("Selecton.bmp"), dash->using_bar ? 894 : 784, 423, 55, 38);
		break;

	case MENU_SCREEN_SET_TIME: {
		sprintf(str_set_time_digit0, "%d", menu->set_time_digit0);
		sprintf(str_set_time_digit1, "%d", menu->set_time_digit1);
		sprintf(str_set_time_digit2, "%d", menu->set_time_digit2);
		sprintf(str_set_time_digit3, "%d", menu->set_time_digit3);
		draw_medium_string(str_set_time_digit0, 300, 277);
		draw_medium_string(str_set_time_digit1, 408, 277);
		draw_medium_string(str_set_time_digit2, 583, 277);
		draw_medium_string(str_set_time_digit3, 693, 277);
		break;
	}
	case MENU_SCREEN_MAIN:
	case MENU_SCREEN_THEME:
	case MENU_SCREEN_NONE:
		break;
	}

	SDL_RenderPresent(renderer);
}

void dashboard_startup () {
	startup_anim_count++;


	if (dash->theme == 0 || dash->theme == 7) {

		if (startup_anim_count > 0 && startup_anim_count < 255) {
			startup_anim_count+=3;
			SDL_SetRenderDrawColor(renderer, startup_anim_count, startup_anim_count, startup_anim_count, SDL_ALPHA_OPAQUE);
		}

		if (startup_anim_count >= 255) {
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);	
		}
	} else {
		if (startup_anim_count < 255) {
			startup_anim_count = 255;
		}
	}

	SDL_RenderClear(renderer);

	if (startup_anim_count > 255) {
		startup_anim_count+=8;

		if (startup_anim_count < 1000) {
			render_top_icon_grey_texture ((0-1000)+startup_anim_count, 0, 627, 138);		
			render_texture (tex("Topiconedge1.bmp"), (625-1000)+startup_anim_count, 0, 98, 75);
			render_texture (tex("Topiconedge2.bmp"), (721-1000)+startup_anim_count, 0, 84, 24);	
			
			// Trip 1 and 2 and Odometer, Ambient Temp and Time section
			render_texture (tex("Mileinfo.bmp"), (0-1000)+startup_anim_count, 434, 435, 169);

			if (dash->info_mode == 0) {
				if (dash->using_km) {
					render_texture (tex("InfotopKM.bmp"), (0-1000)+startup_anim_count, 176, 510, 97);
				} else {
					render_texture (tex("Infotop.bmp"), (0-1000)+startup_anim_count, 176, 510, 97);	
				}
				
				render_texture (tex("Infobottom.bmp"), (0-1000)+startup_anim_count, 273, 454, 125);
			}
		}

		if (startup_anim_count >= 1000) {
			render_top_icon_grey_texture (0, 0, 627, 138);		
			render_texture (tex("Topiconedge1.bmp"), 625, 0, 98, 75);
			render_texture (tex("Topiconedge2.bmp"), 721, 0, 84, 24);	

			// Trip 1 and 2 and Odometer, Ambient Temp and Time section
			render_texture (tex("Mileinfo.bmp"), 0, 434, 435, 169);

			if (dash->info_mode == 0) {
				if (dash->using_km) {
					render_texture (tex("InfotopKM.bmp"), 0, 176, 510, 97);
				} else {
					render_texture (tex("Infotop.bmp"), 0, 176, 510, 97);	
				}
				
				render_texture (tex("Infobottom.bmp"), 0, 273, 454, 125);
			}
		}

		int revamountinc = 10;
		int revinc = 100;
		if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R0.bmp"), nullptr, &rectg0);
		}
		revamountinc+=revinc;
		if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R1.bmp"), nullptr, &rectg1);
		}
		revamountinc+=revinc;
		if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R2.bmp"), nullptr, &rectg2);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R3.bmp"), nullptr, &rectg3);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R4.bmp"), nullptr, &rect_g4);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R5.bmp"), nullptr, &rect_g5);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R6.bmp"), nullptr, &rect_g6);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R7.bmp"), nullptr, &rect_g7);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R8.bmp"), nullptr, &rect_g8);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R9.bmp"), nullptr, &rect_g9);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R10.bmp"), nullptr, &rect_g10);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R11.bmp"), nullptr, &rect_g11);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderTexture(renderer, tex("R12-13.bmp"), nullptr, &rect_g1213);
		}
	}

	if (startup_anim_count > 500) {
		
		if (startup_anim_count < 1000) {
			// Fuel Gauge		
			render_texture (tex("Fuelgauge.bmp"), (676+500)-(startup_anim_count-500), 294, 316, 44);
			render_texture (tex("Fuelgaugewhite.bmp"), (714+(spin_angle*3)+500)-(startup_anim_count-500), 303, 274, 28);

			// Coolant Temp
			if (dash->using_fh) {
				render_texture (tex("CoolantF.bmp"), (808+500)-(startup_anim_count-500), 196, 209, 80);
			} else {
				render_texture (tex("Coolant.bmp"), (808+500)-(startup_anim_count-500), 196, 209, 80);	
			}
			
			render_texture (tex("Coolanticon.bmp"), (772+500)-(startup_anim_count-500), 221, 41, 39);
		} else {
						// Fuel Gauge
			render_texture (tex("Fuelgauge.bmp"), 676, 294, 316, 44);
			render_texture (tex("Fuelgaugewhite.bmp"), 714+(spin_angle*3), 303, 274, 28);

			// Coolant Temp
			if (dash->using_fh) {
				render_texture (tex("CoolantF.bmp"), 808, 196, 209, 80);
			} else {
				render_texture (tex("Coolant.bmp"), 808, 196, 209, 80);	
			}
			
			render_texture (tex("Coolanticon.bmp"), 772, 221, 41, 39);
		}
	}

	if (startup_anim_count > 1000) {
		startup_done = true;
	}

	SDL_RenderPresent(renderer);


}

void animate_info_mode() {

	if (infoanimationreverse == false) {
		info_animation_count+=15;
	} else {
		info_animation_count-=15;
	}
	
	if (info_animation_count > 600) {
		infoanimationreverse = true;
	}

	if (dash->info_mode == 3 && currentinfomode == 2) {
		
		if (infoanimationreverse == false) {
			render_texture(tex("tyretop.bmp"), 0-info_animation_count, 176, 510, 97);
			render_texture(tex("tyrebottom.bmp"), 0-info_animation_count, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			render_texture(tex("Navbg.bmp"), 0 - info_animation_count, 158, 434, 268);
			//render_texture(tex("Infobottomdiag.bmp"), 0 - info_animation_count, 273, 454, 125);
		}

		if (info_animation_count <= 0 && infoanimationreverse == true) {
			currentinfomode = 3;
			info_animation_in_progress = false;
		}
		return;
	}

	if (dash->info_mode == 3 && currentinfomode == 1) {
		
		if (infoanimationreverse == false) {
			render_texture(tex("Infotopdiag.bmp"), 0 - info_animation_count, 176, 510, 97);
			render_texture(tex("Infobottomdiag.bmp"), 0 - info_animation_count, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			render_texture(tex("Navbg.bmp"), 0 - info_animation_count, 158, 434, 268);
			//render_texture(tex("Infobottomdiag.bmp"), 0 - info_animation_count, 273, 454, 125);
		}

		if (info_animation_count <= 0 && infoanimationreverse == true) {
			currentinfomode = 3;
			info_animation_in_progress = false;
		}
		return;
	}

	if (dash->info_mode == 3 && currentinfomode == 0) {
		
		if (infoanimationreverse == false) {
			if (dash->using_km) {
				render_texture(tex("InfotopKM.bmp"), 0 - info_animation_count, 176, 510, 97);
			} else {
				render_texture(tex("Infotop.bmp"), 0 - info_animation_count, 176, 510, 97);	
			}
			
			render_texture(tex("Infobottom.bmp"), 0 - info_animation_count, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			render_texture(tex("Navbg.bmp"), 0 - info_animation_count, 158, 434, 268);
			//render_texture(tex("Infobottomdiag.bmp"), 0 - info_animation_count, 273, 454, 125);
		}

		if (info_animation_count <= 0 && infoanimationreverse == true) {
			currentinfomode = 3;
			info_animation_in_progress = false;
		}
		return;
	}

	if (dash->info_mode == 2 && currentinfomode == 1) {
		
		if (infoanimationreverse == false) {
			render_texture(tex("Infotopdiag.bmp"), 0 - info_animation_count, 176, 510, 97);
			render_texture(tex("Infobottomdiag.bmp"), 0 - info_animation_count, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			render_texture(tex("tyretop.bmp"), 0-info_animation_count, 176, 510, 97);
			render_texture(tex("tyrebottom.bmp"), 0-info_animation_count, 273, 454, 125);
		}

		if (info_animation_count <= 0 && infoanimationreverse == true) {
			currentinfomode = 2;
			info_animation_in_progress = false;
		}
		return;
	}

	if (dash->info_mode == 1 && currentinfomode == 0) {
		
		if (infoanimationreverse == false) {
			if (dash->using_km) {
				render_texture(tex("InfotopKM.bmp"), 0 - info_animation_count, 176, 510, 97);
			} else {
				render_texture(tex("Infotop.bmp"), 0 - info_animation_count, 176, 510, 97);	
			}
			
			render_texture(tex("Infobottom.bmp"), 0 - info_animation_count, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			render_texture(tex("Infotopdiag.bmp"), 0-info_animation_count, 176, 510, 97);
			render_texture(tex("Infobottomdiag.bmp"), 0-info_animation_count, 273, 454, 125);
		}

		if (info_animation_count <= 0 && infoanimationreverse == true) {
			currentinfomode = 1;
			info_animation_in_progress = false;
		}
		return;
	}

	if (dash->info_mode == 0 && currentinfomode == 3) {
		if (infoanimationreverse == false) {
			//render_texture(tex("Infotopdiag.bmp"), 0 - info_animation_count, 176, 510, 97);
			//render_texture(tex("Infobottomdiag.bmp"), 0 - info_animation_count, 273, 454, 125);
			render_texture(tex("Navbg.bmp"), 0 - info_animation_count, 158, 434, 268);
		}

		if (infoanimationreverse == true) {
			if (dash->using_km) {
				render_texture(tex("InfotopKM.bmp"), 0- info_animation_count, 176, 510, 97);
			} else {
				render_texture(tex("Infotop.bmp"), 0- info_animation_count, 176, 510, 97);	
			}
			
			render_texture(tex("Infobottom.bmp"), 0- info_animation_count, 273, 454, 125);
		}

		if (info_animation_count <= 0 && infoanimationreverse == true) {
			currentinfomode = 0;
			info_animation_in_progress = false;
		}
		return;
	}

	if (dash->info_mode == 0 && currentinfomode == 2) {
		
		// This one goes away
		if (infoanimationreverse == false) {
			render_texture(tex("tyretop.bmp"), 0-info_animation_count, 176, 510, 97);
			render_texture(tex("tyrebottom.bmp"), 0-info_animation_count, 273, 454, 125);
		}

		// This one comes in
		if (infoanimationreverse == true) {
			if (dash->using_km) {
				render_texture(tex("InfotopKM.bmp"), 0- info_animation_count, 176, 510, 97);
			} else {
				render_texture(tex("Infotop.bmp"), 0- info_animation_count, 176, 510, 97);	
			}
			
			render_texture(tex("Infobottom.bmp"), 0- info_animation_count, 273, 454, 125);
		}

		if (info_animation_count <= 0 && infoanimationreverse == true) {
			currentinfomode = 0;
			info_animation_in_progress = false;
		}
		return;
	}
}

void draw_dashboard () {
	flash_count++;
	if (flash_count > 50) {
		flash_count = 0;
		flash = !flash;
	}

	if ((dash->info_mode != currentinfomode && warningbadgeactive)) {		
		warningbadgecancelled = true;
		warningbadgeactive = false;
	}

	if (dash->theme == 0 || dash->theme == 7) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	}

	SDL_RenderClear(renderer);


	// Rev counter rev line texture
	SDL_RenderTexture(renderer, tex("Revline.bmp"), nullptr, &grevline);

	target_rpm_rotation = get_rpm_rotation (dash->rpm);
	if (target_rpm_rotation > current_rpm_rotation) {
		if (current_rpm_rotation < 100) {
			current_rpm_rotation++;
		}
	}

	if (target_rpm_rotation < current_rpm_rotation) {
		if (current_rpm_rotation > 0) {
			current_rpm_rotation--;
		}
	}

	// White rev counter cover to reveal the rev line
	SDL_RenderTextureRotated(renderer, tex("Whitesq.bmp"), nullptr, &grrevwhite, current_rpm_rotation , &gwhitepoint, SDL_FLIP_NONE);

	// Top grey icons
	render_top_icon_grey_texture (0, 0, 627, 138);
	render_texture (tex("Topiconedge1.bmp"), 625, 0, 98, 75);
	render_texture (tex("Topiconedge2.bmp"), 721, 0, 84, 24);

	// Trip 1 and 2 and Odometer, Ambient Temp and Time section
	render_texture (tex("Mileinfo.bmp"), 0, 434, 435, 169);

	// Fuel Gauge
	if (fuelwarning == true && enginerunning == true) {
		if (flash) 
		{
			render_texture (tex("Fuelgauge.bmp"), 676, 294, 316, 44);
			render_texture (tex("Fuelgaugewhite.bmp"), 714+get_fuel_reveal_from_bars (get_num_fuel_bars(dash->fuel_float)), 303, 274, 28);
		}
	} else {
		render_texture (tex("Fuelgauge.bmp"), 676, 294, 316, 44);
		render_texture (tex("Fuelgaugewhite.bmp"), 714+get_fuel_reveal_from_bars (get_num_fuel_bars(dash->fuel_float)), 303, 274, 28);
	}
	// Coolant Temp
	if (overheatwarning == true && enginerunning == true) {
		if (flash) {
			if (dash->using_fh) {
				render_texture (tex("CoolantF.bmp"), 808, 196, 209, 80);
			} else {
				render_texture (tex("Coolant.bmp"), 808, 196, 209, 80);	
			}	
		}
	} else {
		if (dash->using_fh) {
			render_texture (tex("CoolantF.bmp"), 808, 196, 209, 80);
		} else {
			render_texture (tex("Coolant.bmp"), 808, 196, 209, 80);	
		}
	}

	render_texture (tex("Coolanticon.bmp"), 772, 221, 41, 39);

	// Top light icons (Neutral, Oil, Indicator, High beam)

	if (dash->neutral) {
		render_texture (tex("Neutrallight.bmp"), 0, 0, 130, 136);
	}

	if (!dash->neutral && dash->current_gear != 0) {
		render_texture (tex("Gear.bmp"), 0, 0, 131, 136);
		draw_large_num (dash->current_gear, 30, 22);
	}

	if (dash->oil_warning) {
		render_oil_light_texture (131, 0, 135, 138);
	}

	if (dash->indicate_right && !dash->indicate_left) {
		render_texture (tex("Indicateright.bmp"), 267, 0, 135, 136);
	}

	if (dash->indicate_left && !dash->indicate_right) {
		render_texture (tex("Indicateleft.bmp"), 267, 0, 135, 136);		
	}

	if (dash->indicate_left && dash->indicate_right) {
		render_texture (tex("Indicateboth.bmp"), 267, 0, 135, 136);			
	}

	if (dash->high_beam) {
		render_texture (tex("Highbeamlight.bmp"), 404, 0, 133, 136);
	}

	// Middle info section (Tank Range, MPG, etc..)
	if (dash->info_mode == 0 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {
			if (dash->using_km) {
				render_texture(tex("InfotopKM.bmp"), 0, 176, 510, 97);
			} else {
				render_texture(tex("Infotop.bmp"), 0, 176, 510, 97);	
			}
			
			render_texture(tex("Infobottom.bmp"), 0, 273, 454, 125);
		} 
	}

	if (dash->info_mode == 1 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {
			render_texture(tex("Infotopdiag.bmp"), 0, 176, 510, 97);
			render_texture(tex("Infobottomdiag.bmp"), 0, 273, 454, 125);
		} 
	}

	if (dash->info_mode == 2 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {
			render_texture(tex("tyretop.bmp"), 0, 176, 510, 97);
			render_texture(tex("tyrebottom.bmp"), 0, 273, 454, 125);
		} 
	}

	if (dash->info_mode == 3 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {			
			render_texture(tex("Navbg.bmp"), 0, 158, 434, 268);
		} 
	}

	if ((dash->info_mode != currentinfomode && !warningbadgeactive)) {		
		if (info_animation_in_progress == false) {
			info_animation_count = 0;
			infoanimationreverse = false;
			info_animation_in_progress = true;
		}		

		animate_info_mode();
	}

	/*
	if (dash->info_mode == 2) { // Low Fuel
		render_texture (tex("Fuelwarningbadge.bmp"), 0, 163, 444, 249);
		if (dash->neutral) {
			render_texture (tex("Lowfuel.bmp"), 222, 236, 134, 105);
		}
	}
	*/


	if (tpms_connected == true) {

		if (front_sensor_read) {
			if (g_sensor2_psi <= dash->front_pressure_low) {
				front_tyre_warning_triggered = true;
			}
		}

		if (rear_sensor_read) {
			if (g_sensor4_psi <= dash->rear_pressure_low) {
				rear_tyre_warning_triggered = true;
			}	
		}
			
	}

	// Warning badges — checked in priority order (highest first)
	struct warning_badge { bool active; const char *badge_bmp; const char *flash_bmp; int fx, fy, fw, fh; };

	auto resolve_warning = [&]() -> warning_badge {
		if (front_tyre_warning_triggered || rear_tyre_warning_triggered) {
			const char *detail = front_tyre_warning_triggered && rear_tyre_warning_triggered ? "Frontrearlow.bmp"
			                   : front_tyre_warning_triggered ? "Fronttyrelow.bmp" : "Reartyrelow.bmp";
			return { true, "Lowtyrebadge.bmp", detail, 168, 244, 257, 76 };
		}
		if (dash->oil_warning && enginerunning)
			return { true, "Lowoilbadge.bmp", "Lowoil.bmp", 222, 236, 134, 105 };
		if (overheatwarning && enginerunning)
			return { true, "Overheatbadge.bmp", "Engineoverheat.bmp", 189, 237, 212, 95 };
		if (fuelwarning && enginerunning && dash->info_mode != 3)
			return { true, "Fuelwarningbadge.bmp", "Lowfuel.bmp", 222, 236, 134, 105 };
		return { false, nullptr, nullptr, 0, 0, 0, 0 };
	};

	if (!warningbadgecancelled) {
		auto w = resolve_warning();
		warningbadgeactive = w.active;
		if (w.active) {
			render_texture(tex(w.badge_bmp), 0, 163, 444, 249);
			if (flash) render_texture(tex(w.flash_bmp), w.fx, w.fy, w.fw, w.fh);
		}
	}

	// Units - either MPH or KM
	if (dash->using_km == 1) {
		render_texture (tex("kph.bmp"), 844, 553, 179, 44);
		render_texture (tex("km.bmp"), 350, 448, 57, 18);
	} else {
		render_texture (tex("mph.bmp"), 844, 553, 142, 45);
		render_texture (tex("Miles.bmp"), 350, 448, 57, 18);	
	}
	

	// Rev counter numbers
	SDL_RenderTexture(renderer, tex("R12-13.bmp"), nullptr, &rect_g1213);
	SDL_RenderTexture(renderer, tex("R11.bmp"), nullptr, &rect_g11);
	SDL_RenderTexture(renderer, tex("R10.bmp"), nullptr, &rect_g10);
	SDL_RenderTexture(renderer, tex("R9.bmp"), nullptr, &rect_g9);
	SDL_RenderTexture(renderer, tex("R8.bmp"), nullptr, &rect_g8);
	SDL_RenderTexture(renderer, tex("R7.bmp"), nullptr, &rect_g7);
	SDL_RenderTexture(renderer, tex("R6.bmp"), nullptr, &rect_g6);
	SDL_RenderTexture(renderer, tex("R5.bmp"), nullptr, &rect_g5);
	SDL_RenderTexture(renderer, tex("R4.bmp"), nullptr, &rect_g4);
	SDL_RenderTexture(renderer, tex("R3.bmp"), nullptr, &rectg3);
	SDL_RenderTexture(renderer, tex("R2.bmp"), nullptr, &rectg2);
	SDL_RenderTexture(renderer, tex("R1.bmp"), nullptr, &rectg1);
	SDL_RenderTexture(renderer, tex("R0.bmp"), nullptr, &rectg0);

	sprintf( str_current_speed, "%d", dash->current_speed);
	sprintf( str_trip1, "%.1f", dash->trip1);
	sprintf( str_trip2, "%.1f", dash->trip2);
	sprintf( str_odo, "%.0f", dash->odo);

	if (dash->using_fh) { // If using fahrenheit
		tempf = ((double)dash->ambient_temp*1.8) + 32;
		sprintf(str_ambient_temp, "%df", (int)tempf);
	} else {
		sprintf(str_ambient_temp, "%dc", dash->ambient_temp);
	}
	
	if (dash->using_fh) { //If using fahrenheit
		coolant_temp_f = ((double)dash->coolant_temp*1.8) + 32;
		sprintf(str_coolant_temp, "%d", (int)coolant_temp_f);
	} else {
		sprintf(str_coolant_temp, "%d", dash->coolant_temp);
	}
	
	if (dash->using_km) {
		if (dash->mpg > 0) {
			sprintf(str_mpg, "%d", (int)((double)282.481 / (double)dash->mpg));	
		} else {
			sprintf(str_mpg, "%d", dash->mpg);	
		}
		
	} else {
		sprintf(str_mpg, "%d", dash->mpg);
	}
	
	if (dash->using_km) {
		sprintf(str_range, "%d", (int)((double)dash->range * 1.609));
	} else {
		sprintf(str_range, "%d", dash->range);	
	}
	
	sprintf(str_max_speed, "%d", dash->max_speed);
	sprintf(str_rpm, "%d", dash->rpm);
	
	if (dash->oil_pressure_available) {
		sprintf(str_oil_press, "%.1f", get_precise_bar(dash->oil_pressure_ohms));
		sprintf(str_oil_temp, "%d", (int)get_precise_temp(dash->oil_temp_ohms));	
	}

	if (g_sensor2_psi != -99) {
		if (dash->using_bar) {
			sprintf (str_sensor2_psi, "%.1f", g_sensor2_bar);
		} else {
			sprintf (str_sensor2_psi, "%.1f", g_sensor2_psi);
		}
		
	} else {
		strcpy (str_sensor2_psi, "..");
	}

	if (g_sensor4_psi != -99) {
		if (dash->using_bar) {
			sprintf (str_sensor4_psi, "%.1f", g_sensor4_bar);	
		} else {
			sprintf (str_sensor4_psi, "%.1f", g_sensor4_psi);
		}
		
	} else {
		strcpy (str_sensor4_psi, "..");
	}
	
	if (g_sensor2_temp != -99) {
		if (dash->using_fh) {
			g_sensor2_temp_f = ((double)g_sensor2_temp*1.8) + 32;
			sprintf (str_sensor2_temp, "%df", (int)g_sensor2_temp_f);	
		} else {
			sprintf (str_sensor2_temp, "%dc", g_sensor2_temp);	
		}		
	} else {
		strcpy (str_sensor2_temp, "..");
	}
	
	if (g_sensor4_temp != -99) {
		if (dash->using_fh) {
			g_sensor4_temp_f = ((double)g_sensor4_temp*1.8) + 32;
			sprintf (str_sensor4_temp, "%df", (int)g_sensor4_temp_f);
		} else {
			sprintf (str_sensor4_temp, "%dc", g_sensor4_temp);
		}		
	} else {
		strcpy (str_sensor4_temp, "..");
	}
	

	if (get_litres_remaining(dash->fuel_float) != 0) {
		sprintf(str_fuel, "%.1f", litres_remaining);
	} else {
		strcpy (str_fuel, "..");
	}
	
	sprintf(str_batt, "%.1f", dash->batt);
	sprintf(str_spd_correct, "%.1f", dash->spd_correct);


	// Draw the current speed
	draw_speed(str_current_speed);		

	// Draw some numbers
	draw_medium_string (str_ambient_temp, 18, 461);
	draw_medium_string (str_coolant_temp, 873, 224);

	// Middle Info
	if (dash->info_mode == 0 && info_animation_in_progress == false && !warningbadgeactive) {
		draw_medium_string (str_mpg, 60, 224);
		draw_medium_string (str_range, 292, 224);
		draw_medium_string (str_trip_time, 17, 332);
		draw_medium_string (str_max_speed, 246, 332);
	}

	if (dash->info_mode == 1 && info_animation_in_progress == false && !warningbadgeactive) {
		// RPM, FUEL, BATT, SPEED CORRECTION %
		//char str_rpm[16];
		//char str_fuel[16];
		draw_medium_string(str_rpm, 60, 224);
		draw_medium_string(str_fuel, 292, 224);
		draw_medium_string (str_batt, 17, 332);
		if (strlen (str_spd_correct) > 4) {
			draw_medium_string (str_spd_correct, 216, 332);
		} else {
			draw_medium_string (str_spd_correct, 236, 332);	
		}		
	}

	//g_sensor2_psi
	if (dash->info_mode == 2 && info_animation_in_progress == false && !warningbadgeactive) {
		draw_medium_string (str_sensor2_psi, 40, 224);
		draw_medium_string (str_sensor4_psi, 304, 224);
		draw_medium_string (str_sensor2_temp, 37, 342);
		draw_medium_string (str_sensor4_temp, 246, 342);
	}

	if (dash->info_mode == 3 && info_animation_in_progress == false && !warningbadgeactive) {
		

		if (nav->nav_active == true) {
			// Format nav distance strings from numeric state
			sprintf(str_nav_yards, "%d", nav->nav_yards);
			sprintf(str_nav_miles, "%.1f", nav->nav_miles);
			sprintf(str_nav_metres, "%d", (int)((double)nav->nav_yards / 1.094));
			sprintf(str_nav_km, "%.1f", nav->nav_miles * 1.609);
			sprintf(str_nav_dest_miles, "%.1f", nav->nav_dest_distance);
			sprintf(str_nav_dest_km, "%.1f", nav->nav_dest_distance * 1.609);

			if (strlen (nav->nav_road) > 0) {
				draw_nav_symbol (NAV_ICON_ONTO, 13, 350);
			}

			if (strlen (nav->nav_road) > 0 && strlen (nav->nav_road) <= 14) {
				draw_nav_large_string (nav->nav_road, 69, 347);			
			} else {
				draw_nav_small_string (nav->nav_road, 69, 347);			
			}
			
			if (strlen (nav->nav_towards) > 0) {
				draw_nav_symbol (NAV_ICON_TWRDS, 13, 391);
			}

			if (strlen (nav->nav_towards) > 0 && strlen (nav->nav_towards) <= 16) {			
				draw_nav_large_string (nav->nav_towards, 99, 386);
			} else {			
				draw_nav_small_string (nav->nav_towards, 99, 386);
			}
			
			draw_nav_small_string (sz_arrive_in, 14, 175);
			
			if (dash->using_km) {
				draw_nav_large_string (str_nav_dest_km, 77, 171);
				draw_nav_small_string (sz_km, 165, 175);
			} else {
				draw_nav_large_string (str_nav_dest_miles, 77, 171);
				draw_nav_small_string (sz_mls, 165, 175);
			}
			

			if (nav->nav_yards <= 300) {
				//draw_nav_numbers (87, 16, 191);				
				if (dash->using_km) {
					draw_nav_digits (str_nav_metres, 16, 211);
					draw_nav_symbol (NAV_ICON_METRE, 22, 308);
				} else {
					draw_nav_digits (str_nav_yards, 16, 211);
					draw_nav_symbol (NAV_ICON_YARD, 22, 308);
				}				
			} else {
				if (dash->using_km) {
					draw_nav_digits (str_nav_km, 16, 211);
					draw_nav_symbol (NAV_ICON_KM, 22, 308);
				} else {
					draw_nav_digits (str_nav_miles, 16, 211);
					draw_nav_symbol (NAV_ICON_MILE, 22, 308);
				}				
			}
			
			const nav_symbol_entry *nav_entry = lookup_nav_symbol(nav->nav_symbol);
			if (nav_entry) {
				if (nav_entry->icon_lhd != NAV_ICON_NONE) {
					nav_icon icon = (nav_entry->icon_rhd != NAV_ICON_NONE && !nav->driving_left) ? nav_entry->icon_rhd : nav_entry->icon_lhd;
					draw_nav_symbol(icon, nav_entry->x, nav_entry->y);
				}
				if (strcmp(nav_entry->code, "MEX") == 0) draw_nav_large_string(nav->nav_exit, 344, 275);
				if (strcmp(nav_entry->code, "PRK") == 0) draw_nav_large_string(sz_find_parking, 99, 386);
				if (strcmp(nav_entry->code, "FRY") == 0) draw_nav_large_string(sz_take_ferry, 99, 386);
			}
		} else {
			draw_nav_large_string (sz_no_nav_data, 40, 265);
			draw_nav_small_string (sz_smartphone_app, 40, 350);
			draw_nav_small_string (sz_set_destination, 56, 380);
		}
		

	}

	if (dash->indicate_left && !warningbadgeactive) {		
		if (dash->info_mode == 0 || dash->info_mode == 1) {
			render_texture (tex("Indicateleftfar.bmp"), 2, 186, 250, 98);
		}		
	}

	if (dash->indicate_right && !warningbadgeactive) {		
		render_texture (tex("Indicaterightfar.bmp"), 802, 192, 209, 84);
	}
	

	// Essential info
	draw_medium_string(str_time, 14, 543);
	draw_small_blue_string (str_trip1, 252, 460);
	draw_small_blue_string (str_trip2, 246, 509);
	draw_small_grey_string (str_odo, 223, 560);

	if (dash->oil_pressure_available) {
		draw_medium_string(str_oil_press, 163, 32);
		draw_medium_string(str_oil_temp, 163, 89);		
	}

	if (tpms_connected) {
		render_texture (tex("tyreicon.bmp"), 630, 553, 22, 45);
	}

	if (tpms_connected && tpms_signal && tpms_signal_count < 300) {
		render_texture (tex("tyresignal.bmp"), 653, 559, 33, 34);
		tpms_signal_count++;
	}

	SDL_RenderPresent(renderer);
}

int main(int argc, char* args[]) {


	strcpy (str_trip_time, "00:00");
	strcpy(str_time, "14:35");

	// base readings for oil pressure
	barohms[0] = 6;
	barohms[1] = 23;
	barohms[2] = 39;
	barohms[3] = 54;
	barohms[4] = 69;
	barohms[5] = 89;
	barohms[6] = 106;
	barohms[7] = 121;
	barohms[8] = 140;
	barohms[9] = 150;
	barohms[10] = 160;

	// base readings for oil temperature
	tempnum[0] = 50;
	tempnum[1] = 70;
	tempnum[2] = 80;
	tempnum[3] = 95;
	tempnum[4] = 120;
	tempnum[5] = 150;

	tempohms[0] = 680;
	tempohms[1] = 310;
	tempohms[2] = 230;
	tempohms[3] = 140;
	tempohms[4] = 75;
	tempohms[5] = 40;

	//std::thread t1(thread_worker, "hello");
	SDL_HideCursor();
	
	sensor_feed *feed = sensor_feed_create(NULL);
	sensor_feed_start(feed);
	dash = sensor_feed_dashboard(feed);
	menu = sensor_feed_menu(feed);
	nav = sensor_feed_nav(feed);
	printf("\n Thread created successfully\n");

	int err;

	if (tpms_model == STANDARDTPMS) {
		err = pthread_create(&(tpms_tid[0]), nullptr, &pollTPMSInterface, nullptr);
	}		

	if (tpms_model == EBAYTPMS) {
		err = pthread_create(&(tpms_tid[0]), nullptr, &pollTPMSInterface2, nullptr);
	}
	
    if (err != 0)
        printf("\ncan't create TPMS thread :[%s]", strerror(err));
    else
        printf("\n TPMS Thread created successfully\n");


	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "could not initialize sdl3: %s\n", SDL_GetError());
		return 1;
	}

	// Use fullscreen on KMSDRM so the app takes over the DRM scanout
	Uint64 window_flags = 0;
	const char *video_driver = SDL_GetCurrentVideoDriver();
	fprintf(stderr, "Video driver: %s\n", video_driver ? video_driver : "(null)");
	if (video_driver && strcmp(video_driver, "KMSDRM") == 0) {
		window_flags = SDL_WINDOW_FULLSCREEN;
	}

	if (!SDL_CreateWindowAndRenderer("TFT Dash", SCREEN_WIDTH, SCREEN_HEIGHT, window_flags, &window, &renderer)) {
		fprintf(stderr, "could not create window & renderer: %s\n", SDL_GetError());
		return 1;
	}

	if (window == nullptr) {
		fprintf(stderr, "could not create window: %s\n", SDL_GetError());
		return 1;
	}

	const char *renderer_name = SDL_GetRendererName(renderer);
	fprintf(stderr, "Renderer: %s\n", renderer_name ? renderer_name : "(null)");

	g_assets = asset_store_create(renderer);
	if (!g_assets) {
		fprintf(stderr, "Failed to create asset store\n");
		return 1;
	}
	int total_assets = 0;
	for (int i = 0; i < THEME_COUNT; i++) {
		char dir[256];
		snprintf(dir, sizeof(dir), "assets/themes/%s", THEME_NAMES[i]);
		int n = asset_store_load_theme(g_assets, THEME_NAMES[i], dir);
		if (n < 0) {
			fprintf(stderr, "Failed to load theme: %s (dir: %s)\n", THEME_NAMES[i], dir);
		} else {
			total_assets += n;
		}
	}
	fprintf(stderr, "Loaded %d assets across %d themes\n", total_assets, THEME_COUNT);
	g_current_theme = theme_name_from_id(dash->theme);

	init_rects();

	SDL_HideCursor();

	////////////////////////////////////////  LOOP ///////////////////////////////////////////
	SDL_Event e;

	while (!quit) {
		// Refresh state pointers from sensor feed each frame
		dash = sensor_feed_dashboard(feed);
		menu = sensor_feed_menu(feed);
		nav = sensor_feed_nav(feed);

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				quit = true;
			}
			if (e.type == SDL_EVENT_KEY_DOWN) {
				//quit = true;
			}

			if (e.type == SDL_EVENT_KEY_UP) {
				//fprintf (stderr, "choice");
				//spin_angle--;
				//fprintf (stderr, "Key: %d", e.key.scancode);

				if (e.key.scancode == 41) {
					quit = true;
				}

				/*
				if(e.key.scancode != SDL_GetScancodeFromKey(e.key.key, NULL)) {
					printf("Physical %s key acting as %s key",
      				SDL_GetScancodeName(e.key.scancode),
      				SDL_GetKeyName(e.key.key));
				}
				*/


			}

			if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
				fprintf(stderr, "select");
				//spin_angle++;

			}

			if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
				//quit = true;
			}
		}

		//run_partial_test();

		if (dash->theme != currenttheme) {
			currenttheme = dash->theme;
			g_current_theme = theme_name_from_id(dash->theme);
		}

		//dash->odo = spin_angle;
		//dash->odo = get_fuel_reveal(fuel_percent);

		if (dash->choice_state == 0) {
			//draw_dashboard();
			if (startup_done == false) {
				dashboard_startup();
			}
			else {

				//run_rpm_test();
				draw_dashboard();
			}

			//printf("%i\n", get_fuel_reveal(50));

			g_down_arrow_x = -140;
			g_up_arrow_x = -140;

		}
		else {
			if (dash->choice_state > 0) {
				draw_menu(dash->choice_state);
			}
		}



		// Thresholds for triggering warnings
		if (get_num_fuel_bars(dash->fuel_float) <= 2) { // 2 bars remaining - 4.5 litres left
			fuelwarning = true;
		} else {
			fuelwarning = false;
		}

		if (dash->coolant_temp >= 120) {
			overheatwarning = true;
		}

		if (dash->coolant_temp < 120) {
			overheatwarning = false;
		}

		// Quick check on the RPM to set a flag if the engine is running - used for triggering warning badges
		if (dash->rpm > 2000) {
			enginerunning = true;
		} 

		if (dash->rpm < 1000) {
			enginerunning = false;
		}


		SDL_Delay(5);

		//fprintf(stderr, "Main thread: %s\n", g_sz_comms_msg);
	}

	asset_store_destroy(g_assets);

	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}


