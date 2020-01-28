#include "transitions.h"

#include <esplibs/libmain.h>
#include <etstimer.h>
#include <stdio.h>

#include "timed_latch.h"

const char *machine_state_description(machine_state_t machine_state) {
  const char *state_description_map[] = {
      [MACHINE_STATE_OPEN] = "open",       [MACHINE_STATE_CLOSED] = "closed",
      [MACHINE_STATE_OPENING] = "opening", [MACHINE_STATE_CLOSING] = "closing",
      [MACHINE_STATE_STOPPED] = "stopped", [MACHINE_STATE_UNKNOWN] = "unknown",
  };

  const char *description = state_description_map[machine_state]
                                ? state_description_map[machine_state]
                                : "unknown";

  return description;
}

const char *machine_event_description(machine_event_t machine_event) {
  const char *state_description_map[] = {
      [MACHINE_INPUT_OPEN] = "input open",
      [MACHINE_INPUT_CLOSE] = "input close",
      [MACHINE_DOOR_OPEN] = "door open",
      [MACHINE_DOOR_MOVING] = "door moving",
      [MACHINE_DOOR_CLOSED] = "door closed",
      [MACHINE_DOOR_UNKNOWN] = "door unknown",
      [MACHINE_TIMEOUT_MOVEMENT] = "door movement timed out",
      [MACHINE_TIMEOUT_REVERSE] = "door reversing"};

  const char *description = state_description_map[machine_event]
                                ? state_description_map[machine_event]
                                : "unknown";

  return description;
}

static uint16_t g_movement_timeout;
static ETSTimer g_movement_timer;
static uint16_t g_reverse_delay;
static ETSTimer g_reverse_timer;
static machine_state_t g_reverse_state = MACHINE_STATE_UNKNOWN;

static void transition_arm_movement_timer() {
  sdk_os_timer_arm(&g_movement_timer, g_movement_timeout, false);
}

static void transition_disarm_movement_timer() {
  sdk_os_timer_disarm(&g_movement_timer);
}

static void transition_handle_movement_timer() {
  printf("Movement timer fired.\n");
  transition_disarm_movement_timer();
  machine_handle_event(MACHINE_TIMEOUT_MOVEMENT);
}

static void transition_arm_reverse_timer() {
  sdk_os_timer_arm(&g_reverse_timer, g_reverse_delay, false);
}

static void transition_disarm_reverse_timer() {
  sdk_os_timer_disarm(&g_reverse_timer);
}

static void transition_handle_reverse_timer() {
  printf("Reverse timer fired.\n");
  transition_disarm_movement_timer();
  machine_handle_event(MACHINE_TIMEOUT_REVERSE);
}

static machine_state_t transition_set_state_open(machine_state_t current_state,
                                                 machine_event_t event) {
  transition_disarm_movement_timer();
  return MACHINE_STATE_OPEN;
}

static machine_state_t
transition_set_state_closed(machine_state_t current_state,
                            machine_event_t event) {
  transition_disarm_movement_timer();
  return MACHINE_STATE_CLOSED;
}

static machine_state_t
transition_set_state_opening(machine_state_t current_state,
                             machine_event_t event) {
  transition_arm_movement_timer();
  return MACHINE_STATE_OPENING;
}

static machine_state_t
transition_set_state_closing(machine_state_t current_state,
                             machine_event_t event) {
  transition_arm_movement_timer();
  return MACHINE_STATE_CLOSING;
}

static machine_state_t
transition_set_state_unknown(machine_state_t current_state,
                             machine_event_t event) {
  return MACHINE_STATE_UNKNOWN;
}

static machine_state_t transition_toggle_door(machine_state_t current_state,
                                              machine_event_t event) {
  timed_latch_trigger();

  return current_state;
}

static machine_state_t transition_stop_door(machine_state_t current_state,
                                            machine_event_t event) {
  timed_latch_trigger();
  transition_disarm_movement_timer();

  switch (current_state) {
  case MACHINE_STATE_OPENING:
    g_reverse_state = MACHINE_STATE_CLOSING;
    break;
  case MACHINE_STATE_CLOSING:
    g_reverse_state = MACHINE_STATE_OPENING;
    break;
  default:
    g_reverse_state = MACHINE_STATE_UNKNOWN;
    printf("Door state unknown while reversing\n");
    return MACHINE_STATE_UNKNOWN;
  }

  transition_arm_reverse_timer();
  return MACHINE_STATE_STOPPED;
}

static machine_state_t transition_reverse_door(machine_state_t current_state,
                                               machine_event_t event) {
  timed_latch_trigger();

  return g_reverse_state;
}

const machine_transition_fn
    g_state_transition_matrix[MACHINE_STATE_COUNT][MACHINE_EVENT_COUNT] = {
        [MACHINE_STATE_OPEN] =
            {
                [MACHINE_INPUT_CLOSE] = transition_toggle_door,
                [MACHINE_DOOR_MOVING] = transition_set_state_closing,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        [MACHINE_STATE_OPENING] =
            {
                [MACHINE_INPUT_CLOSE] = transition_stop_door,
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_TIMEOUT_MOVEMENT] = transition_set_state_unknown,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        [MACHINE_STATE_CLOSED] =
            {
                [MACHINE_INPUT_OPEN] = transition_toggle_door,
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_MOVING] = transition_set_state_opening,
            },
        [MACHINE_STATE_CLOSING] =
            {
                [MACHINE_INPUT_OPEN] = transition_stop_door,
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_TIMEOUT_MOVEMENT] = transition_set_state_unknown,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        [MACHINE_STATE_STOPPED] =
            {
                [MACHINE_TIMEOUT_REVERSE] = transition_reverse_door,
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        // MACHINE_STATE_UNKNOWN
        [MACHINE_STATE_COUNT - 1] =
            {
                [MACHINE_INPUT_OPEN] = transition_toggle_door,
                [MACHINE_INPUT_CLOSE] = transition_toggle_door,
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
};

void transition_init(uint16_t movement_timeout, uint16_t reverse_delay) {
  g_movement_timeout = movement_timeout;
  g_reverse_delay = reverse_delay;

  transition_disarm_movement_timer();
  sdk_os_timer_setfn(&g_movement_timer, transition_handle_movement_timer, NULL);

  transition_disarm_reverse_timer();
  sdk_os_timer_setfn(&g_reverse_timer, transition_handle_reverse_timer, NULL);
}
