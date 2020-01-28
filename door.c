#include "door.h"

#include <assert.h>
#include <stdio.h>
#include <toggle.h>

// Reed switches are Normally-Open and configured with pull-up resistor so a
// high input indicates sensor not-triggered - should be made configurable to
// support different switch configurations
typedef enum { SENSOR_CLOSED = 0, SENSOR_OPEN = 1 } sensor_state_t;

static uint8_t g_bottom_sensor_gpio;
static uint8_t g_top_sensor_gpio;

static sensor_state_t g_top_sensor_state;
static sensor_state_t g_bottom_sensor_state;

static door_callback_fn g_on_door_state_changed;

const char *sensor_description(uint8_t sensor_gpio) {
  const char *description = "unknown";

  if (sensor_gpio == g_bottom_sensor_gpio) {
    description = "bottom";
  } else if (sensor_gpio == g_top_sensor_gpio) {
    description = "top";
  }

  return description;
}

const char *sensor_state_description(sensor_state_t sensor_state) {
  const char *state_description_map[] = {
      [SENSOR_CLOSED] = "closed",
      [SENSOR_OPEN] = "open",
  };

  const char *description = state_description_map[sensor_state]
                                ? state_description_map[sensor_state]
                                : "unknown";

  return description;
}

const char *door_state_description(door_state_t door_state) {
  const char *state_description_map[] = {
      [DOOR_OPEN] = "open",
      [DOOR_MOVING] = "between sensors",
      [DOOR_CLOSED] = "closed",
      [DOOR_UNKNOWN] = "unknown",
  };

  const char *description = state_description_map[door_state]
                                ? state_description_map[door_state]
                                : "unknown";

  return description;
}

void reset_sensor_state() {
  g_bottom_sensor_state = gpio_read(g_bottom_sensor_gpio);
  g_top_sensor_state = gpio_read(g_top_sensor_gpio);
}

static void on_sensor_change(bool high, void *p_sensor_gpio) {
  uint8_t sensor_gpio = *((uint8_t *)p_sensor_gpio);
  sensor_state_t sensor_state = high;
  if (sensor_gpio == g_bottom_sensor_gpio) {
    g_bottom_sensor_state = sensor_state;
  } else if (sensor_gpio == g_top_sensor_gpio) {
    g_top_sensor_state = sensor_state;
  }

  printf("%s(%d) sensor %s\n", sensor_description(sensor_gpio), sensor_gpio,
         sensor_state_description(sensor_state));

  door_state_t door_state = door_current_state();

  printf("Door appears to be %s\n", door_state_description(door_state));

  g_on_door_state_changed(door_state);
}

static void sensor_init(uint8_t *p_sensor_gpio) {
  gpio_set_pullup(*p_sensor_gpio, true, true);

  int result = toggle_create(*p_sensor_gpio, on_sensor_change, p_sensor_gpio);
  // This should succeed regardless of whether sensors are connected. If this
  // fails a hardware or programming error exists
  assert(result >= 0);
}

void door_init(uint8_t bottom_sensor_gpio, uint8_t top_sensor_gpio,
               door_callback_fn on_door_state_changed) {
  // These should not be set prior to init and init should fail if run more than
  // once
  assert(g_bottom_sensor_gpio == 0 && g_top_sensor_gpio == 0 &&
         g_on_door_state_changed == 0);

  g_bottom_sensor_gpio = bottom_sensor_gpio;
  g_top_sensor_gpio = top_sensor_gpio;
  g_on_door_state_changed = on_door_state_changed;

  sensor_init(&g_bottom_sensor_gpio);
  sensor_init(&g_top_sensor_gpio);

  reset_sensor_state();
}

door_state_t door_current_state() {
  const door_state_t g_sensor_state_matrix[][2] = {{DOOR_UNKNOWN, DOOR_CLOSED},
                                                   {DOOR_OPEN, DOOR_MOVING}};

  return g_sensor_state_matrix[g_bottom_sensor_state][g_top_sensor_state];
}
