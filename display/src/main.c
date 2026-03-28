/*
 * main.c — TFT Dash entry point
 *
 * Creates sensor feeds, initialises the rendering system, then runs
 * the main loop. No SDL dependency — all rendering is owned by draw.h.
 */

#include "sensor_feed.h"
#include "tpms_feed.h"
#include "animation.h"
#include "warnings.h"
#include "draw.h"

static sensor_feed *feed = NULL;
static tpms_feed *tpms_fd = NULL;

static void init_feeds(void) {
    feed = sensor_feed_create(NULL);
    sensor_feed_start(feed);

    tpms_fd = tpms_feed_create(TPMS_MODEL_EBAY, 2, 4, NULL);
    tpms_feed_start(tpms_fd);
}

int main(int argc, char *args[]) {
    init_feeds();

    if (!draw_init()) return 1;

    for (;;) {
        anim_tick_all();
        draw_update(
            sensor_feed_dashboard(feed),
            sensor_feed_menu(feed),
            sensor_feed_nav(feed),
            tpms_feed_state(tpms_fd)
        );
        warnings_update(
            sensor_feed_dashboard(feed),
            tpms_feed_state(tpms_fd),
            sensor_feed_ms_since_update(feed)
        );
        draw_frame();
    }
}
