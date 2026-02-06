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

//#include <SDL.h> // Windows
//#include <string.h> // Windows only - for strlen to work
//#include "/Library/Frameworks/SDL2.framework/Headers/SDL.h" // OSX only
#include <SDL2/SDL.h> // Rpi Only
#include <stdio.h>
#include <pthread.h>
#include <fstream>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

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

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* screenSurface = NULL;
SDL_Surface* gImage = NULL;

// Surfaces
SDL_Surface* gR1213 = NULL;
SDL_Surface* gR11 = NULL;
SDL_Surface* gR10 = NULL;
SDL_Surface* gR9 = NULL;
SDL_Surface* gR8 = NULL;
SDL_Surface* gR7 = NULL;
SDL_Surface* gR6 = NULL;
SDL_Surface* gR5 = NULL;
SDL_Surface* gR4 = NULL;
SDL_Surface* gR3 = NULL;
SDL_Surface* gR2 = NULL;
SDL_Surface* gR1 = NULL;
SDL_Surface* gR0 = NULL;

// Rev line
SDL_Surface* gRevline = NULL;
SDL_Surface* gRevwhite = NULL;
SDL_Surface* gSpeednumbers = NULL;

// Small numbers for Temp, Trip, Odo, additional info
SDL_Surface* gSmallnumbers = NULL;

// Top Greyed out icons
SDL_Surface* gTopiconsgrey = NULL;
SDL_Surface* gTopiconsgreyOP = NULL;
SDL_Surface* gTopiconsedge1 = NULL;
SDL_Surface* gTopiconsedge2 = NULL;

// Tyre icons if TPMS is connected
SDL_Surface* gTyreicon = NULL;
SDL_Surface* gTyresignal = NULL;

// Mile info box
SDL_Surface* gMileinfo = NULL;

// Fuel Gauge
SDL_Surface *gFuelgauge = NULL;
SDL_Surface *gFuelwhite = NULL;

SDL_Surface *gLowtyrebadge = NULL;
SDL_Surface *gReartyrelow = NULL;
SDL_Surface *gFronttyrelow = NULL;
SDL_Surface *gBothtyrelow = NULL;
SDL_Surface *gSpeedcorrection = NULL;
SDL_Surface *gSettime = NULL;
SDL_Surface *gSetunits = NULL;
SDL_Surface *gSelecton = NULL;
SDL_Surface *gMenuoptions = NULL;
SDL_Surface *gControloptions = NULL;
SDL_Surface *gControlselect = NULL;
SDL_Surface *gTPMSoptions = NULL;
SDL_Surface *gLightoptions = NULL;
SDL_Surface *gWhitethumb = NULL;
SDL_Surface *gBrightthumb = NULL;
SDL_Surface *gDarkthumb = NULL;
SDL_Surface *gGreenthumb = NULL;
SDL_Surface *gRedthumb = NULL;
SDL_Surface *gBluethumb = NULL;
SDL_Surface *gOrangethumb = NULL;
SDL_Surface *gYellowthumb = NULL;
SDL_Surface *gNightthumb = NULL;
SDL_Surface *gUparrow = NULL;
SDL_Surface *gUparrowsmall = NULL;
SDL_Surface *gMenuarrowright = NULL;
SDL_Surface *gMenuarrowleft = NULL;
SDL_Surface *gMenusmallarrowright = NULL;
SDL_Surface *gMenusmallarrowleft = NULL;
SDL_Surface *gCoolant = NULL;
SDL_Surface *gCoolantF = NULL;
SDL_Surface *gKm = NULL;
SDL_Surface *gMiles = NULL;
SDL_Surface *gKph = NULL;
SDL_Surface *gMph = NULL;
SDL_Surface *gHighbeamlight = NULL;
SDL_Surface *gIndicateright = NULL;
SDL_Surface *gIndicateleft = NULL;
SDL_Surface *gIndicateboth = NULL;
SDL_Surface *gIndicaterightfar = NULL;
SDL_Surface *gIndicateleftfar = NULL;
SDL_Surface *gOillight = NULL;
SDL_Surface *gOillightOP = NULL;
SDL_Surface *gNeutrallight = NULL;
SDL_Surface *gEngineoverheat = NULL;
SDL_Surface *gOverheatbadge = NULL;
SDL_Surface *gLowoil = NULL;
SDL_Surface *gLowoilbadge = NULL;
SDL_Surface *gLowfuel = NULL;
SDL_Surface *gLowfuelbadge = NULL;
SDL_Surface *gInfobottomdiag = NULL;
SDL_Surface *gInfotopdiag = NULL;
SDL_Surface *gInfobottom = NULL;
SDL_Surface *gInfotop = NULL;
SDL_Surface *gInfotopKM = NULL;
SDL_Surface *gTyrebottom = NULL;
SDL_Surface *gTyretop = NULL;
SDL_Surface *gCoolanticon = NULL;
SDL_Surface *gThemeoptions = NULL;
SDL_Surface *gArrowrighttheme = NULL;
SDL_Surface *gArrowlefttheme = NULL;
SDL_Surface *gDownarrow = NULL;
SDL_Surface *gSetodometer = NULL;
SDL_Surface *gOdoerror1 = NULL;
SDL_Surface *gOdoerror2 = NULL;
SDL_Surface *gSprocketsetup = NULL;
SDL_Surface *gCoolantfantemp = NULL;
SDL_Surface *gGear = NULL;
SDL_Surface *gNavbg = NULL;
SDL_Surface *gNavicons = NULL;

// Surface Rects
// Rev counter
SDL_Rect rectg1213;
SDL_Rect rectg11;
SDL_Rect rectg10;
SDL_Rect rectg9;
SDL_Rect rectg8;
SDL_Rect rectg7;
SDL_Rect rectg6;
SDL_Rect rectg5;
SDL_Rect rectg4;
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
int gNaviconsrctexloc[4][28] = {
	//MUT  EXITL   EXITR  TNR   TNL  SLL  SLR  RB2L  RB3L  RB4L  RB5L RB1R RB3R RB4R RB5R KPL KPR CON ARV RB1L RB2R MUTR MILE YARD KM METRE ONTO TWRDS
	{21,   169,    317,   449,  628, 809, 1002,17,   163,  312,  460, 598, 771, 937, 1092,6,  174,333,491,642, 823, 983, 11,  106,215,280,  404, 456  }, // X pos
	{12,   11,     12,    20,   19,  22,  22,  184,  216,  217,  217, 214, 217, 216, 216, 383,385,384,394,387, 364, 385, 549, 550,550,549,  556, 557  }, // Y pos
	{112,  112,    112,   157,  147, 158, 176, 115,  134,  114,  110, 131, 131, 131, 131, 141,150,131,112,138, 116, 108, 81,  87,  42,105,  45,  76   }, // Width
	{150,  150,    150,   146,  150, 144, 146, 167,  134,  135,  135, 137, 134, 134, 134, 140,138,138,129,135, 167, 145, 24,  21,  22,22,   16,  14   }  // Height
};

char gNavLargeUpperLetterref[40] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9',',','.','(',')'};
char gNavLargeLowerLetterref[40] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9',',','.','(',')'};


int gNavletterslargesrctexloc[4][40] = {
// 	A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  COM, STP,  (,  ) 
   {15, 46, 75, 106,137,165,190,222,253,268,295,325,351,386,416,449,476,509,537,565,593,622,649,686,714,743,771,797,821,846,871,896,921,946,970,996,1022,1039,1055,1079 },
   {602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602, 602, 602, 602  },
   {22, 19, 20, 20, 17, 16, 22, 19, 5,  15, 20, 16, 24, 19, 22, 18, 22, 18, 19, 18, 19, 19, 29, 20, 21, 19, 16, 11, 16, 16, 16, 16, 16, 15, 17, 16, 5,   5,   9,   8,   },
   {22, 22, 23, 21, 21, 21, 23, 21, 21, 22, 21, 21, 21, 21, 23, 22, 24, 22, 23, 21, 22, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 21, 21, 21, 21, 22, 27,  22,  27,  27,  }
};

int gNavletterssmallsrctexloc[4][40] = {
	
// 	A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  COM, STP,  (,  ) 
   {17, 42, 65, 90, 114,137,157,182,207,219,241,265,286,314,337,364,386,412,435,457,479,502,524,553,576,599,622,643,662,681,701,721,742,762,781,802,822, 836, 849, 861 },
   {778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778, 778, 778, 778 },
   {18, 15, 17, 16, 14, 13, 17,16,  4,  13, 16, 13, 19, 16,	18,	14, 18, 15, 15, 15, 16, 16, 24, 18, 17, 15, 13, 8,  13, 14, 14, 14, 13, 12,	14, 13,	5,	 5,	  7,   7   },
   {17, 17, 17, 17, 17, 17, 17,17,  17, 17, 17, 17, 17, 17,	17,	17, 19, 17, 18, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18,	18, 21,	 18,  22,  22  }

};

int gNavnumbersref[11] = {'0','1','2','3','4','5','6','7','8','9','.'};
int gNavnumberssrctexloc[4][11] = {
	
//   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,     .
	{10, 115,209,310,409,509,611,711,809,910, 447},
	{660,661,660,661,661,661,660,660,660,660, 734},
	{68, 46, 69, 66, 68, 67, 66, 63, 68, 66,  17},
	{93, 93, 93, 93, 90, 91, 92, 91, 92, 92, 15 }
};


// Large speedometer numbers - texture mapping
int gSpeedsrctexloc[4][10] = {
//    0    1    2    3    4    5    6    7    8     9		// The digit in the bitmap
	{ 0  , 143, 255, 396, 530, 669, 808, 939, 1080, 1211 }, // X position from Top Left
	{ 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1   , 1 },    // Y position from Top Left
	{ 135, 107, 142, 134, 139, 138, 131, 141, 131 , 137 },  // Width of digit
	{ 172, 172, 166, 172, 172, 172, 172, 172, 172 , 172 }   // Height of digit
};


// Medium sized numbers - clock & temp displays
int gMednumberssrctexloc[4][16] = {
//    0    1    2    3    4    5    6    7    8     9	  :   +   -     .   oC.   oF	// The digit in the bitmap
	{ 314, 2,  32,  68,  104, 139, 175, 209, 243, 279,   5,   32,  73, 102, 138, 183},	// X position
	{ 6,   6,   6,   6,  6,   6,   6,   6,   6,    6,    50,  52,  51, 47,  50,  50},  // Y position
	{ 26, 20,  29,  28,  28,  27,  25,  27, 26 ,  26,    16,  35,  34, 21,  40,  40},  // Width of digit
	{ 36, 39,  36,  36,  36,  36,  36,  36, 36 ,  35,    31,  31,  31, 39,  33,  33}   // Height of digit
};

// Small blue numbers - mileage and trip displays
int gSmallbluesrctexloc[4][11] = {
//    0    1    2    3    4    5    6    7    8     9     .	  // The digit in the bitmap
	{ 546, 355, 372, 394, 416, 438, 460, 483, 503, 525, 354 }, // X position
	{ 19,  19,  19,  19,  19,  19,  19,  19,  19,  19, 37 },  // Y position
	{ 18,  12,  18,  18,  18,  18,  17,  17,  18,  18, 11 },  // Width of digit
	{ 23,  23,  23,  22,  23,  23,  23,  22,  23,  23, 4 }   // Height of digit
};

// Small grey numbers - odometer
int gSmallgreysrctexloc[4][11] = {
	//    0    1    2    3    4    5    6    7    8     9    .	  // The digit in the bitmap
	{ 546, 355, 372, 394, 416, 438, 460, 483, 503, 525, 354 }, // X position
	{ 52,  52,  52,  52,  52,  52,  52,  52,  52,  52, 37 },  // Y position
	{ 18,  12,  18,  18,  18,  18,  17,  17,  18,  18, 11 },  // Width of digit
	{ 23,  23,  23,  22,  23,  23,  23,  22,  23,  23, 4 }   // Height of digit
};

int gLargenumberssrctexloc[4][10] = { // NEW FOR LARGE GEAR INDICATOR
	// 1    2    3    4    5    6    7    8    9     0    	  // The digit in the bitmap
	{ 0,   66, 166, 264, 367, 468, 571, 667, 766, 865 }, // X position
	{ 91,  91,  91,  91,  91,  91,  91,  91,  91,  91 },  // Y position
	{ 63,  68,  68,  68,  68,  68,  68,  68,  68,  68 },  // Width of digit
	{ 94,  94,  94,  94,  94,  94,  94,  94,  94,  94 }   // Height of digit
};

// RPM lookup table - Converting RPM values into rotation value to reveal rev line
int gRPMlookup[53][2] = {
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

char gNumref[16] = { '0','1','2','3','4','5','6','7','8','9',':','+','-','.','c','f'};
char gSmallNumref[11] = { '0','1','2','3','4','5','6','7','8','9','.'};
char gLargeNumref[10] = { '1','2','3','4','5','6','7','8','9','0'};


// Destination rect for speedo numbers
SDL_Rect spdDigitone;
SDL_Rect spdDigittwo;
SDL_Rect spdDigitthree;

// Textures (Converted from Surfaces)
SDL_Texture* gr1213tex;
SDL_Texture* gr11tex;
SDL_Texture* gr10tex;
SDL_Texture* gr9tex;
SDL_Texture* gr8tex;
SDL_Texture* gr7tex;
SDL_Texture* gr6tex;
SDL_Texture* gr5tex;
SDL_Texture* gr4tex;
SDL_Texture* gr3tex;
SDL_Texture* gr2tex;
SDL_Texture* gr1tex;
SDL_Texture* gr0tex;

SDL_Texture* grevlinetex;
SDL_Texture* grevwhitetex;
SDL_Texture* gspeednumberstex;

SDL_Texture* gsmallnumberstex;
SDL_Texture* gtopiconsgreytex;
SDL_Texture* gtopiconsgreyoptex;
SDL_Texture* gtopiconsedge1tex;
SDL_Texture* gtopiconsedge2tex;

SDL_Texture* gmileinfotex;

SDL_Texture* gfuelgaugetex;
SDL_Texture* gfuelgaugewhitetex;

SDL_Texture *gSpeedcorrectiontex;
SDL_Texture *gSettimetex;
SDL_Texture *gSelectontex;
SDL_Texture *gSetunitstex;
SDL_Texture *gMenuoptionstex;
SDL_Texture *gControloptionstex;
SDL_Texture *gControlselecttex;
SDL_Texture *gTPMSoptionstex;
SDL_Texture *gUparrowtex;
SDL_Texture *gUparrowsmalltex;
SDL_Texture *gMenuarrowrighttex;
SDL_Texture *gMenuarrowlefttex;
SDL_Texture *gMenusmallarrowrighttex;
SDL_Texture *gMenusmallarrowlefttex;
SDL_Texture *gLightoptionstex;
SDL_Texture *gWhitethumbtex;
SDL_Texture *gBrightthumbtex;
SDL_Texture *gDarkthumbtex;
SDL_Texture *gGreenthumbtex;
SDL_Texture *gRedthumbtex;
SDL_Texture *gBluethumbtex;
SDL_Texture *gOrangethumbtex;
SDL_Texture *gYellowthumbtex;
SDL_Texture *gNightthumbtex;
SDL_Texture *gCoolanttex;
SDL_Texture *gCoolantFtex;
SDL_Texture *gKmtex;
SDL_Texture *gMilestex;
SDL_Texture *gKphtex;
SDL_Texture *gMphtex;
SDL_Texture *gTyreicontex;
SDL_Texture *gTyresignaltex;
SDL_Texture *gHighbeamlighttex;
SDL_Texture *gIndicaterighttex;
SDL_Texture *gIndicatelefttex;
SDL_Texture *gIndicatebothtex;
SDL_Texture *gIndicaterightfartex;
SDL_Texture *gIndicateleftfartex;
SDL_Texture *gOillighttex;
SDL_Texture *gOillightoptex;
SDL_Texture *gNeutrallighttex;
SDL_Texture *gEngineoverheattex;
SDL_Texture *gOverheatbadgetex;
SDL_Texture *gLowoiltex;
SDL_Texture *gLowoilbadgetex;
SDL_Texture *gLowfueltex;
SDL_Texture *gLowfuelbadgetex;
SDL_Texture *gInfobottomdiagtex;
SDL_Texture *gInfotopdiagtex;
SDL_Texture *gInfobottomtex;
SDL_Texture *gInfotoptex;
SDL_Texture *gInfotopKMtex;
SDL_Texture *gTyrebottomtex;
SDL_Texture *gTyretoptex;
SDL_Texture *gCoolanticontex;
SDL_Texture *gThemeoptionstex = NULL;
SDL_Texture *gArrowrightthemetex = NULL;
SDL_Texture *gArrowleftthemetex = NULL;
SDL_Texture *gDownarrowtex = NULL;
SDL_Texture *gSetodometertex = NULL;
SDL_Texture *gOdoerror1tex = NULL;
SDL_Texture *gOdoerror2tex = NULL;
SDL_Texture *gSprocketsetuptex = NULL;
SDL_Texture *gCoolantfantemptex = NULL;
SDL_Texture *gGeartex = NULL;
SDL_Texture *gLowtyrebadgetex = NULL;
SDL_Texture *gReartyrelowtex = NULL;
SDL_Texture *gFronttyrelowtex = NULL;
SDL_Texture *gBothtyrelowtex = NULL;
SDL_Texture *gNavbgtex = NULL;
SDL_Texture *gNaviconstex = NULL;

char gszCommsmsg[1024];
int serial_port;
pthread_t tid[2];

// Bike specific values
int spinangle = 0; // revline reveal
int testcounter = 0; // Number to use for test scenarios

int tpmsmodel = EBAYTPMS; // Standard version I recommend or EBAYTPMS


/*
	RPM		Spinangle
	0		6
	500		
*/


// Core values
int currentSpeed = 68;
double trip1 = 1234.2;
double trip2 = 3456.4;
double odo = 23456;
int coolanttemp = 56;
double coolanttempf = 0;
int mpg = 10;
int range = 100;
int maxspeed = 40;
int triptimehour = 0;
int triptimemin = 0;
int usingkm = 0;
int drivingleft = 1;
int usingfh = 0;
int usingbar = 0;
int rpm = 5000;
double batt = 12.5;
double spdcorrect = 0;
int fuelpercent = 47;
int fuelfloat = 0;
int ambientTemp = 21;
double tempf = 0;
bool neutral = false;
bool oilwarning = false;
bool indicateleft = false;
bool indicateright = false;
bool highbeam = false;
int infomode = 0;
int theme = 0; // The theme set in the arduino
int currentgear = 0;
double litresremaining = 0;
int controllayout = 0;
int daytheme = 0;
int nighttheme = 0;
int lightswitchvalue = 0;
int currentlightlevel = 0;
int fanneutraloption = 0;
int gearratiointerval = 0;


bool fronttyrewarningtriggered = false;
bool reartyrewarningtriggered = false;
bool frontsensorread = false;
bool rearsensorread = false;

bool tpmsconnected = false;
bool tpmssignal = false;
int tpmssignalcount = 0;

int tpms_serial_port;
pthread_t tpms_tid[2];

double gSensor1bar = 0.0;
double gSensor2bar = 0.0;
double gSensor3bar = 0.0;
double gSensor4bar = 0.0;

double gSensor1psi = 0.0;
double gSensor2psi = -99;
double gSensor3psi = 0.0;
double gSensor4psi = -99;

int gSensor1temp = 0;
int gSensor2temp = -99;
int gSensor3temp = 0;
int gSensor4temp = -99;
double gSensor2tempF = 0;
double gSensor4tempF = 0;

int gSensor1state = 0;
int gSensor2state = 0;
int gSensor3state = 0;
int gSensor4state = 0;

// TPMS configurable values
int frontsensorid = 1;
int rearsensorid = 3;
int frontpressurelow = -1;
int rearpressurelow = -1;
char strFrontsensorid[16];
char strRearsensorid[16];
char strFrontpressurelow[16];
char strRearpressurelow[16];


char strSensor2psi[16];
char strSensor4psi[16];

char strSensor2temp[16];
char strSensor4temp[16];

char strCoolanttemp[16];
char strCurrentspeed[16];
char strTrip1[16];
char strTrip2[16];
char strOdo[16];
char strTime[16];
char strNav[255];
char strMpg[16];
char strRpm[16];
char strFuel[16];
char strRange[16];
char strMaxspeed[16];
char strTriptime[16];
char strAmbienttemp[16];
char strBatt[16];
char strSpdcorrect[16];
char strOilpress[16];
char strOiltemp[16];
char strLightswitchvalue[16];
char strCurrentlightlevel[16];

char strNavSymbol[16];
char strNavRoad[255];
char strNavTowards[255];
char strNavExit[16];
char strNavYards[16];
char strNavMiles[16];
char strNavDestMiles[16];
char strNavDestKm[16];

char strNavMetres[16];
char strNavKm[16];

int navYards = 0;
double navMiles = 0;
bool navActive = false;
double navDestdistance = 0;

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
int gDownarrowx  = 0;
int gUparrowx  = 0;
int startupanimcount = 0;
bool startupdone = false;
int flashcount = 0;
bool flash = false;
int targetrpmrotation = 0;
int currentrpmrotation = 0;

// Info mode animation
bool infoanimationinprogress = false;
int infoanimationcount = 0;
bool infoanimationreverse = false;
int currentinfomode = 0;

int frontsprocket = 16;
int rearsprocket = 44;
int coolantfantemp = 100;

// This bit is emulating the button presses from the arduino
// To control menu states
int choicestate = 0; // Set to -1 for production

// Set time digits
int settimedigit0 = 0;
int settimedigit1 = 0;
int settimedigit2 = 0;
int settimedigit3 = 0;

char strSettimedigit0[16];
char strSettimedigit1[16];
char strSettimedigit2[16];
char strSettimedigit3[16];

// Speed correction digits
int spcdigit0 = 0;
int spcdigit1 = 0;
int spcdigit2 = 0;
int spcdigit3 = 0;

char strSpcdigit0[16];
char strSpcdigit1[16];
char strSpcdigit2[16];
char strSpcdigit3[16];

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
bool oilpressureavailable = false;
int oilpressureohms = 0;
int oiltempohms = 0;

int barohms[11];
int tempnum[6];
int tempohms[6];

char szFindparking[] = "FIND PARKING";
char szTakeFerry[] = "TAKE THE FERRY";
char szNoNavData[] = "No Navigation Data";
char szSmartphoneapp[] = "launch smartphone app";
char szSetdestination[] = "and set a destination";
char szArrivein[] = "arv";
char szMls[] = "mls";
char szKm[] = "km";

int cycledigit (int value, int max) {
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

	if (choicestate >= 0 && choicestate < 20) { // Main menu
		choicestate++;	
	}
	
	if (choicestate > 12 && choicestate < 20) { // Main menu return
		choicestate = 0;
	}

	// Set time menu
	if (choicestate >= 20 && choicestate < 90) { // Set time menu
		choicestate+=10;
	}

	if (choicestate > 70 && choicestate < 90) { // Set time menu recycle
		choicestate = 20;
	}

	// Speed correction menu
	if (choicestate >= 100 && choicestate < 160) { // Speed correction menu
		choicestate+=10;
	}

	if (choicestate > 150 && choicestate < 170) { // Speed correction recycle
		choicestate = 100;
	}


	if (choicestate >= 200 && choicestate < 290) { // Theme menu
		choicestate+=10;
	}

	if (choicestate > 260 && choicestate < 290) { // Theme recycle
		choicestate = 200;
	}

	if (choicestate >= 300 && choicestate < 390) { // Set odometer menu
		odoerror = 0;
		choicestate += 5;
	}

	if (choicestate > 360 && choicestate < 390) { // Set odometer menu recycle
		choicestate = 300;
	}

	if (choicestate >= 600 && choicestate < 640) { // Set units menu
		choicestate += 5;
	}

	if (choicestate > 635 && choicestate < 690) { // Set units menu recycle
		choicestate = 600;
	}
}

void select() { // Pressed the select button

	if (choicestate == 0) {
		if (infomode == 3) {
			infomode = 0;
			return;
		}
		
		if (infomode == 2) {
			infomode = 3;
			return;
		}		

		if (infomode == 1) {
			infomode = 2;
			return;
		}

		if (infomode == 0) {
			infomode = 1;
			return;
		}
	}

	if (choicestate == 1) {
		// Reset trip one
		choicestate = 0;
		return;
	}

	if (choicestate == 2) {
		// Reset trip two
		choicestate = 0;
		return;
	}


	if (choicestate == 3) { // Will become set units menu
		// Toggle miles / km
		
		/*
		if (usingkm == 0) {
			usingkm = 1;
		} else {
			usingkm = 0;
		}
		*/
		choicestate = 600;
		return;
	}

	if (choicestate == 605) { // Set units to miles
		usingkm = 0;
	}

	if (choicestate == 610) { // Set units to km
		usingkm = 1;
	}

	if (choicestate == 615) { // Set units to celcius
		usingfh = 0;
	}	

	if (choicestate == 620) { // Set units to fahrenheit
		usingfh = 1;
	}	

	if (choicestate == 625) { // Set units to psi
		usingbar = 0;
	}	

	if (choicestate == 630) { // Set units to bar
		usingbar = 1;
	}	

	if (choicestate == 4) { // Main menu set time
		choicestate = 20; // Jump to time menu
		return;
	}

	if (choicestate == 5) {
		choicestate = 100; // Jump to speed correction menu
		return;
	}

	if (choicestate == 6) {
		choicestate = 200; // Jump to theme selection menu
		return;
	}

	if (choicestate == 20) { // Set time cancel
		choicestate = 0;
		return;
	}

	if (choicestate == 30) { // Digit 0
		settimedigit0++;
		if (settimedigit0 > 2) {
			settimedigit0 = 0;
		}
		return;
	}

	if (choicestate == 40) { // Set time digit 1
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

	if (choicestate == 50) { // Set time digit 2
		settimedigit2++;
		if (settimedigit2 > 5) {
			settimedigit2 = 0;
		}
		return;
	}

	if (choicestate == 60) { // Set time digit 3
		settimedigit3++;
		if (settimedigit3 > 9) {
			settimedigit3 = 0;
		}
		return;
	}

	if (choicestate == 70) { // Set time ok

		// Do stuff to set the time here but on the arduino.
		// Update time vars

		choicestate = 0; // Return to dashboard
		return;
	}


	if (choicestate == 100) { // Speed correction cancel
		choicestate = 0; // Jump to main menu
		return;
	}

	if (choicestate == 110) { // Speed correction digit 1
		spcdigit0++;
		if (spcdigit0 > 1) {
			spcdigit0 = 0;
		}
		return;
	}

	if (choicestate == 120) { // Speed correction digit 2
		spcdigit1++;
		if (spcdigit1 > 9) {
			spcdigit1 = 0;
		}
		return;
	}

	if (choicestate == 130) { // Speed correction digit 3
		spcdigit2++;
		if (spcdigit2 > 9) {
			spcdigit2 = 0;
		}
		return;
	}

	if (choicestate == 140) { // Speed correction digit 4
		spcdigit3++;
		if (spcdigit3 > 9) {
			spcdigit3 = 0;
		}
		return;
	}

	if (choicestate == 150) {

		// Do stuff to update and save the speed correction value
		choicestate = 0;
		return;
	}

	if (choicestate == 200) {
		// Set theme
		theme = 0;
		choicestate = 0;
		return;
	}

	if (choicestate == 210) {
		// Set theme
		theme = 1;
		choicestate = 0;
		return;
	}

	if (choicestate == 220) {
		// Set theme
		theme = 2;
		choicestate = 0;
		return;
	}

	if (choicestate == 230) {
		// Set theme
		theme = 3;
		choicestate = 0;
		return;
	}

	if (choicestate == 240) {
		// Set theme
		theme = 4;
		choicestate = 0;
		return;
	}

	if (choicestate == 250) {
		// Set theme
		theme = 5;
		choicestate = 0;
		return;
	}

	if (choicestate == 260) {
		// Set theme
		theme = 6;
		choicestate = 0;
		return;
	}

	// Set units menu - Cancel
	if (choicestate == 600) {
		choicestate = 0;
		return;
	}

	if (choicestate == 605) {

	}

	// Set units - OK
	if (choicestate == 635) {
		choicestate = 0;
		return;
	}



	// Odometer menu digit setting
	if (choicestate == 300) {ododigit1 = cycledigit (ododigit1, 9); return;}
	if (choicestate == 305) {ododigit2 = cycledigit (ododigit2, 9); return;}
	if (choicestate == 310) {ododigit3 = cycledigit (ododigit3, 9); return;}
	if (choicestate == 315) {ododigit4 = cycledigit (ododigit4, 9); return;}
	if (choicestate == 320) {ododigit5 = cycledigit (ododigit5, 9); return;}
	if (choicestate == 325) {ododigit6 = cycledigit (ododigit6, 9); return;}

	if (choicestate == 330) {odo2digit1 = cycledigit (odo2digit1, 9); return;}
	if (choicestate == 335) {odo2digit2 = cycledigit (odo2digit2, 9); return;}
	if (choicestate == 340) {odo2digit3 = cycledigit (odo2digit3, 9); return;}
	if (choicestate == 345) {odo2digit4 = cycledigit (odo2digit4, 9); return;}
	if (choicestate == 350) {odo2digit5 = cycledigit (odo2digit5, 9); return;}
	if (choicestate == 355) {odo2digit6 = cycledigit (odo2digit6, 9); return;}

	if (choicestate == 360) {
		// Set and save the odometer here
		if (ododigit1 != odo2digit1) {odoerror = 1; return;}
		if (ododigit2 != odo2digit2) {odoerror = 1; return;}
		if (ododigit3 != odo2digit3) {odoerror = 1; return;}
		if (ododigit4 != odo2digit4) {odoerror = 1; return;}
		if (ododigit5 != odo2digit5) {odoerror = 1; return;}
		if (ododigit6 != odo2digit6) {odoerror = 1; return;}

		int numdigitsatzero = 0;
		if (ododigit1 == 0) {numdigitsatzero++;}
		if (ododigit2 == 0) {numdigitsatzero++;}
		if (ododigit3 == 0) {numdigitsatzero++;}
		if (ododigit4 == 0) {numdigitsatzero++;}
		if (ododigit5 == 0) {numdigitsatzero++;}
		if (ododigit6 == 0) {numdigitsatzero++;}
		if (odo2digit1 == 0) {numdigitsatzero++;}
		if (odo2digit2 == 0) {numdigitsatzero++;}
		if (odo2digit3 == 0) {numdigitsatzero++;}
		if (odo2digit4 == 0) {numdigitsatzero++;}
		if (odo2digit5 == 0) {numdigitsatzero++;}
		if (odo2digit6 == 0) {numdigitsatzero++;}

		if (numdigitsatzero == 12) {
			odoerror = 2;
			return;
		}

		choicestate = 0; 
	}
}


int GetDiff (int n1, int n2) {
	if (n1 >= n2) {
		return n1 - n2;
	}

	if (n2 > n1) {
		return n2 - n1;
	}
	return 0;
}

double GetPreciseBar (int ohms) {
	int smallestdiff = 1000;
	int closestbar = 0;
	int testdiff = 0;
	int wanteddiff = 0;
	int valuefromminbar = 0;
	double tenthsbar = 0.0;

	if (ohms < barohms[0]) {
		return (double)0;
	}

	if (ohms > barohms[10]) {
		return (double)10;
	}

	for (int a=0;a<11;a++) {
		testdiff = GetDiff (ohms, barohms[a]);

		//printf ("testdiff: %i\n", testdiff);
		if (testdiff < smallestdiff) {
			smallestdiff = testdiff;
			closestbar = a;
		}
	}


	if (ohms > barohms[closestbar]) {
		wanteddiff = barohms[closestbar+1] - barohms[closestbar];
		valuefromminbar = ohms - barohms[closestbar];
		tenthsbar = (double)valuefromminbar / (double)wanteddiff;
		return (double)closestbar + tenthsbar;
	}

	if (ohms < barohms[closestbar]) {
		wanteddiff = barohms[closestbar] - barohms[closestbar-1];
		valuefromminbar = ohms - barohms[closestbar-1];
		tenthsbar = (double)valuefromminbar / (double)wanteddiff;
		return (double)closestbar-1 + tenthsbar;
	}

	if (ohms == barohms[closestbar]) {
		return (double)closestbar;
	}

	return 0.0;
}

double GetPreciseTemp (int ohms) {
	int testdiff = 0;
	int smallestdiff = 1000;
	int closesttemp = 0;
	int wanteddiff = 0;
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
		testdiff = GetDiff (ohms, tempohms[a]);

		if (testdiff < smallestdiff) {
			smallestdiff = testdiff;
			closesttemp = a;
		}
	}

	//printf ("closesttemp: %i\n", closesttemp);

	if (ohms > tempohms[closesttemp]) {
		wanteddiff = tempohms[closesttemp-1] - tempohms[closesttemp];
		numdiff = tempnum[closesttemp] - tempnum[closesttemp-1];
		valuefrommintemp = ohms - tempohms[closesttemp];
		tenthstemp = (double)valuefrommintemp / (double)wanteddiff;

		//printf ("MNNumdiff: %i\n", numdiff);
		//printf ("MNAddition: %.2f\n", (tenthstemp * numdiff));

		return (double)tempnum[closesttemp] - (tenthstemp * numdiff);
	}

	if (ohms < tempohms[closesttemp]) {
		wanteddiff = tempohms[closesttemp] - tempohms[closesttemp+1];
		numdiff = tempnum[closesttemp+1] - tempnum[closesttemp];
		valuefrommintemp = ohms - tempohms[closesttemp+1];
		tenthstemp = (double)valuefrommintemp / (double)wanteddiff;

		//printf ("Numdiff: %i\n", numdiff);
		//printf ("Addition: %.2f\n", (tenthstemp * numdiff));

		return (double)tempnum[closesttemp+1] - (tenthstemp * numdiff);
	}

	if (ohms == tempohms[closesttemp]) {
		return (double)tempnum[closesttemp];
	}

	return 0.0;
}


SDL_Surface *Loadsurface (SDL_Surface *existing, const char *file, const char *theme)
{
	if (existing != NULL) {
		SDL_FreeSurface (existing);
	}

	char szFile[255];
	memset(szFile, 0, 255);

	if (theme != NULL) {
		strcat (szFile, theme);
		strcat (szFile, "-");	
	}
	
	strcat (szFile, file);

	return SDL_LoadBMP (szFile);
}

int loadsurfaces(const char *theme)
{
	// Load Images into Surfaces
	// Rev number lines - needed to be individual textures to preserve the curve when revealing the rev line	


	gR1213 = Loadsurface (gR1213, "R12-13.bmp", theme);
	if (gR1213 == NULL) { fprintf(stderr, "could not load R12-13: %s\n", SDL_GetError()); return 1; }

	gR11 = Loadsurface(gR11, "R11.bmp", theme);
	if (gR11 == NULL) { fprintf(stderr, "could not load R11: %s\n", SDL_GetError()); return 1; }

	gR10 = Loadsurface(gR10, "R10.bmp", theme);
	if (gR10 == NULL) { fprintf(stderr, "could not load R10: %s\n", SDL_GetError()); return 1; }

	gR9 = Loadsurface(gR9, "R9.bmp", theme);
	if (gR9 == NULL) { fprintf(stderr, "could not load R9: %s\n", SDL_GetError()); return 1; }

	gR8 = Loadsurface(gR8, "R8.bmp", theme);
	if (gR8 == NULL) { fprintf(stderr, "could not load R8: %s\n", SDL_GetError()); return 1; }

	gR7 = Loadsurface(gR7, "R7.bmp", theme);
	if (gR7 == NULL) { fprintf(stderr, "could not load R7: %s\n", SDL_GetError()); return 1; }

	gR6 = Loadsurface(gR6, "R6.bmp", theme);
	if (gR6 == NULL) { fprintf(stderr, "could not load R6: %s\n", SDL_GetError()); return 1; }

	gR5 = Loadsurface(gR5, "R5.bmp", theme);
	if (gR5 == NULL) { fprintf(stderr, "could not load R5: %s\n", SDL_GetError()); return 1; }

	gR4 = Loadsurface(gR4, "R4.bmp", theme);
	if (gR4 == NULL) { fprintf(stderr, "could not load R4: %s\n", SDL_GetError()); return 1; }

	gR3 = Loadsurface(gR3, "R3.bmp", theme);
	if (gR3 == NULL) { fprintf(stderr, "could not load R3: %s\n", SDL_GetError()); return 1; }

	gR2 = Loadsurface(gR2, "R2.bmp", theme);
	if (gR2 == NULL) { fprintf(stderr, "could not load R2: %s\n", SDL_GetError()); return 1; }

	gR1 = Loadsurface(gR1, "R1.bmp", theme);
	if (gR1 == NULL) { fprintf(stderr, "could not load R1: %s\n", SDL_GetError()); return 1; }

	gR0 = Loadsurface(gR0, "R0.bmp", theme);
	if (gR0 == NULL) { fprintf(stderr, "could not load R0: %s\n", SDL_GetError()); return 1; }

	// The blue rev line
	gRevline = Loadsurface(gRevline, "Revline.bmp", theme);
	if (gRevline == NULL) { fprintf(stderr, "could not load Revline: %s\n", SDL_GetError()); return 1; }

	// A white square covering the rev line - rotation is applied to this to reveal the rev line
	gRevwhite = Loadsurface(gRevwhite, "Whitesq.bmp", theme);
	if (gRevwhite == NULL) { fprintf(stderr, "could not load Revwhite: %s\n", SDL_GetError()); return 1; }

	// Large speedometer numbers
	gSpeednumbers = Loadsurface(gSpeednumbers, "Speednumbers.bmp", theme);
	if (gSpeednumbers == NULL) { fprintf(stderr, "could not load Speednumbers: %s\n", SDL_GetError()); return 1; }

	gSmallnumbers = Loadsurface(gSmallnumbers, "Smallnumbers.bmp", theme);
	if (gSmallnumbers == NULL) { fprintf(stderr, "could not load Smallnumbers: %s\n", SDL_GetError()); return 1; }	

	gTopiconsgrey = Loadsurface(gTopiconsgrey, "Topiconsgrey.bmp", theme);
	if (gTopiconsgrey == NULL) { fprintf(stderr, "could not load Topiconsgrey: %s\n", SDL_GetError()); return 1; }	

	gTopiconsgreyOP = Loadsurface(gTopiconsgreyOP, "TopiconsgreyOP.bmp", theme);
	if (gTopiconsgreyOP == NULL) { fprintf(stderr, "could not load TopiconsgreyOP: %s\n", SDL_GetError()); return 1; }	

	gTopiconsedge1 = Loadsurface(gTopiconsedge1, "Topiconedge1.bmp", theme);
	if (gTopiconsedge1 == NULL) { fprintf(stderr, "could not load Topiconedge1: %s\n", SDL_GetError()); return 1; }	

	gTopiconsedge2 = Loadsurface(gTopiconsedge2, "Topiconedge2.bmp", theme);
	if (gTopiconsedge2 == NULL) { fprintf(stderr, "could not load Topiconedge2: %s\n", SDL_GetError()); return 1; }	

	gMileinfo = Loadsurface(gMileinfo, "Mileinfo.bmp", theme);
	if (gMileinfo == NULL) { fprintf(stderr, "could not load Mileinfo: %s\n", SDL_GetError()); return 1; }

	gFuelgauge = Loadsurface(gFuelgauge, "Fuelgauge.bmp", theme);
	if (gFuelgauge == NULL) { fprintf(stderr, "could not load Fuelgauge: %s\n", SDL_GetError()); return 1; }	

	gFuelwhite = Loadsurface(gFuelwhite, "Fuelgaugewhite.bmp", theme);
	if (gFuelwhite == NULL) { fprintf(stderr, "could not load Fuelgaugewhite: %s\n", SDL_GetError()); return 1; }

	gSpeedcorrection = Loadsurface(gSpeedcorrection, "Speedcorrection.bmp", theme);
	if (gSpeedcorrection == NULL) { fprintf(stderr, "could not load gSpeedcorrection: %s\n", SDL_GetError()); return 1; }

	gSettime = Loadsurface(gSettime, "Settime.bmp", theme);
	if (gSettime == NULL) { fprintf(stderr, "could not load gSettime: %s\n", SDL_GetError()); return 1; }

	gSetunits = Loadsurface (gSetunits, "Setunits.bmp", theme);
	if (gSetunits == NULL) { fprintf(stderr, "could not load gSetunits: %s\n", SDL_GetError()); return 1; }

	gSelecton = Loadsurface (gSelecton, "Selecton.bmp", theme);
	if (gSelecton == NULL) { fprintf(stderr, "could not load gSelecton: %s\n", SDL_GetError()); return 1; }

	gMenuoptions = Loadsurface(gMenuoptions, "Menuoptionsex.bmp", theme);
	if (gMenuoptions == NULL) { fprintf(stderr, "could not load gMenuoptions: %s\n", SDL_GetError()); return 1; }

	gControloptions = Loadsurface(gControloptions, "Controloptions.bmp", theme);
	if (gControloptions == NULL) { fprintf(stderr, "could not load gControloptions: %s\n", SDL_GetError()); return 1; }	

	gControlselect = Loadsurface(gControlselect, "Selectedcontrol.bmp", theme);
	if (gControlselect == NULL) { fprintf(stderr, "could not load gControlselect: %s\n", SDL_GetError()); return 1; }	

	gLightoptions = Loadsurface(gLightoptions, "Lightoptions.bmp", theme);
	if (gLightoptions == NULL) { fprintf(stderr, "could not load gLightoptions: %s\n", SDL_GetError()); return 1; }

	gWhitethumb = Loadsurface(gWhitethumb, "whitethumb.bmp", NULL);
	if (gWhitethumb == NULL) { fprintf(stderr, "could not load gWhitethumb: %s\n", SDL_GetError()); return 1; }

	gBrightthumb = Loadsurface(gBrightthumb, "brightthumb.bmp", NULL);
	if (gBrightthumb == NULL) { fprintf(stderr, "could not load gBrightthumb: %s\n", SDL_GetError()); return 1; }

	gDarkthumb = Loadsurface(gDarkthumb, "darkthumb.bmp", NULL);
	if (gDarkthumb == NULL) { fprintf(stderr, "could not load gDarkthumb: %s\n", SDL_GetError()); return 1; }

	gGreenthumb = Loadsurface(gGreenthumb, "greenthumb.bmp", NULL);
	if (gGreenthumb == NULL) { fprintf(stderr, "could not load gGreenthumb: %s\n", SDL_GetError()); return 1; }

	gRedthumb = Loadsurface(gRedthumb, "redthumb.bmp", NULL);
	if (gRedthumb == NULL) { fprintf(stderr, "could not load gRedthumb: %s\n", SDL_GetError()); return 1; }

	gBluethumb = Loadsurface(gBluethumb, "bluethumb.bmp", NULL);
	if (gBluethumb == NULL) { fprintf(stderr, "could not load gBluethumb: %s\n", SDL_GetError()); return 1; }

	gOrangethumb = Loadsurface(gOrangethumb, "orangethumb.bmp", NULL);
	if (gOrangethumb == NULL) { fprintf(stderr, "could not load gOrangethumb: %s\n", SDL_GetError()); return 1; }

	gYellowthumb = Loadsurface(gYellowthumb, "yellowthumb.bmp", NULL);
	if (gYellowthumb == NULL) { fprintf(stderr, "could not load gYellowthumb: %s\n", SDL_GetError()); return 1; }

	gNightthumb = Loadsurface(gNightthumb, "nightthumb.bmp", NULL);
	if (gNightthumb == NULL) { fprintf(stderr, "could not load gNightthumb: %s\n", SDL_GetError()); return 1; }

	gTPMSoptions = Loadsurface(gTPMSoptions, "TPMSsetup.bmp", theme);
	if (gTPMSoptions == NULL) { fprintf(stderr, "could not load gTPMSoptions: %s\n", SDL_GetError()); return 1; }

	gUparrow = Loadsurface(gUparrow, "Uparrow.bmp", theme);
	if (gUparrow == NULL) { fprintf(stderr, "could not load gUparrow: %s\n", SDL_GetError()); return 1; }

	gUparrowsmall = Loadsurface(gUparrowsmall, "Uparrowsmall.bmp", theme);
	if (gUparrowsmall == NULL) { fprintf(stderr, "could not load gUparrowsmall: %s\n", SDL_GetError()); return 1; }	

	gMenuarrowright = Loadsurface(gMenuarrowright, "Menuarrowright.bmp", theme);
	if (gMenuarrowright == NULL) { fprintf(stderr, "could not load gMenuarrowright: %s\n", SDL_GetError()); return 1; }

	gMenuarrowleft = Loadsurface(gMenuarrowleft, "Menuarrowleft.bmp", theme);
	if (gMenuarrowleft == NULL) { fprintf(stderr, "could not load gMenuarrowleft: %s\n", SDL_GetError()); return 1; }

	gMenusmallarrowright = Loadsurface(gMenusmallarrowright, "Rightmenuarrowex.bmp", theme);
	if (gMenusmallarrowright == NULL) { fprintf(stderr, "could not load gMenusmallarrowright: %s\n", SDL_GetError()); return 1; }

	gMenusmallarrowleft = Loadsurface(gMenusmallarrowleft, "Leftmenuarrowex.bmp", theme);
	if (gMenusmallarrowleft == NULL) { fprintf(stderr, "could not load gMenusmallarrowleft: %s\n", SDL_GetError()); return 1; }

	gCoolant = Loadsurface(gCoolant, "Coolant.bmp", theme);
	if (gCoolant == NULL) { fprintf(stderr, "could not load gCoolant: %s\n", SDL_GetError()); return 1; }

	gCoolantF = Loadsurface(gCoolantF, "CoolantF.bmp", theme);
	if (gCoolantF == NULL) { fprintf(stderr, "could not load gCoolantF: %s\n", SDL_GetError()); return 1; }

	gKm = Loadsurface(gKm, "km.bmp", theme);
	if (gKm == NULL) { fprintf(stderr, "could not load gKm: %s\n", SDL_GetError()); return 1; }

	gMiles = Loadsurface(gMiles, "Miles.bmp", theme);
	if (gMiles == NULL) { fprintf(stderr, "could not load gMiles: %s\n", SDL_GetError()); return 1; }

	gKph = Loadsurface(gKph, "kph.bmp", theme);
	if (gKph == NULL) { fprintf(stderr, "could not load gKph: %s\n", SDL_GetError()); return 1; }

	gMph = Loadsurface(gMph, "mph.bmp", theme);
	if (gMph == NULL) { fprintf(stderr, "could not load gMph: %s\n", SDL_GetError()); return 1; }

	gTyreicon = Loadsurface(gTyreicon, "tyreicon.bmp", theme);
	if (gTyreicon == NULL) { fprintf(stderr, "could not load gTyreicon: %s\n", SDL_GetError()); return 1; }

	gTyresignal = Loadsurface(gTyresignal, "tyresignal.bmp", theme);
	if (gTyresignal == NULL) { fprintf(stderr, "could not load gTyresignal: %s\n", SDL_GetError()); return 1; }	

	gHighbeamlight = Loadsurface(gHighbeamlight, "Highbeamlight.bmp", theme);
	if (gHighbeamlight == NULL) { fprintf(stderr, "could not load gHighbeamlight: %s\n", SDL_GetError()); return 1; }

	gIndicateright = Loadsurface(gIndicateright, "Indicateright.bmp", theme);
	if (gIndicateright == NULL) { fprintf(stderr, "could not load gIndicateright: %s\n", SDL_GetError()); return 1; }

	gIndicateleft = Loadsurface(gIndicateleft, "Indicateleft.bmp", theme);
	if (gIndicateleft == NULL) { fprintf(stderr, "could not load gIndicateleft: %s\n", SDL_GetError()); return 1; }

	gIndicateboth = Loadsurface(gIndicateboth, "Indicateboth.bmp", theme);
	if (gIndicateboth == NULL) { fprintf(stderr, "could not load gIndicateboth: %s\n", SDL_GetError()); return 1; }

	gIndicaterightfar = Loadsurface(gIndicaterightfar, "Indicaterightfar.bmp", theme);
	if (gIndicaterightfar == NULL) { fprintf(stderr, "could not load gIndicaterightfar: %s\n", SDL_GetError()); return 1; }

	gIndicateleftfar = Loadsurface(gIndicateleftfar, "Indicateleftfar.bmp", theme);
	if (gIndicateleftfar == NULL) { fprintf(stderr, "could not load gIndicateleftfar: %s\n", SDL_GetError()); return 1; }

	gOillight = Loadsurface(gOillight, "Oillight.bmp", theme);
	if (gOillight == NULL) { fprintf(stderr, "could not load gOillight: %s\n", SDL_GetError()); return 1; }

	gOillightOP = Loadsurface(gOillightOP, "OillightOP.bmp", theme);
	if (gOillightOP == NULL) { fprintf(stderr, "could not load gOillightOP: %s\n", SDL_GetError()); return 1; }

	gNeutrallight = Loadsurface(gNeutrallight, "Neutrallight.bmp", theme);
	if (gNeutrallight == NULL) { fprintf(stderr, "could not load gNeutrallight: %s\n", SDL_GetError()); return 1; }

	gEngineoverheat = Loadsurface(gEngineoverheat, "Engineoverheat.bmp", theme);
	if (gEngineoverheat == NULL) { fprintf(stderr, "could not load gEngineoverheat: %s\n", SDL_GetError()); return 1; }

	gOverheatbadge = Loadsurface(gOverheatbadge, "Overheatbadge.bmp", theme);
	if (gOverheatbadge == NULL) { fprintf(stderr, "could not load gOverheatbadge: %s\n", SDL_GetError()); return 1; }

	gLowoil = Loadsurface(gLowoil, "Lowoil.bmp", theme);
	if (gLowoil == NULL) { fprintf(stderr, "could not load gLowoil: %s\n", SDL_GetError()); return 1; }

	gLowoilbadge = Loadsurface(gLowoilbadge, "Lowoilbadge.bmp", theme);
	if (gLowoilbadge == NULL) { fprintf(stderr, "could not load gLowoilbadge: %s\n", SDL_GetError()); return 1; }

	gLowfuel = Loadsurface(gLowfuel, "Lowfuel.bmp", theme);
	if (gLowfuel == NULL) { fprintf(stderr, "could not load gLowfuel: %s\n", SDL_GetError()); return 1; }

	gLowfuelbadge = Loadsurface(gLowfuelbadge, "Fuelwarningbadge.bmp", theme);
	if (gLowfuelbadge == NULL) { fprintf(stderr, "could not load gLowfuelbadge: %s\n", SDL_GetError()); return 1; }

	gInfobottomdiag = Loadsurface(gInfobottomdiag, "Infobottomdiag.bmp", theme);
	if (gInfobottomdiag == NULL) { fprintf(stderr, "could not load gInfobottomdiag: %s\n", SDL_GetError()); return 1; }

	gInfotopdiag = Loadsurface(gInfotopdiag, "Infotopdiag.bmp", theme);
	if (gInfotopdiag == NULL) { fprintf(stderr, "could not load gInfotopdiag: %s\n", SDL_GetError()); return 1; }

	gInfobottom = Loadsurface(gInfobottom, "Infobottom.bmp", theme);
	if (gInfobottom == NULL) { fprintf(stderr, "could not load gInfobottom: %s\n", SDL_GetError()); return 1; }

	gInfotop = Loadsurface(gInfotop, "Infotop.bmp", theme);
	if (gInfotop == NULL) { fprintf(stderr, "could not load gInfotop: %s\n", SDL_GetError()); return 1; }

	gInfotopKM = Loadsurface(gInfotopKM, "InfotopKM.bmp", theme);
	if (gInfotopKM == NULL) { fprintf(stderr, "could not load gInfotopKM: %s\n", SDL_GetError()); return 1; }

	gTyrebottom = Loadsurface(gTyrebottom, "tyrebottom.bmp", theme);
	if (gTyrebottom == NULL) { fprintf(stderr, "could not load gTyrebottom: %s\n", SDL_GetError()); return 1; }

	gTyretop = Loadsurface(gTyretop, "tyretop.bmp", theme);
	if (gTyretop == NULL) { fprintf(stderr, "could not load gTyretop: %s\n", SDL_GetError()); return 1; }

	gCoolanticon = Loadsurface(gCoolanticon, "Coolanticon.bmp", theme);
	if (gCoolanticon == NULL) { fprintf(stderr, "could not load gCoolanticon: %s\n", SDL_GetError()); return 1; }

	gThemeoptions = Loadsurface(gThemeoptions, "Themeoptions.bmp", NULL);
	if (gThemeoptions == NULL) { fprintf(stderr, "could not load gThemeoptions: %s\n", SDL_GetError()); return 1; }

	gArrowrighttheme = Loadsurface(gArrowrighttheme, "Arrowrighttheme.bmp", NULL);
	if (gArrowrighttheme == NULL) { fprintf(stderr, "could not load gArrowrighttheme: %s\n", SDL_GetError()); return 1; }

	gArrowlefttheme = Loadsurface(gArrowlefttheme, "Arrowlefttheme.bmp", NULL);
	if (gArrowlefttheme == NULL) { fprintf(stderr, "could not load gArrowlefttheme: %s\n", SDL_GetError()); return 1; }

	gDownarrow = Loadsurface(gDownarrow, "Downarrow.bmp", theme);
	if (gDownarrow == NULL) { fprintf(stderr, "could not load gDownarrow: %s\n", SDL_GetError()); return 1; }

	gSetodometer = Loadsurface(gSetodometer, "Setodometer.bmp", NULL);
	if (gSetodometer == NULL) { fprintf(stderr, "could not load gSetodometer: %s\n", SDL_GetError()); return 1; }

	gOdoerror1 = Loadsurface(gOdoerror1, "Odoerror1.bmp", NULL);
	if (gOdoerror1 == NULL) { fprintf(stderr, "could not load gOdoerror1: %s\n", SDL_GetError()); return 1; }

	gOdoerror2 = Loadsurface(gOdoerror2, "Odoerror2.bmp", NULL);
	if (gOdoerror2 == NULL) { fprintf(stderr, "could not load gOdoerror2: %s\n", SDL_GetError()); return 1; }

	gSprocketsetup = Loadsurface(gSprocketsetup, "Sprocketsetup.bmp", theme);
	if (gSprocketsetup == NULL) { fprintf(stderr, "could not load gSprocketsetup: %s\n", SDL_GetError()); return 1; }	

	gCoolantfantemp = Loadsurface(gCoolantfantemp, "Coolantfantemp.bmp", theme);
	if (gCoolantfantemp == NULL) { fprintf(stderr, "could not load gCoolantfantemp: %s\n", SDL_GetError()); return 1; }	

	gLowtyrebadge = Loadsurface (gLowtyrebadge, "Lowtyrebadge.bmp", theme);
	if (gLowtyrebadge == NULL) { fprintf(stderr, "could not load gLowtyrebadge: %s\n", SDL_GetError()); return 1; }	

	gReartyrelow = Loadsurface (gReartyrelow, "Reartyrelow.bmp", theme);
	if (gReartyrelow == NULL) { fprintf(stderr, "could not load gReartyrelow: %s\n", SDL_GetError()); return 1; }	

	gFronttyrelow = Loadsurface (gFronttyrelow, "Fronttyrelow.bmp", theme);
	if (gFronttyrelow == NULL) { fprintf(stderr, "could not load gFronttyrelow: %s\n", SDL_GetError()); return 1; }

	gBothtyrelow = Loadsurface (gBothtyrelow, "Frontrearlow.bmp", theme);
	if (gBothtyrelow == NULL) { fprintf(stderr, "could not load gBothtyrelow: %s\n", SDL_GetError()); return 1; }

	gGear = Loadsurface(gGear, "Gear.bmp", theme);
	if (gGear == NULL) { fprintf(stderr, "could not load gGear: %s\n", SDL_GetError()); return 1; }	

	gNavbg = Loadsurface(gNavbg, "Navbg.bmp", theme);
	if (gNavbg == NULL) { fprintf(stderr, "could not load gNavbg: %s\n", SDL_GetError()); return 1; }

	gNavicons = Loadsurface(gNavicons, "Navgfx.bmp", theme);
	if (gNavicons == NULL) { fprintf(stderr, "could not load gNavicons: %s\n", SDL_GetError()); return 1; }

	return 0;
}

void initrects()
{
	// Rev Counter
	rectg1213.x = 898; rectg1213.y = 61; rectg1213.w = 108; rectg1213.h = 102;
	rectg11.x = 846; rectg11.y = 78; rectg11.w = 48; rectg11.h = 97;
	rectg10.x = 796; rectg10.y = 95; rectg10.w = 45; rectg10.h = 96;
	rectg9.x = 739; rectg9.y = 117; rectg9.w = 52; rectg9.h = 89;
	rectg8.x = 688; rectg8.y = 145; rectg8.w = 58; rectg8.h = 86;
	rectg7.x = 641; rectg7.y = 179; rectg7.w = 64; rectg7.h = 80;
	rectg6.x = 601; rectg6.y = 219; rectg6.w = 72; rectg6.h = 68;
	rectg5.x = 564; rectg5.y = 258; rectg5.w = 79; rectg5.h = 62;
	rectg4.x = 533; rectg4.y = 300; rectg4.w = 82; rectg4.h = 58;
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
	spdDigitone.x = 614;
	spdDigitone.y = 363;
	
	spdDigittwo.x = 735;
	spdDigittwo.y = 363;
	
	spdDigitthree.x = 875;
	spdDigitthree.y = 363;
}

/*
SDL_Texture* texturefromsurface (SDL_Surface *surface, char *surfacename) {

	SDL_Texture *tex;
	if (surface != NULL) {
		tex = SDL_CreateTextureFromSurface (renderer, surface);

		if (tex == NULL) {
			fprintf(stderr, "could not convert %s to texture: %s\n", surfacename, SDL_GetError());
		} else {
			return tex;
		}
	} else {
		fprintf(stderr, "%s was NULL: %s\n", surfacename);
	}

	return NULL;
}

int inittexturesex () {

}

*/

int inittextures()
{	
	if (grevlinetex != NULL) {SDL_DestroyTexture (grevlinetex);}
	grevlinetex = SDL_CreateTextureFromSurface(renderer, gRevline);
	if (grevlinetex == NULL) {
		fprintf(stderr, "could not convert gRevline to texture: %s\n", SDL_GetError());
		return 1;
	}
	
	if (grevwhitetex != NULL) {SDL_DestroyTexture (grevwhitetex);}
	grevwhitetex = SDL_CreateTextureFromSurface(renderer, gRevwhite);
	if (grevwhitetex == NULL) {
		fprintf(stderr, "could not convert gRevwhite to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gspeednumberstex != NULL) {SDL_DestroyTexture (gspeednumberstex);}
	gspeednumberstex = SDL_CreateTextureFromSurface(renderer, gSpeednumbers);
	if (gspeednumberstex == NULL) {
		fprintf(stderr, "could not convert gSpeednumbers to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr1213tex != NULL) {SDL_DestroyTexture (gr1213tex);}
	gr1213tex = SDL_CreateTextureFromSurface(renderer, gR1213);
	if (gr1213tex == NULL) {
		fprintf(stderr, "could not convert gR1213 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr11tex != NULL) {SDL_DestroyTexture (gr11tex);}
	gr11tex = SDL_CreateTextureFromSurface(renderer, gR11);
	if (gr11tex == NULL) {
		fprintf(stderr, "could not convert gR11 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr10tex != NULL) {SDL_DestroyTexture (gr10tex);}
	gr10tex = SDL_CreateTextureFromSurface(renderer, gR10);
	if (gr10tex == NULL) {
		fprintf(stderr, "could not convert gR10 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr9tex != NULL) {SDL_DestroyTexture (gr9tex);}
	gr9tex = SDL_CreateTextureFromSurface(renderer, gR9);
	if (gr9tex == NULL) {
		fprintf(stderr, "could not convert gR9 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr8tex != NULL) {SDL_DestroyTexture (gr8tex);}
	gr8tex = SDL_CreateTextureFromSurface(renderer, gR8);
	if (gr8tex == NULL) {
		fprintf(stderr, "could not convert gR8 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr7tex != NULL) {SDL_DestroyTexture (gr7tex);}
	gr7tex = SDL_CreateTextureFromSurface(renderer, gR7);
	if (gr7tex == NULL) {
		fprintf(stderr, "could not convert gR7 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr6tex != NULL) {SDL_DestroyTexture (gr6tex);}
	gr6tex = SDL_CreateTextureFromSurface(renderer, gR6);
	if (gr6tex == NULL) {
		fprintf(stderr, "could not convert gR6 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr5tex != NULL) {SDL_DestroyTexture (gr5tex);}
	gr5tex = SDL_CreateTextureFromSurface(renderer, gR5);
	if (gr5tex == NULL) {
		fprintf(stderr, "could not convert gR5 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr4tex != NULL) {SDL_DestroyTexture (gr4tex);}
	gr4tex = SDL_CreateTextureFromSurface(renderer, gR4);
	if (gr4tex == NULL) {
		fprintf(stderr, "could not convert gR4 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr3tex != NULL) {SDL_DestroyTexture (gr3tex);}
	gr3tex = SDL_CreateTextureFromSurface(renderer, gR3);
	if (gr3tex == NULL) {
		fprintf(stderr, "could not convert gR3 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr2tex != NULL) {SDL_DestroyTexture (gr2tex);}
	gr2tex = SDL_CreateTextureFromSurface(renderer, gR2);
	if (gr2tex == NULL) {
		fprintf(stderr, "could not convert gR2 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr1tex != NULL) {SDL_DestroyTexture (gr1tex);}
	gr1tex = SDL_CreateTextureFromSurface(renderer, gR1);
	if (gr1tex == NULL) {
		fprintf(stderr, "could not convert gR1 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gr0tex != NULL) {SDL_DestroyTexture (gr0tex);}
	gr0tex = SDL_CreateTextureFromSurface(renderer, gR0);
	if (gr0tex == NULL) {
		fprintf(stderr, "could not convert gR0 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gsmallnumberstex != NULL) {SDL_DestroyTexture (gsmallnumberstex);}
	gsmallnumberstex = SDL_CreateTextureFromSurface(renderer, gSmallnumbers);
	if (gsmallnumberstex == NULL) {
		fprintf(stderr, "could not convert gSmallnumbers to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gtopiconsgreytex != NULL) {SDL_DestroyTexture (gtopiconsgreytex);}
	gtopiconsgreytex = SDL_CreateTextureFromSurface(renderer, gTopiconsgrey);
	if (gtopiconsgreytex == NULL) {
		fprintf(stderr, "could not convert gTopiconsgrey to texture: %s\n", SDL_GetError());
		return 1;
	}		

	if (gtopiconsgreyoptex != NULL) {SDL_DestroyTexture (gtopiconsgreyoptex);}
	gtopiconsgreyoptex = SDL_CreateTextureFromSurface(renderer, gTopiconsgreyOP);
	if (gtopiconsgreyoptex == NULL) {
		fprintf(stderr, "could not convert gTopiconsgreyOP to texture: %s\n", SDL_GetError());
		return 1;
	}		

	if (gtopiconsedge1tex != NULL) {SDL_DestroyTexture (gtopiconsedge1tex);}
	gtopiconsedge1tex = SDL_CreateTextureFromSurface(renderer, gTopiconsedge1);
	if (gtopiconsedge1tex == NULL) {
		fprintf(stderr, "could not convert gTopiconsedge1 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gtopiconsedge2tex != NULL) {SDL_DestroyTexture (gtopiconsedge2tex);}
	gtopiconsedge2tex = SDL_CreateTextureFromSurface(renderer, gTopiconsedge2);
	if (gtopiconsedge2tex == NULL) {
		fprintf(stderr, "could not convert gTopiconsedge2 to texture: %s\n", SDL_GetError());
		return 1;
	}		


	if (gmileinfotex != NULL) {SDL_DestroyTexture (gmileinfotex);}
	gmileinfotex = SDL_CreateTextureFromSurface(renderer, gMileinfo);
	if (gmileinfotex == NULL) {
		fprintf(stderr, "could not convert gMileinfo to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gfuelgaugetex != NULL) {SDL_DestroyTexture (gfuelgaugetex);}
	gfuelgaugetex = SDL_CreateTextureFromSurface(renderer, gFuelgauge);
	if (gfuelgaugetex == NULL) {
		fprintf(stderr, "could not convert gFuelgauge to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gfuelgaugewhitetex != NULL) {SDL_DestroyTexture (gfuelgaugewhitetex);}
	gfuelgaugewhitetex = SDL_CreateTextureFromSurface(renderer, gFuelwhite);
	if (gfuelgaugewhitetex == NULL) {
		fprintf(stderr, "could not convert gFuelwhite to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gLightoptionstex != NULL) {SDL_DestroyTexture (gLightoptionstex);}
	gLightoptionstex = SDL_CreateTextureFromSurface(renderer, gLightoptions);
	if (gLightoptionstex == NULL) {
		fprintf(stderr, "could not convert gLightoptions to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gWhitethumbtex != NULL) {SDL_DestroyTexture (gWhitethumbtex);}
	gWhitethumbtex = SDL_CreateTextureFromSurface(renderer, gWhitethumb);
	if (gWhitethumbtex == NULL) {
		fprintf(stderr, "could not convert gWhitethumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gBrightthumbtex != NULL) {SDL_DestroyTexture (gBrightthumbtex);}
	gBrightthumbtex = SDL_CreateTextureFromSurface(renderer, gBrightthumb);
	if (gBrightthumbtex == NULL) {
		fprintf(stderr, "could not convert gBrightthumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gDarkthumbtex != NULL) {SDL_DestroyTexture (gDarkthumbtex);}
	gDarkthumbtex = SDL_CreateTextureFromSurface(renderer, gDarkthumb);
	if (gDarkthumbtex == NULL) {
		fprintf(stderr, "could not convert gDarkthumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gGreenthumbtex != NULL) {SDL_DestroyTexture (gGreenthumbtex);}
	gGreenthumbtex = SDL_CreateTextureFromSurface(renderer, gGreenthumb);
	if (gGreenthumbtex == NULL) {
		fprintf(stderr, "could not convert gGreenthumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gRedthumbtex != NULL) {SDL_DestroyTexture (gRedthumbtex);}
	gRedthumbtex = SDL_CreateTextureFromSurface(renderer, gRedthumb);
	if (gRedthumbtex == NULL) {
		fprintf(stderr, "could not convert gRedthumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gBluethumbtex != NULL) {SDL_DestroyTexture (gBluethumbtex);}
	gBluethumbtex = SDL_CreateTextureFromSurface(renderer, gBluethumb);
	if (gBluethumbtex == NULL) {
		fprintf(stderr, "could not convert gBluethumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gOrangethumbtex != NULL) {SDL_DestroyTexture (gOrangethumbtex);}
	gOrangethumbtex = SDL_CreateTextureFromSurface(renderer, gOrangethumb);
	if (gOrangethumbtex == NULL) {
		fprintf(stderr, "could not convert gOrangethumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gYellowthumbtex != NULL) {SDL_DestroyTexture (gYellowthumbtex);}
	gYellowthumbtex = SDL_CreateTextureFromSurface(renderer, gYellowthumb);
	if (gYellowthumbtex == NULL) {
		fprintf(stderr, "could not convert gYellowthumb to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gNightthumbtex != NULL) {SDL_DestroyTexture (gNightthumbtex);}
	gNightthumbtex = SDL_CreateTextureFromSurface(renderer, gNightthumb);
	if (gNightthumbtex == NULL) {
		fprintf(stderr, "could not convert gNightthumb to texture: %s\n", SDL_GetError());
		return 1;
	}


	if (gSpeedcorrectiontex != NULL) {SDL_DestroyTexture (gSpeedcorrectiontex);}
	gSpeedcorrectiontex = SDL_CreateTextureFromSurface(renderer, gSpeedcorrection);
	if (gSpeedcorrectiontex == NULL) {
		fprintf(stderr, "could not convert gSpeedcorrection to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gSettimetex != NULL) {SDL_DestroyTexture (gSettimetex);}
	gSettimetex = SDL_CreateTextureFromSurface(renderer, gSettime);
	if (gSettimetex == NULL) {
		fprintf(stderr, "could not convert gSettime to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gSetunitstex != NULL) {SDL_DestroyTexture (gSetunitstex);}
	gSetunitstex = SDL_CreateTextureFromSurface (renderer, gSetunits);
	if (gSetunitstex == NULL) {
		fprintf(stderr, "could not convert gSetunits to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gSelectontex != NULL) {SDL_DestroyTexture (gSelectontex);}
	gSelectontex = SDL_CreateTextureFromSurface (renderer, gSelecton);
	if (gSelectontex == NULL) {
		fprintf(stderr, "could not convert gSelecton to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gMenuoptionstex != NULL) {SDL_DestroyTexture (gMenuoptionstex);}
	gMenuoptionstex = SDL_CreateTextureFromSurface(renderer, gMenuoptions);
	if (gMenuoptionstex == NULL) {
		fprintf(stderr, "could not convert gMenuoptions to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gControloptionstex != NULL) {SDL_DestroyTexture (gControloptionstex);}
	gControloptionstex = SDL_CreateTextureFromSurface(renderer, gControloptions);
	if (gControloptionstex == NULL) {
		fprintf(stderr, "could not convert gControloptions to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gControlselecttex != NULL) {SDL_DestroyTexture (gControlselecttex);}
	gControlselecttex = SDL_CreateTextureFromSurface(renderer, gControlselect);
	if (gControlselecttex == NULL) {
		fprintf(stderr, "could not convert gControlselect to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gTPMSoptionstex != NULL) {SDL_DestroyTexture (gTPMSoptionstex);}
	gTPMSoptionstex = SDL_CreateTextureFromSurface(renderer, gTPMSoptions);
	if (gTPMSoptionstex == NULL) {
		fprintf(stderr, "could not convert gTPMSoptions to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gUparrowtex != NULL) {SDL_DestroyTexture (gUparrowtex);}
	gUparrowtex = SDL_CreateTextureFromSurface(renderer, gUparrow);
	if (gUparrowtex == NULL) {
		fprintf(stderr, "could not convert gUparrow to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gUparrowsmalltex != NULL) {SDL_DestroyTexture (gUparrowsmalltex);}
	gUparrowsmalltex = SDL_CreateTextureFromSurface(renderer, gUparrowsmall);
	if (gUparrowsmalltex == NULL) {
		fprintf(stderr, "could not convert gUparrowsmall to texture: %s\n", SDL_GetError());
		return 1;
	}	

	if (gMenuarrowrighttex != NULL) {SDL_DestroyTexture (gMenuarrowrighttex);}
	gMenuarrowrighttex = SDL_CreateTextureFromSurface(renderer, gMenuarrowright);
	if (gMenuarrowrighttex == NULL) {
		fprintf(stderr, "could not convert gMenuarrowright to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gMenuarrowlefttex != NULL) {SDL_DestroyTexture (gMenuarrowlefttex);}
	gMenuarrowlefttex = SDL_CreateTextureFromSurface(renderer, gMenuarrowleft);
	if (gMenuarrowlefttex == NULL) {
		fprintf(stderr, "could not convert gMenuarrowleft to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gMenusmallarrowrighttex != NULL) {SDL_DestroyTexture (gMenusmallarrowrighttex);}
	gMenusmallarrowrighttex = SDL_CreateTextureFromSurface(renderer, gMenusmallarrowright);
	if (gMenusmallarrowrighttex == NULL) {
		fprintf(stderr, "could not convert gMenusmallarrowright to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gMenusmallarrowlefttex != NULL) {SDL_DestroyTexture (gMenusmallarrowlefttex);}
	gMenusmallarrowlefttex = SDL_CreateTextureFromSurface(renderer, gMenusmallarrowleft);
	if (gMenusmallarrowlefttex == NULL) {
		fprintf(stderr, "could not convert gMenusmallarrowleft to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gCoolanttex != NULL) {SDL_DestroyTexture (gCoolanttex);}
	gCoolanttex = SDL_CreateTextureFromSurface(renderer, gCoolant);
	if (gCoolanttex == NULL) {
		fprintf(stderr, "could not convert gCoolant to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gCoolantFtex != NULL) {SDL_DestroyTexture (gCoolantFtex);}
	gCoolantFtex = SDL_CreateTextureFromSurface(renderer, gCoolantF);
	if (gCoolantFtex == NULL) {
		fprintf(stderr, "could not convert gCoolantF to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gKmtex != NULL) {SDL_DestroyTexture (gKmtex);}
	gKmtex = SDL_CreateTextureFromSurface(renderer, gKm);
	if (gKmtex == NULL) {
		fprintf(stderr, "could not convert gKm to texture: %s\n", SDL_GetError());
		return 1;
	}


	if (gMilestex != NULL) {SDL_DestroyTexture (gMilestex);}
	gMilestex = SDL_CreateTextureFromSurface(renderer, gMiles);
	if (gMilestex == NULL) {
		fprintf(stderr, "could not convert gMiles to texture: %s\n", SDL_GetError());
		return 1;
	}


	if (gKphtex != NULL) {SDL_DestroyTexture (gKphtex);}
	gKphtex = SDL_CreateTextureFromSurface(renderer, gKph);
	if (gKphtex == NULL) {
		fprintf(stderr, "could not convert gKph to texture: %s\n", SDL_GetError());
		return 1;
	}


	if (gMphtex != NULL) {SDL_DestroyTexture (gMphtex);}
	gMphtex = SDL_CreateTextureFromSurface(renderer, gMph);
	if (gMphtex == NULL) {
		fprintf(stderr, "could not convert gMph to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gTyreicontex != NULL) {SDL_DestroyTexture (gTyreicontex);}
	gTyreicontex = SDL_CreateTextureFromSurface(renderer, gTyreicon);
	if (gTyreicontex == NULL) {
		fprintf(stderr, "could not convert gTyreicon to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gTyresignaltex != NULL) {SDL_DestroyTexture (gTyresignaltex);}
	gTyresignaltex = SDL_CreateTextureFromSurface(renderer, gTyresignal);
	if (gTyresignaltex == NULL) {
		fprintf(stderr, "could not convert gTyresignal to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gHighbeamlighttex != NULL) {SDL_DestroyTexture (gHighbeamlighttex);}
	gHighbeamlighttex = SDL_CreateTextureFromSurface(renderer, gHighbeamlight);
	if (gHighbeamlighttex == NULL) {
		fprintf(stderr, "could not convert gHighbeamlight to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gIndicaterighttex != NULL) {SDL_DestroyTexture (gIndicaterighttex);}
	gIndicaterighttex = SDL_CreateTextureFromSurface(renderer, gIndicateright);
	if (gIndicaterighttex == NULL) {
		fprintf(stderr, "could not convert gIndicateright to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gIndicatelefttex != NULL) {SDL_DestroyTexture (gIndicatelefttex);}
	gIndicatelefttex = SDL_CreateTextureFromSurface(renderer, gIndicateleft);
	if (gIndicatelefttex == NULL) {
		fprintf(stderr, "could not convert gIndicateleft to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gIndicatebothtex != NULL) {SDL_DestroyTexture (gIndicatebothtex);}
	gIndicatebothtex = SDL_CreateTextureFromSurface(renderer, gIndicateboth);
	if (gIndicateboth == NULL) {
		fprintf(stderr, "could not convert gIndicateboth to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gIndicaterightfartex != NULL) {SDL_DestroyTexture (gIndicaterightfartex);}
	gIndicaterightfartex = SDL_CreateTextureFromSurface(renderer, gIndicaterightfar);
	if (gIndicaterightfartex == NULL) {
		fprintf(stderr, "could not convert gIndicaterightfar to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gIndicateleftfartex != NULL) {SDL_DestroyTexture (gIndicateleftfartex);}
	gIndicateleftfartex = SDL_CreateTextureFromSurface(renderer, gIndicateleftfar);
	if (gIndicateleftfartex == NULL) {
		fprintf(stderr, "could not convert gIndicateleftfar to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gOillighttex != NULL) {SDL_DestroyTexture (gOillighttex);}
	gOillighttex = SDL_CreateTextureFromSurface(renderer, gOillight);
	if (gOillighttex == NULL) {
		fprintf(stderr, "could not convert gOillight to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gOillightoptex != NULL) {SDL_DestroyTexture (gOillightoptex);}
	gOillightoptex = SDL_CreateTextureFromSurface(renderer, gOillightOP);
	if (gOillightoptex == NULL) {
		fprintf(stderr, "could not convert gOillightOP to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gNeutrallighttex != NULL) {SDL_DestroyTexture (gNeutrallighttex);}
	gNeutrallighttex = SDL_CreateTextureFromSurface(renderer, gNeutrallight);
	if (gNeutrallighttex == NULL) {
		fprintf(stderr, "could not convert gNeutrallight to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gEngineoverheattex != NULL) {SDL_DestroyTexture (gEngineoverheattex);}
	gEngineoverheattex = SDL_CreateTextureFromSurface(renderer, gEngineoverheat);
	if (gEngineoverheattex == NULL) {
		fprintf(stderr, "could not convert gEngineoverheat to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gOverheatbadgetex != NULL) {SDL_DestroyTexture (gOverheatbadgetex);}
	gOverheatbadgetex = SDL_CreateTextureFromSurface(renderer, gOverheatbadge);
	if (gOverheatbadgetex == NULL) {
		fprintf(stderr, "could not convert gOverheatbadge to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gLowoiltex != NULL) {SDL_DestroyTexture (gLowoiltex);}
	gLowoiltex = SDL_CreateTextureFromSurface(renderer, gLowoil);
	if (gLowoiltex == NULL) {
		fprintf(stderr, "could not convert gLowoil to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gLowoilbadgetex != NULL) {SDL_DestroyTexture (gLowoilbadgetex);}
	gLowoilbadgetex = SDL_CreateTextureFromSurface(renderer, gLowoilbadge);
	if (gLowoilbadgetex == NULL) {
		fprintf(stderr, "could not convert gLowoilbadge to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gLowfueltex != NULL) {SDL_DestroyTexture (gLowfueltex);}
	gLowfueltex = SDL_CreateTextureFromSurface(renderer, gLowfuel);
	if (gLowfueltex == NULL) {
		fprintf(stderr, "could not convert gLowfuel to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gLowfuelbadgetex != NULL) {SDL_DestroyTexture (gLowfuelbadgetex);}
	gLowfuelbadgetex = SDL_CreateTextureFromSurface(renderer, gLowfuelbadge);
	if (gLowfuelbadgetex == NULL) {
		fprintf(stderr, "could not convert gLowfuelbadge to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gInfobottomdiagtex != NULL) {SDL_DestroyTexture (gInfobottomdiagtex);}
	gInfobottomdiagtex = SDL_CreateTextureFromSurface(renderer, gInfobottomdiag);
	if (gInfobottomdiagtex == NULL) {
		fprintf(stderr, "could not convert gInfobottomdiag to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gInfotopdiagtex != NULL) {SDL_DestroyTexture (gInfotopdiagtex);}
	gInfotopdiagtex = SDL_CreateTextureFromSurface(renderer, gInfotopdiag);
	if (gInfotopdiagtex == NULL) {
		fprintf(stderr, "could not convert gInfotopdiag to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gInfobottomtex != NULL) {SDL_DestroyTexture (gInfobottomtex);}
	gInfobottomtex = SDL_CreateTextureFromSurface(renderer, gInfobottom);
	if (gInfobottomtex == NULL) {
		fprintf(stderr, "could not convert gInfobottom to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gInfotoptex != NULL) {SDL_DestroyTexture (gInfotoptex);}
	gInfotoptex = SDL_CreateTextureFromSurface(renderer, gInfotop);
	if (gInfotoptex == NULL) {
		fprintf(stderr, "could not convert gInfotop to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gInfotopKMtex != NULL) {SDL_DestroyTexture (gInfotopKMtex);}
	gInfotopKMtex = SDL_CreateTextureFromSurface(renderer, gInfotopKM);
	if (gInfotopKMtex == NULL) {
		fprintf(stderr, "could not convert gInfotopKM to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gTyrebottomtex != NULL) {SDL_DestroyTexture (gTyrebottomtex);}
	gTyrebottomtex = SDL_CreateTextureFromSurface(renderer, gTyrebottom);
	if (gTyrebottomtex == NULL) {
		fprintf(stderr, "could not convert gTyrebottom to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gTyretoptex != NULL) {SDL_DestroyTexture (gTyretoptex);}
	gTyretoptex = SDL_CreateTextureFromSurface(renderer, gTyretop);
	if (gTyretoptex == NULL) {
		fprintf(stderr, "could not convert gTyretop to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gCoolanticontex != NULL) {SDL_DestroyTexture (gCoolanticontex);}
	gCoolanticontex = SDL_CreateTextureFromSurface(renderer, gCoolanticon);
	if (gCoolanticontex == NULL) {
		fprintf(stderr, "could not convert gCoolanticon to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gThemeoptionstex != NULL) {SDL_DestroyTexture (gThemeoptionstex);}
	gThemeoptionstex = SDL_CreateTextureFromSurface(renderer, gThemeoptions);
	if (gThemeoptionstex == NULL) {
		fprintf(stderr, "could not convert gThemeoptions to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gArrowrightthemetex != NULL) {SDL_DestroyTexture (gArrowrightthemetex);}
	gArrowrightthemetex = SDL_CreateTextureFromSurface(renderer, gArrowrighttheme);
	if (gArrowrightthemetex == NULL) {
		fprintf(stderr, "could not convert gArrowrighttheme to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gArrowleftthemetex != NULL) {SDL_DestroyTexture (gArrowleftthemetex);}
	gArrowleftthemetex = SDL_CreateTextureFromSurface(renderer, gArrowlefttheme);
	if (gArrowleftthemetex == NULL) {
		fprintf(stderr, "could not convert gArrowlefttheme to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gDownarrowtex != NULL) {SDL_DestroyTexture (gDownarrowtex);}
	gDownarrowtex = SDL_CreateTextureFromSurface(renderer, gDownarrow);
	if (gDownarrowtex == NULL) {
		fprintf(stderr, "could not convert gDownarrow to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gSetodometertex != NULL) {SDL_DestroyTexture (gSetodometertex);}
	gSetodometertex = SDL_CreateTextureFromSurface(renderer, gSetodometer);
	if (gSetodometertex == NULL) {
		fprintf(stderr, "could not convert gSetodometer to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gOdoerror1tex != NULL) {SDL_DestroyTexture (gOdoerror1tex);}
	gOdoerror1tex = SDL_CreateTextureFromSurface(renderer, gOdoerror1);
	if (gOdoerror1tex == NULL) {
		fprintf(stderr, "could not convert gOdoerror1 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gOdoerror2tex != NULL) {SDL_DestroyTexture (gOdoerror2tex);}
	gOdoerror2tex = SDL_CreateTextureFromSurface(renderer, gOdoerror2);
	if (gOdoerror2tex == NULL) {
		fprintf(stderr, "could not convert gOdoerror2 to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gSprocketsetuptex != NULL) {SDL_DestroyTexture (gSprocketsetuptex);}
	gSprocketsetuptex = SDL_CreateTextureFromSurface(renderer, gSprocketsetup);
	if (gSprocketsetuptex == NULL) {
		fprintf(stderr, "could not convert gSprocketsetup to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gCoolantfantemptex != NULL) {SDL_DestroyTexture (gCoolantfantemptex);}
	gCoolantfantemptex = SDL_CreateTextureFromSurface(renderer, gCoolantfantemp);
	if (gCoolantfantemptex == NULL) {
		fprintf(stderr, "could not convert gCoolantfantemp to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gLowtyrebadgetex != NULL) {SDL_DestroyTexture (gLowtyrebadgetex);}
	gLowtyrebadgetex = SDL_CreateTextureFromSurface(renderer, gLowtyrebadge);
	if (gLowtyrebadgetex == NULL) {
		fprintf(stderr, "could not convert gLowtyrebadge to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gReartyrelowtex != NULL) {SDL_DestroyTexture (gReartyrelowtex);}
	gReartyrelowtex = SDL_CreateTextureFromSurface(renderer, gReartyrelow);
	if (gReartyrelowtex == NULL) {
		fprintf(stderr, "could not convert gReartyrelow to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gFronttyrelowtex != NULL) {SDL_DestroyTexture (gFronttyrelowtex);}
	gFronttyrelowtex = SDL_CreateTextureFromSurface(renderer, gFronttyrelow);
	if (gFronttyrelowtex == NULL) {
		fprintf(stderr, "could not convert gFronttyrelow to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gBothtyrelowtex != NULL) {SDL_DestroyTexture (gBothtyrelowtex);}
	gBothtyrelowtex = SDL_CreateTextureFromSurface(renderer, gBothtyrelow);
	if (gBothtyrelowtex == NULL) {
		fprintf(stderr, "could not convert gBothtyrelow to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gGeartex != NULL) {SDL_DestroyTexture (gGeartex);}
	gGeartex = SDL_CreateTextureFromSurface(renderer, gGear);
	if (gGeartex == NULL) {
		fprintf(stderr, "could not convert gGear to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gNavbgtex != NULL) {SDL_DestroyTexture (gNavbgtex);}
	gNavbgtex = SDL_CreateTextureFromSurface(renderer, gNavbg);
	if (gNavbgtex == NULL) {
		fprintf(stderr, "could not convert gNavbg to texture: %s\n", SDL_GetError());
		return 1;
	}

	if (gNaviconstex != NULL) {SDL_DestroyTexture (gNaviconstex);}
	gNaviconstex = SDL_CreateTextureFromSurface(renderer, gNavicons);
	if (gNavicons == NULL) {
		fprintf(stderr, "could not convert gNavicons to texture: %s\n", SDL_GetError());
		return 1;
	}

	//gNaviconstex

	return 0;
}

bool file_exists (const char* name) {

	std::ifstream ifile (name);
	return (bool)ifile;

}

int GetLitresRemaining (int fuelintfloat) {

  // Depending on where the fuel float is it will have a different rate of change 
  // as the fuel decreases. These values are based on observations I made
  // Each 'Unit' per litre is how many units make up one litre for the current float reading

  if (fuelintfloat >= 0 && fuelintfloat <= 93) {
    litresremaining = (21000 - (((double)(1000 / 93)) * ((double)fuelintfloat - 0))) / 1000;
    return 93;
  }

  if (fuelintfloat > 93 && fuelintfloat <= 121) {
    litresremaining = (20000 - (((double)(1000 / 28)) * ((double)fuelintfloat - 93))) / 1000; // 8.47
    return 28;
  }
  
  if (fuelintfloat > 121 && fuelintfloat <= 148) {
    litresremaining = (18000 - (((double)(1000 / 27)) * ((double)fuelintfloat - 121))) / 1000; // 30.30
    return 27;
  }

  if (fuelintfloat > 148 && fuelintfloat <= 180) {
    litresremaining = (17000 - (((double)(1000 / 32)) * ((double)fuelintfloat - 148))) / 1000;
    return 32;
  }

  if (fuelintfloat > 180 && fuelintfloat <= 211) {
    litresremaining = (15000 - (((double)(1000 / 31)) * ((double)fuelintfloat - 180))) / 1000;
    return 31;
  }

  if (fuelintfloat > 211 && fuelintfloat <= 241) {
    litresremaining = (14000 - (((double)(1000 / 30)) * ((double)fuelintfloat - 211))) / 1000;
    return 30;
  }

  if (fuelintfloat > 241 && fuelintfloat <= 281) {
    litresremaining = (12000 - (((double)(1000 / 40)) * ((double)fuelintfloat - 241))) / 1000;
    return 40;
  }

  if (fuelintfloat > 281 && fuelintfloat <= 320) {
    litresremaining = (10000 - (((double)(1000 / 39)) * ((double)fuelintfloat - 281))) / 1000;
    return 39;
  }

  if (fuelintfloat > 320 && fuelintfloat <= 367) {
    litresremaining = (9000 - (((double)(1000 / 47)) * ((double)fuelintfloat - 320))) / 1000;
    return 47;
  }

  if (fuelintfloat > 367 && fuelintfloat <= 426) {
    litresremaining = (7000 - (((double)(1000 / 59)) * ((double)fuelintfloat - 367))) / 1000;
    return 59;
  }

  if (fuelintfloat > 426 && fuelintfloat <= 481) {
    litresremaining = (6000 - (((double)(1000 / 55)) * ((double)fuelintfloat - 426))) / 1000;
    return 55;
  }

  if (fuelintfloat > 481 && fuelintfloat <= 506) {
    litresremaining = (5000 - (((double)(1000 / 25)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  if (fuelintfloat > 506) {
    litresremaining = (5000 - (((double)(1000 / 25)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  return 0;
}

int GetFuelRevealFromBars(int bars)
{
    return 17 * bars;
}

int GetNumFuelBars(int value)
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

int GetFuelreveal(int percent) {
	if (percent >= 100) {
		return 273;
	}

	if (percent <= 0) {
		return 0;
	}
	
	return (273 * percent) / 100;
}

int	GetRPMrotation(int rpm) {
	// Function to get the most appropriate rotation angle based on supplied
	// rpm. This will return the closest rotation value that matches the rpm in the lookup table

	if (rpm >= 13000) {
		return gRPMlookup[52][1];
	}

	if (rpm <= 0) {
		return gRPMlookup[0][1];
	}

	int lowestdiff = 30000;
	int closest = 0;

	for (int r = 0; r < 52; r ++ ) {		
		int lrpm = gRPMlookup[r][0];

		if (rpm >= lrpm) {
			if ((rpm - lrpm) < lowestdiff) {
				lowestdiff = (rpm - lrpm);
				closest = r;
			}
		}

		if (rpm < lrpm) {
			if ((lrpm - rpm) < lowestdiff) {
				lowestdiff = (lrpm - rpm);
				closest = r;
			}
		}
	}

	// by the time we get here we should have a lookup value is closest to what we want
	return gRPMlookup[closest][1];
}



void drawSmallgreystring (char* digits, int xpos, int ypos)
{
	int xoffset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d <= 10; d++) {
			if (digit == gSmallNumref[d]) {
				SDL_Rect gSrcrect;
				gSrcrect.x = gSmallgreysrctexloc[0][d];
				gSrcrect.y = gSmallgreysrctexloc[1][d];
				gSrcrect.w = gSmallgreysrctexloc[2][d];
				gSrcrect.h = gSmallgreysrctexloc[3][d];

				SDL_Rect gDstrect;
				gDstrect.x = xpos + xoffset;
				gDstrect.y = ypos;

				if (d == 10) {
					gDstrect.y = ypos + 18;
				}

				gDstrect.w = gSrcrect.w;
				gDstrect.h = gSrcrect.h;

				SDL_RenderCopy(renderer, gsmallnumberstex, &gSrcrect, &gDstrect);

				xoffset+=gSrcrect.w;
			}
		}

	}		
}



void drawSmallbluestring (char* digits, int xpos, int ypos)
{
	int xoffset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d <= 10; d++) {
			if (digit == gSmallNumref[d]) {
				SDL_Rect gSrcrect;
				gSrcrect.x = gSmallbluesrctexloc[0][d];
				gSrcrect.y = gSmallbluesrctexloc[1][d];
				gSrcrect.w = gSmallbluesrctexloc[2][d];
				gSrcrect.h = gSmallbluesrctexloc[3][d];

				SDL_Rect gDstrect;
				gDstrect.x = xpos + xoffset;
				gDstrect.y = ypos;

				if (d == 10) {
					gDstrect.y = ypos + 18;
				}

				gDstrect.w = gSrcrect.w;
				gDstrect.h = gSrcrect.h;

				SDL_RenderCopy(renderer, gsmallnumberstex, &gSrcrect, &gDstrect);

				xoffset+=gSrcrect.w;
			}
		}

	}		
}

void drawMediumchar (char digit, int xpos, int ypos)
{
	int xoffset = 0;
	
	for (int d = 0; d <= 15; d++) {
		if (digit == gNumref[d]) {
			SDL_Rect gSrcrect;
			gSrcrect.x = gMednumberssrctexloc[0][d];
			gSrcrect.y = gMednumberssrctexloc[1][d];
			gSrcrect.w = gMednumberssrctexloc[2][d];
			gSrcrect.h = gMednumberssrctexloc[3][d];

			SDL_Rect gDstrect;
			gDstrect.x = xpos + xoffset;
			gDstrect.y = ypos;
			gDstrect.w = gSrcrect.w;
			gDstrect.h = gSrcrect.h;

			SDL_RenderCopy(renderer, gsmallnumberstex, &gSrcrect, &gDstrect);

			xoffset+=gSrcrect.w;
		}
	}

}

void drawLargestring (char *digits, int xpos, int ypos) 
{
	//gLargenumberssrctexloc
	int xoffset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d <= 10; d++) {
			if (digit == gLargeNumref[d]) {
				SDL_Rect gSrcrect;
				gSrcrect.x = gLargenumberssrctexloc[0][d];
				gSrcrect.y = gLargenumberssrctexloc[1][d];
				gSrcrect.w = gLargenumberssrctexloc[2][d];
				gSrcrect.h = gLargenumberssrctexloc[3][d];

				SDL_Rect gDstrect;
				gDstrect.x = xpos + xoffset;
				gDstrect.y = ypos;
				gDstrect.w = gSrcrect.w;
				gDstrect.h = gSrcrect.h;

				SDL_RenderCopy(renderer, gsmallnumberstex, &gSrcrect, &gDstrect);

				xoffset+=gSrcrect.w;
			}
		}
	}	
}

void drawMediumstring (char* digits, int xpos, int ypos)
{
	int xoffset = 0;
	for (int s=0;s<strlen(digits);s++) {

		char digit = digits[s];

		for (int d = 0; d <= 15; d++) {
			if (digit == gNumref[d]) {
				SDL_Rect gSrcrect;
				gSrcrect.x = gMednumberssrctexloc[0][d];
				gSrcrect.y = gMednumberssrctexloc[1][d];
				gSrcrect.w = gMednumberssrctexloc[2][d];
				gSrcrect.h = gMednumberssrctexloc[3][d];

				SDL_Rect gDstrect;
				gDstrect.x = xpos + xoffset;
				gDstrect.y = ypos;
				gDstrect.w = gSrcrect.w;
				gDstrect.h = gSrcrect.h;

				SDL_RenderCopy(renderer, gsmallnumberstex, &gSrcrect, &gDstrect);

				xoffset+=gSrcrect.w;
			}
		}

	}		
}

void drawNavLargeString (char *digits, int xpos, int ypos) {
	int xoffset = 0;

	for (int s=0;s<strlen (digits);s++) {

		char digit = digits[s];
		
		if (digit == 32 || digit == '-') {
			xoffset+=7;
		}

		for (int d=0;d<40;d++) {

			if (xpos + xoffset < 412) {
				if (digit == gNavLargeLowerLetterref[d] || digit == gNavLargeUpperLetterref[d]) {

					SDL_Rect gSrcrect;
					gSrcrect.x = gNavletterslargesrctexloc[0][d];
					gSrcrect.y = gNavletterslargesrctexloc[1][d];
					gSrcrect.w = gNavletterslargesrctexloc[2][d];
					gSrcrect.h = gNavletterslargesrctexloc[3][d];

					SDL_Rect gDstrect;
					gDstrect.x = xpos + xoffset;
					gDstrect.y = ypos;
					gDstrect.w = gSrcrect.w;
					gDstrect.h = gSrcrect.h;

					SDL_RenderCopy(renderer, gNaviconstex, &gSrcrect, &gDstrect);

					xoffset+=gSrcrect.w + 2;				
				}
			}
		}		
	}
}

void drawNavSmallString (char *digits, int xpos, int ypos) {
	int xoffset = 0;

	for (int s=0;s<strlen (digits);s++) {

		char digit = digits[s];
		
		if (digit == 32 || digit == '-') {
			xoffset+=7;
		}

		for (int d=0;d<40;d++) {

			if (xpos + xoffset < 412) {
				if (digit == gNavLargeLowerLetterref[d] || digit == gNavLargeUpperLetterref[d]) {

					SDL_Rect gSrcrect;
					gSrcrect.x = gNavletterssmallsrctexloc[0][d];
					gSrcrect.y = gNavletterssmallsrctexloc[1][d];
					gSrcrect.w = gNavletterssmallsrctexloc[2][d];
					gSrcrect.h = gNavletterssmallsrctexloc[3][d];

					SDL_Rect gDstrect;
					gDstrect.x = xpos + xoffset;
					gDstrect.y = ypos;
					gDstrect.w = gSrcrect.w;
					gDstrect.h = gSrcrect.h;

					SDL_RenderCopy(renderer, gNaviconstex, &gSrcrect, &gDstrect);

					xoffset+=gSrcrect.w + 2;				
				}
			}
		}		
	}
}

void drawNavDigits (char *digits, int xpos, int ypos) {
	int xoffset = 0;

	for (int s=0;s<strlen (digits);s++) {

		char digit = digits[s];
		
		if (digit == 32) {
			xoffset+=7;
		}

		for (int d=0;d<11;d++) {

			if (digit == gNavnumbersref[d]) {

				SDL_Rect gSrcrect;
				gSrcrect.x = gNavnumberssrctexloc[0][d];
				gSrcrect.y = gNavnumberssrctexloc[1][d];
				gSrcrect.w = gNavnumberssrctexloc[2][d];
				gSrcrect.h = gNavnumberssrctexloc[3][d];

				SDL_Rect gDstrect;
				gDstrect.x = xpos + xoffset;
				gDstrect.y = ypos;
				gDstrect.w = gSrcrect.w;
				gDstrect.h = gSrcrect.h;

				if (digit == '.') {
					gDstrect.y = ypos + 71;
				}

				SDL_RenderCopy(renderer, gNaviconstex, &gSrcrect, &gDstrect);

				xoffset+=gSrcrect.w + 2;				
			}

		}		
	}
}

void drawNavNumbers (int num, int xpos, int ypos) {
	char szDigit[5];
	memset (szDigit, 0, 5);
	sprintf (szDigit, "%d", num);

	drawNavDigits (szDigit, xpos, ypos);
}

void drawNavSymbol (int sym, int xpos, int ypos)
{
	SDL_Rect gSrcrect;
	gSrcrect.x = gNaviconsrctexloc[0][sym];
	gSrcrect.y = gNaviconsrctexloc[1][sym];
	gSrcrect.w = gNaviconsrctexloc[2][sym];
	gSrcrect.h = gNaviconsrctexloc[3][sym];

	SDL_Rect gDstrect;
	gDstrect.x = xpos;
	gDstrect.y = ypos;
	gDstrect.w = gSrcrect.w;
	gDstrect.h = gSrcrect.h;

	SDL_RenderCopy(renderer, gNaviconstex, &gSrcrect, &gDstrect);
}

void drawMediumnum (int singledigit, int xpos, int ypos) {
	char szDigit[4];
	memset (szDigit, 0, 4);
	sprintf (szDigit, "%d", singledigit);

	drawMediumstring (szDigit, xpos, ypos);
}

void drawLargenum (int singledigit, int xpos, int ypos) {
	char szDigit[4];
	memset (szDigit, 0, 4);
	sprintf (szDigit, "%d", singledigit);

	drawLargestring (szDigit, xpos, ypos);
}

void drawSpeeddigit (char digit, int position)
{

	for (int d = 0; d <= 9; d++) {
		if (digit == gNumref[d]) {
			SDL_Rect gSrcrect;
			gSrcrect.x = gSpeedsrctexloc[0][d];
			gSrcrect.y = gSpeedsrctexloc[1][d];
			gSrcrect.w = gSpeedsrctexloc[2][d];
			gSrcrect.h = gSpeedsrctexloc[3][d];

			if (position == 3) {
				spdDigitthree.w = gSpeedsrctexloc[2][d];
				spdDigitthree.h = gSpeedsrctexloc[3][d];
				SDL_RenderCopy(renderer, gspeednumberstex, &gSrcrect, &spdDigitthree);
			}

			if (position == 2) {
				spdDigittwo.w = gSpeedsrctexloc[2][d];
				spdDigittwo.h = gSpeedsrctexloc[3][d];
				SDL_RenderCopy(renderer, gspeednumberstex, &gSrcrect, &spdDigittwo);
			}

			if (position == 1) {
				spdDigitone.w = gSpeedsrctexloc[2][d];
				spdDigitone.h = gSpeedsrctexloc[3][d];
				SDL_RenderCopy(renderer, gspeednumberstex, &gSrcrect, &spdDigitone);
			}
		}
	}	
}

void drawSpeed(char *speed) {
	if (strlen(speed) == 1) {
		drawSpeeddigit(speed[0], 3);
	}

	if (strlen(speed) == 2) {
		drawSpeeddigit(speed[0], 2);
		drawSpeeddigit(speed[1], 3);
	}

	if (strlen(speed) == 3) {
		drawSpeeddigit(speed[0], 1);
		drawSpeeddigit(speed[1], 2);
		drawSpeeddigit(speed[2], 3);
	}
}

/*
void threadworker(char* msg) {
	
	bool x = true;
	int i = 0;
	while (x == true) {
		SDL_Delay(1000);
		i++;
		fprintf(stderr, "Thread is running"); 
		sprintf(gszCommsmsg, "Thread count is: %d", i);
	}
}*/

void UnpackMenuMessage() {

	//S,300,0,0,0,0,0,0,0,0,0,0,0,0,E
	//M,006,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,44,N

	char szCurrent[16];
	int messagenumber = 0;
	int currentpointer = 0;

	memset(szCurrent, 0, 16);

	for (int c=0;c<strlen(gszCommsmsg);c++) {



		if (gszCommsmsg[c] == ',') {

			if (messagenumber == 1) { // Menu choice state
				choicestate = atoi (szCurrent);
			}

			if (messagenumber == 2) { // Odo digit 1
				ododigit1 = atoi (szCurrent);
			}
			if (messagenumber == 3) { // Odo digit 2
				ododigit2 = atoi (szCurrent);
			}
			if (messagenumber == 4) { // Odo digit 3
				ododigit3 = atoi (szCurrent);
			}
			if (messagenumber == 5) { // Odo digit 4
				ododigit4 = atoi (szCurrent);
			}
			if (messagenumber == 6) { // Odo digit 5
				ododigit5 = atoi (szCurrent);
			}
			if (messagenumber == 7) { // Odo digit 6
				ododigit6 = atoi (szCurrent);
			}

			if (messagenumber == 8) { // Odo2 digit 1
				odo2digit1 = atoi (szCurrent);
			}
			if (messagenumber == 9) { // Odo2 digit 2
				odo2digit2 = atoi (szCurrent);
			}
			if (messagenumber == 10) { // Odo2 digit 3
				odo2digit3 = atoi (szCurrent);
			}
			if (messagenumber == 11) { // Odo2 digit 4
				odo2digit4 = atoi (szCurrent);
			}
			if (messagenumber == 12) { // Odo2 digit 5
				odo2digit5 = atoi (szCurrent);
			}
			if (messagenumber == 13) { // Odo2 digit 6
				odo2digit6 = atoi (szCurrent);
			}
			if (messagenumber == 14) { // Odo error
				odoerror = atoi (szCurrent);
			}
			if (messagenumber == 15) { // Set time digit 0
				settimedigit0 = atoi (szCurrent);
			}
			if (messagenumber == 16) { // Set time digit 1
				settimedigit1 = atoi (szCurrent);
			}
			if (messagenumber == 17) { // Set time digit 2
				settimedigit2 = atoi (szCurrent);
			}
			if (messagenumber == 18) { // Set time digit 3
				settimedigit3 = atoi (szCurrent);
			}
			if (messagenumber == 19) { // Speed correction digit 0 (+ or -)
				spcdigit0 = atoi (szCurrent);
			}
			if (messagenumber == 20) { // Speed correction digit 1 
				spcdigit1 = atoi (szCurrent);
			}
			if (messagenumber == 21) { // Speed correction digit 2 
				spcdigit2 = atoi (szCurrent);
			}
			if (messagenumber == 22) { // Speed correction digit 3 
				spcdigit3 = atoi (szCurrent);
			}
			if (messagenumber == 23) { // Front sprocket
				frontsprocket = atoi (szCurrent);
			}
			if (messagenumber == 24) { // Rear sprocket
				rearsprocket = atoi (szCurrent);
			}
			if (messagenumber == 25) { // Coolant fan temp
				coolantfantemp = atoi (szCurrent);
			}
			if (messagenumber == 26) { // Usingkm
				usingkm = atoi (szCurrent);
			}

			if (messagenumber == 27) { // Usingfh
				usingfh = atoi (szCurrent);
			}

			if (messagenumber == 28) { // Usingbar
				usingbar = atoi (szCurrent);
			}

			if (messagenumber == 29) { // Front sensor ID
				frontsensorid = atoi (szCurrent);
			}

			if (messagenumber == 30) { // Rear sensor ID
				rearsensorid = atoi (szCurrent);
			}

			if (messagenumber == 31) { // Front pressure low setting
				frontpressurelow = atoi (szCurrent);
			}

			if (messagenumber == 32) { // Rear pressure low setting
				rearpressurelow = atoi (szCurrent);
			}

			if (messagenumber == 33) { // Control layout
				controllayout = atoi (szCurrent);
			}

			if (messagenumber == 34) { // Day Theme
				daytheme = atoi (szCurrent);
			}

			if (messagenumber == 35) { // Night Theme
				nighttheme = atoi (szCurrent);
			}

			if (messagenumber == 36) { // Current light level
				currentlightlevel = atoi (szCurrent);
			}

			if (messagenumber == 37) { // Light switch value
				lightswitchvalue = atoi (szCurrent);
			}

			if (messagenumber == 38) { // Fan Neutral optin in coolant fan setup menu
				fanneutraloption = atoi (szCurrent);
			}

			if (messagenumber == 39) { // Gear Ratio Interval setting
				gearratiointerval = atoi (szCurrent);
			}
			
			//fprintf(stderr, "%s\n", szCurrent);
			memset(szCurrent, 0, 16);
			currentpointer = 0;
			messagenumber++;

		} else {
			if (currentpointer < 16) {
				szCurrent[currentpointer] = gszCommsmsg[c];
				currentpointer++;
			}
		}

	}

}

void UnpackNavMessage () {

	// Just a routine to unpack the delimited Nav message handed to us via BLE from a Phone
	char szCurrent[255];
	int messagenumber = 0;
	int currentpointer = 0;

	memset (szCurrent, 0, 255);

	for (int c=0;c<strlen (strNav);c++) {

		if (strNav[c] == '%') {

			if (messagenumber == 1) { // Nav Symbol
				//fprintf(stderr, "Nav MSG 1: %s\n", szCurrent);
				memset (strNavSymbol, 0, 16);
				strcpy (strNavSymbol, szCurrent);
			}

			if (messagenumber == 2) { // Onto Roadname
				//fprintf(stderr, "Nav MSG 2: %s\n", szCurrent);
				memset (strNavRoad, 0, 255);
				strcpy (strNavRoad, szCurrent);
			}

			if (messagenumber == 3) { // Towards Placename
				//fprintf(stderr, "Nav MSG 3: %s\n", szCurrent);
				memset (strNavTowards, 0, 255);
				strcpy (strNavTowards, szCurrent);
			}

			if (messagenumber == 4) { // Motorway Exit Number
				//fprintf(stderr, "Nav MSG 4: %s\n", szCurrent);
				memset (strNavExit, 0, 16);
				strcpy (strNavExit, szCurrent);
			}

			if (messagenumber == 5) { // Yards Away
				//fprintf(stderr, "Nav MSG 5: %s\n", szCurrent);
				memset (strNavYards, 0, 16);
				strcpy (strNavYards, szCurrent);
				navYards = atoi (strNavYards);

				memset (strNavMetres, 0, 16);
				double navmeters = (double)navYards / 1.094;
				sprintf( strNavMetres, "%d", (int)navmeters);				
			}			

			if (messagenumber == 6) { // Miles Away 
				//fprintf(stderr, "Nav MSG 6: %s\n", szCurrent);
				memset (strNavMiles, 0, 16);
				strcpy (strNavMiles, szCurrent);
				navMiles = atof (strNavMiles);

				memset (strNavKm, 0, 16);
				double navkm = navMiles * 1.609;
				sprintf( strNavKm, "%.1f", navkm);
			}

			if (messagenumber == 7) { // Driving on the left
				if (atoi (szCurrent) == 1) {
					drivingleft = 1;
				} else {
					drivingleft = 0;
				}
			}

			if (messagenumber == 8) { // Distance to Destination
				memset (strNavDestMiles, 0, 16);
				strcpy (strNavDestMiles, szCurrent);
				navDestdistance = atof (strNavDestMiles);

				memset (strNavDestKm, 0, 16);
				double navdestkm = navDestdistance * 1.609;
				sprintf( strNavDestKm, "%.1f", navdestkm);
			}

			memset(szCurrent, 0, 255);
			currentpointer = 0;
			messagenumber++;

		} else {
			if (currentpointer < 255) {
				szCurrent[currentpointer] = strNav[c];
				currentpointer++;
			}
		}

	}

}

void UnpackMessage () {

	//char gszCommsmsg[255];
	//gszCommsmsg[255];
	//strcpy (gszCommsmsg, "S,078,2244,3456,23456,56,10,100,40,0,5000,12.5,9.2,47,21,0,0,0,0,0,0,0,200,0,1,4,2,0,9,9,2");

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
	if (oilwarning) {
	if (indicateright) {
	if (indicateleft) {
	if (highbeam)	
*/


	char szCurrent[255];
	int messagenumber = 0;
	int currentpointer = 0;

	memset(szCurrent, 0, 255);

	for (int c=0;c<strlen(gszCommsmsg);c++) {



		if (gszCommsmsg[c] == ',') {

			if (messagenumber == 1) { // Speed
				currentSpeed = atoi (szCurrent);
			}

			if (messagenumber == 2) { // Rpm
				rpm = atoi (szCurrent);
			}

			if (messagenumber == 3) { // Coolant temp
				coolanttemp = atoi (szCurrent);
			}

			if (messagenumber == 4) { // Battery voltage
				batt = atof (szCurrent);
			}

			if (messagenumber == 5) { // Current time - real time clock
				memset (strTime, 0, 16);
				strcpy (strTime, szCurrent);
			}

			if (messagenumber == 6) { // Fuel value - float level
				fuelfloat = atoi (szCurrent);
			}

			if (messagenumber == 7) { // Neutral Light
				if (atoi (szCurrent) == 1) {
					neutral = true;
				} else {
					neutral = false;
				}
			}

			if (messagenumber == 8) { // Oil Light
				if (atoi (szCurrent) == 1) {
					oilwarning = true;
				} else {
					oilwarning = false;
				}
			}

			if (messagenumber == 9) { // highbeam Light
				if (atoi (szCurrent) == 1) {
					highbeam = true;
				} else {
					highbeam = false;
				}
			}

			if (messagenumber == 10) { // indicateleft Light
				if (atoi (szCurrent) == 1) {
					indicateleft = true;
				} else {
					indicateleft = false;
				}
			}

			if (messagenumber == 11) { // indicateright Light
				if (atoi (szCurrent) == 1) {
					indicateright = true;
				} else {
					indicateright = false;
				}
			}
		
			if (messagenumber == 12) { // Choice state - menu options
				choicestate = atoi (szCurrent);
			}

			if (messagenumber == 13) { // Info Mode - menu options
				infomode = atoi (szCurrent);
			}

			if (messagenumber == 14) { // Trip 1
				trip1 = atof (szCurrent);
			}

			if (messagenumber == 15) { // Trip 2
				trip2 = atof (szCurrent);
			}

			if (messagenumber == 16) { // Odo
				odo = atof (szCurrent);
			}

			if (messagenumber == 17) { // KM / Miles
				usingkm = atoi (szCurrent);
			}

			if (messagenumber == 18) { // Speed correction value
				spdcorrect = atof (szCurrent);
			}

			if (messagenumber == 19) { // Speed correction value
				theme = atoi (szCurrent);
			}

			if (messagenumber == 20) { // Ambient temp
				ambientTemp = atoi (szCurrent);
			}

			if (messagenumber == 21) { // Current gear (gear indicator)
				currentgear = atoi (szCurrent);
			}

			if (messagenumber == 22) { // MPG
				mpg = atoi (szCurrent);
			}

			if (messagenumber == 23) { // Range
				range = atoi (szCurrent);
			}

			if (messagenumber == 24) { // MaxSpeed
				maxspeed = atoi (szCurrent);
			}

			if (messagenumber == 25) { // Trip time hour
				triptimehour = atoi (szCurrent);
			}

			if (messagenumber == 26) { // Trip time min
				triptimemin = atoi (szCurrent);
			}

			if (messagenumber == 27) { // Oil pressure available (1 or 0)
				if (atoi (szCurrent) == 1) {
					oilpressureavailable = true;
				} else {
					oilpressureavailable = false;
				}
			}

			if (messagenumber == 28) { // Oil pressure in ohms
				oilpressureohms = atoi (szCurrent);
			}

			if (messagenumber == 29) { // Oil temp in ohms
				oiltempohms = atoi (szCurrent);
			}

			memset (strTriptime, 0, 16);
			if (triptimemin < 10) {
				sprintf (strTriptime, "%d:0%d", triptimehour, triptimemin);			
			} else {
				sprintf (strTriptime, "%d:%d", triptimehour, triptimemin);			
			}
			

			if (messagenumber == 30) { // Fahrenheit or Celcius
				usingfh = atoi (szCurrent);
			}

			if (messagenumber == 31) { // Bar or PSI
				usingbar = atoi (szCurrent);
			}

			if (messagenumber == 32) { // Front sensor ID
				frontsensorid = atoi (szCurrent);
			}

			if (messagenumber == 33) { // Rear sensor ID
				rearsensorid = atoi (szCurrent);
			}

			if (messagenumber == 34) { // Front pressure low setting
				frontpressurelow = atoi (szCurrent);
			}

			if (messagenumber == 35) { // Rear pressure low setting
				rearpressurelow = atoi (szCurrent);				
			}

			if (messagenumber == 36) { // Nav Message from Phone
				memset (strNav, 0, 255);
				strcpy (strNav, szCurrent);

				if (strlen (strNav) > 0) {
					//fprintf(stderr, "%s\n", strNav);
					UnpackNavMessage ();
					navActive = true;
				}

				
				//fprintf(stderr, "%i\n", strlen (strNav));
				//printf(szCurrent);
				//printf (strlen (strNav));
				//rearpressurelow = atoi (szCurrent);

				//fprintf(stderr, szCurrent); 
			}

			//fprintf(stderr, "%s\n", szCurrent);
			memset(szCurrent, 0, 255);
			currentpointer = 0;


			messagenumber++;



		} else {
			if (currentpointer < 255) {
				szCurrent[currentpointer] = gszCommsmsg[c];
				currentpointer++;
			}
		}

	}

}


int connectInterface ()
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
	//UnpackMessage();
/*
	bool x = true;
	int i = 0;
	while (x == true) {
		
		i++;
		//fprintf(stderr, "Thread is running"); 
		sprintf(gszCommsmsg, "Thread count is: %d", i);
	}
	return 0;
	*/

	char appendbuf [1024];
	
	char read_buf [1024];
	bool quit = false;
	int appendpointer = 0;

	connectInterface();

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
		    connectInterface();
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
			memset (gszCommsmsg, 0, 1024);
			strcpy (gszCommsmsg, appendbuf);

			//fprintf(stderr, "Data: %s\n", gszCommsmsg);

			UnpackMessage();
		} else {
			//if (appendbuf[0] == 'M' && strlen(appendbuf) == 59 && appendbuf[58] == 'N') {
			if (appendbuf[0] == '[' && strlen(appendbuf) == 95 && appendbuf[94] == ']') {

				memset (gszCommsmsg, 0, 1024);
				strcpy (gszCommsmsg, appendbuf);
				UnpackMenuMessage();
			}
			//fprintf(stderr, "NO Messages received! Buffer length: %i",(int)strlen(appendbuf)); 
		}

		//usleep (1000);	
	}
	

	close(serial_port);

	return 0;
}

int connectTPMSInterface ()
{
// Open the serial port. Change device path as needed (currently set to an standard FTDI USB-UART cable type device)

	if (file_exists ("/dev/cu.usbserial-D3077502")) {
		tpms_serial_port = open("/dev/cu.usbserial-D3077502", O_RDWR); // Nano board connected!
		//cu.usbserial-D3077502
		fprintf(stderr, "TPMS interface connected!"); 
		tpmsconnected= true;
	} else {
		if (file_exists ("/dev/ttyUSB0")) {		
			fprintf(stderr, "TPMS interface connected!"); 
			tpms_serial_port = open("/dev/ttyUSB0", O_RDWR);
			tpmsconnected= true;
		} else {

			if (file_exists ("/dev/ttyUSB1")) {
				fprintf(stderr, "TPMS interface connected!"); 
				tpms_serial_port = open("/dev/ttyUSB1", O_RDWR);
				tpmsconnected= true;
			} else {

				if (file_exists ("/dev/cu.usbserial-210")) {
					fprintf(stderr, "Attempt 210 connect....");
					tpms_serial_port = open("/dev/cu.usbserial-210", O_RDWR);
					//tty.usbserial-1420
					fprintf(stderr, "TPMS interface connected!"); 
					tpmsconnected= true;
				} else {
					fprintf(stderr, "TPMS interface NOT connected!"); 
					tpmsconnected= false;
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
	if (tpmsmodel == STANDARDTPMS) {
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
	}

	if (tpmsmodel == EBAYTPMS) {
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

	connectTPMSInterface();

	while (quit == false) {		
		memset(&read_buf, '\0', sizeof(read_buf));
		int num_bytes = read(tpms_serial_port, &read_buf, sizeof(read_buf));

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		if (num_bytes < 0) {
		    //printf("Error reading: %s\n", strerror(errno));
		    close (tpms_serial_port);
		    connectTPMSInterface();
		} else {
			//fprintf(stderr, "Num bytes: %i\n", num_bytes); 
		}


		for (int r=0;r<num_bytes;r++) {
			
			if (read_buf[r] == 85) {
				if (r < (num_bytes-1)) {
					if (read_buf[r+1] == -86) {
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



		if (appendbuf[0] == 85 && appendpointer > 5 && appendbuf[1] == -86) {
			

			int sensornum = -1;
			//int sensornum = appendbuf[3];
			// Sensor Number. 0 = Left Front, 1 = Right Front, 16 = Back Left, 17 = Back Right

			if (appendbuf[3] == 0) {sensornum = 1;}
			if (appendbuf[3] == 1) {sensornum = 2;}
			if (appendbuf[3] == 16) {sensornum = 3;}
			if (appendbuf[3] == 17) {sensornum = 4;}			

			int tpbyte1 = appendbuf[4];			
			int tempbyte = appendbuf[5];
			int statebyte = appendbuf[6];
			int sensorstate = 0;

			double psi = (double) (((tpbyte1 & 255) * 3.44));
			double bar = psi / 14.5;

			//double d = (double) (frame[4] & 255); tiresState.AirPressure = (int) (d * 3.44d);

			//double psi = bar * 14.5;
			
			int celcius = (tempbyte & 255) - 50;
			sensorstate = NORMAL;

			if (sensornum == 1) {
				gSensor1bar = bar;
				gSensor1psi = psi;
				gSensor1temp = celcius;
				gSensor1state = sensorstate;
			}

			if (sensornum == frontsensorid) {
				gSensor2bar = bar;
				gSensor2psi = psi;
				gSensor2temp = celcius;
				gSensor2state = sensorstate;
				tpmssignal = true;
				tpmssignalcount = 0;
				frontsensorread = true;
			}

			if (sensornum == 3) {
				gSensor3bar = bar;
				gSensor3psi = psi;
				gSensor3temp = celcius;
				gSensor3state = sensorstate;
			}

			if (sensornum == rearsensorid) {
				gSensor4bar = bar;
				gSensor4psi = psi;
				gSensor4temp = celcius;
				gSensor4state = sensorstate;
				tpmssignal = true;
				tpmssignalcount = 0;
				rearsensorread = true;
			}
			

			//fprintf (stderr, "\nPSI: S1: %f, S2: %f, S3: %f, S4: %f\n", gSensor1psi, gSensor2psi, gSensor3psi, gSensor4psi);
			//fprintf (stderr, "TMP: S1: %i, S2: %i, S3: %i, S4: %i\n", gSensor1temp, gSensor2temp, gSensor3temp, gSensor4temp);
			//fprintf (stderr, "STATE: S1: %s, S2: %s, S3: %s, S4: %s\n", stringSensorstate(gSensor1state), stringSensorstate(gSensor2state), stringSensorstate(gSensor3state), stringSensorstate(gSensor4state));
			
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

	connectTPMSInterface();

	while (quit == false) {
		memset(&read_buf, '\0', sizeof(read_buf));
		int num_bytes = read(tpms_serial_port, &read_buf, sizeof(read_buf));

		// n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
		if (num_bytes < 0) {
		    printf("Error reading: %s\n", strerror(errno));
		    close (tpms_serial_port);
		    connectTPMSInterface();
		} else {
			//fprintf(stderr, "Num bytes: %i\n", num_bytes); 
		}


		for (int r=0;r<num_bytes;r++) {
			
			if (read_buf[r] == -86 || read_buf[r] == (char)170) {
				if (r < (num_bytes-1)) {
					if (read_buf[r+1] == -95 || read_buf[r+1] == (char)161) {
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



		if ((appendbuf[0] == -86 || appendbuf[0] == (char)170) && appendpointer > 13 && (appendbuf[1] == -95 || appendbuf[1] == (char)161)) {
			
			int sensornum = appendbuf[5];
			int tpbyte1 = appendbuf[9];
			int tpbyte2 = appendbuf[10];
			int tempbyte = appendbuf[11];
			int statebyte = appendbuf[12];
			int sensorstate = 0;

			double bar = 0.025 * ((double) (((tpbyte1 & 3) * 256) + (tpbyte2 & 255)));
			double psi = bar * 14.5;
			int celcius = (tempbyte & 255) - 50;

			if ((statebyte & 3) == 1) {
				sensorstate = FASTLEAK;
			} else if ((statebyte & 16) == 16) {
				sensorstate = HIGHPRESSURE;
			} else if ((statebyte & 8) == 8) {
				sensorstate = LOWPRESSURE;
			} else if ((statebyte & 4) == 4) {
				sensorstate = HIGHTEMP;
			} else if ((statebyte & -128) == 128) {
				sensorstate = LOWBATTERY;
			} else {
				sensorstate = NORMAL;
			}


			if (sensornum == 1) {
				gSensor1bar = bar;
				gSensor1psi = psi;
				gSensor1temp = celcius;
				gSensor1state = sensorstate;
			}

			if (sensornum == frontsensorid) {
				gSensor2bar = bar;
				gSensor2psi = psi;
				gSensor2temp = celcius;
				gSensor2state = sensorstate;
				tpmssignal = true;
				tpmssignalcount = 0;
				frontsensorread = true;
			}

			if (sensornum == 3) {
				gSensor3bar = bar;
				gSensor3psi = psi;
				gSensor3temp = celcius;
				gSensor3state = sensorstate;
			}

			if (sensornum == rearsensorid) {
				gSensor4bar = bar;
				gSensor4psi = psi;
				gSensor4temp = celcius;
				gSensor4state = sensorstate;
				tpmssignal = true;
				tpmssignalcount = 0;
				rearsensorread = true;
			}
			

			//fprintf (stderr, "\nPSI: S1: %f, S2: %f, S3: %f, S4: %f\n", gSensor1psi, gSensor2psi, gSensor3psi, gSensor4psi);
			//fprintf (stderr, "TMP: S1: %i, S2: %i, S3: %i, S4: %i\n", gSensor1temp, gSensor2temp, gSensor3temp, gSensor4temp);
			//fprintf (stderr, "STATE: S1: %s, S2: %s, S3: %s, S4: %s\n", stringSensorstate(gSensor1state), stringSensorstate(gSensor2state), stringSensorstate(gSensor3state), stringSensorstate(gSensor4state));
			
			appendpointer = 0;
			memset (appendbuf, 0, 256);
		}		
		
	}
	

	close(tpms_serial_port);

	return 0;
}

int RenderTexture (SDL_Texture* tex, int x, int y, int w, int h) 
{
	SDL_Rect dstRect;
	dstRect.x = x;
	dstRect.y = y;
	dstRect.w = w;
	dstRect.h = h;

	return SDL_RenderCopy(renderer, tex, NULL, &dstRect);
}

int RenderTopIconGreyTexture (int x, int y, int w, int h) 
{
	SDL_Rect dstRect;
	dstRect.x = x;
	dstRect.y = y;
	dstRect.w = w;
	dstRect.h = h;

	if (oilpressureavailable) {
		return SDL_RenderCopy(renderer, gtopiconsgreyoptex, NULL, &dstRect);	
	} else {
		return SDL_RenderCopy(renderer, gtopiconsgreytex, NULL, &dstRect);	
	}
}

int RenderOilLightTexture (int x, int y, int w, int h)
{
	SDL_Rect dstRect;
	dstRect.x = x;
	dstRect.y = y;
	dstRect.w = w;
	dstRect.h = h;

	if (oilpressureavailable) {
		return SDL_RenderCopy(renderer, gOillightoptex, NULL, &dstRect);	
	} else {
		return SDL_RenderCopy(renderer, gOillighttex, NULL, &dstRect);	
	}
}

void RunRpmTest()
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

void RunPartialTest() {
	
	if (reverse == false) {
		testcounter++;
	}
	else {
		testcounter--;
	}

	if (testcounter > 100) {
		reverse = true;
		
	}

	if (testcounter < 1) {
		reverse = false;
		
	}

	//fuelwarning = true;

	indicateleft = reverse;
	indicateright = reverse;
	//bool indicateleft = false;
	//bool indicateright = false;


}

void RunFullTestScenarios () {
		if (reverse == false) {
			spinangle++;
		}
		else {
			spinangle--;
		}

		if (spinangle > 100) {
			reverse = true;
			ncount++;
		}

		if (spinangle < 1) {
			reverse = false;
			ncount++;
		}
		

		if (reverse == false) {
				indicateleft = false;
			//if ((spinangle % 10) == 0) {
				if (currentSpeed < 200) {
					currentSpeed+=2;	
					maxspeed+=2;
					trip1+=2;	
					trip2+=3;
					odo+=14;
					coolanttemp++;
					mpg++;
					range++;
					rpm+=100;
				}	

				if ((spinangle % 30) == 0) {
					neutral = !neutral;
					highbeam = !highbeam;
				}			

				if ((spinangle % 40) == 0) {
					oilwarning = !oilwarning;
				}		

				if ((spinangle % 20) == 0) {
					indicateright = !indicateright;
					//indicateleft = !indicateright;
				}		
			//}			
		} else {
			indicateright = false;
			//if ((spinangle % 10) == 0) {
				if (currentSpeed > 0) {
					currentSpeed-=2;	
					maxspeed-=2;
					trip1-=2;
					trip2-=3;
					odo-=14;
					coolanttemp--;
					mpg--;
					range--;
					rpm-=100;
				}	

				if ((spinangle % 30) == 0) {
					neutral = !neutral;
					highbeam = !highbeam;
				}

				if ((spinangle % 40) == 0) {
					oilwarning = !oilwarning;
				}		

				if ((spinangle % 20) == 0) {
					indicateleft = !indicateleft;					
				}							
			//}				
			
		}


		if (ncount > 13) {
			ncount = 0;
		}

		if (ncount == 1 || ncount == 2 || ncount == 3) {
			infomode = 1;				
		}

		if (ncount == 4 || ncount == 5 || ncount == 6) {
			infomode = 2;				
		}

		if (ncount == 7 || ncount == 8 || ncount == 9) {
			infomode = 3;				
		}

		if (ncount == 10 || ncount == 11 || ncount == 12 || ncount == 13) {
			infomode = 0;				
		}
}

void Drawmenu (int state) {
	if (theme == 0 || theme == 7) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	}

	SDL_RenderClear(renderer);

	// COOLANT FAN TEMP MENU
	if (state >= 500 && state < 590) {RenderTexture (gCoolantfantemptex, 0, 0, 1024, 600);}

	if (state >= 500 && state < 590) {
		if (state == 500) {RenderTexture (gUparrowsmalltex, 115, 185, 60, 62);}
		if (state == 510) {RenderTexture (gUparrowsmalltex, 341, 173, 60, 62);}
		if (state == 520) {RenderTexture (gUparrowsmalltex, 625, 173, 60, 62);}
		if (state == 530) {RenderTexture (gMenusmallarrowlefttex, 667, 300, 56, 50);}
		if (state == 540) {RenderTexture (gMenusmallarrowlefttex, 667, 378, 56, 50);}
		if (state == 550) {RenderTexture (gMenusmallarrowlefttex, 667, 454, 56, 50);}
		if (state == 560) {RenderTexture (gMenusmallarrowlefttex, 667, 532, 56, 50);}
		//gMenusmallarrowlefttex

		if (state == 570) {RenderTexture (gUparrowsmalltex, 884, 534, 60, 62);}

		drawMediumnum (coolantfantemp, 467, 114);

		if (fanneutraloption == 0) {
			RenderTexture (gSelectontex, 741, 306, 55, 38); // Coolant temp fan always on in Neutral
		}

		if (fanneutraloption == 1) {
			RenderTexture (gSelectontex, 741, 383, 55, 38); // Coolant temp fan on after 1 minute
		}

		if (fanneutraloption == 2) {
			RenderTexture (gSelectontex, 741, 460, 55, 38); // Coolant temp fan on after 50 degrees engine temp
		}

		if (fanneutraloption == 3) {
			RenderTexture (gSelectontex, 741, 537, 55, 38); // Coolant temp fan on after throttle blip
		}
	}

	// SPROCKET SETUP MENU
	if (state >= 400 && state < 490) {RenderTexture (gSprocketsetuptex, 0, 0, 1024, 600);}
	
	if (state >= 400 && state < 490) {
		if (state == 400) {RenderTexture (gUparrowsmalltex, 113, 296, 60, 62);}
		if (state == 410) {RenderTexture (gUparrowsmalltex, 286, 300, 60, 62);}
		if (state == 420) {RenderTexture (gUparrowsmalltex, 373, 300, 60, 62);}
		if (state == 430) {RenderTexture (gUparrowsmalltex, 590, 300, 60, 62);}
		if (state == 440) {RenderTexture (gUparrowsmalltex, 678, 300, 60, 62);}
		if (state == 450) {RenderTexture (gUparrowsmalltex, 594, 485, 60, 62);}
		if (state == 460) {RenderTexture (gUparrowsmalltex, 861, 485, 60, 62);}
		if (state == 470) {RenderTexture (gUparrowsmalltex, 848, 300, 60, 62);}

		drawMediumnum (frontsprocket, 332, 127);
		drawMediumnum (rearsprocket, 638, 127);
		drawMediumnum (gearratiointerval, 715, 415);

	}


	// SET ODOMETER MENU
	if (state >= 300 && state < 390) {RenderTexture (gSetodometertex, 0, 0, 1024, 600);}

	// Arrow positions for odometer first line
	if (state == 300) {RenderTexture (gUparrowtex, 175, 279, 104, 107);}
	if (state == 305) {RenderTexture (gUparrowtex, 286, 279, 104, 107);}
	if (state == 310) {RenderTexture (gUparrowtex, 399, 279, 104, 107);}
	if (state == 315) {RenderTexture (gUparrowtex, 510, 279, 104, 107);}
	if (state == 320) {RenderTexture (gUparrowtex, 620, 279, 104, 107);}
	if (state == 325) {RenderTexture (gUparrowtex, 733, 279, 104, 107);}
	// Arrow positions for odometer second line
	if (state == 330) {RenderTexture (gUparrowtex, 175, 493, 104, 107);}
	if (state == 335) {RenderTexture (gUparrowtex, 286, 493, 104, 107);}
	if (state == 340) {RenderTexture (gUparrowtex, 399, 493, 104, 107);}
	if (state == 345) {RenderTexture (gUparrowtex, 510, 493, 104, 107);}
	if (state == 350) {RenderTexture (gUparrowtex, 620, 493, 104, 107);}
	if (state == 355) {RenderTexture (gUparrowtex, 733, 493, 104, 107);}
	if (state == 360) {RenderTexture (gUparrowtex, 877, 493, 104, 107);} // Ok button

	// Odo digits
	if (state >= 300 && state < 390) {
		int ododigitspacing = 111;
		drawMediumnum (ododigit1, 214, 209);
		drawMediumnum (ododigit2, 325, 209);
		drawMediumnum (ododigit3, 325+(ododigitspacing*1), 209);
		drawMediumnum (ododigit4, 325+(ododigitspacing*2), 209);
		drawMediumnum (ododigit5, 325+(ododigitspacing*3), 209);
		drawMediumnum (ododigit6, 325+(ododigitspacing*4), 209);
		// Second row odo digits
		drawMediumnum (odo2digit1, 214, 415);
		drawMediumnum (odo2digit2, 325, 415);
		drawMediumnum (odo2digit3, 325+(ododigitspacing*1), 415);
		drawMediumnum (odo2digit4, 325+(ododigitspacing*2), 415);
		drawMediumnum (odo2digit5, 325+(ododigitspacing*3), 415);
		drawMediumnum (odo2digit6, 325+(ododigitspacing*4), 415);	
	}		

	// If the odo error flag has been set then show the error
	if (odoerror == 1) {
		RenderTexture (gOdoerror1tex, 130, 524, 758, 45);
	}
	if (odoerror == 2) {
		RenderTexture (gOdoerror2tex, 130, 524, 758, 45);	
	}

	// THEME MENU
	if (state >= 200 && state < 290) {
		RenderTexture (gThemeoptionstex, 0, 0, 1024, 600);
	}

	if (state == 200) { // Default theme 0
		RenderTexture (gArrowleftthemetex, 8, 28, 48, 159);
		RenderTexture (gArrowrightthemetex, 330, 29, 48, 159);
	}
	if (state == 210) { // theme 1
		RenderTexture (gArrowleftthemetex, 330, 28, 48, 159);
		RenderTexture (gArrowrightthemetex, 652, 29, 48, 159);		
	}
	if (state == 220) { // theme 2
		RenderTexture (gArrowleftthemetex, 660, 28, 48, 159);
		RenderTexture (gArrowrightthemetex, 982, 29, 48, 159);		
	}
	if (state == 230) { // theme 3
		RenderTexture (gArrowleftthemetex, 3, 221, 48, 159);
		RenderTexture (gArrowrightthemetex, 325, 221, 48, 159);		
	}
	if (state == 240) { // theme 4
		RenderTexture (gArrowleftthemetex, 331, 221, 48, 159);
		RenderTexture (gArrowrightthemetex, 653, 221, 48, 159);		
	}
	if (state == 250) { // theme 5
		RenderTexture (gArrowleftthemetex, 661, 221, 48, 159);
		RenderTexture (gArrowrightthemetex, 983, 221, 48, 159);		
	}
	if (state == 260) { // theme 6
		RenderTexture (gArrowleftthemetex, 8, 423, 48, 159);
		RenderTexture (gArrowrightthemetex, 330, 423, 48, 159);		
	}

	if (state == 270) { // theme 7
		RenderTexture (gArrowleftthemetex, 328, 423, 48, 159);
		RenderTexture (gArrowrightthemetex, 650, 423, 48, 159);		
	}

	if (state == 280) { // theme 8
		RenderTexture (gArrowleftthemetex, 653, 423, 48, 159);
		RenderTexture (gArrowrightthemetex, 982, 423, 48, 159);		
	}


	// SPEED CORRECTION MENU
	if (state >= 100 && state < 170) {
		RenderTexture (gSpeedcorrectiontex, 0, 0, 1024, 600);

		if (spcdigit0 == 0) {
			strcpy (strSpcdigit0, "-");
		}

		if (spcdigit0 == 1) {
			strcpy (strSpcdigit0, "+");
		}

		sprintf(strSpcdigit1, "%d", spcdigit1);
		sprintf(strSpcdigit2, "%d", spcdigit2);
		sprintf(strSpcdigit3, "%d", spcdigit3);

		drawMediumstring (strSpcdigit0, 294, 282);
		drawMediumstring (strSpcdigit1, 408, 277);
		drawMediumstring (strSpcdigit2, 516, 277);
		drawMediumstring (strSpcdigit3, 693, 277);
	}

	if (state == 100) { // Speed correction cancel
		RenderTexture (gUparrowtex, 83, 352, 104, 107);
	}

	if (state == 110) { // Speed correction digit 1
		RenderTexture (gUparrowtex, 262, 352, 104, 107);
	}

	if (state == 120) { // Speed correction digit 2
		RenderTexture (gUparrowtex, 372, 352, 104, 107);
	}

	if (state == 130) { // Speed correction digit 3
		RenderTexture (gUparrowtex, 477, 352, 104, 107);
	}

	if (state == 140) { // Speed correction digit 4
		RenderTexture (gUparrowtex, 657, 352, 104, 107);
	}

	if (state == 150) { // Speed correction ok
		RenderTexture (gUparrowtex, 824, 352, 104, 107);
	}

	// SET TPMS MENU
	if (state >= 700 && state < 790) { // Set TPMS menu options

		RenderTexture (gTPMSoptionstex, 0, 0, 1024, 600);
		
		sprintf(strFrontsensorid, "%d", frontsensorid);
		sprintf(strRearsensorid, "%d", rearsensorid);
		sprintf(strFrontpressurelow, "%d", frontpressurelow);
		sprintf(strRearpressurelow, "%d", rearpressurelow);

		drawMediumstring (strFrontsensorid, 527, 170);
		drawMediumstring (strRearsensorid, 832, 170);
		drawMediumstring (strFrontpressurelow, 830, 288);
		drawMediumstring (strRearpressurelow, 830, 405);

		if (state == 700) { // TPMS menu cancel
			RenderTexture (gUparrowsmalltex, 65, 182, 60, 62);
		}				

		if (state == 705) { // Front sensor +
			RenderTexture (gUparrowsmalltex, 420, 220, 60, 62);
		}

		if (state == 710) { // Front sensor -
			RenderTexture (gUparrowsmalltex, 610, 220, 60, 62);
		}

		if (state == 715) { // Rear sensor +
			RenderTexture (gUparrowsmalltex, 726, 220, 60, 62);
		}

		if (state == 720) { // Rear sensor -
			RenderTexture (gUparrowsmalltex, 917, 220, 60, 62);
		}

		if (state == 725) { // Front pressure warning +
			RenderTexture (gUparrowsmalltex, 727, 337, 60, 62);
		}

		if (state == 730) { // Front pressure warning -
			RenderTexture (gUparrowsmalltex, 918, 337, 60, 62);
		}

		if (state == 735) { // Rear pressure warning  +
			RenderTexture (gUparrowsmalltex, 728, 456, 60, 62);
		}

		if (state == 740) { // Rear pressure warning -
			RenderTexture (gUparrowsmalltex, 918, 456, 60, 62);
		}

		if (state == 745) { // TPMS setup OK
			RenderTexture (gUparrowsmalltex, 65, 528, 60, 62);
		}
	}

	// SET CONTROL MENU
	if (state >= 800 && state < 830) { // Set control options
		RenderTexture (gControloptionstex, 0, 0, 1024, 600);	

		if (controllayout == 0) {
			RenderTexture (gControlselecttex, 230, 195, 236, 30);
		}

		if (controllayout == 1) {
			RenderTexture (gControlselecttex, 555, 195, 236, 30);
		}

		if (state == 800) { // Control cancel
			RenderTexture (gUparrowsmalltex, 69, 363, 60, 62);
		}

		if (state == 805) { // Control layout 1
			RenderTexture (gUparrowsmalltex, 316, 394, 60, 62);
		}

		if (state == 810) { // Control layout 2
			RenderTexture (gUparrowsmalltex, 648, 394, 60, 62);
		}

		if (state == 815) { // OK
			RenderTexture (gUparrowsmalltex, 890, 363, 60, 62);
		}
	}


	// LIGHT SETUP MENU
/*
	SDL_Texture *gLightoptionstex;
SDL_Texture *gWhitethumbtex;
SDL_Texture *gGreenthumbtex;
SDL_Texture *gRedthumbtex;
SDL_Texture *gBluethumbtex;
SDL_Texture *gOrangethumbtex;
SDL_Texture *gYellowthumbtex;
SDL_Texture *gNightthumbtex;
*/
	if (state >= 900 && state < 950) { // Set light options		
		RenderTexture (gLightoptionstex, 0, 0, 1024, 600);	

		sprintf(strLightswitchvalue, "%d", lightswitchvalue);
		sprintf(strCurrentlightlevel, "%d", currentlightlevel);

		drawMediumstring (strLightswitchvalue, 625, 477);
		drawMediumstring (strCurrentlightlevel, 625, 396);

		if (daytheme == 0) {
			RenderTexture (gWhitethumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 1) {
			RenderTexture (gGreenthumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 2) {
			RenderTexture (gRedthumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 3) {
			RenderTexture (gBluethumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 4) {
			RenderTexture (gOrangethumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 5) {
			RenderTexture (gYellowthumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 6) {
			RenderTexture (gNightthumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 7) {
			RenderTexture (gBrightthumbtex, 772, 118, 126, 74);
		}

		if (daytheme == 8) {
			RenderTexture (gDarkthumbtex, 772, 118, 126, 74);
		}

		if (nighttheme == 0) {
			RenderTexture (gWhitethumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 1) {
			RenderTexture (gGreenthumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 2) {
			RenderTexture (gRedthumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 3) {
			RenderTexture (gBluethumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 4) {
			RenderTexture (gOrangethumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 5) {
			RenderTexture (gYellowthumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 6) {
			RenderTexture (gNightthumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 7) {
			RenderTexture (gBrightthumbtex, 772, 236, 126, 74);
		}

		if (nighttheme == 8) {
			RenderTexture (gDarkthumbtex, 772, 236, 126, 74);
		}


		if (state == 900) {
			RenderTexture (gUparrowsmalltex, 66, 207, 60, 62);
		}

		if (state == 905) {
			RenderTexture (gUparrowsmalltex, 708, 183, 60, 62);
		}

		if (state == 910) {
			RenderTexture (gUparrowsmalltex, 899, 183, 60, 62);
		}

		if (state == 915) {
			RenderTexture (gUparrowsmalltex, 708, 302, 60, 62);
		}

		if (state == 920) {
			RenderTexture (gUparrowsmalltex, 898, 302, 60, 62);
		}

		if (state == 925) {
			RenderTexture (gUparrowsmalltex, 531, 529, 60, 62);
		}

		if (state == 930) {
			RenderTexture (gUparrowsmalltex, 722, 529, 60, 62);
		}

		if (state == 935) {
			RenderTexture (gUparrowsmalltex, 890, 529, 60, 62);
		}		
	}

	// SET UNITS MENU
	if (state >= 600 && state < 690) { // Set units options
		RenderTexture (gSetunitstex, 0, 0, 1024, 600);	

		if (usingkm) {
			RenderTexture (gSelectontex, 894, 101, 55, 38);
		} else {
			RenderTexture (gSelectontex, 784, 101, 55, 38);
		}

		if (usingfh) {
			RenderTexture (gSelectontex, 894, 262, 55, 38);
		} else {	
			RenderTexture (gSelectontex, 784, 262, 55, 38);
		}

		if (usingbar) {
			RenderTexture (gSelectontex, 894, 423, 55, 38);
		} else {
			RenderTexture (gSelectontex, 784, 423, 55, 38);
		}
		

		if (state == 600) { // Set units cancel
			RenderTexture (gUparrowsmalltex, 34, 197, 60, 62);
		}

		if (state == 605) { // Set units digit 1
			RenderTexture (gUparrowsmalltex, 780, 154, 60, 62);
		}

		if (state == 610) { // Set units digit 2
			RenderTexture (gUparrowsmalltex, 891, 154, 60, 62);
		}

		if (state == 615) { // Set units digit 3
			RenderTexture (gUparrowsmalltex, 780, 314, 60, 62); 
		}

		if (state == 620) { // Set units digit 4
			RenderTexture (gUparrowsmalltex, 891, 314, 60, 62);
		}

		if (state == 625) { // Set units ok
			RenderTexture (gUparrowsmalltex, 780, 475, 60, 62);
		}

		if (state == 630) { // Set units ok
			RenderTexture (gUparrowsmalltex, 891, 475, 60, 62);
		}

		if (state == 635) { // Set units ok
			RenderTexture (gUparrowsmalltex, 35, 539, 60, 62);
		}
	}

	//SET TIME MENU
	if (state >= 20 && state < 90) { // Set time options
		RenderTexture (gSettimetex, 0, 0, 1024, 600);	

		sprintf(strSettimedigit0, "%d", settimedigit0);
		sprintf(strSettimedigit1, "%d", settimedigit1);
		sprintf(strSettimedigit2, "%d", settimedigit2);
		sprintf(strSettimedigit3, "%d", settimedigit3);

		drawMediumstring (strSettimedigit0, 300, 277);
		drawMediumstring (strSettimedigit1, 408, 277);
		drawMediumstring (strSettimedigit2, 583, 277);
		drawMediumstring (strSettimedigit3, 693, 277);
	}

	if (state == 20) { // Set time cancel
		RenderTexture (gUparrowtex, 92, 355, 104, 107);
	}

	if (state == 30) { // Set time digit 1
		RenderTexture (gUparrowtex, 262, 355, 104, 107);
	}

	if (state == 40) { // Set time digit 2
		RenderTexture (gUparrowtex, 371, 355, 104, 107);
	}

	if (state == 50) { // Set time digit 3
		RenderTexture (gUparrowtex, 545, 355, 104, 107);
	}

	if (state == 60) { // Set time digit 4
		RenderTexture (gUparrowtex, 657, 355, 104, 107);
	}

	if (state == 70) { // Set time ok
		RenderTexture (gUparrowtex, 824, 355, 104, 107);
	}

	//MAIN MENU
	if (state > 0 && state < 20) {
		RenderTexture (gMenuoptionstex, 0, 0, 1024, 600);	
	}

	int yarrowoffset = 51;
	if (state == 1) { // Reset trip 1
		RenderTexture (gMenusmallarrowlefttex, 77, 94, 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 465, 94, 56, 50);
	}

	if (state == 2) { // Reset trip 2
		RenderTexture (gMenusmallarrowlefttex, 505, 94, 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94, 56, 50);
	}

	if (state == 3) { // Control setup
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 465, 94+(yarrowoffset), 56, 50);
	}

	if (state == 4) { // Set units
		RenderTexture (gMenusmallarrowlefttex, 505, 94+(yarrowoffset), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset), 56, 50);
	}

	if (state == 5) { // Set time
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset*2), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*2), 56, 50);
	}

	//if (state == 6) { // Accel timer - skipping this feature for the July August 2020 update due to time constraints
		//RenderTexture (gMenusmallarrowlefttex, 505, 94+(yarrowoffset*2), 56, 50);
		//RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*2), 56, 50);
	//}

	if (state == 6) { // Ambient light setup
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset*3), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*3), 56, 50);
	}

	if (state == 7) { // Speed correction menu
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset*4), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*4), 56, 50);
	}

	if (state == 8) { // Set theme
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset*5), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*5), 56, 50);
	}

	if (state == 9) { // Gear indicator setup
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset*6), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*6), 56, 50);
	}

	if (state == 10) { // Coolant fan setup
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset*7), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*7), 56, 50);
	}

	if (state == 11) { // TPMS Setup
		RenderTexture (gMenusmallarrowlefttex, 77, 94+(yarrowoffset*8), 56, 50);
		RenderTexture (gMenusmallarrowrighttex, 897, 94+(yarrowoffset*8), 56, 50);
	}



	SDL_RenderPresent(renderer);
}

void Dashboardstartup () {
	startupanimcount++;


	if (theme == 0 || theme == 7) {

		if (startupanimcount > 0 && startupanimcount < 255) {
			startupanimcount+=3;
			SDL_SetRenderDrawColor(renderer, startupanimcount, startupanimcount, startupanimcount, SDL_ALPHA_OPAQUE);
		}

		if (startupanimcount >= 255) {
			SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);	
		}
	} else {
		if (startupanimcount < 255) {
			startupanimcount = 255;
		}
	}

	SDL_RenderClear(renderer);

	if (startupanimcount > 255) {
		startupanimcount+=8;

		if (startupanimcount < 1000) {
			RenderTopIconGreyTexture ((0-1000)+startupanimcount, 0, 627, 138);		
			RenderTexture (gtopiconsedge1tex, (625-1000)+startupanimcount, 0, 98, 75);
			RenderTexture (gtopiconsedge2tex, (721-1000)+startupanimcount, 0, 84, 24);	
			
			// Trip 1 and 2 and Odometer, Ambient Temp and Time section
			RenderTexture (gmileinfotex, (0-1000)+startupanimcount, 434, 435, 169);

			if (infomode == 0) {
				if (usingkm) {
					RenderTexture (gInfotopKMtex, (0-1000)+startupanimcount, 176, 510, 97);
				} else {
					RenderTexture (gInfotoptex, (0-1000)+startupanimcount, 176, 510, 97);	
				}
				
				RenderTexture (gInfobottomtex, (0-1000)+startupanimcount, 273, 454, 125);
			}
		}

		if (startupanimcount >= 1000) {
			RenderTopIconGreyTexture (0, 0, 627, 138);		
			RenderTexture (gtopiconsedge1tex, 625, 0, 98, 75);
			RenderTexture (gtopiconsedge2tex, 721, 0, 84, 24);	

			// Trip 1 and 2 and Odometer, Ambient Temp and Time section
			RenderTexture (gmileinfotex, 0, 434, 435, 169);

			if (infomode == 0) {
				if (usingkm) {
					RenderTexture (gInfotopKMtex, 0, 176, 510, 97);
				} else {
					RenderTexture (gInfotoptex, 0, 176, 510, 97);	
				}
				
				RenderTexture (gInfobottomtex, 0, 273, 454, 125);
			}
		}

		int revamountinc = 10;
		int revinc = 100;
		if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr0tex, NULL, &rectg0);
		}
		revamountinc+=revinc;
		if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr1tex, NULL, &rectg1);
		}
		revamountinc+=revinc;
		if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr2tex, NULL, &rectg2);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr3tex, NULL, &rectg3);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr4tex, NULL, &rectg4);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr5tex, NULL, &rectg5);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr6tex, NULL, &rectg6);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr7tex, NULL, &rectg7);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr8tex, NULL, &rectg8);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr9tex, NULL, &rectg9);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr10tex, NULL, &rectg10);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr11tex, NULL, &rectg11);
		}
		revamountinc+=revinc;
			if (startupanimcount > (100+revamountinc)) {
			SDL_RenderCopy(renderer, gr1213tex, NULL, &rectg1213);
		}
	}

	if (startupanimcount > 500) {
		
		if (startupanimcount < 1000) {
			// Fuel Gauge		
			RenderTexture (gfuelgaugetex, (676+500)-(startupanimcount-500), 294, 316, 44);
			RenderTexture (gfuelgaugewhitetex, (714+(spinangle*3)+500)-(startupanimcount-500), 303, 274, 28);

			// Coolant Temp
			if (usingfh) {
				RenderTexture (gCoolantFtex, (808+500)-(startupanimcount-500), 196, 209, 80);
			} else {
				RenderTexture (gCoolanttex, (808+500)-(startupanimcount-500), 196, 209, 80);	
			}
			
			RenderTexture (gCoolanticontex, (772+500)-(startupanimcount-500), 221, 41, 39);
		} else {
						// Fuel Gauge
			RenderTexture (gfuelgaugetex, 676, 294, 316, 44);
			RenderTexture (gfuelgaugewhitetex, 714+(spinangle*3), 303, 274, 28);

			// Coolant Temp
			if (usingfh) {
				RenderTexture (gCoolantFtex, 808, 196, 209, 80);
			} else {
				RenderTexture (gCoolanttex, 808, 196, 209, 80);	
			}
			
			RenderTexture (gCoolanticontex, 772, 221, 41, 39);
		}
	}

	if (startupanimcount > 1000) {
		startupdone = true;
	}

	SDL_RenderPresent(renderer);


}

void Animateinfomode() {

	if (infoanimationreverse == false) {
		infoanimationcount+=15;
	} else {
		infoanimationcount-=15;
	}
	
	if (infoanimationcount > 600) {
		infoanimationreverse = true;
	}

	if (infomode == 3 && currentinfomode == 2) {
		
		if (infoanimationreverse == false) {
			RenderTexture(gTyretoptex, 0-infoanimationcount, 176, 510, 97);
			RenderTexture(gTyrebottomtex, 0-infoanimationcount, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			RenderTexture(gNavbgtex, 0 - infoanimationcount, 158, 434, 268);
			//RenderTexture(gInfobottomdiagtex, 0 - infoanimationcount, 273, 454, 125);
		}

		if (infoanimationcount <= 0 && infoanimationreverse == true) {
			currentinfomode = 3;
			infoanimationinprogress = false;
		}
		return;
	}

	if (infomode == 3 && currentinfomode == 1) {
		
		if (infoanimationreverse == false) {
			RenderTexture(gInfotopdiagtex, 0 - infoanimationcount, 176, 510, 97);
			RenderTexture(gInfobottomdiagtex, 0 - infoanimationcount, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			RenderTexture(gNavbgtex, 0 - infoanimationcount, 158, 434, 268);
			//RenderTexture(gInfobottomdiagtex, 0 - infoanimationcount, 273, 454, 125);
		}

		if (infoanimationcount <= 0 && infoanimationreverse == true) {
			currentinfomode = 3;
			infoanimationinprogress = false;
		}
		return;
	}

	if (infomode == 3 && currentinfomode == 0) {
		
		if (infoanimationreverse == false) {
			if (usingkm) {
				RenderTexture(gInfotopKMtex, 0 - infoanimationcount, 176, 510, 97);
			} else {
				RenderTexture(gInfotoptex, 0 - infoanimationcount, 176, 510, 97);	
			}
			
			RenderTexture(gInfobottomtex, 0 - infoanimationcount, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			RenderTexture(gNavbgtex, 0 - infoanimationcount, 158, 434, 268);
			//RenderTexture(gInfobottomdiagtex, 0 - infoanimationcount, 273, 454, 125);
		}

		if (infoanimationcount <= 0 && infoanimationreverse == true) {
			currentinfomode = 3;
			infoanimationinprogress = false;
		}
		return;
	}

	if (infomode == 2 && currentinfomode == 1) {
		
		if (infoanimationreverse == false) {
			RenderTexture(gInfotopdiagtex, 0 - infoanimationcount, 176, 510, 97);
			RenderTexture(gInfobottomdiagtex, 0 - infoanimationcount, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			RenderTexture(gTyretoptex, 0-infoanimationcount, 176, 510, 97);
			RenderTexture(gTyrebottomtex, 0-infoanimationcount, 273, 454, 125);
		}

		if (infoanimationcount <= 0 && infoanimationreverse == true) {
			currentinfomode = 2;
			infoanimationinprogress = false;
		}
		return;
	}

	if (infomode == 1 && currentinfomode == 0) {
		
		if (infoanimationreverse == false) {
			if (usingkm) {
				RenderTexture(gInfotopKMtex, 0 - infoanimationcount, 176, 510, 97);
			} else {
				RenderTexture(gInfotoptex, 0 - infoanimationcount, 176, 510, 97);	
			}
			
			RenderTexture(gInfobottomtex, 0 - infoanimationcount, 273, 454, 125);
		}

		if (infoanimationreverse == true) {
			RenderTexture(gInfotopdiagtex, 0-infoanimationcount, 176, 510, 97);
			RenderTexture(gInfobottomdiagtex, 0-infoanimationcount, 273, 454, 125);
		}

		if (infoanimationcount <= 0 && infoanimationreverse == true) {
			currentinfomode = 1;
			infoanimationinprogress = false;
		}
		return;
	}

	if (infomode == 0 && currentinfomode == 3) {
		if (infoanimationreverse == false) {
			//RenderTexture(gInfotopdiagtex, 0 - infoanimationcount, 176, 510, 97);
			//RenderTexture(gInfobottomdiagtex, 0 - infoanimationcount, 273, 454, 125);
			RenderTexture(gNavbgtex, 0 - infoanimationcount, 158, 434, 268);
		}

		if (infoanimationreverse == true) {
			if (usingkm) {
				RenderTexture(gInfotopKMtex, 0- infoanimationcount, 176, 510, 97);
			} else {
				RenderTexture(gInfotoptex, 0- infoanimationcount, 176, 510, 97);	
			}
			
			RenderTexture(gInfobottomtex, 0- infoanimationcount, 273, 454, 125);
		}

		if (infoanimationcount <= 0 && infoanimationreverse == true) {
			currentinfomode = 0;
			infoanimationinprogress = false;
		}
		return;
	}

	if (infomode == 0 && currentinfomode == 2) {
		
		// This one goes away
		if (infoanimationreverse == false) {
			RenderTexture(gTyretoptex, 0-infoanimationcount, 176, 510, 97);
			RenderTexture(gTyrebottomtex, 0-infoanimationcount, 273, 454, 125);
		}

		// This one comes in
		if (infoanimationreverse == true) {
			if (usingkm) {
				RenderTexture(gInfotopKMtex, 0- infoanimationcount, 176, 510, 97);
			} else {
				RenderTexture(gInfotoptex, 0- infoanimationcount, 176, 510, 97);	
			}
			
			RenderTexture(gInfobottomtex, 0- infoanimationcount, 273, 454, 125);
		}

		if (infoanimationcount <= 0 && infoanimationreverse == true) {
			currentinfomode = 0;
			infoanimationinprogress = false;
		}
		return;
	}
}

void Drawdashboard () {
	flashcount++;
	if (flashcount > 50) {
		flashcount = 0;
		flash = !flash;
	}

	if ((infomode != currentinfomode && warningbadgeactive)) {		
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
	SDL_RenderCopy(renderer, grevlinetex, NULL, &grevline);

	targetrpmrotation = GetRPMrotation (rpm);
	if (targetrpmrotation > currentrpmrotation) {
		if (currentrpmrotation < 100) {
			currentrpmrotation++;
		}
	}

	if (targetrpmrotation < currentrpmrotation) {
		if (currentrpmrotation > 0) {
			currentrpmrotation--;
		}
	}

	// White rev counter cover to reveal the rev line
	SDL_RenderCopyEx(renderer, grevwhitetex, NULL, &grrevwhite, currentrpmrotation , &gwhitepoint, SDL_FLIP_NONE);

	// Top grey icons
	RenderTopIconGreyTexture (0, 0, 627, 138);
	RenderTexture (gtopiconsedge1tex, 625, 0, 98, 75);
	RenderTexture (gtopiconsedge2tex, 721, 0, 84, 24);

	// Trip 1 and 2 and Odometer, Ambient Temp and Time section
	RenderTexture (gmileinfotex, 0, 434, 435, 169);

	// Fuel Gauge
	if (fuelwarning == true && enginerunning == true) {
		if (flash) 
		{
			RenderTexture (gfuelgaugetex, 676, 294, 316, 44);
			RenderTexture (gfuelgaugewhitetex, 714+GetFuelRevealFromBars (GetNumFuelBars(fuelfloat)), 303, 274, 28);
		}
	} else {
		RenderTexture (gfuelgaugetex, 676, 294, 316, 44);
		RenderTexture (gfuelgaugewhitetex, 714+GetFuelRevealFromBars (GetNumFuelBars(fuelfloat)), 303, 274, 28);
	}
	// Coolant Temp
	if (overheatwarning == true && enginerunning == true) {
		if (flash) {
			if (usingfh) {
				RenderTexture (gCoolantFtex, 808, 196, 209, 80);
			} else {
				RenderTexture (gCoolanttex, 808, 196, 209, 80);	
			}	
		}
	} else {
		if (usingfh) {
			RenderTexture (gCoolantFtex, 808, 196, 209, 80);
		} else {
			RenderTexture (gCoolanttex, 808, 196, 209, 80);	
		}
	}

	RenderTexture (gCoolanticontex, 772, 221, 41, 39);

	// Top light icons (Neutral, Oil, Indicator, High beam)

	if (neutral) {
		RenderTexture (gNeutrallighttex, 0, 0, 130, 136);
	}

	if (!neutral && currentgear != 0) {
		RenderTexture (gGeartex, 0, 0, 131, 136);
		drawLargenum (currentgear, 30, 22);
	}

	if (oilwarning) {
		RenderOilLightTexture (131, 0, 135, 138);
	}

	if (indicateright && !indicateleft) {
		RenderTexture (gIndicaterighttex, 267, 0, 135, 136);
	}

	if (indicateleft && !indicateright) {
		RenderTexture (gIndicatelefttex, 267, 0, 135, 136);		
	}

	if (indicateleft && indicateright) {
		RenderTexture (gIndicatebothtex, 267, 0, 135, 136);			
	}

	if (highbeam) {
		RenderTexture (gHighbeamlighttex, 404, 0, 133, 136);
	}

	// Middle info section (Tank Range, MPG, etc..)
	if (infomode == 0 && !warningbadgeactive) {
		if (infoanimationinprogress == false) {
			if (usingkm) {
				RenderTexture(gInfotopKMtex, 0, 176, 510, 97);
			} else {
				RenderTexture(gInfotoptex, 0, 176, 510, 97);	
			}
			
			RenderTexture(gInfobottomtex, 0, 273, 454, 125);
		} 
	}

	if (infomode == 1 && !warningbadgeactive) {
		if (infoanimationinprogress == false) {
			RenderTexture(gInfotopdiagtex, 0, 176, 510, 97);
			RenderTexture(gInfobottomdiagtex, 0, 273, 454, 125);
		} 
	}

	if (infomode == 2 && !warningbadgeactive) {
		if (infoanimationinprogress == false) {
			RenderTexture(gTyretoptex, 0, 176, 510, 97);
			RenderTexture(gTyrebottomtex, 0, 273, 454, 125);
		} 
	}

	if (infomode == 3 && !warningbadgeactive) {
		if (infoanimationinprogress == false) {			
			RenderTexture(gNavbgtex, 0, 158, 434, 268);
		} 
	}

	if ((infomode != currentinfomode && !warningbadgeactive)) {		
		if (infoanimationinprogress == false) {
			infoanimationcount = 0;
			infoanimationreverse = false;
			infoanimationinprogress = true;
		}		

		Animateinfomode();
	}

	/*
	if (infomode == 2) { // Low Fuel
		RenderTexture (gLowfuelbadgetex, 0, 163, 444, 249);
		if (neutral) {
			RenderTexture (gLowfueltex, 222, 236, 134, 105);
		}
	}
	*/


	if (tpmsconnected == true) {

		if (frontsensorread) {
			if (gSensor2psi <= frontpressurelow) {
				fronttyrewarningtriggered = true;
			}
		}

		if (rearsensorread) {
			if (gSensor4psi <= rearpressurelow) {
				reartyrewarningtriggered = true;
			}	
		}
			
	}

	// Warning badges
	if (!warningbadgecancelled) {
		if (fronttyrewarningtriggered || reartyrewarningtriggered) {
			
			warningbadgeactive = true;
			RenderTexture (gLowtyrebadgetex, 0, 163, 444, 249);

			if (flash) {
				if (fronttyrewarningtriggered && !reartyrewarningtriggered) {
					// Front only
					RenderTexture (gFronttyrelowtex, 168, 244, 257, 76);
				}

				if (reartyrewarningtriggered && !fronttyrewarningtriggered) {
					// Rear only
					RenderTexture (gReartyrelowtex, 168, 244, 257, 76);
				}

				if (reartyrewarningtriggered && fronttyrewarningtriggered) {
					// Front and Rear
					RenderTexture (gBothtyrelowtex, 168, 244, 257, 76);
				}

			}

		} else {
			
			if (oilwarning == true && enginerunning == true) { // Oil warning takes priority
				
				warningbadgeactive = true;
				RenderTexture (gLowoilbadgetex, 0, 163, 444, 249);
				
				if (flash) {
					RenderTexture (gLowoiltex, 222, 236, 134, 105);
				}

			} else {
				
				if (overheatwarning == true && enginerunning == true) { // Second priority is engine overheat warning
					warningbadgeactive = true;
					RenderTexture(gOverheatbadgetex, 0, 163, 444, 249);
					if (flash) {
						RenderTexture(gEngineoverheattex, 189, 237, 212, 95);
					}
				} else {
					
					if (fuelwarning == true && enginerunning == true && infomode != 3) { // Third priority is fuel warning
						warningbadgeactive = true;
						RenderTexture(gLowfuelbadgetex, 0, 163, 444, 249);
						if (flash) {
							RenderTexture(gLowfueltex, 222, 236, 134, 105);
						}
					} else {
						warningbadgeactive = false;
					}
				}

			}	
		}
	}

	// Units - either MPH or KM
	if (usingkm == 1) {
		RenderTexture (gKphtex, 844, 553, 179, 44);
		RenderTexture (gKmtex, 350, 448, 57, 18);
	} else {
		RenderTexture (gMphtex, 844, 553, 142, 45);
		RenderTexture (gMilestex, 350, 448, 57, 18);	
	}
	

	// Rev counter numbers
	SDL_RenderCopy(renderer, gr1213tex, NULL, &rectg1213);
	SDL_RenderCopy(renderer, gr11tex, NULL, &rectg11);
	SDL_RenderCopy(renderer, gr10tex, NULL, &rectg10);
	SDL_RenderCopy(renderer, gr9tex, NULL, &rectg9);
	SDL_RenderCopy(renderer, gr8tex, NULL, &rectg8);
	SDL_RenderCopy(renderer, gr7tex, NULL, &rectg7);
	SDL_RenderCopy(renderer, gr6tex, NULL, &rectg6);
	SDL_RenderCopy(renderer, gr5tex, NULL, &rectg5);
	SDL_RenderCopy(renderer, gr4tex, NULL, &rectg4);
	SDL_RenderCopy(renderer, gr3tex, NULL, &rectg3);
	SDL_RenderCopy(renderer, gr2tex, NULL, &rectg2);
	SDL_RenderCopy(renderer, gr1tex, NULL, &rectg1);
	SDL_RenderCopy(renderer, gr0tex, NULL, &rectg0);

	sprintf( strCurrentspeed, "%d", currentSpeed);
	sprintf( strTrip1, "%.1f", trip1);
	sprintf( strTrip2, "%.1f", trip2);
	sprintf( strOdo, "%.0f", odo);

	if (usingfh) { // If using fahrenheit
		tempf = ((double)ambientTemp*1.8) + 32;
		sprintf(strAmbienttemp, "%df", (int)tempf);
	} else {
		sprintf(strAmbienttemp, "%dc", ambientTemp);
	}
	
	if (usingfh) { //If using fahrenheit
		coolanttempf = ((double)coolanttemp*1.8) + 32;
		sprintf(strCoolanttemp, "%d", (int)coolanttempf);
	} else {
		sprintf(strCoolanttemp, "%d", coolanttemp);
	}
	
	if (usingkm) {
		if (mpg > 0) {
			sprintf(strMpg, "%d", (int)((double)282.481 / (double)mpg));	
		} else {
			sprintf(strMpg, "%d", mpg);	
		}
		
	} else {
		sprintf(strMpg, "%d", mpg);
	}
	
	if (usingkm) {
		sprintf(strRange, "%d", (int)((double)range * 1.609));
	} else {
		sprintf(strRange, "%d", range);	
	}
	
	sprintf(strMaxspeed, "%d", maxspeed);
	sprintf(strRpm, "%d", rpm);
	
	if (oilpressureavailable) {
		sprintf(strOilpress, "%.1f", GetPreciseBar(oilpressureohms));
		sprintf(strOiltemp, "%d", (int)GetPreciseTemp(oiltempohms));	
	}

	if (gSensor2psi != -99) {
		if (usingbar) {
			sprintf (strSensor2psi, "%.1f", gSensor2bar);
		} else {
			sprintf (strSensor2psi, "%.1f", gSensor2psi);
		}
		
	} else {
		strcpy (strSensor2psi, "..");
	}

	if (gSensor4psi != -99) {
		if (usingbar) {
			sprintf (strSensor4psi, "%.1f", gSensor4bar);	
		} else {
			sprintf (strSensor4psi, "%.1f", gSensor4psi);
		}
		
	} else {
		strcpy (strSensor4psi, "..");
	}
	
	if (gSensor2temp != -99) {
		if (usingfh) {
			gSensor2tempF = ((double)gSensor2temp*1.8) + 32;
			sprintf (strSensor2temp, "%df", (int)gSensor2tempF);	
		} else {
			sprintf (strSensor2temp, "%dc", gSensor2temp);	
		}		
	} else {
		strcpy (strSensor2temp, "..");
	}
	
	if (gSensor4temp != -99) {
		if (usingfh) {
			gSensor4tempF = ((double)gSensor4temp*1.8) + 32;
			sprintf (strSensor4temp, "%df", (int)gSensor4tempF);
		} else {
			sprintf (strSensor4temp, "%dc", gSensor4temp);
		}		
	} else {
		strcpy (strSensor4temp, "..");
	}
	

	if (GetLitresRemaining(fuelfloat) != 0) {
		sprintf(strFuel, "%.1f", litresremaining);
	} else {
		strcpy (strFuel, "..");
	}
	
	sprintf(strBatt, "%.1f", batt);
	sprintf(strSpdcorrect, "%.1f", spdcorrect);


	// Draw the current speed
	drawSpeed(strCurrentspeed);		

	// Draw some numbers
	drawMediumstring (strAmbienttemp, 18, 461);
	drawMediumstring (strCoolanttemp, 873, 224);

	// Middle Info
	if (infomode == 0 && infoanimationinprogress == false && !warningbadgeactive) {
		drawMediumstring (strMpg, 60, 224);
		drawMediumstring (strRange, 292, 224);
		drawMediumstring (strTriptime, 17, 332);
		drawMediumstring (strMaxspeed, 246, 332);
	}

	if (infomode == 1 && infoanimationinprogress == false && !warningbadgeactive) {
		// RPM, FUEL, BATT, SPEED CORRECTION %
		//char strRpm[16];
		//char strFuel[16];
		drawMediumstring(strRpm, 60, 224);
		drawMediumstring(strFuel, 292, 224);
		drawMediumstring (strBatt, 17, 332);
		if (strlen (strSpdcorrect) > 4) {
			drawMediumstring (strSpdcorrect, 216, 332);
		} else {
			drawMediumstring (strSpdcorrect, 236, 332);	
		}		
	}

	//gSensor2psi
	if (infomode == 2 && infoanimationinprogress == false && !warningbadgeactive) {
		drawMediumstring (strSensor2psi, 40, 224);
		drawMediumstring (strSensor4psi, 304, 224);
		drawMediumstring (strSensor2temp, 37, 342);
		drawMediumstring (strSensor4temp, 246, 342);
	}

	if (infomode == 3 && infoanimationinprogress == false && !warningbadgeactive) {
		

		if (navActive == true) {
			if (strlen (strNavRoad) > 0) {
				drawNavSymbol (ONTO, 13, 350);
			}

			if (strlen (strNavRoad) > 0 && strlen (strNavRoad) <= 14) {
				drawNavLargeString (strNavRoad, 69, 347);			
			} else {
				drawNavSmallString (strNavRoad, 69, 347);			
			}
			
			if (strlen (strNavTowards) > 0) {
				drawNavSymbol (TWRDS, 13, 391);
			}

			if (strlen (strNavTowards) > 0 && strlen (strNavTowards) <= 16) {			
				drawNavLargeString (strNavTowards, 99, 386);
			} else {			
				drawNavSmallString (strNavTowards, 99, 386);
			}
			
			drawNavSmallString (szArrivein, 14, 175);
			
			if (usingkm) {
				drawNavLargeString (strNavDestKm, 77, 171);
				drawNavSmallString (szKm, 165, 175);
			} else {
				drawNavLargeString (strNavDestMiles, 77, 171);
				drawNavSmallString (szMls, 165, 175);
			}
			

			if (navYards <= 300) {
				//drawNavNumbers (87, 16, 191);				
				if (usingkm) {
					drawNavDigits (strNavMetres, 16, 211);
					drawNavSymbol (METRE, 22, 308);
				} else {
					drawNavDigits (strNavYards, 16, 211);
					drawNavSymbol (YARD, 22, 308);
				}				
			} else {
				if (usingkm) {
					drawNavDigits (strNavKm, 16, 211);
					drawNavSymbol (KM, 22, 308);
				} else {
					drawNavDigits (strNavMiles, 16, 211);
					drawNavSymbol (MILE, 22, 308);
				}				
			}
			
			if (strcmp (strNavSymbol, "MUT") == 0) {
				if (drivingleft == 1) {
					drawNavSymbol (MUT, 249, 180);
				}

				if (drivingleft == 0) {
					drawNavSymbol (MUTR, 249, 180);
				}
			}

			if (strcmp (strNavSymbol, "MEX") == 0) {
				if (drivingleft == 1) {
					drawNavSymbol (EXITL, 249, 180);
				}

				if (drivingleft == 0) {
					drawNavSymbol (EXITR, 249, 180);
				}

				// Draw Exit number
				drawNavLargeString (strNavExit, 344, 275);
			}

			if (strcmp (strNavSymbol, "TNR") == 0) {
				drawNavSymbol (TNR, 249, 180);			
			}

			if (strcmp (strNavSymbol, "TNL") == 0) {
				drawNavSymbol (TNL, 249, 180);			
			}

			if (strcmp (strNavSymbol, "SLL") == 0) {
				drawNavSymbol (SLL, 249, 180);			
			}

			if (strcmp (strNavSymbol, "SLR") == 0) {
				drawNavSymbol (SLR, 249, 180);			
			}

			if (strcmp (strNavSymbol, "RB1") == 0) {
				if (drivingleft == 1) {
					drawNavSymbol (RB1L, 249, 180);
				}

				if (drivingleft == 0) {
					drawNavSymbol (RB1R, 249, 180);
				}
			}

			if (strcmp (strNavSymbol, "RB2") == 0) {
				if (drivingleft == 1) {
					drawNavSymbol (RB2L, 255, 170);
				}

				if (drivingleft == 0) {
					drawNavSymbol (RB2R, 255, 170);
				}
			}		

			if (strcmp (strNavSymbol, "PRK") == 0) {
				drawNavLargeString (szFindparking, 99, 386);
			}

			if (strcmp (strNavSymbol, "FRY") == 0) {
				drawNavLargeString (szTakeFerry, 99, 386);
			}

			if (strcmp (strNavSymbol, "RB3") == 0) {
				if (drivingleft == 1) {
					drawNavSymbol (RB3L, 249, 180);
				}

				if (drivingleft == 0) {
					drawNavSymbol (RB3R, 249, 180);
				}
			}		

			if (strcmp (strNavSymbol, "RB4") == 0) {
				if (drivingleft == 1) {
					drawNavSymbol (RB4L, 249, 180);
				}

				if (drivingleft == 0) {
					drawNavSymbol (RB4R, 249, 180);
				}
			}

			if (strcmp (strNavSymbol, "RB5") == 0) {
				if (drivingleft == 1) {
					drawNavSymbol (RB5L, 249, 180);
				}

				if (drivingleft == 0) {
					drawNavSymbol (RB5R, 249, 180);
				}
			}

			if (strcmp (strNavSymbol, "LNR") == 0) {
				drawNavSymbol (KPR, 249, 180);			
			}

			if (strcmp (strNavSymbol, "LNL") == 0) {
				drawNavSymbol (KPL, 249, 180);			
			}

			if (strcmp (strNavSymbol, "CON") == 0) {
				drawNavSymbol (CON, 249, 180);			
			}

			if (strcmp (strNavSymbol, "ARV") == 0) {
				drawNavSymbol (ARV, 249, 180);			
			}
		} else {
			drawNavLargeString (szNoNavData, 40, 265);
			drawNavSmallString (szSmartphoneapp, 40, 350);
			drawNavSmallString (szSetdestination, 56, 380);
		}
		

	}

	if (indicateleft && !warningbadgeactive) {		
		if (infomode == 0 || infomode == 1) {
			RenderTexture (gIndicateleftfartex, 2, 186, 250, 98);
		}		
	}

	if (indicateright && !warningbadgeactive) {		
		RenderTexture (gIndicaterightfartex, 802, 192, 209, 84);
	}
	

	// Essential info
	drawMediumstring(strTime, 14, 543);
	drawSmallbluestring (strTrip1, 252, 460);
	drawSmallbluestring (strTrip2, 246, 509);
	drawSmallgreystring (strOdo, 223, 560);

	if (oilpressureavailable) {
		drawMediumstring(strOilpress, 163, 32);
		drawMediumstring(strOiltemp, 163, 89);		
	}

	if (tpmsconnected) {
		RenderTexture (gTyreicontex, 630, 553, 22, 45);
	}

	if (tpmsconnected && tpmssignal && tpmssignalcount < 300) {
		RenderTexture (gTyresignaltex, 653, 559, 33, 34);
		tpmssignalcount++;
	}

	SDL_RenderPresent(renderer);
}

int main(int argc, char* args[]) {


	strcpy (strTriptime, "00:00");
	strcpy(strTime, "14:35");

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

	//std::thread t1(threadworker, "hello");
	SDL_ShowCursor(SDL_DISABLE);
	
	int err;
	err = pthread_create(&(tid[0]), NULL, &pollInterface, NULL);
    if (err != 0)
        printf("\ncan't create thread :[%s]", strerror(err));
    else
        printf("\n Thread created successfully\n");

	if (tpmsmodel == STANDARDTPMS) {
		err = pthread_create(&(tpms_tid[0]), NULL, &pollTPMSInterface, NULL);
	}		

	if (tpmsmodel == EBAYTPMS) {
		err = pthread_create(&(tpms_tid[0]), NULL, &pollTPMSInterface2, NULL);
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

	if (window == NULL) {
		fprintf(stderr, "could not create window: %s\n", SDL_GetError());
		return 1;
	}

	screenSurface = SDL_GetWindowSurface(window);

	if (theme == 0) {
		if (loadsurfaces(NULL) != 0) {
			return 1;
		}
	}

	if (theme == 1) {
		if (loadsurfaces("green") != 0) {
			return 1;
		}
	}

	if (theme == 2) {
		if (loadsurfaces("red") != 0) {
			return 1;
		}
	}

	if (theme == 3) {

		if (loadsurfaces("blue") != 0) {
			return 1;
		}
	}

	if (theme == 4) {
		if (loadsurfaces("orange") != 0) {
			return 1;
		}
	}

	if (theme == 5) {
		if (loadsurfaces("yellow") != 0) {
			return 1;
		}
	}

	if (theme == 6) {
		if (loadsurfaces("night") != 0) {
			return 1;
		}
	}

	if (theme == 7) {
		if (loadsurfaces("bright") != 0) {
			return 1;
		}
	}

	if (theme == 8) {
		if (loadsurfaces("dark") != 0) {
			return 1;
		}
	}

	
	initrects();

	//if (loadsurfaces() != 0) {
	//	return 1;
	//}

	if (inittextures() != 0) {
		return 1;
	}

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
				//spinangle--;
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
				//spinangle++;
				
			}

			if (e.type == SDL_MOUSEBUTTONDOWN) {
				//quit = true;
			}
		}

		//RunPartialTest();

		if (theme != currenttheme) {			
			currenttheme = theme;
			if (theme == 0) {
				loadsurfaces(NULL);
			}

			if (theme == 1) {
				loadsurfaces("green");
			}

			if (theme == 2) {
				loadsurfaces("red");
			}

			if (theme == 3) {
				loadsurfaces("blue");
			}

			if (theme == 4) {
				loadsurfaces("orange");
			}

			if (theme == 5) {
				loadsurfaces("yellow");
			}

			if (theme == 6) {
				loadsurfaces("night");
			}

			if (theme == 7) {
				loadsurfaces("bright");
			}

			if (theme == 8) {
				loadsurfaces("dark");
			}
			inittextures();
		}

		//odo = spinangle;
		//odo = GetFuelreveal(fuelpercent);

		if (choicestate == 0) {
			//Drawdashboard();
			if (startupdone == false) {
				Dashboardstartup();
			}
			else {

				//RunRpmTest();
				Drawdashboard();
			}

			//printf("%i\n", GetFuelreveal(50));

			gDownarrowx = -140;
			gUparrowx = -140;

		}
		else {
			if (choicestate > 0) {
				Drawmenu(choicestate);
			}
		}



		// Thresholds for triggering warnings
		if (GetNumFuelBars(fuelfloat) <= 2) { // 2 bars remaining - 4.5 litres left
			fuelwarning = true;
		} else {
			fuelwarning = false;
		}

		if (coolanttemp >= 120) {
			overheatwarning = true;
		}

		if (coolanttemp < 120) {
			overheatwarning = false;
		}

		// Quick check on the RPM to set a flag if the engine is running - used for triggering warning badges
		if (rpm > 2000) {
			enginerunning = true;
		} 

		if (rpm < 1000) {
			enginerunning = false;
		}


		//SDL_FillRect(screenSurface, &rect, SDL_MapRGB(screenSurface->format, 0x0, 0x0, 0x0));
		
		//SDL_UpdateWindowSurface(window);
		SDL_Delay(5);

		//fprintf(stderr, "Main thread: %s\n", gszCommsmsg);
	}

	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}


