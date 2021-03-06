#include <FreeRTOS.h>
#include <esp/uart.h>
#include <stdio.h>
#include <wifi_config.h>

#include "accessory.h"
#include "door.h"
#include "machine.h"
#include "timed_latch.h"

#define RELAY_HOLD_DURATION 100

static const accessory_config_t g_accessory_config = {
    .name = "Garage Door",
    .manufacturer = "Chamberlain",
    .model = "Merlin Professional MT60P",
    .serial_number = "1",
    .version = "2.1",
    .password = "123-45-678",
    .movement_timeout = 23000,
    .reverse_delay = 2000,
};

static void handle_door_state_changed(door_state_t new_state) {
  const machine_state_t door_state_map[] = {
      [DOOR_OPEN] = MACHINE_DOOR_OPEN,
      [DOOR_MOVING] = MACHINE_DOOR_MOVING,
      [DOOR_CLOSED] = MACHINE_DOOR_CLOSED,
      [DOOR_UNKNOWN] = MACHINE_DOOR_UNKNOWN,
  };

  machine_handle_event(door_state_map[new_state]);
}

static void handle_wifi_ready() {
  door_init(BOTTOM_PIN, TOP_PIN, handle_door_state_changed);
  timed_latch_init(RELAY_PIN, RELAY_HOLD_DURATION);

  const machine_state_t initial_state_map[] = {
      [DOOR_OPEN] = MACHINE_STATE_OPEN,
      [DOOR_MOVING] = MACHINE_STATE_UNKNOWN,
      [DOOR_CLOSED] = MACHINE_STATE_CLOSED,
      [DOOR_UNKNOWN] = MACHINE_STATE_UNKNOWN,
  };

  accessory_init(&g_accessory_config, initial_state_map[door_current_state()]);
}

void user_init(void) {
  uart_set_baud(0, 115200);

  printf("Staring Garage Door Opener...");

  wifi_config_init("garage-door", NULL, handle_wifi_ready);
}
