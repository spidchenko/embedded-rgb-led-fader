#define _POSIX_C_SOURCE 200112L

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define FREQUENCY_HZ          60
#define INTERVAL_NS           (1000000000L / FREQUENCY_HZ)
#define TIME_CHANGE_COLOR_SEC 2
#define STEPS_CHANGE_COLOR    (FREQUENCY_HZ * TIME_CHANGE_COLOR_SEC)

#define LED_LEFT_R  "/dev/aep/led_sx_r"
#define LED_LEFT_G  "/dev/aep/led_sx_g"
#define LED_LEFT_B  "/dev/aep/led_sx_b"

#define LED_RIGHT_R "/dev/aep/led_dx_r"
#define LED_RIGHT_G "/dev/aep/led_dx_g"
#define LED_RIGHT_B "/dev/aep/led_dx_b"

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} RGBColor;

RGBColor color_black = {.r = 0, .g = 0, .b = 0};

typedef struct {
  const char *r;
  const char *g;
  const char *b;
} LEDPaths;

typedef struct {
  RGBColor previous_color;
  RGBColor current_color;
  RGBColor next_color;
  uint16_t iteration;
} LED;

static volatile sig_atomic_t running = 1; 

static void handle_signal(int sig)
{
  (void)sig;
  running = 0;
}

/* Returns a random fully-saturated RGB color (HSV: S=100%, V=100%).
 * Covers all 6 hue sectors evenly.
 * Requires srand() called before first use.
 */
RGBColor random_hsv_color(void)
{
  uint8_t rnd    = (uint8_t)(rand() % 256);
  int     sector = rand() % 6;
  RGBColor color;

  switch (sector) {
    case 0:  color.r = 255; color.g = rnd; color.b = 0;   break;
    case 1:  color.r = rnd; color.g = 255; color.b = 0;   break;
    case 2:  color.r = 0;   color.g = 255; color.b = rnd; break;
    case 3:  color.r = 0;   color.g = rnd; color.b = 255; break;
    case 4:  color.r = rnd; color.g = 0;   color.b = 255; break;
    case 5:  color.r = 255; color.g = 0;   color.b = rnd; break;
    default: color.r = 255; color.g = 255; color.b = 255; break;
  }

  return color;
}

static void timespec_add_ns(struct timespec *ts, long ns)
{
  ts->tv_nsec += ns;
  if (ts->tv_nsec >= 1000000000L) {
    ts->tv_sec++;
    ts->tv_nsec -= 1000000000L;
  }
}

static void write_led_channel(const char *path, uint8_t value)
{
  char    buffer[16];
  int     len     = snprintf(buffer, sizeof(buffer), "%d\n", value);
  int     fd      = open(path, O_WRONLY);
  ssize_t written;

  if (fd == -1) {
    fprintf(stderr, "Can't open %s\n", path);
    return;
  }

  written = write(fd, buffer, (size_t)len);
  if (written < 0) {
    fprintf(stderr, "Write failed for %s\n", path);
  }

  close(fd);
}

static void apply_led_rgb_state(LED *led, const LEDPaths *paths)
{
  write_led_channel(paths->r, led->current_color.r);
  write_led_channel(paths->g, led->current_color.g);
  write_led_channel(paths->b, led->current_color.b);
}

void update_led_rgb_state(LED *led)
{
 
  if(led->next_color.r == 0 && led->next_color.g == 0 && led->next_color.b == 0){
    led->next_color = random_hsv_color();
  }
  
  uint8_t current_r = led->previous_color.r + (led->next_color.r - led->previous_color.r) * (led->iteration / (double)STEPS_CHANGE_COLOR);
  uint8_t current_g = led->previous_color.g + (led->next_color.g - led->previous_color.g) * (led->iteration / (double)STEPS_CHANGE_COLOR);
  uint8_t current_b = led->previous_color.b + (led->next_color.b - led->previous_color.b) * (led->iteration / (double)STEPS_CHANGE_COLOR);
  
  led->current_color.r = current_r;
  led->current_color.g = current_g;
  led->current_color.b = current_b;

  led->iteration++;
  if(led->iteration >= STEPS_CHANGE_COLOR) {
    led->previous_color = led->next_color;
    led->next_color = random_hsv_color();
    led->iteration = 0;
  }
}


int main(void)
{
  LED left_led  = {.previous_color = color_black, .next_color = color_black};
  LED right_led = {.previous_color = color_black, .next_color = color_black};
  right_led.previous_color = color_black;
  
  struct timespec next_tick;

  const LEDPaths left_paths  = { .r = LED_LEFT_R,  .g = LED_LEFT_G,  .b = LED_LEFT_B  };
  const LEDPaths right_paths = { .r = LED_RIGHT_R, .g = LED_RIGHT_G, .b = LED_RIGHT_B };

  if (signal(SIGINT,  handle_signal) == SIG_ERR || signal(SIGTERM, handle_signal) == SIG_ERR) {
    fprintf(stderr, "Failed to set signal handler\n");
    return 1;
  }

  srand((unsigned int)time(NULL));

  clock_gettime(CLOCK_MONOTONIC, &next_tick);

  while (running) {
    update_led_rgb_state(&left_led);
    update_led_rgb_state(&right_led);

    apply_led_rgb_state(&left_led,  &left_paths);
    apply_led_rgb_state(&right_led, &right_paths);

    timespec_add_ns(&next_tick, INTERVAL_NS);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_tick, NULL);
  }

  printf("Shutting down cleanly\n");
  return 0;
}
