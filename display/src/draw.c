/*
 * draw.c — Rendering system for TFT Dash
 *
 * Owns SDL initialisation, asset loading, event pumping, and all
 * drawing primitives. Main.c has no SDL dependency.
 */

#include "draw.h"
#include "draw_dashboard.h"
#include "draw_menu.h"
#include "animation.h"
#include "dashboard_anims.h"
#include "warnings.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Shared state */
SDL_Renderer *renderer = NULL;
asset_store *g_assets = NULL;
const char *g_current_theme = "default";

const dashboard_state *dash = NULL;
const menu_state *menu = NULL;
const nav_state *nav = NULL;
const tpms_state *tpms = NULL;

static SDL_Window *window = NULL;
static int currenttheme = 0;

/* Theme names and lookup */
const char *THEME_NAMES[] = {
	"default", "green", "red", "blue", "orange", "yellow", "night", "bright", "dark"
};

const char *theme_name_from_id(int id) {
	if (id >= 0 && id < THEME_COUNT) return THEME_NAMES[id];
	return "default";
}

/* Texture helpers */
SDL_Texture *tex(const char *name) {
	return asset_store_get_texture(g_assets, g_current_theme, name);
}

SDL_Texture *tex_from(const char *theme, const char *name) {
	return asset_store_get_texture(g_assets, theme, name);
}

/* Speedometer digit rects */
SDL_FRect spd_digit_one;
SDL_FRect spd_digit_two;
SDL_FRect spd_digit_three;

/* Navigation icon sprite atlas — indexed by nav_icon enum */
const int g_nav_icons_src_tex_loc[4][NAV_ICON_COUNT] = {
	{21,   169,    317,   449,  628, 809, 1002,17,   163,  312,  460, 598, 771, 937, 1092,6,  174,333,491,642, 823, 983, 11,  106,215,280,  404, 456  }, // X pos
	{12,   11,     12,    20,   19,  22,  22,  184,  216,  217,  217, 214, 217, 216, 216, 383,385,384,394,387, 364, 385, 549, 550,550,549,  556, 557  }, // Y pos
	{112,  112,    112,   157,  147, 158, 176, 115,  134,  114,  110, 131, 131, 131, 131, 141,150,131,112,138, 116, 108, 81,  87,  42,105,  45,  76   }, // Width
	{150,  150,    150,   146,  150, 144, 146, 167,  134,  135,  135, 137, 134, 134, 134, 140,138,138,129,135, 167, 145, 24,  21,  22,22,   16,  14   }  // Height
};

/* Navigation symbol lookup table */
const nav_symbol_entry NAV_SYMBOLS[] = {
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

const nav_symbol_entry *lookup_nav_symbol(const char *code) {
	for (int i = 0; i < (int)(sizeof(NAV_SYMBOLS) / sizeof(NAV_SYMBOLS[0])); i++) {
		if (strcmp(code, NAV_SYMBOLS[i].code) == 0) return &NAV_SYMBOLS[i];
	}
	return NULL;
}

/* Letter reference arrays */
const char g_nav_large_upper_letter_ref[40] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3','4','5','6','7','8','9',',','.','(',')'};
const char g_nav_large_lower_letter_ref[40] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9',',','.','(',')'};

/* Navigation letter sprite atlas — large */
const int g_nav_letters_large_src_tex_loc[4][40] = {
// 	A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  COM, STP,  (,  )
   {15, 46, 75, 106,137,165,190,222,253,268,295,325,351,386,416,449,476,509,537,565,593,622,649,686,714,743,771,797,821,846,871,896,921,946,970,996,1022,1039,1055,1079 },
   {602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602,602, 602, 602, 602  },
   {22, 19, 20, 20, 17, 16, 22, 19, 5,  15, 20, 16, 24, 19, 22, 18, 22, 18, 19, 18, 19, 19, 29, 20, 21, 19, 16, 11, 16, 16, 16, 16, 16, 15, 17, 16, 5,   5,   9,   8,   },
   {22, 22, 23, 21, 21, 21, 23, 21, 21, 22, 21, 21, 21, 21, 23, 22, 24, 22, 23, 21, 22, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 21, 21, 21, 21, 22, 27,  22,  27,  27,  }
};

/* Navigation letter sprite atlas — small */
const int g_nav_letters_small_src_tex_loc[4][40] = {

// 	A,  B,  C,  D,  E,  F,  G,  H,  I,  J,  K,  L,  M,  N,  O,  P,  Q,  R,  S,  T,  U,  V,  W,  X,  Y,  Z,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  COM, STP,  (,  )
   {17, 42, 65, 90, 114,137,157,182,207,219,241,265,286,314,337,364,386,412,435,457,479,502,524,553,576,599,622,643,662,681,701,721,742,762,781,802,822, 836, 849, 861 },
   {778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778,778, 778, 778, 778 },
   {18, 15, 17, 16, 14, 13, 17,16,  4,  13, 16, 13, 19, 16,	18,	14, 18, 15, 15, 15, 16, 16, 24, 18, 17, 15, 13, 8,  13, 14, 14, 14, 13, 12,	14, 13,	5,	 5,	  7,   7   },
   {17, 17, 17, 17, 17, 17, 17,17,  17, 17, 17, 17, 17, 17,	17,	17, 19, 17, 18, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18,	18, 21,	 18,  22,  22  }

};

/* Navigation number reference and sprite atlas */
const int g_nav_numbers_ref[11] = {'0','1','2','3','4','5','6','7','8','9','.'};
const int g_nav_numbers_src_tex_loc[4][11] = {

//   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,     .
	{10, 115,209,310,409,509,611,711,809,910, 447},
	{660,661,660,661,661,661,660,660,660,660, 734},
	{68, 46, 69, 66, 68, 67, 66, 63, 68, 66,  17},
	{93, 93, 93, 93, 90, 91, 92, 91, 92, 92, 15 }
};

/* Large speedometer numbers — texture mapping */
const int g_speed_src_tex_loc[4][10] = {
//    0    1    2    3    4    5    6    7    8     9		// The digit in the bitmap
	{ 0  , 143, 255, 396, 530, 669, 808, 939, 1080, 1211 }, // X position from Top Left
	{ 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1  , 1   , 1 },    // Y position from Top Left
	{ 135, 107, 142, 134, 139, 138, 131, 141, 131 , 137 },  // Width of digit
	{ 172, 172, 166, 172, 172, 172, 172, 172, 172 , 172 }   // Height of digit
};

/* Medium sized numbers — clock & temp displays */
const int g_med_numbers_src_tex_loc[4][16] = {
//    0    1    2    3    4    5    6    7    8     9	  :   +   -     .   oC.   oF	// The digit in the bitmap
	{ 314, 2,  32,  68,  104, 139, 175, 209, 243, 279,   5,   32,  73, 102, 138, 183},	// X position
	{ 6,   6,   6,   6,  6,   6,   6,   6,   6,    6,    50,  52,  51, 47,  50,  50},  // Y position
	{ 26, 20,  29,  28,  28,  27,  25,  27, 26 ,  26,    16,  35,  34, 21,  40,  40},  // Width of digit
	{ 36, 39,  36,  36,  36,  36,  36,  36, 36 ,  35,    31,  31,  31, 39,  33,  33}   // Height of digit
};

/* Small blue numbers — mileage and trip displays */
const int g_small_blue_src_tex_loc[4][11] = {
//    0    1    2    3    4    5    6    7    8     9     .	  // The digit in the bitmap
	{ 546, 355, 372, 394, 416, 438, 460, 483, 503, 525, 354 }, // X position
	{ 19,  19,  19,  19,  19,  19,  19,  19,  19,  19, 37 },  // Y position
	{ 18,  12,  18,  18,  18,  18,  17,  17,  18,  18, 11 },  // Width of digit
	{ 23,  23,  23,  22,  23,  23,  23,  22,  23,  23, 4 }   // Height of digit
};

/* Small grey numbers — odometer */
const int g_small_grey_src_tex_loc[4][11] = {
	//    0    1    2    3    4    5    6    7    8     9    .	  // The digit in the bitmap
	{ 546, 355, 372, 394, 416, 438, 460, 483, 503, 525, 354 }, // X position
	{ 52,  52,  52,  52,  52,  52,  52,  52,  52,  52, 37 },  // Y position
	{ 18,  12,  18,  18,  18,  18,  17,  17,  18,  18, 11 },  // Width of digit
	{ 23,  23,  23,  22,  23,  23,  23,  22,  23,  23, 4 }   // Height of digit
};

/* Large numbers — gear indicator */
const int g_large_numbers_src_tex_loc[4][10] = {
	// 1    2    3    4    5    6    7    8    9     0    	  // The digit in the bitmap
	{ 0,   66, 166, 264, 367, 468, 571, 667, 766, 865 }, // X position
	{ 91,  91,  91,  91,  91,  91,  91,  91,  91,  91 },  // Y position
	{ 63,  68,  68,  68,  68,  68,  68,  68,  68,  68 },  // Width of digit
	{ 94,  94,  94,  94,  94,  94,  94,  94,  94,  94 }   // Height of digit
};

/* RPM lookup table — converting RPM values into rotation value to reveal rev line */
const int g_rpm_lookup[53][2] = {
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

/* Character reference tables */
const char g_num_ref[16] = { '0','1','2','3','4','5','6','7','8','9',':','+','-','.','c','f'};
const char g_small_num_ref[11] = { '0','1','2','3','4','5','6','7','8','9','.'};
const char g_large_num_ref[10] = { '1','2','3','4','5','6','7','8','9','0'};

/* --- Glyph font descriptor — parameterises all the draw_*_string() variants --- */

typedef struct {
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
} glyph_font;

static const glyph_font FONT_SMALL_GREY = {
	"Smallnumbers.png", g_small_grey_src_tex_loc[0], g_small_grey_src_tex_loc[1], g_small_grey_src_tex_loc[2], g_small_grey_src_tex_loc[3],
	"0123456789.", NULL, 11, 0, 0, false, 0, '.', 18
};
static const glyph_font FONT_SMALL_BLUE = {
	"Smallnumbers.png", g_small_blue_src_tex_loc[0], g_small_blue_src_tex_loc[1], g_small_blue_src_tex_loc[2], g_small_blue_src_tex_loc[3],
	"0123456789.", NULL, 11, 0, 0, false, 0, '.', 18
};
static const glyph_font FONT_MEDIUM = {
	"Smallnumbers.png", g_med_numbers_src_tex_loc[0], g_med_numbers_src_tex_loc[1], g_med_numbers_src_tex_loc[2], g_med_numbers_src_tex_loc[3],
	"0123456789:+-.cf", NULL, 16, 0, 0, false, 0, 0, 0
};
static const glyph_font FONT_LARGE = {
	"Smallnumbers.png", g_large_numbers_src_tex_loc[0], g_large_numbers_src_tex_loc[1], g_large_numbers_src_tex_loc[2], g_large_numbers_src_tex_loc[3],
	"1234567890", NULL, 10, 0, 0, false, 0, 0, 0
};
static const glyph_font FONT_NAV_LARGE = {
	"Navgfx.png", g_nav_letters_large_src_tex_loc[0], g_nav_letters_large_src_tex_loc[1], g_nav_letters_large_src_tex_loc[2], g_nav_letters_large_src_tex_loc[3],
	"abcdefghijklmnopqrstuvwxyz0123456789,.()", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.()", 40, 2, 7, true, 412, 0, 0
};
static const glyph_font FONT_NAV_SMALL = {
	"Navgfx.png", g_nav_letters_small_src_tex_loc[0], g_nav_letters_small_src_tex_loc[1], g_nav_letters_small_src_tex_loc[2], g_nav_letters_small_src_tex_loc[3],
	"abcdefghijklmnopqrstuvwxyz0123456789,.()", "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.()", 40, 2, 7, true, 412, 0, 0
};
static const glyph_font FONT_NAV_DIGITS = {
	"Navgfx.png", g_nav_numbers_src_tex_loc[0], g_nav_numbers_src_tex_loc[1], g_nav_numbers_src_tex_loc[2], g_nav_numbers_src_tex_loc[3],
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

void draw_nav_numbers(int num, int xpos, int ypos) {
	char sz_digit[5];
	snprintf(sz_digit, sizeof(sz_digit), "%d", num);
	draw_nav_digits(sz_digit, xpos, ypos);
}

void draw_nav_symbol(nav_icon sym, int xpos, int ypos)
{
	if (sym < 0 || sym >= NAV_ICON_COUNT) return;

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

	SDL_RenderTexture(renderer, tex("Navgfx.png"), &g_src_rect, &g_dst_rect);
}

void draw_medium_num(int singledigit, int xpos, int ypos) {
	char sz_digit[4];
	snprintf(sz_digit, sizeof(sz_digit), "%d", singledigit);
	draw_medium_string(sz_digit, xpos, ypos);
}

void draw_large_num(int singledigit, int xpos, int ypos) {
	char sz_digit[4];
	snprintf(sz_digit, sizeof(sz_digit), "%d", singledigit);
	draw_large_string(sz_digit, xpos, ypos);
}

void draw_speed_digit(char digit, int position)
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
				SDL_RenderTexture(renderer, tex("Speednumbers.png"), &g_src_rect, &spd_digit_three);
			}

			if (position == 2) {
				spd_digit_two.w = g_speed_src_tex_loc[2][d];
				spd_digit_two.h = g_speed_src_tex_loc[3][d];
				SDL_RenderTexture(renderer, tex("Speednumbers.png"), &g_src_rect, &spd_digit_two);
			}

			if (position == 1) {
				spd_digit_one.w = g_speed_src_tex_loc[2][d];
				spd_digit_one.h = g_speed_src_tex_loc[3][d];
				SDL_RenderTexture(renderer, tex("Speednumbers.png"), &g_src_rect, &spd_digit_one);
			}
		}
	}
}

void draw_speed(const char *speed) {
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

bool render_texture(SDL_Texture *t, int x, int y, int w, int h)
{
	SDL_FRect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	return SDL_RenderTexture(renderer, t, NULL, &dst_rect);
}

bool render_top_icon_grey_texture(int x, int y, int w, int h)
{
	SDL_FRect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	if (dash->oil_pressure_available) {
		return SDL_RenderTexture(renderer, tex("TopiconsgreyOP.png"), NULL, &dst_rect);
	} else {
		return SDL_RenderTexture(renderer, tex("Topiconsgrey.png"), NULL, &dst_rect);
	}
}

bool render_oil_light_texture(int x, int y, int w, int h)
{
	SDL_FRect dst_rect;
	dst_rect.x = x;
	dst_rect.y = y;
	dst_rect.w = w;
	dst_rect.h = h;

	if (dash->oil_pressure_available) {
		return SDL_RenderTexture(renderer, tex("OillightOP.png"), NULL, &dst_rect);
	} else {
		return SDL_RenderTexture(renderer, tex("Oillight.png"), NULL, &dst_rect);
	}
}

/* --- Lifecycle --- */

bool draw_init(void) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "Could not initialise SDL3: %s\n", SDL_GetError());
		return false;
	}

	Uint64 window_flags = 0;
	const char *video_driver = SDL_GetCurrentVideoDriver();
	fprintf(stderr, "Video driver: %s\n", video_driver ? video_driver : "(null)");
	if (video_driver && strcmp(video_driver, "KMSDRM") == 0)
		window_flags = SDL_WINDOW_FULLSCREEN;

	if (!SDL_CreateWindowAndRenderer("TFT Dash", SCREEN_WIDTH, SCREEN_HEIGHT, window_flags, &window, &renderer)) {
		fprintf(stderr, "Could not create window & renderer: %s\n", SDL_GetError());
		return false;
	}

	const char *renderer_name = SDL_GetRendererName(renderer);
	fprintf(stderr, "Renderer: %s\n", renderer_name ? renderer_name : "(null)");

	SDL_HideCursor();

	g_assets = asset_store_create(renderer);
	if (!g_assets) {
		fprintf(stderr, "Failed to create asset store\n");
		return false;
	}

	int total = 0;
	for (int i = 0; i < THEME_COUNT; i++) {
		char dir[256];
		snprintf(dir, sizeof(dir), "assets/themes/%s", THEME_NAMES[i]);
		int n = asset_store_load_theme(g_assets, THEME_NAMES[i], dir);
		if (n < 0)
			fprintf(stderr, "Failed to load theme: %s (dir: %s)\n", THEME_NAMES[i], dir);
		else
			total += n;
	}
	fprintf(stderr, "Loaded %d assets across %d themes\n", total, THEME_COUNT);

	draw_dashboard_init_rects();
	dashboard_anims_init();

	return true;
}

void draw_cleanup(void) {
	asset_store_destroy(g_assets);
	if (renderer) SDL_DestroyRenderer(renderer);
	if (window) SDL_DestroyWindow(window);
	SDL_Quit();
}

void draw_update(const dashboard_state *d, const menu_state *m, const nav_state *n, const tpms_state *t) {
	dash = d;
	menu = m;
	nav = n;
	tpms = t;

	if (dash->theme != currenttheme) {
		currenttheme = dash->theme;
		g_current_theme = theme_name_from_id(dash->theme);
	}

	/* Set initial theme on first frame */
	if (g_current_theme == NULL || currenttheme == 0)
		g_current_theme = theme_name_from_id(dash->theme);
}

void draw_frame(void) {
	/* Pump SDL events */
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_EVENT_QUIT) {
			draw_cleanup();
			exit(0);
		}
	}

	if (dash->choice_state == 0) {
		if (!anim_is_done(&anim_startup))
			dashboard_startup();
		else
			draw_dashboard();
	} else {
		draw_menu(dash->choice_state);
	}

	SDL_Delay(5);
}
