/*
 * draw_menu.c — Menu screen rendering for TFT Dash
 *
 * Renders the menu overlay when the user enters the settings screens.
 * Driven by the data tables in menu.h and the shared draw primitives.
 */

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#include "draw_menu.h"
#include "draw.h"
#include "menu.h"

/* Menu-specific format buffers (sprintf'd each frame from menu/dash state) */
static char str_spc_digit0[16];
static char str_spc_digit1[16];
static char str_spc_digit2[16];
static char str_spc_digit3[16];
static char str_set_time_digit0[16];
static char str_set_time_digit1[16];
static char str_set_time_digit2[16];
static char str_set_time_digit3[16];
static char str_front_sensor_id[16];
static char str_rear_sensor_id[16];
static char str_front_pressure_low[16];
static char str_rear_pressure_low[16];
static char str_light_switch_value[16];
static char str_current_light_level[16];

void draw_menu(int state) {
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
			render_texture(tex("Selecton.png"), 741, fan_option_y[menu->fan_neutral_option], 55, 38);
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
		if (menu->odo_error == 1) render_texture(tex_from("default", "Odoerror1.png"), 130, 524, 758, 45);
		if (menu->odo_error == 2) render_texture(tex_from("default", "Odoerror2.png"), 130, 524, 758, 45);
		break;
	}
	case MENU_SCREEN_SPEED_CORRECTION: {
		strcpy(str_spc_digit0, menu->spc_digit0 == 0 ? "-" : "+");
		snprintf(str_spc_digit1, sizeof(str_spc_digit1), "%d", menu->spc_digit1);
		snprintf(str_spc_digit2, sizeof(str_spc_digit2), "%d", menu->spc_digit2);
		snprintf(str_spc_digit3, sizeof(str_spc_digit3), "%d", menu->spc_digit3);
		draw_medium_string(str_spc_digit0, 294, 282);
		draw_medium_string(str_spc_digit1, 408, 277);
		draw_medium_string(str_spc_digit2, 516, 277);
		draw_medium_string(str_spc_digit3, 693, 277);
		break;
	}
	case MENU_SCREEN_TPMS: {
		snprintf(str_front_sensor_id, sizeof(str_front_sensor_id), "%d", dash->front_sensor_id);
		snprintf(str_rear_sensor_id, sizeof(str_rear_sensor_id), "%d", dash->rear_sensor_id);
		snprintf(str_front_pressure_low, sizeof(str_front_pressure_low), "%d", dash->front_pressure_low);
		snprintf(str_rear_pressure_low, sizeof(str_rear_pressure_low), "%d", dash->rear_pressure_low);
		draw_medium_string(str_front_sensor_id, 527, 170);
		draw_medium_string(str_rear_sensor_id, 832, 170);
		draw_medium_string(str_front_pressure_low, 830, 288);
		draw_medium_string(str_rear_pressure_low, 830, 405);
		break;
	}
	case MENU_SCREEN_CONTROL:
		render_texture(tex("Selectedcontrol.png"), menu->control_layout == 0 ? 230 : 555, 195, 236, 30);
		break;

	case MENU_SCREEN_LIGHT: {
		snprintf(str_light_switch_value, sizeof(str_light_switch_value), "%d", menu->light_switch_value);
		snprintf(str_current_light_level, sizeof(str_current_light_level), "%d", menu->current_light_level);
		draw_medium_string(str_light_switch_value, 625, 477);
		draw_medium_string(str_current_light_level, 625, 396);
		const theme_thumb *day = menu_theme_thumb(menu->day_theme);
		if (day) render_texture(tex_from(day->theme_name, day->thumb_file), 772, 118, 126, 74);
		const theme_thumb *night = menu_theme_thumb(menu->night_theme);
		if (night) render_texture(tex_from(night->theme_name, night->thumb_file), 772, 236, 126, 74);
		break;
	}
	case MENU_SCREEN_SET_UNITS:
		render_texture(tex("Selecton.png"), dash->using_km  ? 894 : 784, 101, 55, 38);
		render_texture(tex("Selecton.png"), dash->using_fh  ? 894 : 784, 262, 55, 38);
		render_texture(tex("Selecton.png"), dash->using_bar ? 894 : 784, 423, 55, 38);
		break;

	case MENU_SCREEN_SET_TIME: {
		snprintf(str_set_time_digit0, sizeof(str_set_time_digit0), "%d", menu->set_time_digit0);
		snprintf(str_set_time_digit1, sizeof(str_set_time_digit1), "%d", menu->set_time_digit1);
		snprintf(str_set_time_digit2, sizeof(str_set_time_digit2), "%d", menu->set_time_digit2);
		snprintf(str_set_time_digit3, sizeof(str_set_time_digit3), "%d", menu->set_time_digit3);
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
