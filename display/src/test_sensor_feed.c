/*
 * test_sensor_feed.c — Integration tests for sensor_feed module
 *
 * Uses named pipes (mkfifo) to simulate serial data and test the full
 * pipeline: connect → read → frame → parse → const accessor.
 */

#define _GNU_SOURCE
#include "sensor_feed.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

/* --- Test helpers --- */

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { \
    test_count++; \
    printf("  %d. %-50s", test_count, #name); \
    test_##name(); \
    pass_count++; \
    printf("PASS\n"); \
} while (0)

#define ASSERT_TRUE(cond)  assert(cond)
#define ASSERT_FALSE(cond) assert(!(cond))
#define ASSERT_EQ(a, b)    assert((a) == (b))
#define ASSERT_NEQ(a, b)   assert((a) != (b))
#define ASSERT_NULL(p)     assert((p) == NULL)
#define ASSERT_NOT_NULL(p) assert((p) != NULL)
#define ASSERT_FLOAT_EQ(a, b) assert(fabs((double)(a) - (double)(b)) < 0.01)
#define ASSERT_STR_EQ(a, b) assert(strcmp((a), (b)) == 0)

/* Create a named pipe for testing. Returns the path (caller must free and unlink). */
static char *make_test_pipe(void) {
    char *path = strdup("/tmp/tft-dash-test-XXXXXX");
    int fd = mkstemp(path);
    close(fd);
    unlink(path);  /* Remove the temp file, we'll create a FIFO at the same path */
    if (mkfifo(path, 0644) != 0) {
        perror("mkfifo");
        free(path);
        return NULL;
    }
    return path;
}

/* Open the write end of a pipe, retrying until the reader has opened the read end */
static int open_pipe_writer(const char *path) {
    for (int i = 0; i < 100; i++) {
        int fd = open(path, O_WRONLY | O_NONBLOCK);
        if (fd >= 0) return fd;
        usleep(20000); /* 20ms */
    }
    return -1;
}

/* Write a message to the pipe, retrying briefly if EAGAIN */
static void pipe_write(int fd, const char *msg) {
    size_t len = strlen(msg);
    size_t written = 0;
    int retries = 50;
    while (written < len && retries-- > 0) {
        ssize_t n = write(fd, msg + written, len - written);
        if (n > 0) {
            written += n;
        } else {
            usleep(10000); /* 10ms */
        }
    }
}

/* Give the background thread time to read and parse */
static void wait_for_processing(void) {
    usleep(300000); /* 300ms — give the background thread time to read and parse */
}

/* --- Tests --- */

void test_create_destroy(void) {
    sensor_feed *feed = sensor_feed_create(NULL);
    ASSERT_NOT_NULL(feed);
    ASSERT_FALSE(sensor_feed_connected(feed));
    sensor_feed_destroy(feed);

    /* NULL is safe */
    sensor_feed_destroy(NULL);
}

void test_initial_state(void) {
    sensor_feed *feed = sensor_feed_create(NULL);
    ASSERT_NOT_NULL(feed);

    const dashboard_state *dash = sensor_feed_dashboard(feed);
    const menu_state *menu = sensor_feed_menu(feed);
    const nav_state *nav = sensor_feed_nav(feed);

    ASSERT_NOT_NULL(dash);
    ASSERT_NOT_NULL(menu);
    ASSERT_NOT_NULL(nav);

    /* All zeroed before any data */
    ASSERT_EQ(dash->current_speed, 0);
    ASSERT_EQ(dash->rpm, 0);
    ASSERT_EQ(menu->front_sprocket, 0);
    ASSERT_EQ(nav->nav_yards, 0);

    sensor_feed_destroy(feed);
}

void test_null_safety(void) {
    ASSERT_NULL(sensor_feed_dashboard(NULL));
    ASSERT_NULL(sensor_feed_menu(NULL));
    ASSERT_NULL(sensor_feed_nav(NULL));
    ASSERT_FALSE(sensor_feed_connected(NULL));
}

void test_connect_to_pipe(void) {
    char *path = make_test_pipe();
    ASSERT_NOT_NULL(path);

    sensor_feed *feed = sensor_feed_create(path);
    sensor_feed_start(feed);

    /* Open writer so the reader's open() unblocks */
    int wfd = open_pipe_writer(path);
    wait_for_processing();

    ASSERT_TRUE(sensor_feed_connected(feed));

    close(wfd);
    sensor_feed_destroy(feed);
    unlink(path);
    free(path);
}

void test_live_message(void) {
    char *path = make_test_pipe();
    ASSERT_NOT_NULL(path);

    sensor_feed *feed = sensor_feed_create(path);
    sensor_feed_start(feed);

    int wfd = open_pipe_writer(path);
    wait_for_processing();

    /* Write a live data message: speed=68, rpm=1234, coolant=80, batt=12.5, hour=14, min=23, fuel=477, neutral=1, ... */
    pipe_write(wfd, "{,068,1234,80,12.5,14,23,477,1,0,1,0,0,200,0,1.2,3.4,23456.7,0,0.5,2,21,3,45,120,85,1,30,0,100,200,0,0,1,2,28,30,}");
    wait_for_processing();

    const dashboard_state *dash = sensor_feed_dashboard(feed);
    ASSERT_EQ(dash->current_speed, 68);
    ASSERT_EQ(dash->rpm, 1234);
    ASSERT_EQ(dash->coolant_temp, 80);
    ASSERT_FLOAT_EQ(dash->batt, 12.5);
    ASSERT_EQ(dash->current_hour, 14);
    ASSERT_EQ(dash->current_minute, 23);
    ASSERT_EQ(dash->fuel_float, 477);
    ASSERT_TRUE(dash->neutral);
    ASSERT_TRUE(dash->high_beam);
    ASSERT_EQ(dash->theme, 2);

    close(wfd);
    sensor_feed_destroy(feed);
    unlink(path);
    free(path);
}

void test_menu_message(void) {
    char *path = make_test_pipe();
    ASSERT_NOT_NULL(path);

    sensor_feed *feed = sensor_feed_create(path);
    sensor_feed_start(feed);

    int wfd = open_pipe_writer(path);
    wait_for_processing();

    /* Menu message: field 0=skip([), 1=choice_state, 2-7=odo digits, 8-13=odo2 digits,
       14=odo_error, 15-18=time digits, 19-22=spc digits, 23=front_sprocket, 24=rear_sprocket, 25=coolant_fan_temp, ... */
    pipe_write(wfd, "[,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,15,44,95,0,0,0,1,2,28,30,0,3,6,500,128,0,200,]");
    wait_for_processing();

    const menu_state *menu = sensor_feed_menu(feed);
    ASSERT_EQ(menu->front_sprocket, 15);
    ASSERT_EQ(menu->rear_sprocket, 44);
    ASSERT_EQ(menu->coolant_fan_temp, 95);

    close(wfd);
    sensor_feed_destroy(feed);
    unlink(path);
    free(path);
}

void test_nav_message(void) {
    char *path = make_test_pipe();
    ASSERT_NOT_NULL(path);

    sensor_feed *feed = sensor_feed_create(path);
    sensor_feed_start(feed);

    int wfd = open_pipe_writer(path);
    wait_for_processing();

    /* Live message with nav string in field 36 */
    pipe_write(wfd, "{,040,3200,86,13.9,11,15,420,0,0,0,0,0,0,3,23.4,46.8,1257.9,0,1.0,3,19,3,47,105,48,1,20,0,0,0,0,0,1,2,28,30,%TNR%A82%Edinburgh%3%350%0.2%1%12.5%,}");
    wait_for_processing();

    const nav_state *nav = sensor_feed_nav(feed);
    ASSERT_STR_EQ(nav->nav_symbol, "TNR");
    ASSERT_STR_EQ(nav->nav_road, "A82");
    ASSERT_STR_EQ(nav->nav_towards, "Edinburgh");

    close(wfd);
    sensor_feed_destroy(feed);
    unlink(path);
    free(path);
}

void test_message_framing(void) {
    char *path = make_test_pipe();
    ASSERT_NOT_NULL(path);

    sensor_feed *feed = sensor_feed_create(path);
    sensor_feed_start(feed);

    int wfd = open_pipe_writer(path);
    wait_for_processing();

    /* Write garbage, then a partial message, then a valid message */
    pipe_write(wfd, "garbage garbage garbage");
    pipe_write(wfd, "{,050,999,");  /* Incomplete — too short, won't parse */
    wait_for_processing();

    /* State should still be zeroed */
    ASSERT_EQ(sensor_feed_dashboard(feed)->current_speed, 0);

    /* Now write a complete valid message — must be >90 chars to pass the framing length check */
    pipe_write(wfd, "{,075,5000,90,13.2,10,30,400,0,0,0,0,0,200,0,1.2,3.4,23456.7,0,0.5,2,21,3,45,120,85,1,30,0,100,200,0,0,1,2,28,30,}");
    wait_for_processing();

    ASSERT_EQ(sensor_feed_dashboard(feed)->current_speed, 75);
    ASSERT_EQ(sensor_feed_dashboard(feed)->rpm, 5000);

    close(wfd);
    sensor_feed_destroy(feed);
    unlink(path);
    free(path);
}

void test_multiple_messages(void) {
    char *path = make_test_pipe();
    ASSERT_NOT_NULL(path);

    sensor_feed *feed = sensor_feed_create(path);
    sensor_feed_start(feed);

    int wfd = open_pipe_writer(path);
    wait_for_processing();

    /* First message: speed=30 — must be >90 chars */
    pipe_write(wfd, "{,030,2000,70,12.0,8,0,300,0,0,0,0,0,200,0,1.2,3.4,23456.7,0,0.5,2,21,3,45,120,85,1,30,0,100,200,0,0,1,2,28,30,}");
    wait_for_processing();
    ASSERT_EQ(sensor_feed_dashboard(feed)->current_speed, 30);

    /* Second message: speed=60 — should overwrite */
    pipe_write(wfd, "{,060,4000,85,13.5,9,15,350,0,0,0,0,0,200,0,1.2,3.4,23456.7,0,0.5,2,21,3,45,120,85,1,30,0,100,200,0,0,1,2,28,30,}");
    wait_for_processing();
    ASSERT_EQ(sensor_feed_dashboard(feed)->current_speed, 60);
    ASSERT_EQ(sensor_feed_dashboard(feed)->rpm, 4000);

    close(wfd);
    sensor_feed_destroy(feed);
    unlink(path);
    free(path);
}

/* --- Main --- */

int main(void) {
    /* Ignore SIGPIPE — writing to a closed pipe shouldn't kill us */
    signal(SIGPIPE, SIG_IGN);

    printf("=== Sensor Feed Tests ===\n\n");

    TEST(create_destroy);
    TEST(initial_state);
    TEST(null_safety);
    TEST(connect_to_pipe);
    TEST(live_message);
    TEST(menu_message);
    TEST(nav_message);
    TEST(message_framing);
    TEST(multiple_messages);

    printf("\n  Results: %d/%d passed\n", pass_count, test_count);
    return (pass_count == test_count) ? 0 : 1;
}
