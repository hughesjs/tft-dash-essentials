/*
 * draw_dashboard.c — Dashboard rendering module for TFT Dash
 *
 * Contains the main dashboard drawing function, startup animation,
 * warning badge logic, and all helper functions/state used exclusively
 * by dashboard rendering (fuel calculations, RPM lookup, temperature
 * interpolation, etc.).
 */

#include "draw_dashboard.h"
#include "draw.h"
#include "dashboard_anims.h"
#include "warnings.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Rev counter rects */
static const SDL_FRect grevline = { 430, 23, 594, 577 };
static const SDL_FRect grrevwhite = { 382, 0, 642, 623 };
static const SDL_FPoint gwhitepoint = { 594, 577 };

/* Display format buffers */
char str_trip_time[16] = "00:00";
char str_time[16] = "14:35";

/* Warning flags */
static bool warningbadgeactive = false;

/* Oil pressure lookup tables */
static const int barohms[11] = { 6, 23, 39, 54, 69, 89, 106, 121, 140, 150, 160 };
static const int tempnum[6] = { 50, 70, 80, 95, 120, 150 };
static const int tempohms[6] = { 680, 310, 230, 140, 75, 40 };

/* Navigation display strings */
static char sz_find_parking[] = "FIND PARKING";
static char sz_take_ferry[] = "TAKE THE FERRY";
static char sz_no_nav_data[] = "No Navigation Data";
static char sz_smartphone_app[] = "launch smartphone app";
static char sz_set_destination[] = "and set a destination";
static char sz_arrive_in[] = "arv";
static char sz_mls[] = "mls";
static char sz_km[] = "km";

/* --- Formatted string struct --- */

typedef struct {
	char speed[16], trip1[16], trip2[16], odo[16];
	char ambient_temp[16], coolant_temp[16], mpg[16], rpm[16];
	char fuel[16], range[16], max_speed[16], batt[16], spd_correct[16];
	char oil_press[16], oil_temp[16];
	char front_psi[16], front_temp[16], rear_psi[16], rear_temp[16];
	char nav_yards[16], nav_miles[16], nav_metres[16], nav_km[16];
	char nav_dest_miles[16], nav_dest_km[16];
} dash_strings;

/* --- Helper functions --- */

static int get_diff(int n1, int n2) {
	if (n1 >= n2) return n1 - n2;
	return n2 - n1;
}

double get_precise_bar(int ohms) {
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

double get_precise_temp(int ohms) {
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

	if (ohms > tempohms[closesttemp]) {
		wanted_diff = tempohms[closesttemp-1] - tempohms[closesttemp];
		numdiff = tempnum[closesttemp] - tempnum[closesttemp-1];
		valuefrommintemp = ohms - tempohms[closesttemp];
		tenthstemp = (double)valuefrommintemp / (double)wanted_diff;

		return (double)tempnum[closesttemp] - (tenthstemp * numdiff);
	}

	if (ohms < tempohms[closesttemp]) {
		wanted_diff = tempohms[closesttemp] - tempohms[closesttemp+1];
		numdiff = tempnum[closesttemp+1] - tempnum[closesttemp];
		valuefrommintemp = ohms - tempohms[closesttemp+1];
		tenthstemp = (double)valuefrommintemp / (double)wanted_diff;

		return (double)tempnum[closesttemp+1] - (tenthstemp * numdiff);
	}

	if (ohms == tempohms[closesttemp]) {
		return (double)tempnum[closesttemp];
	}

	return 0.0;
}

static int get_litres_remaining(int fuelintfloat, double *litres_out) {

  if (fuelintfloat >= 0 && fuelintfloat <= 93) {
    *litres_out = (21000 - (((1000.0 / 93.0)) * ((double)fuelintfloat - 0))) / 1000;
    return 93;
  }

  if (fuelintfloat > 93 && fuelintfloat <= 121) {
    *litres_out = (20000 - (((1000.0 / 28.0)) * ((double)fuelintfloat - 93))) / 1000;
    return 28;
  }

  if (fuelintfloat > 121 && fuelintfloat <= 148) {
    *litres_out = (18000 - (((1000.0 / 27.0)) * ((double)fuelintfloat - 121))) / 1000;
    return 27;
  }

  if (fuelintfloat > 148 && fuelintfloat <= 180) {
    *litres_out = (17000 - (((1000.0 / 32.0)) * ((double)fuelintfloat - 148))) / 1000;
    return 32;
  }

  if (fuelintfloat > 180 && fuelintfloat <= 211) {
    *litres_out = (15000 - (((1000.0 / 31.0)) * ((double)fuelintfloat - 180))) / 1000;
    return 31;
  }

  if (fuelintfloat > 211 && fuelintfloat <= 241) {
    *litres_out = (14000 - (((1000.0 / 30.0)) * ((double)fuelintfloat - 211))) / 1000;
    return 30;
  }

  if (fuelintfloat > 241 && fuelintfloat <= 281) {
    *litres_out = (12000 - (((1000.0 / 40.0)) * ((double)fuelintfloat - 241))) / 1000;
    return 40;
  }

  if (fuelintfloat > 281 && fuelintfloat <= 320) {
    *litres_out = (10000 - (((1000.0 / 39.0)) * ((double)fuelintfloat - 281))) / 1000;
    return 39;
  }

  if (fuelintfloat > 320 && fuelintfloat <= 367) {
    *litres_out = (9000 - (((1000.0 / 47.0)) * ((double)fuelintfloat - 320))) / 1000;
    return 47;
  }

  if (fuelintfloat > 367 && fuelintfloat <= 426) {
    *litres_out = (7000 - (((1000.0 / 59.0)) * ((double)fuelintfloat - 367))) / 1000;
    return 59;
  }

  if (fuelintfloat > 426 && fuelintfloat <= 481) {
    *litres_out = (6000 - (((1000.0 / 55.0)) * ((double)fuelintfloat - 426))) / 1000;
    return 55;
  }

  if (fuelintfloat > 481 && fuelintfloat <= 506) {
    *litres_out = (5000 - (((1000.0 / 25.0)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  if (fuelintfloat > 506) {
    *litres_out = (5000 - (((1000.0 / 25.0)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  return 0;
}

int get_fuel_reveal_from_bars(int bars) {
	return 17 * bars;
}

int get_num_fuel_bars(int value) {
	if (value >= 481) {
		return 2;
	}

	if (value >= 426 && value < 481) {
		return 3;
	}

	if (value >= 367 && value < 426) {
		return 5;
	}

	if (value >= 320 && value < 367) {
		return 6;
	}

	if (value >= 281 && value < 320) {
		return 8;
	}

	if (value >= 241 && value < 281) {
		return 9;
	}

	if (value >= 211 && value < 241) {
		return 11;
	}

	if (value >= 180 && value < 211) {
		return 12;
	}

	if (value >= 148 && value < 180) {
		return 13;
	}

	if (value >= 121 && value < 148) {
		return 14;
	}

	if (value >= 93 && value < 121) {
		return 15;
	}

	if (value >= 0 && value < 93) {
		return 16;
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

int get_rpm_rotation(int rpm) {
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

	return g_rpm_lookup[closest][1];
}


/* --- Info screen rendering --- */

static void render_info_screen(const info_screen *screen, int x_offset, bool using_km) {
	for (int i = 0; i < INFO_SCREEN_MAX_TEXTURES; i++) {
		const char *name = (using_km && screen->textures_km[i]) ? screen->textures_km[i] : screen->textures[i];
		if (!name) break;
		render_texture(tex(name), (int)screen->x[i] + x_offset, (int)screen->y[i], (int)screen->w[i], (int)screen->h[i]);
	}
}


/* --- Startup animation --- */

void dashboard_startup(void) {
	static int spin_angle = 0;
	int f = anim_frame(&anim_startup);

	/* Phase 1: Background fade from black to white (all themes — invisible on dark ones) */
	int brightness = f < 255 ? f : 255;
	SDL_SetRenderDrawColor(renderer, brightness, brightness, brightness, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	/* Phase 2: UI panels slide in from the left (frame 255-1000) */
	if (f > 255) {
		int slide = f < 1000 ? f - 1000 : 0;

		render_top_icon_grey_texture(slide, 0, 627, 138);
		render_texture(tex("Topiconedge1.png"), 625 + slide, 0, 98, 75);
		render_texture(tex("Topiconedge2.png"), 721 + slide, 0, 84, 24);
		render_texture(tex("Mileinfo.png"), slide, 434, 435, 169);

		if (dash->info_mode == 0)
			render_info_screen(&INFO_SCREENS[0], slide, dash->using_km);
	}

	/* Phase 3: Rev counter numbers appear at staggered thresholds */
	if (f > 255) {
		for (int i = 0; i < REV_REVEAL_COUNT; i++) {
			if (f > REV_REVEAL[i].threshold)
				SDL_RenderTexture(renderer, tex(REV_REVEAL[i].texture), NULL, &(SDL_FRect){REV_REVEAL[i].x, REV_REVEAL[i].y, REV_REVEAL[i].w, REV_REVEAL[i].h});
		}
	}

	/* Phase 4: Fuel gauge and coolant slide in from the right (frame 500-1000) */
	if (f > 500) {
		int rslide = f < 1000 ? 1000 - f : 0;

		render_texture(tex("Fuelgauge.png"), 676 + rslide, 294, 316, 44);
		render_texture(tex("Fuelgaugewhite.png"), 714 + (spin_angle * 3) + rslide, 303, 274, 28);

		const char *coolant_tex = dash->using_fh ? "CoolantF.png" : "Coolant.png";
		render_texture(tex(coolant_tex), 808 + rslide, 196, 209, 80);
		render_texture(tex("Coolanticon.png"), 772 + rslide, 221, 41, 39);
	}

	SDL_RenderPresent(renderer);
}

/* --- Dashboard predicates --- */

static bool is_light_theme(void) {
	return dash->theme == 0 || dash->theme == 7;
}

static bool is_fuel_low(void) {
	return get_num_fuel_bars(dash->fuel_float) <= 2 && warnings_engine_running();
}

static bool is_coolant_overheating(void) {
	return dash->coolant_temp >= 120 && warnings_engine_running();
}

static bool info_transition_idle(void) {
	return !anim_is_active(&anim_info) || anim_is_done(&anim_info);
}

/* --- Dashboard sub-functions --- */

static void format_dashboard_strings(dash_strings *s) {
	double coolant_temp_f = 0;
	double litres_remaining = 0;
	double tempf = 0;

	snprintf(s->speed, sizeof(s->speed), "%d", dash->current_speed);
	snprintf(s->trip1, sizeof(s->trip1), "%.1f", dash->trip1);
	snprintf(s->trip2, sizeof(s->trip2), "%.1f", dash->trip2);
	snprintf(s->odo, sizeof(s->odo), "%.0f", dash->odo);

	if (dash->using_fh) {
		tempf = ((double)dash->ambient_temp*1.8) + 32;
		snprintf(s->ambient_temp, sizeof(s->ambient_temp), "%df", (int)tempf);
	} else {
		snprintf(s->ambient_temp, sizeof(s->ambient_temp), "%dc", dash->ambient_temp);
	}

	if (dash->using_fh) {
		coolant_temp_f = ((double)dash->coolant_temp*1.8) + 32;
		snprintf(s->coolant_temp, sizeof(s->coolant_temp), "%d", (int)coolant_temp_f);
	} else {
		snprintf(s->coolant_temp, sizeof(s->coolant_temp), "%d", dash->coolant_temp);
	}

	if (dash->using_km) {
		if (dash->mpg > 0) {
			snprintf(s->mpg, sizeof(s->mpg), "%d", (int)((double)282.481 / (double)dash->mpg));
		} else {
			snprintf(s->mpg, sizeof(s->mpg), "%d", dash->mpg);
		}

	} else {
		snprintf(s->mpg, sizeof(s->mpg), "%d", dash->mpg);
	}

	if (dash->using_km) {
		snprintf(s->range, sizeof(s->range), "%d", (int)((double)dash->range * 1.609));
	} else {
		snprintf(s->range, sizeof(s->range), "%d", dash->range);
	}

	snprintf(s->max_speed, sizeof(s->max_speed), "%d", dash->max_speed);
	snprintf(s->rpm, sizeof(s->rpm), "%d", dash->rpm);

	if (dash->oil_pressure_available) {
		snprintf(s->oil_press, sizeof(s->oil_press), "%.1f", get_precise_bar(dash->oil_pressure_ohms));
		snprintf(s->oil_temp, sizeof(s->oil_temp), "%d", (int)get_precise_temp(dash->oil_temp_ohms));
	}

	if (tpms->front.received) {
		snprintf(s->front_psi, sizeof(s->front_psi), "%.1f", dash->using_bar ? tpms->front.bar : tpms->front.psi);
		if (dash->using_fh)
			snprintf(s->front_temp, sizeof(s->front_temp), "%df", (int)((double)tpms->front.temp_celsius * 1.8 + 32));
		else
			snprintf(s->front_temp, sizeof(s->front_temp), "%dc", tpms->front.temp_celsius);
	} else {
		strcpy(s->front_psi, "..");
		strcpy(s->front_temp, "..");
	}

	if (tpms->rear.received) {
		snprintf(s->rear_psi, sizeof(s->rear_psi), "%.1f", dash->using_bar ? tpms->rear.bar : tpms->rear.psi);
		if (dash->using_fh)
			snprintf(s->rear_temp, sizeof(s->rear_temp), "%df", (int)((double)tpms->rear.temp_celsius * 1.8 + 32));
		else
			snprintf(s->rear_temp, sizeof(s->rear_temp), "%dc", tpms->rear.temp_celsius);
	} else {
		strcpy(s->rear_psi, "..");
		strcpy(s->rear_temp, "..");
	}

	if (get_litres_remaining(dash->fuel_float, &litres_remaining) != 0) {
		snprintf(s->fuel, sizeof(s->fuel), "%.1f", litres_remaining);
	} else {
		strcpy(s->fuel, "..");
	}

	snprintf(s->batt, sizeof(s->batt), "%.1f", dash->batt);
	snprintf(s->spd_correct, sizeof(s->spd_correct), "%.1f", dash->spd_correct);
}

static void clear_background(void) {
	if (is_light_theme()) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	}
	SDL_RenderClear(renderer);
}

static void draw_rev_counter(void) {
	SDL_RenderTexture(renderer, tex("Revline.png"), NULL, &grevline);
	anim_set_target(&anim_rpm, get_rpm_rotation(dash->rpm));
	SDL_RenderTextureRotated(renderer, tex("Whitesq.png"), NULL, &grrevwhite, anim_frame(&anim_rpm), &gwhitepoint, SDL_FLIP_NONE);

	for (int i = 0; i < REV_REVEAL_COUNT; i++)
		SDL_RenderTexture(renderer, tex(REV_REVEAL[i].texture), NULL, &(SDL_FRect){REV_REVEAL[i].x, REV_REVEAL[i].y, REV_REVEAL[i].w, REV_REVEAL[i].h});
}

static void draw_top_bar(void) {
	render_top_icon_grey_texture(0, 0, 627, 138);
	render_texture(tex("Topiconedge1.png"), 625, 0, 98, 75);
	render_texture(tex("Topiconedge2.png"), 721, 0, 84, 24);
	render_texture(tex("Mileinfo.png"), 0, 434, 435, 169);
}

static void draw_gauges(void) {
	if (!is_fuel_low() || anim_is_reversing(&anim_flash)) {
		render_texture(tex("Fuelgauge.png"), 676, 294, 316, 44);
		render_texture(tex("Fuelgaugewhite.png"), 714 + get_fuel_reveal_from_bars(get_num_fuel_bars(dash->fuel_float)), 303, 274, 28);
	}

	if (!is_coolant_overheating() || anim_is_reversing(&anim_flash)) {
		const char *coolant_tex = dash->using_fh ? "CoolantF.png" : "Coolant.png";
		render_texture(tex(coolant_tex), 808, 196, 209, 80);
	}

	render_texture(tex("Coolanticon.png"), 772, 221, 41, 39);
}

static void draw_status_icons(void) {
	if (dash->neutral) {
		render_texture(tex("Neutrallight.png"), 0, 0, 130, 136);
	}

	if (!dash->neutral && dash->current_gear != 0) {
		render_texture(tex("Gear.png"), 0, 0, 131, 136);
		draw_large_num(dash->current_gear, 30, 22);
	}

	if (dash->oil_warning) {
		render_oil_light_texture(131, 0, 135, 138);
	}

	if (dash->indicate_right && !dash->indicate_left) {
		render_texture(tex("Indicateright.png"), 267, 0, 135, 136);
	}

	if (dash->indicate_left && !dash->indicate_right) {
		render_texture(tex("Indicateleft.png"), 267, 0, 135, 136);
	}

	if (dash->indicate_left && dash->indicate_right) {
		render_texture(tex("Indicateboth.png"), 267, 0, 135, 136);
	}

	if (dash->high_beam) {
		render_texture(tex("Highbeamlight.png"), 404, 0, 133, 136);
	}
}

static void draw_info_transition(void) {
	if (warningbadgeactive) return;

	if (dash->info_mode != dashboard_anims_info_mode()) {
		if (!anim_is_active(&anim_info))
			anim_start(&anim_info);

		if (!anim_is_reversing(&anim_info))
			render_info_screen(&INFO_SCREENS[dashboard_anims_info_mode()], -anim_frame(&anim_info), dash->using_km);
		else
			render_info_screen(&INFO_SCREENS[dash->info_mode], -anim_frame(&anim_info), dash->using_km);

		if (anim_is_done(&anim_info)) {
			dashboard_anims_set_info_mode(dash->info_mode);
			anim_stop(&anim_info);
		}
	} else {
		render_info_screen(&INFO_SCREENS[dashboard_anims_info_mode()], 0, dash->using_km);
	}
}

static void draw_info_content(const dash_strings *s) {
	if (!info_transition_idle() || warningbadgeactive) return;

	if (dash->info_mode == 0) {
		draw_medium_string(s->mpg, 60, 224);
		draw_medium_string(s->range, 292, 224);
		draw_medium_string(str_trip_time, 17, 332);
		draw_medium_string(s->max_speed, 246, 332);
	}

	if (dash->info_mode == 1) {
		draw_medium_string(s->rpm, 60, 224);
		draw_medium_string(s->fuel, 292, 224);
		draw_medium_string(s->batt, 17, 332);
		if (strlen(s->spd_correct) > 4) {
			draw_medium_string(s->spd_correct, 216, 332);
		} else {
			draw_medium_string(s->spd_correct, 236, 332);
		}
	}

	if (dash->info_mode == 2) {
		draw_medium_string(s->front_psi, 40, 224);
		draw_medium_string(s->rear_psi, 304, 224);
		draw_medium_string(s->front_temp, 37, 342);
		draw_medium_string(s->rear_temp, 246, 342);
	}

	if (dash->info_mode == 3) {
		if (nav->nav_active == true) {
			/* Nav strings are formatted on demand since they're only needed in mode 3 */
			char str_nav_yards[16], str_nav_miles[16], str_nav_metres[16], str_nav_km[16];
			char str_nav_dest_miles[16], str_nav_dest_km[16];

			snprintf(str_nav_yards, sizeof(str_nav_yards), "%d", nav->nav_yards);
			snprintf(str_nav_miles, sizeof(str_nav_miles), "%.1f", nav->nav_miles);
			snprintf(str_nav_metres, sizeof(str_nav_metres), "%d", (int)((double)nav->nav_yards / 1.094));
			snprintf(str_nav_km, sizeof(str_nav_km), "%.1f", nav->nav_miles * 1.609);
			snprintf(str_nav_dest_miles, sizeof(str_nav_dest_miles), "%.1f", nav->nav_dest_distance);
			snprintf(str_nav_dest_km, sizeof(str_nav_dest_km), "%.1f", nav->nav_dest_distance * 1.609);

			if (strlen(nav->nav_road) > 0) {
				draw_nav_symbol(NAV_ICON_ONTO, 13, 350);
			}

			if (strlen(nav->nav_road) > 0 && strlen(nav->nav_road) <= 14) {
				draw_nav_large_string(nav->nav_road, 69, 347);
			} else {
				draw_nav_small_string(nav->nav_road, 69, 347);
			}

			if (strlen(nav->nav_towards) > 0) {
				draw_nav_symbol(NAV_ICON_TWRDS, 13, 391);
			}

			if (strlen(nav->nav_towards) > 0 && strlen(nav->nav_towards) <= 16) {
				draw_nav_large_string(nav->nav_towards, 99, 386);
			} else {
				draw_nav_small_string(nav->nav_towards, 99, 386);
			}

			draw_nav_small_string(sz_arrive_in, 14, 175);

			if (dash->using_km) {
				draw_nav_large_string(str_nav_dest_km, 77, 171);
				draw_nav_small_string(sz_km, 165, 175);
			} else {
				draw_nav_large_string(str_nav_dest_miles, 77, 171);
				draw_nav_small_string(sz_mls, 165, 175);
			}

			if (nav->nav_yards <= 300) {
				if (dash->using_km) {
					draw_nav_digits(str_nav_metres, 16, 211);
					draw_nav_symbol(NAV_ICON_METRE, 22, 308);
				} else {
					draw_nav_digits(str_nav_yards, 16, 211);
					draw_nav_symbol(NAV_ICON_YARD, 22, 308);
				}
			} else {
				if (dash->using_km) {
					draw_nav_digits(str_nav_km, 16, 211);
					draw_nav_symbol(NAV_ICON_KM, 22, 308);
				} else {
					draw_nav_digits(str_nav_miles, 16, 211);
					draw_nav_symbol(NAV_ICON_MILE, 22, 308);
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
			draw_nav_large_string(sz_no_nav_data, 40, 265);
			draw_nav_small_string(sz_smartphone_app, 40, 350);
			draw_nav_small_string(sz_set_destination, 56, 380);
		}
	}
}

static void draw_warning_badges(void) {
	const warning_def *w = warnings_active();
	warningbadgeactive = (w != NULL);
	if (w) {
		render_texture(tex(w->badge), 0, 163, 444, 249);
		if (anim_is_reversing(&anim_flash) && w->flash) {
			const char *ft = w->flash(dash, tpms);
			if (ft) render_texture(tex(ft), w->flash_x, w->flash_y, w->flash_w, w->flash_h);
		}
	}
}

static void draw_units(void) {
	if (dash->using_km == 1) {
		render_texture(tex("kph.png"), 844, 553, 179, 44);
		render_texture(tex("km.png"), 350, 448, 57, 18);
	} else {
		render_texture(tex("mph.png"), 844, 553, 142, 45);
		render_texture(tex("Miles.png"), 350, 448, 57, 18);
	}
}

static void draw_essential_info(const dash_strings *s) {
	draw_speed(s->speed);
	draw_medium_string(s->ambient_temp, 18, 461);
	draw_medium_string(s->coolant_temp, 873, 224);

	draw_medium_string(str_time, 14, 543);
	draw_small_blue_string(s->trip1, 252, 460);
	draw_small_blue_string(s->trip2, 246, 509);
	draw_small_grey_string(s->odo, 223, 560);

	if (dash->oil_pressure_available) {
		draw_medium_string(s->oil_press, 163, 32);
		draw_medium_string(s->oil_temp, 163, 89);
	}

	if (tpms->connected) {
		render_texture(tex("tyreicon.png"), 630, 553, 22, 45);
	}

	if (tpms->connected && tpms->signal_active) {
		render_texture(tex("tyresignal.png"), 653, 559, 33, 34);
	}
}

static void draw_far_indicators(void) {
	if (dash->indicate_left && !warningbadgeactive) {
		if (dash->info_mode == 0 || dash->info_mode == 1) {
			render_texture(tex("Indicateleftfar.png"), 2, 186, 250, 98);
		}
	}

	if (dash->indicate_right && !warningbadgeactive) {
		render_texture(tex("Indicaterightfar.png"), 802, 192, 209, 84);
	}
}

/* --- Main dashboard rendering --- */

void draw_dashboard(void) {
	if (dash->info_mode != dashboard_anims_info_mode() && warningbadgeactive)
		warnings_cancel();

	dash_strings s;
	format_dashboard_strings(&s);
	clear_background();
	draw_rev_counter();
	draw_top_bar();
	draw_gauges();
	draw_status_icons();
	draw_info_transition();
	draw_info_content(&s);
	draw_warning_badges();
	draw_units();
	draw_essential_info(&s);
	draw_far_indicators();
	SDL_RenderPresent(renderer);
}
