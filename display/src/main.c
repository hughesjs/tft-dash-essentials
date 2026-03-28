/*
 * main.c — TFT Dash entry point
 *
 * SDL initialisation, asset loading, event loop, and frame dispatch.
 * All rendering is delegated to draw_dashboard and draw_menu modules.
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "parser.h"
#include "assets.h"
#include "sensor_feed.h"
#include "tpms_feed.h"
#include "animation.h"
#include "dashboard_anims.h"
#include "draw.h"
#include "draw_dashboard.h"
#include "draw_menu.h"
#include "warnings.h"

static sensor_feed *feed = NULL;
static SDL_Window *window = NULL;
static bool quit = false;
static int currenttheme = 0;

static void init_feeds(void) {
	feed = sensor_feed_create(NULL);
	sensor_feed_start(feed);
	dash = sensor_feed_dashboard(feed);
	menu = sensor_feed_menu(feed);
	nav = sensor_feed_nav(feed);

	tpms_feed *tpms_fd = tpms_feed_create(TPMS_MODEL_EBAY, 2, 4, NULL);
	tpms_feed_start(tpms_fd);
	tpms = tpms_feed_state(tpms_fd);
}

static bool init_sdl(void) {
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
	return true;
}

static bool init_assets(void) {
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

	g_current_theme = theme_name_from_id(dash->theme);
	return true;
}

static void refresh_state(void) {
	dash = sensor_feed_dashboard(feed);
	menu = sensor_feed_menu(feed);
	nav = sensor_feed_nav(feed);

	if (dash->theme != currenttheme) {
		currenttheme = dash->theme;
		g_current_theme = theme_name_from_id(dash->theme);
	}
}

static void handle_events(void) {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_EVENT_QUIT)
			quit = true;
		if (e.type == SDL_EVENT_KEY_UP && e.key.scancode == 41)
			quit = true;
	}
}

static void render_frame(void) {
	if (dash->choice_state == 0) {
		if (!anim_is_done(&anim_startup))
			dashboard_startup();
		else
			draw_dashboard();
	} else {
		draw_menu(dash->choice_state);
	}
}

static void update_warnings(void) {
	warnings_update(dash, tpms, sensor_feed_ms_since_update(feed));
}

static void cleanup(void) {
	asset_store_destroy(g_assets);
	if (renderer) SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

int main(int argc, char *args[]) {
	strcpy(str_trip_time, "00:00");
	strcpy(str_time, "14:35");

	init_feeds();

	if (!init_sdl()) return 1;
	if (!init_assets()) return 1;

	draw_dashboard_init_rects();
	dashboard_anims_init();

	while (!quit) {
		anim_tick_all();
		refresh_state();
		handle_events();
		render_frame();
		update_warnings();
		SDL_Delay(5);
	}

	cleanup();
	return 0;
}
