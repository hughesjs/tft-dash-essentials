/*
 * sensor_feed.c — Bike sensor data feed for TFT Dash
 *
 * Background thread reads serial/pipe data from the firmware, frames
 * messages by { and [ markers, parses them via the parser module, and
 * stores the result in structs accessible through const pointers.
 *
 * Threading note: the render thread reads state without a mutex. This is
 * safe because (a) there is exactly one writer and one reader, (b) each
 * field is a naturally-aligned 32-bit int/float — atomic on ARM, and
 * (c) the worst case is a single render frame mixing fields from two
 * consecutive serial updates, which is imperceptible at 60 fps.
 */

#define _GNU_SOURCE
#include "sensor_feed.h"
#include "parser.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>

#define FRAME_BUF_SIZE 1024
#define READ_BUF_SIZE  1024

/* Default paths to search for a connection (in priority order) */
static const char *DEFAULT_PATHS[] = {
    "/tmp/tft-dash-pipe",
    "/dev/cu.usbserial-1430",
    "/dev/cu.usbmodem14301",
    "/dev/cu.usbmodem1101",
    "/dev/ttyACM0",
    "/dev/ttyACM1",
    NULL
};

struct sensor_feed {
    /* Parsed state — written by background thread, read by consumer via const pointers */
    dashboard_state dashboard;
    menu_state menu;
    nav_state nav;

    /* Connection */
    int fd;
    bool connected;
    bool is_pipe;           /* true if connected to a pipe (skip termios) */
    char path[256];         /* configured path, or empty = search defaults */

    /* Thread control */
    bool running;
    pthread_t thread;
    struct timespec last_live_update;

    /* Framing buffers (only touched by background thread) */
    char frame_buf[FRAME_BUF_SIZE];
    int frame_pos;
};

static sensor_feed instance;

/* --- Static helpers --- */

static bool path_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

static bool is_fifo(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISFIFO(st.st_mode);
}

static void configure_serial(int fd) {
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) return;

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;

    tty.c_cc[VTIME] = 10;  /* 1 second timeout */
    tty.c_cc[VMIN] = 0;

    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tcsetattr(fd, TCSANOW, &tty);
}

static int try_connect(sensor_feed *feed, const char *path) {
    if (!path_exists(path)) return -1;

    bool fifo = is_fifo(path);
    int fd = open(path, fifo ? O_RDONLY : O_RDWR);
    if (fd < 0) return -1;

    if (!fifo) configure_serial(fd);

    feed->fd = fd;
    feed->connected = true;
    feed->is_pipe = fifo;
    fprintf(stderr, "sensor_feed: connected to %s%s\n", path, fifo ? " (pipe)" : "");
    return 0;
}

static int connect_feed(sensor_feed *feed) {
    feed->connected = false;
    feed->fd = -1;

    if (feed->path[0]) {
        return try_connect(feed, feed->path);
    }

    for (int i = 0; DEFAULT_PATHS[i]; i++) {
        if (try_connect(feed, DEFAULT_PATHS[i]) == 0) return 0;
    }

    return -1;
}

static void process_frame(sensor_feed *feed) {
    char *buf = feed->frame_buf;
    size_t len = (size_t)feed->frame_pos;

    if (buf[0] == '{' && len > 90 && len < 400) {
        /* Live data message — strip frame marker, parse content */
        dashboard_state state = {0};
        if (parse_live_message(buf + 1, &state)) {
            feed->dashboard = state;
            clock_gettime(CLOCK_MONOTONIC, &feed->last_live_update);

            /* If nav string present, parse it too */
            if (strlen(state.nav_string) > 0) {
                nav_state ns = {0};
                if (parse_nav_message(state.nav_string, &ns)) {
                    feed->nav = ns;
                }
            }
        }
    } else if (buf[0] == '[' && len > 78 && len < 150) {
        /* Menu data message */
        menu_state state = {0};
        if (parse_menu_message(buf + 1, &state)) {
            feed->menu = state;
        }
    }
}

/* --- Background thread --- */

static void *poll_thread(void *arg) {
    sensor_feed *feed = (sensor_feed *)arg;
    char read_buf[READ_BUF_SIZE];

    while (feed->running && connect_feed(feed) != 0) {
        usleep(500000);
    }
    if (!feed->running) return NULL;

    while (feed->running) {
        memset(read_buf, 0, sizeof(read_buf));
        int n = read(feed->fd, read_buf, sizeof(read_buf));

        if (n <= 0) {
            /* Disconnected or error — try to reconnect */
            close(feed->fd);
            feed->fd = -1;
            feed->connected = false;

            if (!feed->running) break;

            usleep(500000); /* 500ms before retry */
            if (connect_feed(feed) != 0) {
                continue;
            }
        }

        for (int i = 0; i < n; i++) {
            char c = read_buf[i];

            if (c == '{' || c == '[') {
                /* Start of new message — reset frame buffer */
                feed->frame_pos = 0;
                memset(feed->frame_buf, 0, FRAME_BUF_SIZE);
                feed->frame_buf[feed->frame_pos++] = c;
            } else {
                if (feed->frame_pos > 0 && feed->frame_pos < FRAME_BUF_SIZE - 1) {
                    feed->frame_buf[feed->frame_pos++] = c;
                }
            }
        }

        /* Check if we have a complete message */
        process_frame(feed);
    }

    if (feed->fd >= 0) {
        close(feed->fd);
        feed->fd = -1;
    }
    feed->connected = false;

    return NULL;
}

/* --- Public API --- */

sensor_feed *sensor_feed_create(const char *path) {
    sensor_feed *feed = &instance;
    memset(feed, 0, sizeof(*feed));
    feed->fd = -1;
    if (path) snprintf(feed->path, sizeof(feed->path), "%s", path);

    return feed;
}

void sensor_feed_destroy(sensor_feed *feed) {
    if (!feed) return;
    sensor_feed_stop(feed);
}

void sensor_feed_start(sensor_feed *feed) {
    if (!feed || feed->running) return;

    feed->running = true;
    pthread_create(&feed->thread, NULL, poll_thread, feed);
}

void sensor_feed_stop(sensor_feed *feed) {
    if (!feed || !feed->running) return;

    feed->running = false;
    pthread_join(feed->thread, NULL);
}

const dashboard_state *sensor_feed_dashboard(const sensor_feed *feed) {
    return feed ? &feed->dashboard : NULL;
}

const menu_state *sensor_feed_menu(const sensor_feed *feed) {
    return feed ? &feed->menu : NULL;
}

const nav_state *sensor_feed_nav(const sensor_feed *feed) {
    return feed ? &feed->nav : NULL;
}

bool sensor_feed_connected(const sensor_feed *feed) {
    return feed ? feed->connected : false;
}

int sensor_feed_ms_since_update(const sensor_feed *feed) {
    if (!feed) return -1;
    if (feed->last_live_update.tv_sec == 0 && feed->last_live_update.tv_nsec == 0) return -1;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int ms = (int)((now.tv_sec - feed->last_live_update.tv_sec) * 1000 +
                   (now.tv_nsec - feed->last_live_update.tv_nsec) / 1000000);
    return ms;
}
