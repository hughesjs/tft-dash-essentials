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
//28/02/2020 - Poll interface no longer checks for an Ending E for the message, now just checks the length is within a good range
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

#include "SDL.h"
#include <stdio.h>
#include <pthread.h>
#include <fstream>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

// Parser module
#include "parser.h"

// Asset management
#include "assets.h"

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

//MUT  EXITL   EXITR  TNR   TNL  SLL  SLR  RB2L  RB3L  RB4L  RB5L RB1R RB3R RB4R RB5R KPL KPR CON ARV RB1L RB2R MUTR MILE YARD KM METRE ONTO TWRDS

#define MUT			0
#define EXITL		1
#define EXITR		2
#define TNR			3
#define TNL			4
#define SLL			5
#define SLR			6
#define RB2L		7
#define RB3L		8
#define RB4L		9
#define RB5L		10
#define RB1R		11
#define RB3R		12
#define RB4R		13
#define RB5R		14
#define KPL			15
#define KPR			16
#define CON			17
#define ARV			18
#define RB1L		19
#define RB2R		20
#define MUTR		21
#define MILE		22
#define YARD		23
#define KM			24
#define METRE		25
#define ONTO		26
#define TWRDS		27

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Surface* screen_surface = nullptr;

// Surface Rects
// Rev counter
SDL_Rect rect_g1213;
SDL_Rect rect_g11;
SDL_Rect rect_g10;
SDL_Rect rect_g9;
SDL_Rect rect_g8;
SDL_Rect rect_g7;
SDL_Rect rect_g6;
SDL_Rect rect_g5;
SDL_Rect rect_g4;
SDL_Rect rectg3;
SDL_Rect rectg2;
SDL_Rect rectg1;
SDL_Rect rectg0;
SDL_Rect grevline;
SDL_Rect grrevwhite;
SDL_Point gwhitepoint;

/*
MUT
MEX
TNR
TNL
SLL
SLR
RB1
RB2
RB3
RB4
RB5
LNR
LNL
CON
ARV
*/

// Navigation symbols texture mapping
int g_nav_icons_src_tex_loc[4][28] = {
	//MUT  EXITL   EXITR  TNR   TNL  SLL  SLR  RB2L  RB3L  RB4L  RB5L RB1R RB3R RB4R RB5R KPL KPR CON ARV RB1L RB2R MUTR MILE YARD KM METRE ONTO TWRDS
	{21,   169,    317,   449,  628, 809, 1002,17,   163,  312,  460, 598, 771, 937, 1092,6,  174,333,491,642, 823, 983, 11,  106,215,280,  404, 456  }, // X pos
	{12,   11,     12,    20,   19,  22,  22,  184,  216,  217,  217, 214, 217, 216, 216, 383,385,384,394,387, 364, 385, 549, 550,550,549,  556, 557  }, // Y pos
	{112,  112,    112,   157,  147, 158, 176, 115,  134,  114,  110, 131, 131, 131, 131, 141,150,131,112,138, 116, 108, 81,  87,  42,105,  45,  76   }, // Width
	{150,  150,    150,   146,  150, 144, 146, 167,  134,  135,  135, 137, 134, 134, 134, 140,138,138,129,135, 167, 145, 24,  21,  22,22,   16,  14   }  // Height
};

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
SDL_Rect spd_digit_one;
SDL_Rect spd_digit_two;
SDL_Rect spd_digit_three;


char g_sz_comms_msg[1024];
int serial_port;
pthread_t tid[2];

// Bike specific values
int spin_angle = 0; // revline reveal
int test_counter = 0; // Number to use for test scenarios

int tpms_model = EBAYTPMS; // Standard version I recommend or EBAYTPMS


/*
	RPM		Spinangle
	0		6
	500		
*/


// Core values
int current_speed = 68;
double trip1 = 1234.2;
double trip2 = 3456.4;
double odo = 23456;
int coolant_temp = 56;
double coolant_temp_f = 0;
int mpg = 10;
int range = 100;
int max_speed = 40;
int trip_time_hour = 0;
int trip_time_min = 0;
int using_km = 0;
int driving_left = 1;
int using_fh = 0;
int using_bar = 0;
int rpm = 5000;
double batt = 12.5;
double spd_correct = 0;
int fuel_percent = 47;
int fuel_float = 0;
int ambient_temp = 21;
double tempf = 0;
bool neutral = false;
bool oil_warning = false;
bool indicate_left = false;
bool indicate_right = false;
bool high_beam = false;
int info_mode = 0;
int theme = 0; // The theme set in the arduino
int current_gear = 0;
double litres_remaining = 0;
int control_layout = 0;
int day_theme = 0;
int night_theme = 0;
int light_switch_value = 0;
int current_light_level = 0;
int fan_neutral_option = 0;
int gear_ratio_interval = 0;


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

// TPMS configurable values
int front_sensor_id = 1;
int rear_sensor_id = 3;
int front_pressure_low = -1;
int rear_pressure_low = -1;
char str_front_sensor_id[16];
char str_rear_sensor_id[16];
char str_front_pressure_low[16];
char str_rear_pressure_low[16];


char str_sensor2_psi[16];
char str_sensor4_psi[16];

char str_sensor2_temp[16];
char str_sensor4_temp[16];

char str_coolant_temp[16];
char str_current_speed[16];
char str_trip1[16];
char str_trip2[16];
char str_odo[16];
char str_time[16];
char str_nav[255];
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

char str_nav_symbol[16];
char str_nav_road[255];
char str_nav_towards[255];
char str_nav_exit[16];
char str_nav_yards[16];
char str_nav_miles[16];
char str_nav_dest_miles[16];
char str_nav_dest_km[16];

char str_nav_metres[16];
char str_nav_km[16];

int nav_yards = 0;
double nav_miles = 0;
bool nav_active = false;
double nav_dest_distance = 0;

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

int front_sprocket = 16;
int rear_sprocket = 44;
int coolant_fan_temp = 100;

// This bit is emulating the button presses from the arduino
// To control menu states
int choice_state = 0; // Set to -1 for production

// Set time digits
int settimedigit0 = 0;
int settimedigit1 = 0;
int settimedigit2 = 0;
int settimedigit3 = 0;

char str_set_time_digit0[16];
char str_set_time_digit1[16];
char str_set_time_digit2[16];
char str_set_time_digit3[16];

// Speed correction digits
int spcdigit0 = 0;
int spcdigit1 = 0;
int spcdigit2 = 0;
int spcdigit3 = 0;

char str_spc_digit0[16];
char str_spc_digit1[16];
char str_spc_digit2[16];
char str_spc_digit3[16];

// Odometer digits
int ododigit1 = 0;
int ododigit2 = 0;
int ododigit3 = 0;
int ododigit4 = 0;
int ododigit5 = 0;
int ododigit6 = 0;

int odo2digit1 = 0;
int odo2digit2 = 0;
int odo2digit3 = 0;
int odo2digit4 = 0;
int odo2digit5 = 0;
int odo2digit6 = 0;
int odoerror = 0;

// Oil pressure values
bool oil_pressure_available = false;
int oil_pressure_ohms = 0;
int oil_temp_ohms = 0;

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

void choice() { // Pressed the choice button

	if (choice_state >= 0 && choice_state < 20) { // Main menu
		choice_state++;	
	}
	
	if (choice_state > 12 && choice_state < 20) { // Main menu return
		choice_state = 0;
	}

	// Set time menu
	if (choice_state >= 20 && choice_state < 90) { // Set time menu
		choice_state+=10;
	}

	if (choice_state > 70 && choice_state < 90) { // Set time menu recycle
		choice_state = 20;
	}

	// Speed correction menu
	if (choice_state >= 100 && choice_state < 160) { // Speed correction menu
		choice_state+=10;
	}

	if (choice_state > 150 && choice_state < 170) { // Speed correction recycle
		choice_state = 100;
	}


	if (choice_state >= 200 && choice_state < 290) { // Theme menu
		choice_state+=10;
	}

	if (choice_state > 260 && choice_state < 290) { // Theme recycle
		choice_state = 200;
	}

	if (choice_state >= 300 && choice_state < 390) { // Set odometer menu
		odoerror = 0;
		choice_state += 5;
	}

	if (choice_state > 360 && choice_state < 390) { // Set odometer menu recycle
		choice_state = 300;
	}

	if (choice_state >= 600 && choice_state < 640) { // Set units menu
		choice_state += 5;
	}

	if (choice_state > 635 && choice_state < 690) { // Set units menu recycle
		choice_state = 600;
	}
}

void select() { // Pressed the select button

	if (choice_state == 0) {
		if (info_mode == 3) {
			info_mode = 0;
			return;
		}
		
		if (info_mode == 2) {
			info_mode = 3;
			return;
		}		

		if (info_mode == 1) {
			info_mode = 2;
			return;
		}

		if (info_mode == 0) {
			info_mode = 1;
			return;
		}
	}

	if (choice_state == 1) {
		// Reset trip one
		choice_state = 0;
		return;
	}

	if (choice_state == 2) {
		// Reset trip two
		choice_state = 0;
		return;
	}


	if (choice_state == 3) { // Will become set units menu
		// Toggle miles / km
		
		/*
		if (using_km == 0) {
			using_km = 1;
		} else {
			using_km = 0;
		}
		*/
		choice_state = 600;
		return;
	}

	if (choice_state == 605) { // Set units to miles
		using_km = 0;
	}

	if (choice_state == 610) { // Set units to km
		using_km = 1;
	}

	if (choice_state == 615) { // Set units to celsius
		using_fh = 0;
	}	

	if (choice_state == 620) { // Set units to fahrenheit
		using_fh = 1;
	}	

	if (choice_state == 625) { // Set units to psi
		using_bar = 0;
	}	

	if (choice_state == 630) { // Set units to bar
		using_bar = 1;
	}	

	if (choice_state == 4) { // Main menu set time
		choice_state = 20; // Jump to time menu
		return;
	}

	if (choice_state == 5) {
		choice_state = 100; // Jump to speed correction menu
		return;
	}

	if (choice_state == 6) {
		choice_state = 200; // Jump to theme selection menu
		return;
	}

	if (choice_state == 20) { // Set time cancel
		choice_state = 0;
		return;
	}

	if (choice_state == 30) { // Digit 0
		settimedigit0++;
		if (settimedigit0 > 2) {
			settimedigit0 = 0;
		}
		return;
	}

	if (choice_state == 40) { // Set time digit 1
		settimedigit1++;
		if (settimedigit1 > 9) {
			settimedigit1 = 0;
		}

		if (settimedigit0 == 2) {
			if (settimedigit1 > 3) {
				settimedigit1 = 0;
			}
		}
		return;
	}

	if (choice_state == 50) { // Set time digit 2
		settimedigit2++;
		if (settimedigit2 > 5) {
			settimedigit2 = 0;
		}
		return;
	}

	if (choice_state == 60) { // Set time digit 3
		settimedigit3++;
		if (settimedigit3 > 9) {
			settimedigit3 = 0;
		}
		return;
	}

	if (choice_state == 70) { // Set time ok

		// Do stuff to set the time here but on the arduino.
		// Update time vars

		choice_state = 0; // Return to dashboard
		return;
	}


	if (choice_state == 100) { // Speed correction cancel
		choice_state = 0; // Jump to main menu
		return;
	}

	if (choice_state == 110) { // Speed correction digit 1
		spcdigit0++;
		if (spcdigit0 > 1) {
			spcdigit0 = 0;
		}
		return;
	}

	if (choice_state == 120) { // Speed correction digit 2
		spcdigit1++;
		if (spcdigit1 > 9) {
			spcdigit1 = 0;
		}
		return;
	}

	if (choice_state == 130) { // Speed correction digit 3
		spcdigit2++;
		if (spcdigit2 > 9) {
			spcdigit2 = 0;
		}
		return;
	}

	if (choice_state == 140) { // Speed correction digit 4
		spcdigit3++;
		if (spcdigit3 > 9) {
			spcdigit3 = 0;
		}
		return;
	}

	if (choice_state == 150) {

		// Do stuff to update and save the speed correction value
		choice_state = 0;
		return;
	}

	if (choice_state == 200) {
		// Set theme
		theme = 0;
		choice_state = 0;
		return;
	}

	if (choice_state == 210) {
		// Set theme
		theme = 1;
		choice_state = 0;
		return;
	}

	if (choice_state == 220) {
		// Set theme
		theme = 2;
		choice_state = 0;
		return;
	}

	if (choice_state == 230) {
		// Set theme
		theme = 3;
		choice_state = 0;
		return;
	}

	if (choice_state == 240) {
		// Set theme
		theme = 4;
		choice_state = 0;
		return;
	}

	if (choice_state == 250) {
		// Set theme
		theme = 5;
		choice_state = 0;
		return;
	}

	if (choice_state == 260) {
		// Set theme
		theme = 6;
		choice_state = 0;
		return;
	}

	// Set units menu - Cancel
	if (choice_state == 600) {
		choice_state = 0;
		return;
	}

	if (choice_state == 605) {

	}

	// Set units - OK
	if (choice_state == 635) {
		choice_state = 0;
		return;
	}



	// Odometer menu digit setting
	if (choice_state == 300) {ododigit1 = cycle_digit (ododigit1, 9); return;}
	if (choice_state == 305) {ododigit2 = cycle_digit (ododigit2, 9); return;}
	if (choice_state == 310) {ododigit3 = cycle_digit (ododigit3, 9); return;}
	if (choice_state == 315) {ododigit4 = cycle_digit (ododigit4, 9); return;}
	if (choice_state == 320) {ododigit5 = cycle_digit (ododigit5, 9); return;}
	if (choice_state == 325) {ododigit6 = cycle_digit (ododigit6, 9); return;}

	if (choice_state == 330) {odo2digit1 = cycle_digit (odo2digit1, 9); return;}
	if (choice_state == 335) {odo2digit2 = cycle_digit (odo2digit2, 9); return;}
	if (choice_state == 340) {odo2digit3 = cycle_digit (odo2digit3, 9); return;}
	if (choice_state == 345) {odo2digit4 = cycle_digit (odo2digit4, 9); return;}
	if (choice_state == 350) {odo2digit5 = cycle_digit (odo2digit5, 9); return;}
	if (choice_state == 355) {odo2digit6 = cycle_digit (odo2digit6, 9); return;}

	if (choice_state == 360) {
		// Set and save the odometer here
		if (ododigit1 != odo2digit1) {odoerror = 1; return;}
		if (ododigit2 != odo2digit2) {odoerror = 1; return;}
		if (ododigit3 != odo2digit3) {odoerror = 1; return;}
		if (ododigit4 != odo2digit4) {odoerror = 1; return;}
		if (ododigit5 != odo2digit5) {odoerror = 1; return;}
		if (ododigit6 != odo2digit6) {odoerror = 1; return;}

		int num_digits_at_zero = 0;
		if (ododigit1 == 0) {num_digits_at_zero++;}
		if (ododigit2 == 0) {num_digits_at_zero++;}
		if (ododigit3 == 0) {num_digits_at_zero++;}
		if (ododigit4 == 0) {num_digits_at_zero++;}
		if (ododigit5 == 0) {num_digits_at_zero++;}
		if (ododigit6 == 0) {num_digits_at_zero++;}
		if (odo2digit1 == 0) {num_digits_at_zero++;}
		if (odo2digit2 == 0) {num_digits_at_zero++;}
		if (odo2digit3 == 0) {num_digits_at_zero++;}
		if (odo2digit4 == 0) {num_digits_at_zero++;}
		if (odo2digit5 == 0) {num_digits_at_zero++;}
		if (odo2digit6 == 0) {num_digits_at_zero++;}

		if (num_digits_at_zero == 12) {
			odoerror = 2;
			return;
		}

		choice_state = 0; 
	}
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



bool file_exists (const char* name) {

	std::ifstream ifile (name);
	return (bool)ifile;

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



void draw_small_grey_string (char* digits, int xpos, int ypos)
{
	int x_offset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d <= 10; d++) {
			if (digit == g_small_num_ref[d]) {
				SDL_Rect g_src_rect;
				g_src_rect.x = g_small_grey_src_tex_loc[0][d];
				g_src_rect.y = g_small_grey_src_tex_loc[1][d];
				g_src_rect.w = g_small_grey_src_tex_loc[2][d];
				g_src_rect.h = g_small_grey_src_tex_loc[3][d];

				SDL_Rect g_dst_rect;
				g_dst_rect.x = xpos + x_offset;
				g_dst_rect.y = ypos;

				if (d == 10) {
					g_dst_rect.y = ypos + 18;
				}

				g_dst_rect.w = g_src_rect.w;
				g_dst_rect.h = g_src_rect.h;

				SDL_RenderCopy(renderer, tex("Smallnumbers.bmp"), &g_src_rect, &g_dst_rect);

				x_offset+=g_src_rect.w;
			}
		}

	}		
}



void draw_small_blue_string (char* digits, int xpos, int ypos)
{
	int x_offset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d <= 10; d++) {
			if (digit == g_small_num_ref[d]) {
				SDL_Rect g_src_rect;
				g_src_rect.x = g_small_blue_src_tex_loc[0][d];
				g_src_rect.y = g_small_blue_src_tex_loc[1][d];
				g_src_rect.w = g_small_blue_src_tex_loc[2][d];
				g_src_rect.h = g_small_blue_src_tex_loc[3][d];

				SDL_Rect g_dst_rect;
				g_dst_rect.x = xpos + x_offset;
				g_dst_rect.y = ypos;

				if (d == 10) {
					g_dst_rect.y = ypos + 18;
				}

				g_dst_rect.w = g_src_rect.w;
				g_dst_rect.h = g_src_rect.h;

				SDL_RenderCopy(renderer, tex("Smallnumbers.bmp"), &g_src_rect, &g_dst_rect);

				x_offset+=g_src_rect.w;
			}
		}

	}		
}

void draw_medium_char (char digit, int xpos, int ypos)
{
	int x_offset = 0;
	
	for (int d = 0; d <= 15; d++) {
		if (digit == g_num_ref[d]) {
			SDL_Rect g_src_rect;
			g_src_rect.x = g_med_numbers_src_tex_loc[0][d];
			g_src_rect.y = g_med_numbers_src_tex_loc[1][d];
			g_src_rect.w = g_med_numbers_src_tex_loc[2][d];
			g_src_rect.h = g_med_numbers_src_tex_loc[3][d];

			SDL_Rect g_dst_rect;
			g_dst_rect.x = xpos + x_offset;
			g_dst_rect.y = ypos;
			g_dst_rect.w = g_src_rect.w;
			g_dst_rect.h = g_src_rect.h;

			SDL_RenderCopy(renderer, tex("Smallnumbers.bmp"), &g_src_rect, &g_dst_rect);

			x_offset+=g_src_rect.w;
		}
	}

}

void draw_large_string (char *digits, int xpos, int ypos) 
{
	//g_large_numbers_src_tex_loc
	int x_offset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d < 10; d++) {
			if (digit == g_large_num_ref[d]) {
				SDL_Rect g_src_rect;
				g_src_rect.x = g_large_numbers_src_tex_loc[0][d];
				g_src_rect.y = g_large_numbers_src_tex_loc[1][d];
				g_src_rect.w = g_large_numbers_src_tex_loc[2][d];
				g_src_rect.h = g_large_numbers_src_tex_loc[3][d];

				SDL_Rect g_dst_rect;
				g_dst_rect.x = xpos + x_offset;
				g_dst_rect.y = ypos;
				g_dst_rect.w = g_src_rect.w;
				g_dst_rect.h = g_src_rect.h;

				SDL_RenderCopy(renderer, tex("Smallnumbers.bmp"), &g_src_rect, &g_dst_rect);

				x_offset+=g_src_rect.w;
			}
		}
	}	
}

void draw_medium_string (char* digits, int xpos, int ypos)
{
	int x_offset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d <= 15; d++) {
			if (digit == g_num_ref[d]) {
				SDL_Rect g_src_rect;
				g_src_rect.x = g_med_numbers_src_tex_loc[0][d];
				g_src_rect.y = g_med_numbers_src_tex_loc[1][d];
				g_src_rect.w = g_med_numbers_src_tex_loc[2][d];
				g_src_rect.h = g_med_numbers_src_tex_loc[3][d];

				SDL_Rect g_dst_rect;
				g_dst_rect.x = xpos + x_offset;
				g_dst_rect.y = ypos;
				g_dst_rect.w = g_src_rect.w;
				g_dst_rect.h = g_src_rect.h;

				SDL_RenderCopy(renderer, tex("Smallnumbers.bmp"), &g_src_rect, &g_dst_rect);

				x_offset+=g_src_rect.w;
			}
		}

	}		
}

void draw_nav_large_string (char *digits, int xpos, int ypos) {
	int x_offset = 0;

	for (int s=0;s<strlen (digits);s++) {

		char digit = digits[s];
		
		if (digit == 32 || digit == '-') {
			x_offset+=7;
		}

		for (int d=0;d<40;d++) {

			if (xpos + x_offset < 412) {
				if (digit == g_nav_large_lower_letter_ref[d] || digit == g_nav_large_upper_letter_ref[d]) {

					SDL_Rect g_src_rect;
					g_src_rect.x = g_nav_letters_large_src_tex_loc[0][d];
					g_src_rect.y = g_nav_letters_large_src_tex_loc[1][d];
					g_src_rect.w = g_nav_letters_large_src_tex_loc[2][d];
					g_src_rect.h = g_nav_letters_large_src_tex_loc[3][d];

					SDL_Rect g_dst_rect;
					g_dst_rect.x = xpos + x_offset;
					g_dst_rect.y = ypos;
					g_dst_rect.w = g_src_rect.w;
					g_dst_rect.h = g_src_rect.h;

					SDL_RenderCopy(renderer, tex("Navgfx.bmp"), &g_src_rect, &g_dst_rect);

					x_offset+=g_src_rect.w + 2;				
				}
			}
		}		
	}
}

void draw_nav_small_string (char *digits, int xpos, int ypos) {
	int x_offset = 0;

	for (int s=0;s<strlen (digits);s++) {

		char digit = digits[s];
		
		if (digit == 32 || digit == '-') {
			x_offset+=7;
		}

		for (int d=0;d<40;d++) {

			if (xpos + x_offset < 412) {
				if (digit == g_nav_large_lower_letter_ref[d] || digit == g_nav_large_upper_letter_ref[d]) {

					SDL_Rect g_src_rect;
					g_src_rect.x = g_nav_letters_small_src_tex_loc[0][d];
					g_src_rect.y = g_nav_letters_small_src_tex_loc[1][d];
					g_src_rect.w = g_nav_letters_small_src_tex_loc[2][d];
					g_src_rect.h = g_nav_letters_small_src_tex_loc[3][d];

					SDL_Rect g_dst_rect;
					g_dst_rect.x = xpos + x_offset;
					g_dst_rect.y = ypos;
					g_dst_rect.w = g_src_rect.w;
					g_dst_rect.h = g_src_rect.h;

					SDL_RenderCopy(renderer, tex("Navgfx.bmp"), &g_src_rect, &g_dst_rect);

					x_offset+=g_src_rect.w + 2;				
				}
			}
		}		
	}
}

void draw_nav_digits (char *digits, int xpos, int ypos) {
	int x_offset = 0;

	for (int s=0;s<strlen (digits);s++) {

		char digit = digits[s];
		
		if (digit == 32) {
			x_offset+=7;
		}

		for (int d=0;d<11;d++) {

			if (digit == g_nav_numbers_ref[d]) {

				SDL_Rect g_src_rect;
				g_src_rect.x = g_nav_numbers_src_tex_loc[0][d];
				g_src_rect.y = g_nav_numbers_src_tex_loc[1][d];
				g_src_rect.w = g_nav_numbers_src_tex_loc[2][d];
				g_src_rect.h = g_nav_numbers_src_tex_loc[3][d];

				SDL_Rect g_dst_rect;
				g_dst_rect.x = xpos + x_offset;
				g_dst_rect.y = ypos;
				g_dst_rect.w = g_src_rect.w;
				g_dst_rect.h = g_src_rect.h;

				if (digit == '.') {
					g_dst_rect.y = ypos + 71;
				}

				SDL_RenderCopy(renderer, tex("Navgfx.bmp"), &g_src_rect, &g_dst_rect);

				x_offset+=g_src_rect.w + 2;				
			}

		}		
	}
}

void draw_nav_numbers (int num, int xpos, int ypos) {
	char sz_digit[5];
	memset (sz_digit, 0, 5);
	sprintf (sz_digit, "%d", num);

	draw_nav_digits (sz_digit, xpos, ypos);
}

void draw_nav_symbol (int sym, int xpos, int ypos)
{
	SDL_Rect g_src_rect;
	g_src_rect.x = g_nav_icons_src_tex_loc[0][sym];
	g_src_rect.y = g_nav_icons_src_tex_loc[1][sym];
	g_src_rect.w = g_nav_icons_src_tex_loc[2][sym];
	g_src_rect.h = g_nav_icons_src_tex_loc[3][sym];

	SDL_Rect g_dst_rect;
	g_dst_rect.x = xpos;
	g_dst_rect.y = ypos;
	g_dst_rect.w = g_src_rect.w;
	g_dst_rect.h = g_src_rect.h;

	SDL_RenderCopy(renderer, tex("Navgfx.bmp"), &g_src_rect, &g_dst_rect);
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
			SDL_Rect g_src_rect;
			g_src_rect.x = g_speed_src_tex_loc[0][d];
			g_src_rect.y = g_speed_src_tex_loc[1][d];
			g_src_rect.w = g_speed_src_tex_loc[2][d];
			g_src_rect.h = g_speed_src_tex_loc[3][d];

			if (position == 3) {
				spd_digit_three.w = g_speed_src_tex_loc[2][d];
				spd_digit_three.h = g_speed_src_tex_loc[3][d];
				SDL_RenderCopy(renderer, tex("Speednumbers.bmp"), &g_src_rect, &spd_digit_three);
			}

			if (position == 2) {
				spd_digit_two.w = g_speed_src_tex_loc[2][d];
				spd_digit_two.h = g_speed_src_tex_loc[3][d];
				SDL_RenderCopy(renderer, tex("Speednumbers.bmp"), &g_src_rect, &spd_digit_two);
			}

			if (position == 1) {
				spd_digit_one.w = g_speed_src_tex_loc[2][d];
				spd_digit_one.h = g_speed_src_tex_loc[3][d];
				SDL_RenderCopy(renderer, tex("Speednumbers.bmp"), &g_src_rect, &spd_digit_one);
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

void unpack_menu_message() {

	//S,300,0,0,0,0,0,0,0,0,0,0,0,0,E
	//M,006,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,44,N

	char sz_current[16];
	int message_number = 0;
	int current_pointer = 0;

	memset(sz_current, 0, 16);

	for (int c=0;c<strlen(g_sz_comms_msg);c++) {



		if (g_sz_comms_msg[c] == ',') {

			if (message_number == 1) { // Menu choice state
				choice_state = atoi (sz_current);
			}

			if (message_number == 2) { // Odo digit 1
				ododigit1 = atoi (sz_current);
			}
			if (message_number == 3) { // Odo digit 2
				ododigit2 = atoi (sz_current);
			}
			if (message_number == 4) { // Odo digit 3
				ododigit3 = atoi (sz_current);
			}
			if (message_number == 5) { // Odo digit 4
				ododigit4 = atoi (sz_current);
			}
			if (message_number == 6) { // Odo digit 5
				ododigit5 = atoi (sz_current);
			}
			if (message_number == 7) { // Odo digit 6
				ododigit6 = atoi (sz_current);
			}

			if (message_number == 8) { // Odo2 digit 1
				odo2digit1 = atoi (sz_current);
			}
			if (message_number == 9) { // Odo2 digit 2
				odo2digit2 = atoi (sz_current);
			}
			if (message_number == 10) { // Odo2 digit 3
				odo2digit3 = atoi (sz_current);
			}
			if (message_number == 11) { // Odo2 digit 4
				odo2digit4 = atoi (sz_current);
			}
			if (message_number == 12) { // Odo2 digit 5
				odo2digit5 = atoi (sz_current);
			}
			if (message_number == 13) { // Odo2 digit 6
				odo2digit6 = atoi (sz_current);
			}
			if (message_number == 14) { // Odo error
				odoerror = atoi (sz_current);
			}
			if (message_number == 15) { // Set time digit 0
				settimedigit0 = atoi (sz_current);
			}
			if (message_number == 16) { // Set time digit 1
				settimedigit1 = atoi (sz_current);
			}
			if (message_number == 17) { // Set time digit 2
				settimedigit2 = atoi (sz_current);
			}
			if (message_number == 18) { // Set time digit 3
				settimedigit3 = atoi (sz_current);
			}
			if (message_number == 19) { // Speed correction digit 0 (+ or -)
				spcdigit0 = atoi (sz_current);
			}
			if (message_number == 20) { // Speed correction digit 1 
				spcdigit1 = atoi (sz_current);
			}
			if (message_number == 21) { // Speed correction digit 2 
				spcdigit2 = atoi (sz_current);
			}
			if (message_number == 22) { // Speed correction digit 3 
				spcdigit3 = atoi (sz_current);
			}
			if (message_number == 23) { // Front sprocket
				front_sprocket = atoi (sz_current);
			}
			if (message_number == 24) { // Rear sprocket
				rear_sprocket = atoi (sz_current);
			}
			if (message_number == 25) { // Coolant fan temp
				coolant_fan_temp = atoi (sz_current);
			}
			if (message_number == 26) { // Usingkm
				using_km = atoi (sz_current);
			}

			if (message_number == 27) { // Usingfh
				using_fh = atoi (sz_current);
			}

			if (message_number == 28) { // Usingbar
				using_bar = atoi (sz_current);
			}

			if (message_number == 29) { // Front sensor ID
				front_sensor_id = atoi (sz_current);
			}

			if (message_number == 30) { // Rear sensor ID
				rear_sensor_id = atoi (sz_current);
			}

			if (message_number == 31) { // Front pressure low setting
				front_pressure_low = atoi (sz_current);
			}

			if (message_number == 32) { // Rear pressure low setting
				rear_pressure_low = atoi (sz_current);
			}

			if (message_number == 33) { // Control layout
				control_layout = atoi (sz_current);
			}

			if (message_number == 34) { // Day Theme
				day_theme = atoi (sz_current);
			}

			if (message_number == 35) { // Night Theme
				night_theme = atoi (sz_current);
			}

			if (message_number == 36) { // Current light level
				current_light_level = atoi (sz_current);
			}

			if (message_number == 37) { // Light switch value
				light_switch_value = atoi (sz_current);
			}

			if (message_number == 38) { // Fan Neutral optin in coolant fan setup menu
				fan_neutral_option = atoi (sz_current);
			}

			if (message_number == 39) { // Gear Ratio Interval setting
				gear_ratio_interval = atoi (sz_current);
			}
			
			//fprintf(stderr, "%s\n", sz_current);
			memset(sz_current, 0, 16);
			current_pointer = 0;
			message_number++;

		} else {
			if (current_pointer < 16) {
				sz_current[current_pointer] = g_sz_comms_msg[c];
				current_pointer++;
			}
		}

	}

}

void unpack_nav_message () {

	// Just a routine to unpack the delimited Nav message handed to us via BLE from a Phone
	char sz_current[255];
	int message_number = 0;
	int current_pointer = 0;

	memset (sz_current, 0, 255);

	for (int c=0;c<strlen (str_nav);c++) {

		if (str_nav[c] == '%') {

			if (message_number == 1) { // Nav Symbol
				//fprintf(stderr, "Nav MSG 1: %s\n", sz_current);
				memset (str_nav_symbol, 0, 16);
				strcpy (str_nav_symbol, sz_current);
			}

			if (message_number == 2) { // Onto Roadname
				//fprintf(stderr, "Nav MSG 2: %s\n", sz_current);
				memset (str_nav_road, 0, 255);
				strcpy (str_nav_road, sz_current);
			}

			if (message_number == 3) { // Towards Placename
				//fprintf(stderr, "Nav MSG 3: %s\n", sz_current);
				memset (str_nav_towards, 0, 255);
				strcpy (str_nav_towards, sz_current);
			}

			if (message_number == 4) { // Motorway Exit Number
				//fprintf(stderr, "Nav MSG 4: %s\n", sz_current);
				memset (str_nav_exit, 0, 16);
				strcpy (str_nav_exit, sz_current);
			}

			if (message_number == 5) { // Yards Away
				//fprintf(stderr, "Nav MSG 5: %s\n", sz_current);
				memset (str_nav_yards, 0, 16);
				strcpy (str_nav_yards, sz_current);
				nav_yards = atoi (str_nav_yards);

				memset (str_nav_metres, 0, 16);
				double nav_meters = (double)nav_yards / 1.094;
				sprintf( str_nav_metres, "%d", (int)nav_meters);				
			}			

			if (message_number == 6) { // Miles Away 
				//fprintf(stderr, "Nav MSG 6: %s\n", sz_current);
				memset (str_nav_miles, 0, 16);
				strcpy (str_nav_miles, sz_current);
				nav_miles = atof (str_nav_miles);

				memset (str_nav_km, 0, 16);
				double nav_km_local = nav_miles * 1.609;
				sprintf( str_nav_km, "%.1f", nav_km_local);
			}

			if (message_number == 7) { // Driving on the left
				if (atoi (sz_current) == 1) {
					driving_left = 1;
				} else {
					driving_left = 0;
				}
			}

			if (message_number == 8) { // Distance to Destination
				memset (str_nav_dest_miles, 0, 16);
				strcpy (str_nav_dest_miles, sz_current);
				nav_dest_distance = atof (str_nav_dest_miles);

				memset (str_nav_dest_km, 0, 16);
				double nav_dest_km = nav_dest_distance * 1.609;
				sprintf( str_nav_dest_km, "%.1f", nav_dest_km);
			}

			memset(sz_current, 0, 255);
			current_pointer = 0;
			message_number++;

		} else {
			if (current_pointer < 255) {
				sz_current[current_pointer] = str_nav[c];
				current_pointer++;
			}
		}

	}

}

void unpack_message_old () {

	//char g_sz_comms_msg[255];
	//g_sz_comms_msg[255];
	//strcpy (g_sz_comms_msg, "S,078,2244,3456,23456,56,10,100,40,0,5000,12.5,9.2,47,21,0,0,0,0,0,0,0,200,0,1,4,2,0,9,9,2");

	//    OK                   OK                 OK    OK         OK      OKOKOKOKOK
	//    SPD T1   T2   ODO    T   MP  RNG  MX  K RPM   BAT  SPD   FUE TMP N O L R H I T CH  1 2 3 4 1 2 3 4
	// "S,068,1234,3456,023456,056,010,0100,040,0,05000,12.5,009.2,047,021,0,0,0,0,0,0,0,200,0,1,4,2,0,9,9,2,E"
	// "S,079,12500,E"

	// Lights order: neutrallight, oillight, highbeamlight, leftlight, rightlight
	// "S,079,12500,100,12.5,14:23,477,0,0,0,0,0,200,E"
	// 

	//S,000,00000,0069,0.00,09:58,007,0,1,0,0,0,000,0,E
	//S,000,00000,-099,0.00,11:26,011,0,1,0,0,0,000,0,   0.0,E
	//S,000,00000,-099,0.00,11:32,010,0,1,0,0,0,000,0,   0.0,   0.0,E

/*

	if (neutral) {
	if (oil_warning) {
	if (indicate_right) {
	if (indicate_left) {
	if (high_beam)	
*/


	char sz_current[255];
	int message_number = 0;
	int current_pointer = 0;

	memset(sz_current, 0, 255);

	for (int c=0;c<strlen(g_sz_comms_msg);c++) {



		if (g_sz_comms_msg[c] == ',') {

			if (message_number == 1) { // Speed
				current_speed = atoi (sz_current);
			}

			if (message_number == 2) { // Rpm
				rpm = atoi (sz_current);
			}

			if (message_number == 3) { // Coolant temp
				coolant_temp = atoi (sz_current);
			}

			if (message_number == 4) { // Battery voltage
				batt = atof (sz_current);
			}

			if (message_number == 5) { // Current time - real time clock
				memset (str_time, 0, 16);
				strcpy (str_time, sz_current);
			}

			if (message_number == 6) { // Fuel value - float level
				fuel_float = atoi (sz_current);
			}

			if (message_number == 7) { // Neutral Light
				if (atoi (sz_current) == 1) {
					neutral = true;
				} else {
					neutral = false;
				}
			}

			if (message_number == 8) { // Oil Light
				if (atoi (sz_current) == 1) {
					oil_warning = true;
				} else {
					oil_warning = false;
				}
			}

			if (message_number == 9) { // high_beam Light
				if (atoi (sz_current) == 1) {
					high_beam = true;
				} else {
					high_beam = false;
				}
			}

			if (message_number == 10) { // indicate_left Light
				if (atoi (sz_current) == 1) {
					indicate_left = true;
				} else {
					indicate_left = false;
				}
			}

			if (message_number == 11) { // indicate_right Light
				if (atoi (sz_current) == 1) {
					indicate_right = true;
				} else {
					indicate_right = false;
				}
			}
		
			if (message_number == 12) { // Choice state - menu options
				choice_state = atoi (sz_current);
			}

			if (message_number == 13) { // Info Mode - menu options
				info_mode = atoi (sz_current);
			}

			if (message_number == 14) { // Trip 1
				trip1 = atof (sz_current);
			}

			if (message_number == 15) { // Trip 2
				trip2 = atof (sz_current);
			}

			if (message_number == 16) { // Odo
				odo = atof (sz_current);
			}

			if (message_number == 17) { // KM / Miles
				using_km = atoi (sz_current);
			}

			if (message_number == 18) { // Speed correction value
				spd_correct = atof (sz_current);
			}

			if (message_number == 19) { // Speed correction value
				theme = atoi (sz_current);
			}

			if (message_number == 20) { // Ambient temp
				ambient_temp = atoi (sz_current);
			}

			if (message_number == 21) { // Current gear (gear indicator)
				current_gear = atoi (sz_current);
			}

			if (message_number == 22) { // MPG
				mpg = atoi (sz_current);
			}

			if (message_number == 23) { // Range
				range = atoi (sz_current);
			}

			if (message_number == 24) { // MaxSpeed
				max_speed = atoi (sz_current);
			}

			if (message_number == 25) { // Trip time hour
				trip_time_hour = atoi (sz_current);
			}

			if (message_number == 26) { // Trip time min
				trip_time_min = atoi (sz_current);
			}

			if (message_number == 27) { // Oil pressure available (1 or 0)
				if (atoi (sz_current) == 1) {
					oil_pressure_available = true;
				} else {
					oil_pressure_available = false;
				}
			}

			if (message_number == 28) { // Oil pressure in ohms
				oil_pressure_ohms = atoi (sz_current);
			}

			if (message_number == 29) { // Oil temp in ohms
				oil_temp_ohms = atoi (sz_current);
			}

			memset (str_trip_time, 0, 16);
			if (trip_time_min < 10) {
				sprintf (str_trip_time, "%d:0%d", trip_time_hour, trip_time_min);			
			} else {
				sprintf (str_trip_time, "%d:%d", trip_time_hour, trip_time_min);			
			}
			

			if (message_number == 30) { // Fahrenheit or Celcius
				using_fh = atoi (sz_current);
			}

			if (message_number == 31) { // Bar or PSI
				using_bar = atoi (sz_current);
			}

			if (message_number == 32) { // Front sensor ID
				front_sensor_id = atoi (sz_current);
			}

			if (message_number == 33) { // Rear sensor ID
				rear_sensor_id = atoi (sz_current);
			}

			if (message_number == 34) { // Front pressure low setting
				front_pressure_low = atoi (sz_current);
			}

			if (message_number == 35) { // Rear pressure low setting
				rear_pressure_low = atoi (sz_current);				
			}

			if (message_number == 36) { // Nav Message from Phone
				memset (str_nav, 0, 255);
				strcpy (str_nav, sz_current);

				if (strlen (str_nav) > 0) {
					//fprintf(stderr, "%s\n", str_nav);
					unpack_nav_message ();
					nav_active = true;
				}

				
				//fprintf(stderr, "%i\n", strlen (str_nav));
				//printf(sz_current);
				//printf (strlen (str_nav));
				//rear_pressure_low = atoi (sz_current);

				//fprintf(stderr, sz_current); 
			}

			//fprintf(stderr, "%s\n", sz_current);
			memset(sz_current, 0, 255);
			current_pointer = 0;


			message_number++;



		} else {
			if (current_pointer < 255) {
				sz_current[current_pointer] = g_sz_comms_msg[c];
				current_pointer++;
			}
		}

	}

}

void unpack_message() {
	// Use new parser module - cleaner and faster than old if-ladder
	dashboard_state state = {0};

	if (!parse_live_message(g_sz_comms_msg, &state)) {
		fprintf(stderr, "Failed to parse live message\n");
		return;
	}

	// Copy parsed values to globals
	current_speed = state.current_speed;
	rpm = state.rpm;
	coolant_temp = state.coolant_temp;
	batt = state.batt;
	// Note: currenthour/currentminute don't exist as globals - we build str_time instead
	fuel_float = state.fuel_float;
	neutral = state.neutral;
	oil_warning = state.oil_warning;
	high_beam = state.high_beam;
	indicate_left = state.indicate_left;
	indicate_right = state.indicate_right;
	choice_state = state.choice_state;
	info_mode = state.info_mode;
	trip1 = state.trip1;
	trip2 = state.trip2;
	odo = state.odo;
	using_km = state.using_km;
	spd_correct = state.spd_correct;
	theme = state.theme;
	ambient_temp = state.ambient_temp;
	current_gear = state.current_gear;
	mpg = state.mpg;
	range = state.range;
	max_speed = state.max_speed;
	trip_time_hour = state.trip_time_hour;
	trip_time_min = state.trip_time_min;
	oil_pressure_available = state.oil_pressure_available;
	oil_pressure_ohms = state.oil_pressure_ohms;
	oil_temp_ohms = state.oil_temp_ohms;
	using_fh = state.using_fh;
	using_bar = state.using_bar;
	front_sensor_id = state.front_sensor_id;
	rear_sensor_id = state.rear_sensor_id;
	front_pressure_low = state.front_pressure_low;
	rear_pressure_low = state.rear_pressure_low;

	// Build time string from hour/minute
	memset(str_time, 0, 16);
	sprintf(str_time, "%.2d:%.2d", state.current_hour, state.current_minute);

	// Build trip time string
	memset(str_trip_time, 0, 16);
	if (trip_time_min < 10) {
		sprintf(str_trip_time, "%d:0%d", trip_time_hour, trip_time_min);
	} else {
		sprintf(str_trip_time, "%d:%d", trip_time_hour, trip_time_min);
	}

	// Handle navigation message
	if (strlen(state.nav_string) > 0) {
		memset(str_nav, 0, 255);
		strcpy(str_nav, state.nav_string);
		unpack_nav_message();
		nav_active = true;
	}
}


int connect_interface ()
{
// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)
// Priority: Check simulator pipe first, then real hardware

	// Try simulator pipe first (for development/testing with tft-dash-simulator)
	if (file_exists ("/tmp/tft-dash-pipe")) {
		fprintf(stderr, "Connecting to simulator pipe...\n");
		serial_port = open("/tmp/tft-dash-pipe", O_RDONLY);
		if (serial_port >= 0) {
			fprintf(stderr, "Simulator connected!\n");
			// No termios setup needed for pipe - just read raw data
			return 0;
		}
	}

	// Try real hardware serial ports
	if (file_exists ("/dev/cu.usbserial-1430")) {
		serial_port = open("/dev/cu.usbserial-1430", O_RDWR); // Nano board connected!
	} else {
		if (file_exists ("/dev/cu.usbmodem14301")) {
			serial_port = open("/dev/cu.usbmodem14301", O_RDWR);
		} else {
			if (file_exists ("/dev/cu.usbmodem1101")) {
				//fprintf(stderr, "Bike interface connected!");
				serial_port = open("/dev/cu.usbmodem1101", O_RDWR);
			} else {

				if (file_exists ("/dev/ttyACM0")) {
					//fprintf(stderr, "Bike interface connected!");
					serial_port = open("/dev/ttyACM0", O_RDWR);
				} else {
					if (file_exists ("/dev/ttyACM1")) {
						//fprintf(stderr, "Bike interface connected!");
						serial_port = open("/dev/ttyACM1", O_RDWR);
					} else {
						//fprintf(stderr, "Bike interface NOT connected!");
						return 0;
					}
				}
			}
		}
	}



	

	// Create new termios struc, we call it 'tty' for convention
	struct termios tty;
	memset(&tty, 0, sizeof(tty));

	// Read in existing settings, and handle any error
	if(tcgetattr(serial_port, &tty) != 0) {
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
	cfsetispeed(&tty, B115200);
	cfsetospeed(&tty, B115200);

	// Save tty settings, also checking for error
	if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
	    //printf("Error %i from tcsetattr: %s\n", errno, strerror(errno));
	}

	return 0;
	// Write to serial port
	//unsigned char msg[] = { 'H', 'e', 'l', 'l', 'o', '\r' };
	//write(serial_port, "Hello, world!", sizeof(msg));

	// Allocate memory for read buffer, set size according to your needs
}


void* pollInterface(void *arg)
{
	//unpack_message();
/*
	bool x = true;
	int i = 0;
	while (x == true) {
		
		i++;
		//fprintf(stderr, "Thread is running"); 
		sprintf(g_sz_comms_msg, "Thread count is: %d", i);
	}
	return 0;
	*/

	char appendbuf [1024];
	
	char read_buf [1024];
	bool quit = false;
	int appendpointer = 0;

	connect_interface();

	while (quit == false) {
		memset(&read_buf, '\0', sizeof(read_buf));

		// Read bytes. The behaviour of read() (e.g. does it block?,
		// how long does it block for?) depends on the configuration
		// settings above, specifically VMIN and VTIME
		//printf ("R..");
		int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		if (num_bytes <= 0) {
		    //printf("Error reading: %s", strerror(errno));
		    close (serial_port);
		    connect_interface();
		}

		// Here we assume we received ASCII data, but you might be sending raw bytes (in that case, don't try and
		// print it to the screen like this!)
		//printf("Read %i bytes. Received message: %s", num_bytes, read_buf);

		//printf("%s", read_buf);

		for (int r=0;r<num_bytes;r++) {
			// S marks the beginning of a live data message, and M marks the beginning of a menu data message
			if (read_buf[r] == '{' || read_buf[r] == '[') {
				appendpointer = 0;
				memset (appendbuf, 0, 1024);
				appendbuf[appendpointer] = read_buf[r];
				appendpointer++;
			} else {
				if (appendpointer < 1024) {
					appendbuf[appendpointer] = read_buf[r];
					appendpointer++;
				}
			}

		}

		//if (appendbuf[0] == 'S' && strlen(appendbuf) == 107 && appendbuf[106] == 'E') {
		if (appendbuf[0] == '{' && strlen(appendbuf) > 90 && strlen (appendbuf) < 400) {
			//printf("%s\n", appendbuf);
			memset (g_sz_comms_msg, 0, 1024);
			strcpy (g_sz_comms_msg, appendbuf);

			//fprintf(stderr, "Data: %s\n", g_sz_comms_msg);

			unpack_message();
		} else {
			//if (appendbuf[0] == 'M' && strlen(appendbuf) == 59 && appendbuf[58] == 'N') {
			if (appendbuf[0] == '[' && strlen(appendbuf) > 78 && strlen(appendbuf) < 150) {

				memset (g_sz_comms_msg, 0, 1024);
				strcpy (g_sz_comms_msg, appendbuf);
				unpack_menu_message();
			}
			//fprintf(stderr, "NO Messages received! Buffer length: %i",(int)strlen(appendbuf)); 
		}

		//usleep (1000);	
	}
	

	close(serial_port);

	return 0;
}

int connect_tpms_interface ()
{
// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)

	if (file_exists ("/dev/cu.usbserial-D3077502")) {
		tpms_serial_port = open("/dev/cu.usbserial-D3077502", O_RDWR); // Nano board connected!
		//cu.usbserial-D3077502
		fprintf(stderr, "TPMS interface connected!"); 
		tpms_connected= true;
	} else {
		if (file_exists ("/dev/ttyUSB0")) {		
			fprintf(stderr, "TPMS interface connected!"); 
			tpms_serial_port = open("/dev/ttyUSB0", O_RDWR);
			tpms_connected= true;
		} else {

			if (file_exists ("/dev/ttyUSB1")) {
				fprintf(stderr, "TPMS interface connected!"); 
				tpms_serial_port = open("/dev/ttyUSB1", O_RDWR);
				tpms_connected= true;
			} else {

				if (file_exists ("/dev/cu.usbserial-210")) {
					fprintf(stderr, "Attempt 210 connect....");
					tpms_serial_port = open("/dev/cu.usbserial-210", O_RDWR);
					//tty.usbserial-1420
					fprintf(stderr, "TPMS interface connected!"); 
					tpms_connected= true;
				} else {
					fprintf(stderr, "TPMS interface NOT connected!"); 
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

			if (sensor_num == front_sensor_id) {
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

			if (sensor_num == rear_sensor_id) {
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

			if (sensor_num == front_sensor_id) {
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

			if (sensor_num == rear_sensor_id) {
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

int render_texture (SDL_Texture* tex, int x, int y, int w, int h) 
{
	SDL_Rect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	return SDL_RenderCopy(renderer, tex, nullptr, &dst_rect);
}

int render_top_icon_grey_texture (int x, int y, int w, int h) 
{
	SDL_Rect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	if (oil_pressure_available) {
		return SDL_RenderCopy(renderer, tex("TopiconsgreyOP.bmp"), nullptr, &dst_rect);
	} else {
		return SDL_RenderCopy(renderer, tex("Topiconsgrey.bmp"), nullptr, &dst_rect);
	}
}

int render_oil_light_texture (int x, int y, int w, int h)
{
	SDL_Rect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	if (oil_pressure_available) {
		return SDL_RenderCopy(renderer, tex("OillightOP.bmp"), nullptr, &dst_rect);
	} else {
		return SDL_RenderCopy(renderer, tex("Oillight.bmp"), nullptr, &dst_rect);
	}
}

void run_rpm_test()
{
	if (reverse == false) {
		rpm+=100;
	}
	else {
		rpm-=100;
	}

	if (rpm > 13000) {
		reverse = true;		
	}

	if (rpm < 1) {
		reverse = false;
	}
}

void run_partial_test() {
	
	if (reverse == false) {
		test_counter++;
	}
	else {
		test_counter--;
	}

	if (test_counter > 100) {
		reverse = true;
		
	}

	if (test_counter < 1) {
		reverse = false;
		
	}

	//fuelwarning = true;

	indicate_left = reverse;
	indicate_right = reverse;
	//bool indicate_left = false;
	//bool indicate_right = false;


}

void run_full_test_scenarios () {
		if (reverse == false) {
			spin_angle++;
		}
		else {
			spin_angle--;
		}

		if (spin_angle > 100) {
			reverse = true;
			ncount++;
		}

		if (spin_angle < 1) {
			reverse = false;
			ncount++;
		}
		

		if (reverse == false) {
				indicate_left = false;
			//if ((spin_angle % 10) == 0) {
				if (current_speed < 200) {
					current_speed+=2;	
					max_speed+=2;
					trip1+=2;	
					trip2+=3;
					odo+=14;
					coolant_temp++;
					mpg++;
					range++;
					rpm+=100;
				}	

				if ((spin_angle % 30) == 0) {
					neutral = !neutral;
					high_beam = !high_beam;
				}			

				if ((spin_angle % 40) == 0) {
					oil_warning = !oil_warning;
				}		

				if ((spin_angle % 20) == 0) {
					indicate_right = !indicate_right;
					//indicate_left = !indicate_right;
				}		
			//}			
		} else {
			indicate_right = false;
			//if ((spin_angle % 10) == 0) {
				if (current_speed > 0) {
					current_speed-=2;	
					max_speed-=2;
					trip1-=2;
					trip2-=3;
					odo-=14;
					coolant_temp--;
					mpg--;
					range--;
					rpm-=100;
				}	

				if ((spin_angle % 30) == 0) {
					neutral = !neutral;
					high_beam = !high_beam;
				}

				if ((spin_angle % 40) == 0) {
					oil_warning = !oil_warning;
				}		

				if ((spin_angle % 20) == 0) {
					indicate_left = !indicate_left;					
				}							
			//}				
			
		}


		if (ncount > 13) {
			ncount = 0;
		}

		if (ncount == 1 || ncount == 2 || ncount == 3) {
			info_mode = 1;				
		}

		if (ncount == 4 || ncount == 5 || ncount == 6) {
			info_mode = 2;				
		}

		if (ncount == 7 || ncount == 8 || ncount == 9) {
			info_mode = 3;				
		}

		if (ncount == 10 || ncount == 11 || ncount == 12 || ncount == 13) {
			info_mode = 0;				
		}
}

void draw_menu (int state) {
	if (theme == 0 || theme == 7) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	}

	SDL_RenderClear(renderer);

	// COOLANT FAN TEMP MENU
	if (state >= 500 && state < 590) {render_texture (tex("Coolantfantemp.bmp"), 0, 0, 1024, 600);}

	if (state >= 500 && state < 590) {
		if (state == 500) {render_texture (tex("Uparrowsmall.bmp"), 115, 185, 60, 62);}
		if (state == 510) {render_texture (tex("Uparrowsmall.bmp"), 341, 173, 60, 62);}
		if (state == 520) {render_texture (tex("Uparrowsmall.bmp"), 625, 173, 60, 62);}
		if (state == 530) {render_texture (tex("Leftmenuarrowex.bmp"), 667, 300, 56, 50);}
		if (state == 540) {render_texture (tex("Leftmenuarrowex.bmp"), 667, 378, 56, 50);}
		if (state == 550) {render_texture (tex("Leftmenuarrowex.bmp"), 667, 454, 56, 50);}
		if (state == 560) {render_texture (tex("Leftmenuarrowex.bmp"), 667, 532, 56, 50);}
		//tex("Leftmenuarrowex.bmp")

		if (state == 570) {render_texture (tex("Uparrowsmall.bmp"), 884, 534, 60, 62);}

		draw_medium_num (coolant_fan_temp, 467, 114);

		if (fan_neutral_option == 0) {
			render_texture (tex("Selecton.bmp"), 741, 306, 55, 38); // Coolant temp fan always on in Neutral
		}

		if (fan_neutral_option == 1) {
			render_texture (tex("Selecton.bmp"), 741, 383, 55, 38); // Coolant temp fan on after 1 minute
		}

		if (fan_neutral_option == 2) {
			render_texture (tex("Selecton.bmp"), 741, 460, 55, 38); // Coolant temp fan on after 50 degrees engine temp
		}

		if (fan_neutral_option == 3) {
			render_texture (tex("Selecton.bmp"), 741, 537, 55, 38); // Coolant temp fan on after throttle blip
		}
	}

	// SPROCKET SETUP MENU
	if (state >= 400 && state < 490) {render_texture (tex("Sprocketsetup.bmp"), 0, 0, 1024, 600);}
	
	if (state >= 400 && state < 490) {
		if (state == 400) {render_texture (tex("Uparrowsmall.bmp"), 113, 296, 60, 62);}
		if (state == 410) {render_texture (tex("Uparrowsmall.bmp"), 286, 300, 60, 62);}
		if (state == 420) {render_texture (tex("Uparrowsmall.bmp"), 373, 300, 60, 62);}
		if (state == 430) {render_texture (tex("Uparrowsmall.bmp"), 590, 300, 60, 62);}
		if (state == 440) {render_texture (tex("Uparrowsmall.bmp"), 678, 300, 60, 62);}
		if (state == 450) {render_texture (tex("Uparrowsmall.bmp"), 594, 485, 60, 62);}
		if (state == 460) {render_texture (tex("Uparrowsmall.bmp"), 861, 485, 60, 62);}
		if (state == 470) {render_texture (tex("Uparrowsmall.bmp"), 848, 300, 60, 62);}

		draw_medium_num (front_sprocket, 332, 127);
		draw_medium_num (rear_sprocket, 638, 127);
		draw_medium_num (gear_ratio_interval, 715, 415);

	}


	// SET ODOMETER MENU
	if (state >= 300 && state < 390) {render_texture (tex_from("default", "Setodometer.bmp"), 0, 0, 1024, 600);}

	// Arrow positions for odometer first line
	if (state == 300) {render_texture (tex("Uparrow.bmp"), 175, 279, 104, 107);}
	if (state == 305) {render_texture (tex("Uparrow.bmp"), 286, 279, 104, 107);}
	if (state == 310) {render_texture (tex("Uparrow.bmp"), 399, 279, 104, 107);}
	if (state == 315) {render_texture (tex("Uparrow.bmp"), 510, 279, 104, 107);}
	if (state == 320) {render_texture (tex("Uparrow.bmp"), 620, 279, 104, 107);}
	if (state == 325) {render_texture (tex("Uparrow.bmp"), 733, 279, 104, 107);}
	// Arrow positions for odometer second line
	if (state == 330) {render_texture (tex("Uparrow.bmp"), 175, 493, 104, 107);}
	if (state == 335) {render_texture (tex("Uparrow.bmp"), 286, 493, 104, 107);}
	if (state == 340) {render_texture (tex("Uparrow.bmp"), 399, 493, 104, 107);}
	if (state == 345) {render_texture (tex("Uparrow.bmp"), 510, 493, 104, 107);}
	if (state == 350) {render_texture (tex("Uparrow.bmp"), 620, 493, 104, 107);}
	if (state == 355) {render_texture (tex("Uparrow.bmp"), 733, 493, 104, 107);}
	if (state == 360) {render_texture (tex("Uparrow.bmp"), 877, 493, 104, 107);} // Ok button

	// Odo digits
	if (state >= 300 && state < 390) {
		int ododigitspacing = 111;
		draw_medium_num (ododigit1, 214, 209);
		draw_medium_num (ododigit2, 325, 209);
		draw_medium_num (ododigit3, 325+(ododigitspacing*1), 209);
		draw_medium_num (ododigit4, 325+(ododigitspacing*2), 209);
		draw_medium_num (ododigit5, 325+(ododigitspacing*3), 209);
		draw_medium_num (ododigit6, 325+(ododigitspacing*4), 209);
		// Second row odo digits
		draw_medium_num (odo2digit1, 214, 415);
		draw_medium_num (odo2digit2, 325, 415);
		draw_medium_num (odo2digit3, 325+(ododigitspacing*1), 415);
		draw_medium_num (odo2digit4, 325+(ododigitspacing*2), 415);
		draw_medium_num (odo2digit5, 325+(ododigitspacing*3), 415);
		draw_medium_num (odo2digit6, 325+(ododigitspacing*4), 415);	
	}		

	// If the odo error flag has been set then show the error
	if (odoerror == 1) {
		render_texture (tex_from("default", "Odoerror1.bmp"), 130, 524, 758, 45);
	}
	if (odoerror == 2) {
		render_texture (tex_from("default", "Odoerror2.bmp"), 130, 524, 758, 45);	
	}

	// THEME MENU
	if (state >= 200 && state < 290) {
		render_texture (tex_from("default", "Themeoptions.bmp"), 0, 0, 1024, 600);
	}

	if (state == 200) { // Default theme 0
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 8, 28, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 330, 29, 48, 159);
	}
	if (state == 210) { // theme 1
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 330, 28, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 652, 29, 48, 159);		
	}
	if (state == 220) { // theme 2
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 660, 28, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 982, 29, 48, 159);		
	}
	if (state == 230) { // theme 3
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 3, 221, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 325, 221, 48, 159);		
	}
	if (state == 240) { // theme 4
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 331, 221, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 653, 221, 48, 159);		
	}
	if (state == 250) { // theme 5
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 661, 221, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 983, 221, 48, 159);		
	}
	if (state == 260) { // theme 6
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 8, 423, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 330, 423, 48, 159);		
	}

	if (state == 270) { // theme 7
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 328, 423, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 650, 423, 48, 159);		
	}

	if (state == 280) { // theme 8
		render_texture (tex_from("default", "Arrowlefttheme.bmp"), 653, 423, 48, 159);
		render_texture (tex_from("default", "Arrowrighttheme.bmp"), 982, 423, 48, 159);		
	}


	// SPEED CORRECTION MENU
	if (state >= 100 && state < 170) {
		render_texture (tex("Speedcorrection.bmp"), 0, 0, 1024, 600);

		if (spcdigit0 == 0) {
			strcpy (str_spc_digit0, "-");
		}

		if (spcdigit0 == 1) {
			strcpy (str_spc_digit0, "+");
		}

		sprintf(str_spc_digit1, "%d", spcdigit1);
		sprintf(str_spc_digit2, "%d", spcdigit2);
		sprintf(str_spc_digit3, "%d", spcdigit3);

		draw_medium_string (str_spc_digit0, 294, 282);
		draw_medium_string (str_spc_digit1, 408, 277);
		draw_medium_string (str_spc_digit2, 516, 277);
		draw_medium_string (str_spc_digit3, 693, 277);
	}

	if (state == 100) { // Speed correction cancel
		render_texture (tex("Uparrow.bmp"), 83, 352, 104, 107);
	}

	if (state == 110) { // Speed correction digit 1
		render_texture (tex("Uparrow.bmp"), 262, 352, 104, 107);
	}

	if (state == 120) { // Speed correction digit 2
		render_texture (tex("Uparrow.bmp"), 372, 352, 104, 107);
	}

	if (state == 130) { // Speed correction digit 3
		render_texture (tex("Uparrow.bmp"), 477, 352, 104, 107);
	}

	if (state == 140) { // Speed correction digit 4
		render_texture (tex("Uparrow.bmp"), 657, 352, 104, 107);
	}

	if (state == 150) { // Speed correction ok
		render_texture (tex("Uparrow.bmp"), 824, 352, 104, 107);
	}

	// SET TPMS MENU
	if (state >= 700 && state < 790) { // Set TPMS menu options

		render_texture (tex("TPMSsetup.bmp"), 0, 0, 1024, 600);
		
		sprintf(str_front_sensor_id, "%d", front_sensor_id);
		sprintf(str_rear_sensor_id, "%d", rear_sensor_id);
		sprintf(str_front_pressure_low, "%d", front_pressure_low);
		sprintf(str_rear_pressure_low, "%d", rear_pressure_low);

		draw_medium_string (str_front_sensor_id, 527, 170);
		draw_medium_string (str_rear_sensor_id, 832, 170);
		draw_medium_string (str_front_pressure_low, 830, 288);
		draw_medium_string (str_rear_pressure_low, 830, 405);

		if (state == 700) { // TPMS menu cancel
			render_texture (tex("Uparrowsmall.bmp"), 65, 182, 60, 62);
		}				

		if (state == 705) { // Front sensor +
			render_texture (tex("Uparrowsmall.bmp"), 420, 220, 60, 62);
		}

		if (state == 710) { // Front sensor -
			render_texture (tex("Uparrowsmall.bmp"), 610, 220, 60, 62);
		}

		if (state == 715) { // Rear sensor +
			render_texture (tex("Uparrowsmall.bmp"), 726, 220, 60, 62);
		}

		if (state == 720) { // Rear sensor -
			render_texture (tex("Uparrowsmall.bmp"), 917, 220, 60, 62);
		}

		if (state == 725) { // Front pressure warning +
			render_texture (tex("Uparrowsmall.bmp"), 727, 337, 60, 62);
		}

		if (state == 730) { // Front pressure warning -
			render_texture (tex("Uparrowsmall.bmp"), 918, 337, 60, 62);
		}

		if (state == 735) { // Rear pressure warning  +
			render_texture (tex("Uparrowsmall.bmp"), 728, 456, 60, 62);
		}

		if (state == 740) { // Rear pressure warning -
			render_texture (tex("Uparrowsmall.bmp"), 918, 456, 60, 62);
		}

		if (state == 745) { // TPMS setup OK
			render_texture (tex("Uparrowsmall.bmp"), 65, 528, 60, 62);
		}
	}

	// SET CONTROL MENU
	if (state >= 800 && state < 830) { // Set control options
		render_texture (tex("Controloptions.bmp"), 0, 0, 1024, 600);	

		if (control_layout == 0) {
			render_texture (tex("Selectedcontrol.bmp"), 230, 195, 236, 30);
		}

		if (control_layout == 1) {
			render_texture (tex("Selectedcontrol.bmp"), 555, 195, 236, 30);
		}

		if (state == 800) { // Control cancel
			render_texture (tex("Uparrowsmall.bmp"), 69, 363, 60, 62);
		}

		if (state == 805) { // Control layout 1
			render_texture (tex("Uparrowsmall.bmp"), 316, 394, 60, 62);
		}

		if (state == 810) { // Control layout 2
			render_texture (tex("Uparrowsmall.bmp"), 648, 394, 60, 62);
		}

		if (state == 815) { // OK
			render_texture (tex("Uparrowsmall.bmp"), 890, 363, 60, 62);
		}
	}


	// LIGHT SETUP MENU
	if (state >= 900 && state < 950) { // Set light options		
		render_texture (tex("Lightoptions.bmp"), 0, 0, 1024, 600);	

		sprintf(str_light_switch_value, "%d", light_switch_value);
		sprintf(str_current_light_level, "%d", current_light_level);

		draw_medium_string (str_light_switch_value, 625, 477);
		draw_medium_string (str_current_light_level, 625, 396);

		if (day_theme == 0) {
			render_texture (tex_from("default", "whitethumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 1) {
			render_texture (tex_from("green", "greenthumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 2) {
			render_texture (tex_from("red", "redthumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 3) {
			render_texture (tex_from("blue", "bluethumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 4) {
			render_texture (tex_from("orange", "orangethumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 5) {
			render_texture (tex_from("yellow", "yellowthumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 6) {
			render_texture (tex_from("night", "nightthumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 7) {
			render_texture (tex_from("bright", "brightthumb.bmp"), 772, 118, 126, 74);
		}

		if (day_theme == 8) {
			render_texture (tex_from("dark", "darkthumb.bmp"), 772, 118, 126, 74);
		}

		if (night_theme == 0) {
			render_texture (tex_from("default", "whitethumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 1) {
			render_texture (tex_from("green", "greenthumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 2) {
			render_texture (tex_from("red", "redthumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 3) {
			render_texture (tex_from("blue", "bluethumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 4) {
			render_texture (tex_from("orange", "orangethumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 5) {
			render_texture (tex_from("yellow", "yellowthumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 6) {
			render_texture (tex_from("night", "nightthumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 7) {
			render_texture (tex_from("bright", "brightthumb.bmp"), 772, 236, 126, 74);
		}

		if (night_theme == 8) {
			render_texture (tex_from("dark", "darkthumb.bmp"), 772, 236, 126, 74);
		}


		if (state == 900) {
			render_texture (tex("Uparrowsmall.bmp"), 66, 207, 60, 62);
		}

		if (state == 905) {
			render_texture (tex("Uparrowsmall.bmp"), 708, 183, 60, 62);
		}

		if (state == 910) {
			render_texture (tex("Uparrowsmall.bmp"), 899, 183, 60, 62);
		}

		if (state == 915) {
			render_texture (tex("Uparrowsmall.bmp"), 708, 302, 60, 62);
		}

		if (state == 920) {
			render_texture (tex("Uparrowsmall.bmp"), 898, 302, 60, 62);
		}

		if (state == 925) {
			render_texture (tex("Uparrowsmall.bmp"), 531, 529, 60, 62);
		}

		if (state == 930) {
			render_texture (tex("Uparrowsmall.bmp"), 722, 529, 60, 62);
		}

		if (state == 935) {
			render_texture (tex("Uparrowsmall.bmp"), 890, 529, 60, 62);
		}		
	}

	// SET UNITS MENU
	if (state >= 600 && state < 690) { // Set units options
		render_texture (tex("Setunits.bmp"), 0, 0, 1024, 600);	

		if (using_km) {
			render_texture (tex("Selecton.bmp"), 894, 101, 55, 38);
		} else {
			render_texture (tex("Selecton.bmp"), 784, 101, 55, 38);
		}

		if (using_fh) {
			render_texture (tex("Selecton.bmp"), 894, 262, 55, 38);
		} else {	
			render_texture (tex("Selecton.bmp"), 784, 262, 55, 38);
		}

		if (using_bar) {
			render_texture (tex("Selecton.bmp"), 894, 423, 55, 38);
		} else {
			render_texture (tex("Selecton.bmp"), 784, 423, 55, 38);
		}
		

		if (state == 600) { // Set units cancel
			render_texture (tex("Uparrowsmall.bmp"), 34, 197, 60, 62);
		}

		if (state == 605) { // Set units digit 1
			render_texture (tex("Uparrowsmall.bmp"), 780, 154, 60, 62);
		}

		if (state == 610) { // Set units digit 2
			render_texture (tex("Uparrowsmall.bmp"), 891, 154, 60, 62);
		}

		if (state == 615) { // Set units digit 3
			render_texture (tex("Uparrowsmall.bmp"), 780, 314, 60, 62); 
		}

		if (state == 620) { // Set units digit 4
			render_texture (tex("Uparrowsmall.bmp"), 891, 314, 60, 62);
		}

		if (state == 625) { // Set units ok
			render_texture (tex("Uparrowsmall.bmp"), 780, 475, 60, 62);
		}

		if (state == 630) { // Set units ok
			render_texture (tex("Uparrowsmall.bmp"), 891, 475, 60, 62);
		}

		if (state == 635) { // Set units ok
			render_texture (tex("Uparrowsmall.bmp"), 35, 539, 60, 62);
		}
	}

	//SET TIME MENU
	if (state >= 20 && state < 90) { // Set time options
		render_texture (tex("Settime.bmp"), 0, 0, 1024, 600);	

		sprintf(str_set_time_digit0, "%d", settimedigit0);
		sprintf(str_set_time_digit1, "%d", settimedigit1);
		sprintf(str_set_time_digit2, "%d", settimedigit2);
		sprintf(str_set_time_digit3, "%d", settimedigit3);

		draw_medium_string (str_set_time_digit0, 300, 277);
		draw_medium_string (str_set_time_digit1, 408, 277);
		draw_medium_string (str_set_time_digit2, 583, 277);
		draw_medium_string (str_set_time_digit3, 693, 277);
	}

	if (state == 20) { // Set time cancel
		render_texture (tex("Uparrow.bmp"), 92, 355, 104, 107);
	}

	if (state == 30) { // Set time digit 1
		render_texture (tex("Uparrow.bmp"), 262, 355, 104, 107);
	}

	if (state == 40) { // Set time digit 2
		render_texture (tex("Uparrow.bmp"), 371, 355, 104, 107);
	}

	if (state == 50) { // Set time digit 3
		render_texture (tex("Uparrow.bmp"), 545, 355, 104, 107);
	}

	if (state == 60) { // Set time digit 4
		render_texture (tex("Uparrow.bmp"), 657, 355, 104, 107);
	}

	if (state == 70) { // Set time ok
		render_texture (tex("Uparrow.bmp"), 824, 355, 104, 107);
	}

	//MAIN MENU
	if (state > 0 && state < 20) {
		render_texture (tex("Menuoptionsex.bmp"), 0, 0, 1024, 600);	
	}

	int yarrowoffset = 51;
	if (state == 1) { // Reset trip 1
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94, 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 465, 94, 56, 50);
	}

	if (state == 2) { // Reset trip 2
		render_texture (tex("Leftmenuarrowex.bmp"), 505, 94, 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94, 56, 50);
	}

	if (state == 3) { // Control setup
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 465, 94+(yarrowoffset), 56, 50);
	}

	if (state == 4) { // Set units
		render_texture (tex("Leftmenuarrowex.bmp"), 505, 94+(yarrowoffset), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset), 56, 50);
	}

	if (state == 5) { // Set time
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset*2), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*2), 56, 50);
	}

	//if (state == 6) { // Accel timer - skipping this feature for the July August 2020 update due to time constraints
		//render_texture (tex("Leftmenuarrowex.bmp"), 505, 94+(yarrowoffset*2), 56, 50);
		//render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*2), 56, 50);
	//}

	if (state == 6) { // Ambient light setup
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset*3), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*3), 56, 50);
	}

	if (state == 7) { // Speed correction menu
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset*4), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*4), 56, 50);
	}

	if (state == 8) { // Set theme
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset*5), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*5), 56, 50);
	}

	if (state == 9) { // Gear indicator setup
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset*6), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*6), 56, 50);
	}

	if (state == 10) { // Coolant fan setup
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset*7), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*7), 56, 50);
	}

	if (state == 11) { // TPMS Setup
		render_texture (tex("Leftmenuarrowex.bmp"), 77, 94+(yarrowoffset*8), 56, 50);
		render_texture (tex("Rightmenuarrowex.bmp"), 897, 94+(yarrowoffset*8), 56, 50);
	}



	SDL_RenderPresent(renderer);
}

void dashboard_startup () {
	startup_anim_count++;


	if (theme == 0 || theme == 7) {

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

			if (info_mode == 0) {
				if (using_km) {
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

			if (info_mode == 0) {
				if (using_km) {
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
			SDL_RenderCopy(renderer, tex("R0.bmp"), nullptr, &rectg0);
		}
		revamountinc+=revinc;
		if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R1.bmp"), nullptr, &rectg1);
		}
		revamountinc+=revinc;
		if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R2.bmp"), nullptr, &rectg2);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R3.bmp"), nullptr, &rectg3);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R4.bmp"), nullptr, &rect_g4);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R5.bmp"), nullptr, &rect_g5);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R6.bmp"), nullptr, &rect_g6);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R7.bmp"), nullptr, &rect_g7);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R8.bmp"), nullptr, &rect_g8);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R9.bmp"), nullptr, &rect_g9);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R10.bmp"), nullptr, &rect_g10);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R11.bmp"), nullptr, &rect_g11);
		}
		revamountinc+=revinc;
			if (startup_anim_count > (100+revamountinc)) {
			SDL_RenderCopy(renderer, tex("R12-13.bmp"), nullptr, &rect_g1213);
		}
	}

	if (startup_anim_count > 500) {
		
		if (startup_anim_count < 1000) {
			// Fuel Gauge		
			render_texture (tex("Fuelgauge.bmp"), (676+500)-(startup_anim_count-500), 294, 316, 44);
			render_texture (tex("Fuelgaugewhite.bmp"), (714+(spin_angle*3)+500)-(startup_anim_count-500), 303, 274, 28);

			// Coolant Temp
			if (using_fh) {
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
			if (using_fh) {
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

	if (info_mode == 3 && currentinfomode == 2) {
		
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

	if (info_mode == 3 && currentinfomode == 1) {
		
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

	if (info_mode == 3 && currentinfomode == 0) {
		
		if (infoanimationreverse == false) {
			if (using_km) {
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

	if (info_mode == 2 && currentinfomode == 1) {
		
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

	if (info_mode == 1 && currentinfomode == 0) {
		
		if (infoanimationreverse == false) {
			if (using_km) {
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

	if (info_mode == 0 && currentinfomode == 3) {
		if (infoanimationreverse == false) {
			//render_texture(tex("Infotopdiag.bmp"), 0 - info_animation_count, 176, 510, 97);
			//render_texture(tex("Infobottomdiag.bmp"), 0 - info_animation_count, 273, 454, 125);
			render_texture(tex("Navbg.bmp"), 0 - info_animation_count, 158, 434, 268);
		}

		if (infoanimationreverse == true) {
			if (using_km) {
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

	if (info_mode == 0 && currentinfomode == 2) {
		
		// This one goes away
		if (infoanimationreverse == false) {
			render_texture(tex("tyretop.bmp"), 0-info_animation_count, 176, 510, 97);
			render_texture(tex("tyrebottom.bmp"), 0-info_animation_count, 273, 454, 125);
		}

		// This one comes in
		if (infoanimationreverse == true) {
			if (using_km) {
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

	if ((info_mode != currentinfomode && warningbadgeactive)) {		
		warningbadgecancelled = true;
		warningbadgeactive = false;
	}

	if (theme == 0 || theme == 7) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	}

	SDL_RenderClear(renderer);


	// Rev counter rev line texture
	SDL_RenderCopy(renderer, tex("Revline.bmp"), nullptr, &grevline);

	target_rpm_rotation = get_rpm_rotation (rpm);
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
	SDL_RenderCopyEx(renderer, tex("Whitesq.bmp"), nullptr, &grrevwhite, current_rpm_rotation , &gwhitepoint, SDL_FLIP_NONE);

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
			render_texture (tex("Fuelgaugewhite.bmp"), 714+get_fuel_reveal_from_bars (get_num_fuel_bars(fuel_float)), 303, 274, 28);
		}
	} else {
		render_texture (tex("Fuelgauge.bmp"), 676, 294, 316, 44);
		render_texture (tex("Fuelgaugewhite.bmp"), 714+get_fuel_reveal_from_bars (get_num_fuel_bars(fuel_float)), 303, 274, 28);
	}
	// Coolant Temp
	if (overheatwarning == true && enginerunning == true) {
		if (flash) {
			if (using_fh) {
				render_texture (tex("CoolantF.bmp"), 808, 196, 209, 80);
			} else {
				render_texture (tex("Coolant.bmp"), 808, 196, 209, 80);	
			}	
		}
	} else {
		if (using_fh) {
			render_texture (tex("CoolantF.bmp"), 808, 196, 209, 80);
		} else {
			render_texture (tex("Coolant.bmp"), 808, 196, 209, 80);	
		}
	}

	render_texture (tex("Coolanticon.bmp"), 772, 221, 41, 39);

	// Top light icons (Neutral, Oil, Indicator, High beam)

	if (neutral) {
		render_texture (tex("Neutrallight.bmp"), 0, 0, 130, 136);
	}

	if (!neutral && current_gear != 0) {
		render_texture (tex("Gear.bmp"), 0, 0, 131, 136);
		draw_large_num (current_gear, 30, 22);
	}

	if (oil_warning) {
		render_oil_light_texture (131, 0, 135, 138);
	}

	if (indicate_right && !indicate_left) {
		render_texture (tex("Indicateright.bmp"), 267, 0, 135, 136);
	}

	if (indicate_left && !indicate_right) {
		render_texture (tex("Indicateleft.bmp"), 267, 0, 135, 136);		
	}

	if (indicate_left && indicate_right) {
		render_texture (tex("Indicateboth.bmp"), 267, 0, 135, 136);			
	}

	if (high_beam) {
		render_texture (tex("Highbeamlight.bmp"), 404, 0, 133, 136);
	}

	// Middle info section (Tank Range, MPG, etc..)
	if (info_mode == 0 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {
			if (using_km) {
				render_texture(tex("InfotopKM.bmp"), 0, 176, 510, 97);
			} else {
				render_texture(tex("Infotop.bmp"), 0, 176, 510, 97);	
			}
			
			render_texture(tex("Infobottom.bmp"), 0, 273, 454, 125);
		} 
	}

	if (info_mode == 1 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {
			render_texture(tex("Infotopdiag.bmp"), 0, 176, 510, 97);
			render_texture(tex("Infobottomdiag.bmp"), 0, 273, 454, 125);
		} 
	}

	if (info_mode == 2 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {
			render_texture(tex("tyretop.bmp"), 0, 176, 510, 97);
			render_texture(tex("tyrebottom.bmp"), 0, 273, 454, 125);
		} 
	}

	if (info_mode == 3 && !warningbadgeactive) {
		if (info_animation_in_progress == false) {			
			render_texture(tex("Navbg.bmp"), 0, 158, 434, 268);
		} 
	}

	if ((info_mode != currentinfomode && !warningbadgeactive)) {		
		if (info_animation_in_progress == false) {
			info_animation_count = 0;
			infoanimationreverse = false;
			info_animation_in_progress = true;
		}		

		animate_info_mode();
	}

	/*
	if (info_mode == 2) { // Low Fuel
		render_texture (tex("Fuelwarningbadge.bmp"), 0, 163, 444, 249);
		if (neutral) {
			render_texture (tex("Lowfuel.bmp"), 222, 236, 134, 105);
		}
	}
	*/


	if (tpms_connected == true) {

		if (front_sensor_read) {
			if (g_sensor2_psi <= front_pressure_low) {
				front_tyre_warning_triggered = true;
			}
		}

		if (rear_sensor_read) {
			if (g_sensor4_psi <= rear_pressure_low) {
				rear_tyre_warning_triggered = true;
			}	
		}
			
	}

	// Warning badges
	if (!warningbadgecancelled) {
		if (front_tyre_warning_triggered || rear_tyre_warning_triggered) {
			
			warningbadgeactive = true;
			render_texture (tex("Lowtyrebadge.bmp"), 0, 163, 444, 249);

			if (flash) {
				if (front_tyre_warning_triggered && !rear_tyre_warning_triggered) {
					// Front only
					render_texture (tex("Fronttyrelow.bmp"), 168, 244, 257, 76);
				}

				if (rear_tyre_warning_triggered && !front_tyre_warning_triggered) {
					// Rear only
					render_texture (tex("Reartyrelow.bmp"), 168, 244, 257, 76);
				}

				if (rear_tyre_warning_triggered && front_tyre_warning_triggered) {
					// Front and Rear
					render_texture (tex("Frontrearlow.bmp"), 168, 244, 257, 76);
				}

			}

		} else {
			
			if (oil_warning == true && enginerunning == true) { // Oil warning takes priority
				
				warningbadgeactive = true;
				render_texture (tex("Lowoilbadge.bmp"), 0, 163, 444, 249);
				
				if (flash) {
					render_texture (tex("Lowoil.bmp"), 222, 236, 134, 105);
				}

			} else {
				
				if (overheatwarning == true && enginerunning == true) { // Second priority is engine overheat warning
					warningbadgeactive = true;
					render_texture(tex("Overheatbadge.bmp"), 0, 163, 444, 249);
					if (flash) {
						render_texture(tex("Engineoverheat.bmp"), 189, 237, 212, 95);
					}
				} else {
					
					if (fuelwarning == true && enginerunning == true && info_mode != 3) { // Third priority is fuel warning
						warningbadgeactive = true;
						render_texture(tex("Fuelwarningbadge.bmp"), 0, 163, 444, 249);
						if (flash) {
							render_texture(tex("Lowfuel.bmp"), 222, 236, 134, 105);
						}
					} else {
						warningbadgeactive = false;
					}
				}

			}	
		}
	}

	// Units - either MPH or KM
	if (using_km == 1) {
		render_texture (tex("kph.bmp"), 844, 553, 179, 44);
		render_texture (tex("km.bmp"), 350, 448, 57, 18);
	} else {
		render_texture (tex("mph.bmp"), 844, 553, 142, 45);
		render_texture (tex("Miles.bmp"), 350, 448, 57, 18);	
	}
	

	// Rev counter numbers
	SDL_RenderCopy(renderer, tex("R12-13.bmp"), nullptr, &rect_g1213);
	SDL_RenderCopy(renderer, tex("R11.bmp"), nullptr, &rect_g11);
	SDL_RenderCopy(renderer, tex("R10.bmp"), nullptr, &rect_g10);
	SDL_RenderCopy(renderer, tex("R9.bmp"), nullptr, &rect_g9);
	SDL_RenderCopy(renderer, tex("R8.bmp"), nullptr, &rect_g8);
	SDL_RenderCopy(renderer, tex("R7.bmp"), nullptr, &rect_g7);
	SDL_RenderCopy(renderer, tex("R6.bmp"), nullptr, &rect_g6);
	SDL_RenderCopy(renderer, tex("R5.bmp"), nullptr, &rect_g5);
	SDL_RenderCopy(renderer, tex("R4.bmp"), nullptr, &rect_g4);
	SDL_RenderCopy(renderer, tex("R3.bmp"), nullptr, &rectg3);
	SDL_RenderCopy(renderer, tex("R2.bmp"), nullptr, &rectg2);
	SDL_RenderCopy(renderer, tex("R1.bmp"), nullptr, &rectg1);
	SDL_RenderCopy(renderer, tex("R0.bmp"), nullptr, &rectg0);

	sprintf( str_current_speed, "%d", current_speed);
	sprintf( str_trip1, "%.1f", trip1);
	sprintf( str_trip2, "%.1f", trip2);
	sprintf( str_odo, "%.0f", odo);

	if (using_fh) { // If using fahrenheit
		tempf = ((double)ambient_temp*1.8) + 32;
		sprintf(str_ambient_temp, "%df", (int)tempf);
	} else {
		sprintf(str_ambient_temp, "%dc", ambient_temp);
	}
	
	if (using_fh) { //If using fahrenheit
		coolant_temp_f = ((double)coolant_temp*1.8) + 32;
		sprintf(str_coolant_temp, "%d", (int)coolant_temp_f);
	} else {
		sprintf(str_coolant_temp, "%d", coolant_temp);
	}
	
	if (using_km) {
		if (mpg > 0) {
			sprintf(str_mpg, "%d", (int)((double)282.481 / (double)mpg));	
		} else {
			sprintf(str_mpg, "%d", mpg);	
		}
		
	} else {
		sprintf(str_mpg, "%d", mpg);
	}
	
	if (using_km) {
		sprintf(str_range, "%d", (int)((double)range * 1.609));
	} else {
		sprintf(str_range, "%d", range);	
	}
	
	sprintf(str_max_speed, "%d", max_speed);
	sprintf(str_rpm, "%d", rpm);
	
	if (oil_pressure_available) {
		sprintf(str_oil_press, "%.1f", get_precise_bar(oil_pressure_ohms));
		sprintf(str_oil_temp, "%d", (int)get_precise_temp(oil_temp_ohms));	
	}

	if (g_sensor2_psi != -99) {
		if (using_bar) {
			sprintf (str_sensor2_psi, "%.1f", g_sensor2_bar);
		} else {
			sprintf (str_sensor2_psi, "%.1f", g_sensor2_psi);
		}
		
	} else {
		strcpy (str_sensor2_psi, "..");
	}

	if (g_sensor4_psi != -99) {
		if (using_bar) {
			sprintf (str_sensor4_psi, "%.1f", g_sensor4_bar);	
		} else {
			sprintf (str_sensor4_psi, "%.1f", g_sensor4_psi);
		}
		
	} else {
		strcpy (str_sensor4_psi, "..");
	}
	
	if (g_sensor2_temp != -99) {
		if (using_fh) {
			g_sensor2_temp_f = ((double)g_sensor2_temp*1.8) + 32;
			sprintf (str_sensor2_temp, "%df", (int)g_sensor2_temp_f);	
		} else {
			sprintf (str_sensor2_temp, "%dc", g_sensor2_temp);	
		}		
	} else {
		strcpy (str_sensor2_temp, "..");
	}
	
	if (g_sensor4_temp != -99) {
		if (using_fh) {
			g_sensor4_temp_f = ((double)g_sensor4_temp*1.8) + 32;
			sprintf (str_sensor4_temp, "%df", (int)g_sensor4_temp_f);
		} else {
			sprintf (str_sensor4_temp, "%dc", g_sensor4_temp);
		}		
	} else {
		strcpy (str_sensor4_temp, "..");
	}
	

	if (get_litres_remaining(fuel_float) != 0) {
		sprintf(str_fuel, "%.1f", litres_remaining);
	} else {
		strcpy (str_fuel, "..");
	}
	
	sprintf(str_batt, "%.1f", batt);
	sprintf(str_spd_correct, "%.1f", spd_correct);


	// Draw the current speed
	draw_speed(str_current_speed);		

	// Draw some numbers
	draw_medium_string (str_ambient_temp, 18, 461);
	draw_medium_string (str_coolant_temp, 873, 224);

	// Middle Info
	if (info_mode == 0 && info_animation_in_progress == false && !warningbadgeactive) {
		draw_medium_string (str_mpg, 60, 224);
		draw_medium_string (str_range, 292, 224);
		draw_medium_string (str_trip_time, 17, 332);
		draw_medium_string (str_max_speed, 246, 332);
	}

	if (info_mode == 1 && info_animation_in_progress == false && !warningbadgeactive) {
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
	if (info_mode == 2 && info_animation_in_progress == false && !warningbadgeactive) {
		draw_medium_string (str_sensor2_psi, 40, 224);
		draw_medium_string (str_sensor4_psi, 304, 224);
		draw_medium_string (str_sensor2_temp, 37, 342);
		draw_medium_string (str_sensor4_temp, 246, 342);
	}

	if (info_mode == 3 && info_animation_in_progress == false && !warningbadgeactive) {
		

		if (nav_active == true) {
			if (strlen (str_nav_road) > 0) {
				draw_nav_symbol (ONTO, 13, 350);
			}

			if (strlen (str_nav_road) > 0 && strlen (str_nav_road) <= 14) {
				draw_nav_large_string (str_nav_road, 69, 347);			
			} else {
				draw_nav_small_string (str_nav_road, 69, 347);			
			}
			
			if (strlen (str_nav_towards) > 0) {
				draw_nav_symbol (TWRDS, 13, 391);
			}

			if (strlen (str_nav_towards) > 0 && strlen (str_nav_towards) <= 16) {			
				draw_nav_large_string (str_nav_towards, 99, 386);
			} else {			
				draw_nav_small_string (str_nav_towards, 99, 386);
			}
			
			draw_nav_small_string (sz_arrive_in, 14, 175);
			
			if (using_km) {
				draw_nav_large_string (str_nav_dest_km, 77, 171);
				draw_nav_small_string (sz_km, 165, 175);
			} else {
				draw_nav_large_string (str_nav_dest_miles, 77, 171);
				draw_nav_small_string (sz_mls, 165, 175);
			}
			

			if (nav_yards <= 300) {
				//draw_nav_numbers (87, 16, 191);				
				if (using_km) {
					draw_nav_digits (str_nav_metres, 16, 211);
					draw_nav_symbol (METRE, 22, 308);
				} else {
					draw_nav_digits (str_nav_yards, 16, 211);
					draw_nav_symbol (YARD, 22, 308);
				}				
			} else {
				if (using_km) {
					draw_nav_digits (str_nav_km, 16, 211);
					draw_nav_symbol (KM, 22, 308);
				} else {
					draw_nav_digits (str_nav_miles, 16, 211);
					draw_nav_symbol (MILE, 22, 308);
				}				
			}
			
			if (strcmp (str_nav_symbol, "MUT") == 0) {
				if (driving_left == 1) {
					draw_nav_symbol (MUT, 249, 180);
				}

				if (driving_left == 0) {
					draw_nav_symbol (MUTR, 249, 180);
				}
			}

			if (strcmp (str_nav_symbol, "MEX") == 0) {
				if (driving_left == 1) {
					draw_nav_symbol (EXITL, 249, 180);
				}

				if (driving_left == 0) {
					draw_nav_symbol (EXITR, 249, 180);
				}

				// Draw Exit number
				draw_nav_large_string (str_nav_exit, 344, 275);
			}

			if (strcmp (str_nav_symbol, "TNR") == 0) {
				draw_nav_symbol (TNR, 249, 180);			
			}

			if (strcmp (str_nav_symbol, "TNL") == 0) {
				draw_nav_symbol (TNL, 249, 180);			
			}

			if (strcmp (str_nav_symbol, "SLL") == 0) {
				draw_nav_symbol (SLL, 249, 180);			
			}

			if (strcmp (str_nav_symbol, "SLR") == 0) {
				draw_nav_symbol (SLR, 249, 180);			
			}

			if (strcmp (str_nav_symbol, "RB1") == 0) {
				if (driving_left == 1) {
					draw_nav_symbol (RB1L, 249, 180);
				}

				if (driving_left == 0) {
					draw_nav_symbol (RB1R, 249, 180);
				}
			}

			if (strcmp (str_nav_symbol, "RB2") == 0) {
				if (driving_left == 1) {
					draw_nav_symbol (RB2L, 255, 170);
				}

				if (driving_left == 0) {
					draw_nav_symbol (RB2R, 255, 170);
				}
			}		

			if (strcmp (str_nav_symbol, "PRK") == 0) {
				draw_nav_large_string (sz_find_parking, 99, 386);
			}

			if (strcmp (str_nav_symbol, "FRY") == 0) {
				draw_nav_large_string (sz_take_ferry, 99, 386);
			}

			if (strcmp (str_nav_symbol, "RB3") == 0) {
				if (driving_left == 1) {
					draw_nav_symbol (RB3L, 249, 180);
				}

				if (driving_left == 0) {
					draw_nav_symbol (RB3R, 249, 180);
				}
			}		

			if (strcmp (str_nav_symbol, "RB4") == 0) {
				if (driving_left == 1) {
					draw_nav_symbol (RB4L, 249, 180);
				}

				if (driving_left == 0) {
					draw_nav_symbol (RB4R, 249, 180);
				}
			}

			if (strcmp (str_nav_symbol, "RB5") == 0) {
				if (driving_left == 1) {
					draw_nav_symbol (RB5L, 249, 180);
				}

				if (driving_left == 0) {
					draw_nav_symbol (RB5R, 249, 180);
				}
			}

			if (strcmp (str_nav_symbol, "LNR") == 0) {
				draw_nav_symbol (KPR, 249, 180);			
			}

			if (strcmp (str_nav_symbol, "LNL") == 0) {
				draw_nav_symbol (KPL, 249, 180);			
			}

			if (strcmp (str_nav_symbol, "CON") == 0) {
				draw_nav_symbol (CON, 249, 180);			
			}

			if (strcmp (str_nav_symbol, "ARV") == 0) {
				draw_nav_symbol (ARV, 249, 180);			
			}
		} else {
			draw_nav_large_string (sz_no_nav_data, 40, 265);
			draw_nav_small_string (sz_smartphone_app, 40, 350);
			draw_nav_small_string (sz_set_destination, 56, 380);
		}
		

	}

	if (indicate_left && !warningbadgeactive) {		
		if (info_mode == 0 || info_mode == 1) {
			render_texture (tex("Indicateleftfar.bmp"), 2, 186, 250, 98);
		}		
	}

	if (indicate_right && !warningbadgeactive) {		
		render_texture (tex("Indicaterightfar.bmp"), 802, 192, 209, 84);
	}
	

	// Essential info
	draw_medium_string(str_time, 14, 543);
	draw_small_blue_string (str_trip1, 252, 460);
	draw_small_blue_string (str_trip2, 246, 509);
	draw_small_grey_string (str_odo, 223, 560);

	if (oil_pressure_available) {
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
	SDL_ShowCursor(SDL_DISABLE);
	
	int err;
	err = pthread_create(&(tid[0]), nullptr, &pollInterface, nullptr);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));
    else
        printf("\n Thread created successfully\n");

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


	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
		return 1;
	}
	

	if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer) != 0) {
		fprintf(stderr, "could not create window & renderer: %s\n", SDL_GetError());
		return 1;
	}

	if (window == nullptr) {
		fprintf(stderr, "could not create window: %s\n", SDL_GetError());
		return 1;
	}

	screen_surface = SDL_GetWindowSurface(window);

	g_assets = asset_store_create(renderer);
	if (!g_assets) {
		fprintf(stderr, "Failed to create asset store\n");
		return 1;
	}
	for (int i = 0; i < THEME_COUNT; i++) {
		char dir[256];
		snprintf(dir, sizeof(dir), "assets/themes/%s", THEME_NAMES[i]);
		asset_store_load_theme(g_assets, THEME_NAMES[i], dir);
	}
	g_current_theme = theme_name_from_id(theme);

	init_rects();

	SDL_ShowCursor(SDL_DISABLE);

	////////////////////////////////////////  LOOP ///////////////////////////////////////////
	SDL_Event e;

	while (!quit) {

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
			if (e.type == SDL_KEYDOWN) {
				//quit = true;
			}

			if (e.type == SDL_KEYUP) {
				//fprintf (stderr, "choice");
				//spin_angle--;
				//fprintf (stderr, "Key: %d", e.key.keysym.scancode);

				if (e.key.keysym.scancode == 229) {
					choice();
				}

				if (e.key.keysym.scancode == 40) {
					select();
				}

				if (e.key.keysym.scancode == 41) {
					quit = true;
				}

				/*
				if(e.key.keysym.scancode != SDL_GetScancodeFromKey(e.key.keysym.sym)) {
					printf("Physical %s key acting as %s key",
      				SDL_GetScancodeName(e.key.keysym.scancode),
      				SDL_GetKeyName(e.key.keysym.sym));
				}
				*/
    				
				
			}

			if (e.type == SDL_MOUSEBUTTONUP) {
				fprintf(stderr, "select");
				//spin_angle++;
				
			}

			if (e.type == SDL_MOUSEBUTTONDOWN) {
				//quit = true;
			}
		}

		//run_partial_test();

		if (theme != currenttheme) {
			currenttheme = theme;
			g_current_theme = theme_name_from_id(theme);
		}

		//odo = spin_angle;
		//odo = get_fuel_reveal(fuel_percent);

		if (choice_state == 0) {
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
			if (choice_state > 0) {
				draw_menu(choice_state);
			}
		}



		// Thresholds for triggering warnings
		if (get_num_fuel_bars(fuel_float) <= 2) { // 2 bars remaining - 4.5 litres left
			fuelwarning = true;
		} else {
			fuelwarning = false;
		}

		if (coolant_temp >= 120) {
			overheatwarning = true;
		}

		if (coolant_temp < 120) {
			overheatwarning = false;
		}

		// Quick check on the RPM to set a flag if the engine is running - used for triggering warning badges
		if (rpm > 2000) {
			enginerunning = true;
		} 

		if (rpm < 1000) {
			enginerunning = false;
		}


		//SDL_FillRect(screen_surface, &rect, SDL_MapRGB(screen_surface->format, 0x0, 0x0, 0x0));
		
		//SDL_UpdateWindowSurface(window);
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


