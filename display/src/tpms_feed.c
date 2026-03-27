/*
 * tpms_feed.c - TPMS serial connection, frame decoding, and background thread
 */

#define _GNU_SOURCE
#include "tpms_feed.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <pthread.h>

#define READ_BUF_SIZE 256
#define FRAME_BUF_SIZE 256

/* ── Default serial paths to try ── */

static const char *DEFAULT_PATHS[] = {
    "/dev/cu.usbserial-D3077502",
    "/dev/ttyUSB0",
    "/dev/ttyUSB1",
    "/dev/cu.usbserial-210",
    NULL
};

/* ── Internal struct ── */

struct tpms_feed {
    tpms_model model;
    int front_sensor_id;
    int rear_sensor_id;
    char path[256];             /* empty = search defaults */
    int fd;
    bool running;
    pthread_t thread;
    tpms_state state;
};

static tpms_feed instance;

/* ── Frame decoding (pure functions) ── */

bool tpms_decode_standard_frame(const uint8_t *buf, int len,
                                int *sensor_id, double *bar, double *psi,
                                int *temp_celsius, tpms_sensor_state *state)
{
    if (!buf || len < 13 || !sensor_id || !bar || !psi || !temp_celsius || !state)
        return false;

    /* Header: 0xAA 0xA1 */
    if (buf[0] != 0xAA || buf[1] != 0xA1)
        return false;

    *sensor_id = buf[5];

    int tp_byte1 = buf[9];
    int tp_byte2 = buf[10];
    *bar = 0.025 * (double)(((tp_byte1 & 3) * 256) + (tp_byte2 & 255));
    *psi = *bar * 14.5;

    *temp_celsius = (buf[11] & 255) - 50;

    int state_byte = buf[12];
    if ((state_byte & 3) == 1)
        *state = TPMS_STATE_FAST_LEAK;
    else if ((state_byte & 16) == 16)
        *state = TPMS_STATE_HIGH_PRESSURE;
    else if ((state_byte & 8) == 8)
        *state = TPMS_STATE_LOW_PRESSURE;
    else if ((state_byte & 4) == 4)
        *state = TPMS_STATE_HIGH_TEMP;
    else if ((state_byte & 0x80) == 0x80)
        *state = TPMS_STATE_LOW_BATTERY;
    else
        *state = TPMS_STATE_NORMAL;

    return true;
}

bool tpms_decode_ebay_frame(const uint8_t *buf, int len,
                            int *sensor_id, double *psi, double *bar,
                            int *temp_celsius)
{
    if (!buf || len < 6 || !sensor_id || !psi || !bar || !temp_celsius)
        return false;

    /* Header: 0x55 0xAA */
    if (buf[0] != 0x55 || buf[1] != 0xAA)
        return false;

    /* Sensor ID mapping: 0→1, 1→2, 16→3, 17→4 */
    switch (buf[3]) {
        case  0: *sensor_id = 1; break;
        case  1: *sensor_id = 2; break;
        case 16: *sensor_id = 3; break;
        case 17: *sensor_id = 4; break;
        default: return false;
    }

    *psi = (double)(buf[4] & 255) * 3.44;
    *bar = *psi / 14.5;
    *temp_celsius = (buf[5] & 255) - 50;

    return true;
}

/* ── Serial connection ── */

static int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

static int configure_serial(int fd, tpms_model model) {
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) return -1;

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

    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    speed_t baud = (model == TPMS_MODEL_STANDARD) ? B9600 : B19200;
    cfsetispeed(&tty, baud);
    cfsetospeed(&tty, baud);

    return tcsetattr(fd, TCSANOW, &tty);
}

static int try_connect(tpms_feed *feed, const char *path) {
    if (!file_exists(path)) return -1;
    int fd = open(path, O_RDWR);
    if (fd < 0) return -1;
    if (configure_serial(fd, feed->model) != 0) {
        close(fd);
        return -1;
    }
    feed->fd = fd;
    feed->state.connected = true;
    fprintf(stderr, "tpms_feed: connected to %s\n", path);
    return 0;
}

static int connect_feed(tpms_feed *feed) {
    if (feed->path[0]) return try_connect(feed, feed->path);
    for (int i = 0; DEFAULT_PATHS[i]; i++) {
        if (try_connect(feed, DEFAULT_PATHS[i]) == 0) return 0;
    }
    return -1;
}

/* ── Sensor ID → front/rear mapping ── */

static void update_sensor(tpms_feed *feed, int sensor_id, double bar, double psi, int temp, tpms_sensor_state sensor_state) {
    if (sensor_id == feed->front_sensor_id) {
        feed->state.front.bar = bar;
        feed->state.front.psi = psi;
        feed->state.front.temp_celsius = temp;
        feed->state.front.state = sensor_state;
        feed->state.front.received = true;
        feed->state.signal_active = true;
    }
    if (sensor_id == feed->rear_sensor_id) {
        feed->state.rear.bar = bar;
        feed->state.rear.psi = psi;
        feed->state.rear.temp_celsius = temp;
        feed->state.rear.state = sensor_state;
        feed->state.rear.received = true;
        feed->state.signal_active = true;
    }
}

/* ── Background thread ── */

static void process_standard_frame(tpms_feed *feed, const uint8_t *buf, int len) {
    int sensor_id;
    double bar, psi;
    int temp;
    tpms_sensor_state sensor_state;
    if (tpms_decode_standard_frame(buf, len, &sensor_id, &bar, &psi, &temp, &sensor_state)) {
        update_sensor(feed, sensor_id, bar, psi, temp, sensor_state);
    }
}

static void process_ebay_frame(tpms_feed *feed, const uint8_t *buf, int len) {
    int sensor_id;
    double psi, bar;
    int temp;
    if (tpms_decode_ebay_frame(buf, len, &sensor_id, &psi, &bar, &temp)) {
        update_sensor(feed, sensor_id, bar, psi, temp, TPMS_STATE_NORMAL);
    }
}

static void *poll_thread(void *arg) {
    tpms_feed *feed = (tpms_feed *)arg;
    uint8_t read_buf[READ_BUF_SIZE];
    uint8_t frame_buf[FRAME_BUF_SIZE];
    int frame_pos = 0;

    if (connect_feed(feed) != 0) {
        fprintf(stderr, "tpms_feed: failed to connect\n");
        return NULL;
    }

    /* Frame header bytes for each model */
    uint8_t hdr0 = (feed->model == TPMS_MODEL_STANDARD) ? 0xAA : 0x55;
    uint8_t hdr1 = 0xAA;
    if (feed->model == TPMS_MODEL_STANDARD) hdr1 = 0xA1;

    int min_frame_len = (feed->model == TPMS_MODEL_STANDARD) ? 13 : 6;

    while (feed->running) {
        memset(read_buf, 0, sizeof(read_buf));
        int n = read(feed->fd, read_buf, sizeof(read_buf));

        if (n < 0) {
            close(feed->fd);
            feed->fd = -1;
            feed->state.connected = false;
            if (!feed->running) break;
            usleep(2000000);
            if (connect_feed(feed) != 0) continue;
        }

        for (int i = 0; i < n; i++) {
            /* Detect frame start */
            if (read_buf[i] == hdr0 && (i + 1) < n && read_buf[i + 1] == hdr1) {
                frame_pos = 0;
                frame_buf[frame_pos++] = read_buf[i];
                continue;
            }

            if (frame_pos > 0 && frame_pos < FRAME_BUF_SIZE) {
                frame_buf[frame_pos++] = read_buf[i];
            }
        }

        /* Check for complete frame */
        if (frame_pos >= min_frame_len) {
            if (feed->model == TPMS_MODEL_STANDARD)
                process_standard_frame(feed, frame_buf, frame_pos);
            else
                process_ebay_frame(feed, frame_buf, frame_pos);
            frame_pos = 0;
        }
    }

    close(feed->fd);
    return NULL;
}

/* ── Public API ── */

tpms_feed *tpms_feed_create(tpms_model model, int front_sensor_id, int rear_sensor_id, const char *path) {
    tpms_feed *feed = &instance;
    memset(feed, 0, sizeof(*feed));
    feed->model = model;
    feed->front_sensor_id = front_sensor_id;
    feed->rear_sensor_id = rear_sensor_id;
    if (path) snprintf(feed->path, sizeof(feed->path), "%s", path);
    feed->fd = -1;
    return feed;
}

void tpms_feed_destroy(tpms_feed *feed) {
    if (!feed) return;
    tpms_feed_stop(feed);
}

void tpms_feed_start(tpms_feed *feed) {
    if (!feed || feed->running) return;
    feed->running = true;
    pthread_create(&feed->thread, NULL, poll_thread, feed);
}

void tpms_feed_stop(tpms_feed *feed) {
    if (!feed || !feed->running) return;
    feed->running = false;
    pthread_join(feed->thread, NULL);
}

const tpms_state *tpms_feed_state(const tpms_feed *feed) {
    if (!feed) return NULL;
    return &feed->state;
}
