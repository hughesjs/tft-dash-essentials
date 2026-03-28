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
SDL_FRect grevline;
SDL_FRect grrevwhite;
SDL_FPoint gwhitepoint;

/* Display format buffers — initialised in main(), written each frame in draw_dashboard */
char str_trip_time[16];
char str_time[16];

/* Warning flags */
static bool warningbadgeactive = false;

/* Oil pressure lookup tables */
int barohms[11];
int tempnum[6];
int tempohms[6];

/* Navigation display strings */
static char sz_find_parking[] = "FIND PARKING";
static char sz_take_ferry[] = "TAKE THE FERRY";
static char sz_no_nav_data[] = "No Navigation Data";
static char sz_smartphone_app[] = "launch smartphone app";
static char sz_set_destination[] = "and set a destination";
static char sz_arrive_in[] = "arv";
static char sz_mls[] = "mls";
static char sz_km[] = "km";

/* --- Helper functions --- */

static int get_diff(int n1, int n2) {
	if (n1 >= n2) {
		return n1 - n2;
	}

	if (n2 > n1) {
		return n2 - n1;
	}
	return 0;
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
    *litres_out = (21000 - (((double)(1000 / 93)) * ((double)fuelintfloat - 0))) / 1000;
    return 93;
  }

  if (fuelintfloat > 93 && fuelintfloat <= 121) {
    *litres_out = (20000 - (((double)(1000 / 28)) * ((double)fuelintfloat - 93))) / 1000;
    return 28;
  }

  if (fuelintfloat > 121 && fuelintfloat <= 148) {
    *litres_out = (18000 - (((double)(1000 / 27)) * ((double)fuelintfloat - 121))) / 1000;
    return 27;
  }

  if (fuelintfloat > 148 && fuelintfloat <= 180) {
    *litres_out = (17000 - (((double)(1000 / 32)) * ((double)fuelintfloat - 148))) / 1000;
    return 32;
  }

  if (fuelintfloat > 180 && fuelintfloat <= 211) {
    *litres_out = (15000 - (((double)(1000 / 31)) * ((double)fuelintfloat - 180))) / 1000;
    return 31;
  }

  if (fuelintfloat > 211 && fuelintfloat <= 241) {
    *litres_out = (14000 - (((double)(1000 / 30)) * ((double)fuelintfloat - 211))) / 1000;
    return 30;
  }

  if (fuelintfloat > 241 && fuelintfloat <= 281) {
    *litres_out = (12000 - (((double)(1000 / 40)) * ((double)fuelintfloat - 241))) / 1000;
    return 40;
  }

  if (fuelintfloat > 281 && fuelintfloat <= 320) {
    *litres_out = (10000 - (((double)(1000 / 39)) * ((double)fuelintfloat - 281))) / 1000;
    return 39;
  }

  if (fuelintfloat > 320 && fuelintfloat <= 367) {
    *litres_out = (9000 - (((double)(1000 / 47)) * ((double)fuelintfloat - 320))) / 1000;
    return 47;
  }

  if (fuelintfloat > 367 && fuelintfloat <= 426) {
    *litres_out = (7000 - (((double)(1000 / 59)) * ((double)fuelintfloat - 367))) / 1000;
    return 59;
  }

  if (fuelintfloat > 426 && fuelintfloat <= 481) {
    *litres_out = (6000 - (((double)(1000 / 55)) * ((double)fuelintfloat - 426))) / 1000;
    return 55;
  }

  if (fuelintfloat > 481 && fuelintfloat <= 506) {
    *litres_out = (5000 - (((double)(1000 / 25)) * ((double)fuelintfloat - 481))) / 1000;
    return 25;
  }

  if (fuelintfloat > 506) {
    *litres_out = (5000 - (((double)(1000 / 25)) * ((double)fuelintfloat - 481))) / 1000;
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

/* --- Rect initialisation --- */

void draw_dashboard_init_rects(void) {
	grevline.x = 430; grevline.y = 23; grevline.w = 594; grevline.h = 577;
	grrevwhite.x = 382; grrevwhite.y = 0; grrevwhite.w = 642; grrevwhite.h = 623;
	gwhitepoint.x = 594; gwhitepoint.y = 577;

	spd_digit_one.x = 614;
	spd_digit_one.y = 363;

	spd_digit_two.x = 735;
	spd_digit_two.y = 363;

	spd_digit_three.x = 875;
	spd_digit_three.y = 363;

	/* Oil pressure lookup tables */
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

	/* Oil temperature lookup tables */
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
}

/* --- Info screen rendering --- */

static void render_info_screen(const info_screen *screen, int x_offset, bool using_km) {
	for (int i = 0; i < INFO_SCREEN_MAX_TEXTURES; i++) {
		const char *name = (using_km && screen->textures_km[i]) ? screen->textures_km[i] : screen->textures[i];
		if (!name) break;
		SDL_FRect r = screen->rects[i];
		r.x += x_offset;
		render_texture(tex(name), (int)r.x, (int)r.y, (int)r.w, (int)r.h);
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
				SDL_RenderTexture(renderer, tex(REV_REVEAL[i].texture), NULL, &(SDL_FRect){REV_REVEAL[i].rect.x, REV_REVEAL[i].rect.y, REV_REVEAL[i].rect.w, REV_REVEAL[i].rect.h});
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

/* --- Main dashboard rendering --- */

void draw_dashboard(void) {
	/* Format buffers — written each frame */
	char str_coolant_temp[16];
	char str_current_speed[16];
	char str_trip1[16];
	char str_trip2[16];
	char str_odo[16];
	char str_mpg[16];
	char str_rpm[16];
	char str_fuel[16];
	char str_range[16];
	char str_max_speed[16];
	char str_ambient_temp[16];
	char str_batt[16];
	char str_spd_correct[16];
	char str_oil_press[16];
	char str_oil_temp[16];
	char str_front_sensor_id[16];
	char str_rear_sensor_id[16];
	char str_front_pressure_low[16];
	char str_rear_pressure_low[16];
	char str_sensor2_psi[16];
	char str_sensor4_psi[16];
	char str_sensor2_temp[16];
	char str_sensor4_temp[16];
	char str_nav_yards[16];
	char str_nav_miles[16];
	char str_nav_metres[16];
	char str_nav_km[16];
	char str_nav_dest_miles[16];
	char str_nav_dest_km[16];

	/* Derived values */
	double coolant_temp_f = 0;
	double litres_remaining = 0;
	double tempf = 0;

	if (dash->info_mode != info_current_mode && warningbadgeactive)
		warnings_cancel();

	if (dash->theme == 0 || dash->theme == 7) {
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE);
	}

	SDL_RenderClear(renderer);


	/* Rev counter rev line texture */
	SDL_RenderTexture(renderer, tex("Revline.png"), NULL, &grevline);

	anim_set_target(&anim_rpm, get_rpm_rotation(dash->rpm));

	/* White rev counter cover to reveal the rev line */
	SDL_RenderTextureRotated(renderer, tex("Whitesq.png"), NULL, &grrevwhite, anim_frame(&anim_rpm), &gwhitepoint, SDL_FLIP_NONE);

	/* Top grey icons */
	render_top_icon_grey_texture (0, 0, 627, 138);
	render_texture (tex("Topiconedge1.png"), 625, 0, 98, 75);
	render_texture (tex("Topiconedge2.png"), 721, 0, 84, 24);

	/* Trip 1 and 2 and Odometer, Ambient Temp and Time section */
	render_texture (tex("Mileinfo.png"), 0, 434, 435, 169);

	/* Fuel Gauge — flash when low fuel and engine running */
	bool fuel_flash = get_num_fuel_bars(dash->fuel_float) <= 2 && warnings_engine_running();
	if (!fuel_flash || anim_is_reversing(&anim_flash)) {
		render_texture(tex("Fuelgauge.png"), 676, 294, 316, 44);
		render_texture(tex("Fuelgaugewhite.png"), 714 + get_fuel_reveal_from_bars(get_num_fuel_bars(dash->fuel_float)), 303, 274, 28);
	}
	/* Coolant Temp — flash when overheating and engine running */
	bool coolant_flash = dash->coolant_temp >= 120 && warnings_engine_running();
	if (!coolant_flash || anim_is_reversing(&anim_flash)) {
		const char *coolant_tex = dash->using_fh ? "CoolantF.png" : "Coolant.png";
		render_texture(tex(coolant_tex), 808, 196, 209, 80);
	}

	render_texture (tex("Coolanticon.png"), 772, 221, 41, 39);

	/* Top light icons (Neutral, Oil, Indicator, High beam) */

	if (dash->neutral) {
		render_texture (tex("Neutrallight.png"), 0, 0, 130, 136);
	}

	if (!dash->neutral && dash->current_gear != 0) {
		render_texture (tex("Gear.png"), 0, 0, 131, 136);
		draw_large_num (dash->current_gear, 30, 22);
	}

	if (dash->oil_warning) {
		render_oil_light_texture (131, 0, 135, 138);
	}

	if (dash->indicate_right && !dash->indicate_left) {
		render_texture (tex("Indicateright.png"), 267, 0, 135, 136);
	}

	if (dash->indicate_left && !dash->indicate_right) {
		render_texture (tex("Indicateleft.png"), 267, 0, 135, 136);
	}

	if (dash->indicate_left && dash->indicate_right) {
		render_texture (tex("Indicateboth.png"), 267, 0, 135, 136);
	}

	if (dash->high_beam) {
		render_texture (tex("Highbeamlight.png"), 404, 0, 133, 136);
	}

	/* Middle info section — static display or animated transition */
	if (!warningbadgeactive) {
		if (dash->info_mode != info_current_mode) {
			if (!anim_is_active(&anim_info))
				anim_start(&anim_info);

			if (!anim_is_reversing(&anim_info))
				render_info_screen(&INFO_SCREENS[info_current_mode], -anim_frame(&anim_info), dash->using_km);
			else
				render_info_screen(&INFO_SCREENS[dash->info_mode], -anim_frame(&anim_info), dash->using_km);

			if (anim_is_done(&anim_info)) {
				info_current_mode = dash->info_mode;
				anim_stop(&anim_info);
			}
		} else {
			render_info_screen(&INFO_SCREENS[info_current_mode], 0, dash->using_km);
		}
	}

	/* Warning badges */
	const warning_def *w = warnings_active();
	warningbadgeactive = (w != NULL);
	if (w) {
		render_texture(tex(w->badge), 0, 163, 444, 249);
		if (anim_is_reversing(&anim_flash) && w->flash) {
			const char *ft = w->flash(dash, tpms);
			if (ft) render_texture(tex(ft), w->flash_x, w->flash_y, w->flash_w, w->flash_h);
		}
	}

	/* Units - either MPH or KM */
	if (dash->using_km == 1) {
		render_texture (tex("kph.png"), 844, 553, 179, 44);
		render_texture (tex("km.png"), 350, 448, 57, 18);
	} else {
		render_texture (tex("mph.png"), 844, 553, 142, 45);
		render_texture (tex("Miles.png"), 350, 448, 57, 18);
	}


	/* Rev counter numbers */
	for (int i = 0; i < REV_REVEAL_COUNT; i++)
		SDL_RenderTexture(renderer, tex(REV_REVEAL[i].texture), NULL, &(SDL_FRect){REV_REVEAL[i].rect.x, REV_REVEAL[i].rect.y, REV_REVEAL[i].rect.w, REV_REVEAL[i].rect.h});

	snprintf( str_current_speed, sizeof(str_current_speed), "%d", dash->current_speed);
	snprintf( str_trip1, sizeof(str_trip1), "%.1f", dash->trip1);
	snprintf( str_trip2, sizeof(str_trip2), "%.1f", dash->trip2);
	snprintf( str_odo, sizeof(str_odo), "%.0f", dash->odo);

	if (dash->using_fh) {
		tempf = ((double)dash->ambient_temp*1.8) + 32;
		snprintf(str_ambient_temp, sizeof(str_ambient_temp), "%df", (int)tempf);
	} else {
		snprintf(str_ambient_temp, sizeof(str_ambient_temp), "%dc", dash->ambient_temp);
	}

	if (dash->using_fh) {
		coolant_temp_f = ((double)dash->coolant_temp*1.8) + 32;
		snprintf(str_coolant_temp, sizeof(str_coolant_temp), "%d", (int)coolant_temp_f);
	} else {
		snprintf(str_coolant_temp, sizeof(str_coolant_temp), "%d", dash->coolant_temp);
	}

	if (dash->using_km) {
		if (dash->mpg > 0) {
			snprintf(str_mpg, sizeof(str_mpg), "%d", (int)((double)282.481 / (double)dash->mpg));
		} else {
			snprintf(str_mpg, sizeof(str_mpg), "%d", dash->mpg);
		}

	} else {
		snprintf(str_mpg, sizeof(str_mpg), "%d", dash->mpg);
	}

	if (dash->using_km) {
		snprintf(str_range, sizeof(str_range), "%d", (int)((double)dash->range * 1.609));
	} else {
		snprintf(str_range, sizeof(str_range), "%d", dash->range);
	}

	snprintf(str_max_speed, sizeof(str_max_speed), "%d", dash->max_speed);
	snprintf(str_rpm, sizeof(str_rpm), "%d", dash->rpm);

	if (dash->oil_pressure_available) {
		snprintf(str_oil_press, sizeof(str_oil_press), "%.1f", get_precise_bar(dash->oil_pressure_ohms));
		snprintf(str_oil_temp, sizeof(str_oil_temp), "%d", (int)get_precise_temp(dash->oil_temp_ohms));
	}

	if (tpms->front.received) {
		snprintf(str_sensor2_psi, sizeof(str_sensor2_psi), "%.1f", dash->using_bar ? tpms->front.bar : tpms->front.psi);
		if (dash->using_fh)
			snprintf(str_sensor2_temp, sizeof(str_sensor2_temp), "%df", (int)((double)tpms->front.temp_celsius * 1.8 + 32));
		else
			snprintf(str_sensor2_temp, sizeof(str_sensor2_temp), "%dc", tpms->front.temp_celsius);
	} else {
		strcpy(str_sensor2_psi, "..");
		strcpy(str_sensor2_temp, "..");
	}

	if (tpms->rear.received) {
		snprintf(str_sensor4_psi, sizeof(str_sensor4_psi), "%.1f", dash->using_bar ? tpms->rear.bar : tpms->rear.psi);
		if (dash->using_fh)
			snprintf(str_sensor4_temp, sizeof(str_sensor4_temp), "%df", (int)((double)tpms->rear.temp_celsius * 1.8 + 32));
		else
			snprintf(str_sensor4_temp, sizeof(str_sensor4_temp), "%dc", tpms->rear.temp_celsius);
	} else {
		strcpy(str_sensor4_psi, "..");
		strcpy(str_sensor4_temp, "..");
	}


	if (get_litres_remaining(dash->fuel_float, &litres_remaining) != 0) {
		snprintf(str_fuel, sizeof(str_fuel), "%.1f", litres_remaining);
	} else {
		strcpy (str_fuel, "..");
	}

	snprintf(str_batt, sizeof(str_batt), "%.1f", dash->batt);
	snprintf(str_spd_correct, sizeof(str_spd_correct), "%.1f", dash->spd_correct);


	/* Draw the current speed */
	draw_speed(str_current_speed);

	/* Draw some numbers */
	draw_medium_string (str_ambient_temp, 18, 461);
	draw_medium_string (str_coolant_temp, 873, 224);

	/* Middle Info */
	if (dash->info_mode == 0 && (!anim_is_active(&anim_info) || anim_is_done(&anim_info)) && !warningbadgeactive) {
		draw_medium_string (str_mpg, 60, 224);
		draw_medium_string (str_range, 292, 224);
		draw_medium_string (str_trip_time, 17, 332);
		draw_medium_string (str_max_speed, 246, 332);
	}

	if (dash->info_mode == 1 && (!anim_is_active(&anim_info) || anim_is_done(&anim_info)) && !warningbadgeactive) {
		draw_medium_string(str_rpm, 60, 224);
		draw_medium_string(str_fuel, 292, 224);
		draw_medium_string (str_batt, 17, 332);
		if (strlen (str_spd_correct) > 4) {
			draw_medium_string (str_spd_correct, 216, 332);
		} else {
			draw_medium_string (str_spd_correct, 236, 332);
		}
	}

	if (dash->info_mode == 2 && (!anim_is_active(&anim_info) || anim_is_done(&anim_info)) && !warningbadgeactive) {
		draw_medium_string (str_sensor2_psi, 40, 224);
		draw_medium_string (str_sensor4_psi, 304, 224);
		draw_medium_string (str_sensor2_temp, 37, 342);
		draw_medium_string (str_sensor4_temp, 246, 342);
	}

	if (dash->info_mode == 3 && (!anim_is_active(&anim_info) || anim_is_done(&anim_info)) && !warningbadgeactive) {


		if (nav->nav_active == true) {
			snprintf(str_nav_yards, sizeof(str_nav_yards), "%d", nav->nav_yards);
			snprintf(str_nav_miles, sizeof(str_nav_miles), "%.1f", nav->nav_miles);
			snprintf(str_nav_metres, sizeof(str_nav_metres), "%d", (int)((double)nav->nav_yards / 1.094));
			snprintf(str_nav_km, sizeof(str_nav_km), "%.1f", nav->nav_miles * 1.609);
			snprintf(str_nav_dest_miles, sizeof(str_nav_dest_miles), "%.1f", nav->nav_dest_distance);
			snprintf(str_nav_dest_km, sizeof(str_nav_dest_km), "%.1f", nav->nav_dest_distance * 1.609);

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
			render_texture (tex("Indicateleftfar.png"), 2, 186, 250, 98);
		}
	}

	if (dash->indicate_right && !warningbadgeactive) {
		render_texture (tex("Indicaterightfar.png"), 802, 192, 209, 84);
	}


	/* Essential info */
	draw_medium_string(str_time, 14, 543);
	draw_small_blue_string (str_trip1, 252, 460);
	draw_small_blue_string (str_trip2, 246, 509);
	draw_small_grey_string (str_odo, 223, 560);

	if (dash->oil_pressure_available) {
		draw_medium_string(str_oil_press, 163, 32);
		draw_medium_string(str_oil_temp, 163, 89);
	}

	if (tpms->connected) {
		render_texture(tex("tyreicon.png"), 630, 553, 22, 45);
	}

	if (tpms->connected && tpms->signal_active) {
		render_texture(tex("tyresignal.png"), 653, 559, 33, 34);
	}

	SDL_RenderPresent(renderer);
}
