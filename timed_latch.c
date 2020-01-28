#include "timed_latch.h"

#include <assert.h>
#include <stdio.h>
#include <task.h>

static bool g_triggered = false;
static uint8_t g_gpio;
static uint16_t g_delay;

static void timed_latch_write(bool on) { gpio_write(g_gpio, !on); }

void timed_latch_trigger() {
  printf("Triggering...\n");
  timed_latch_write(true);
  vTaskDelay(g_delay / portTICK_PERIOD_MS);
  timed_latch_write(false);
}

void timed_latch_init(uint8_t gpio, uint16_t delay) {
  assert(g_gpio == 0 && g_delay == 0);
  g_gpio = gpio;
  g_delay = delay;

  timed_latch_write(g_triggered);
  gpio_enable(g_gpio, GPIO_OUTPUT);
}
