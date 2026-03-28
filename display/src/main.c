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
#include <stdbool.h>
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
#include "tpms_feed.h"
#include "dashboard_anims.h"
#include "draw.h"
#include "draw_dashboard.h"
#include "draw_menu.h"

static sensor_feed *feed = NULL;

SDL_Window* window = NULL;

int test_counter = 0; // Number to use for test scenarios

int currenttheme = 0; // Current theme we are on
bool quit = false;
bool reverse = false;
int ncount = 0;

// Animation values
int g_down_arrow_x  = 0;
int g_up_arrow_x  = 0;

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

int main(int argc, char* args[]) {


	strcpy (str_trip_time, "00:00");
	strcpy(str_time, "14:35");

	//std::thread t1(thread_worker, "hello");
	SDL_HideCursor();

	feed = sensor_feed_create(NULL);
	sensor_feed_start(feed);
	dash = sensor_feed_dashboard(feed);
	menu = sensor_feed_menu(feed);
	nav = sensor_feed_nav(feed);
	printf("\n Thread created successfully\n");

	tpms_feed *tpms_fd = tpms_feed_create(TPMS_MODEL_EBAY, 2, 4, NULL);
	tpms_feed_start(tpms_fd);
	tpms = tpms_feed_state(tpms_fd);
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

	if (window == NULL) {
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

	draw_dashboard_init_rects();
	dashboard_anims_init();

	SDL_HideCursor();

	////////////////////////////////////////  LOOP ///////////////////////////////////////////
	SDL_Event e;

	while (!quit) {
		anim_tick_all();

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
			if (!anim_is_done(&anim_startup)) {
				dashboard_startup();
			} else {
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

		int ms_since = sensor_feed_ms_since_update(feed);
		comms_stale = (ms_since > 1000) || (ms_since == -1);

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
