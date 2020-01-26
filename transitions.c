#include "transitions.h"

#include <esplibs/libmain.h>
#include <etstimer.h>

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
      [MACHINE_DOOR_OPEN] = "door open",
      [MACHINE_DOOR_MOVING] = "door moving",
      [MACHINE_DOOR_CLOSED] = "door closed",
      [MACHINE_DOOR_UNKNOWN] = "door unknown",
      [MACHINE_DOOR_MOVING_TIMEOUT] = "door movement timed out",
  };

  const char *description = state_description_map[machine_event]
                                ? state_description_map[machine_event]
                                : "unknown";

  return description;
}

static ETSTimer g_movement_timer;
static uint16_t g_movement_timer_duration;

static void transition_arm_movement_timer() {
  sdk_os_timer_arm(&g_movement_timer, g_movement_timer_duration, false);
}

static void transition_disarm_movement_timer() {
  sdk_os_timer_disarm(&g_movement_timer);
}

static void transition_handle_movement_timeout() {
  printf("Timer fired. Updating state from sensors.\n");
  transition_disarm_movement_timer();
  machine_handle_event(MACHINE_DOOR_MOVING_TIMEOUT);
}

static machine_state_t transition_set_state_open() {
  transition_disarm_movement_timer();
  return MACHINE_STATE_OPEN;
}

static machine_state_t transition_set_state_closed() {
  transition_disarm_movement_timer();
  return MACHINE_STATE_CLOSED;
}

static machine_state_t transition_set_state_opening() {
  transition_arm_movement_timer();
  return MACHINE_STATE_OPENING;
}

static machine_state_t transition_set_state_closing() {
  transition_arm_movement_timer();
  return MACHINE_STATE_CLOSING;
}

static machine_state_t transition_set_state_stopped() {
  return MACHINE_STATE_STOPPED;
}

static machine_state_t transition_set_state_unknown() {
  return MACHINE_STATE_UNKNOWN;
}

const machine_transition_fn
    g_state_transition_matrix[MACHINE_STATE_COUNT][MACHINE_EVENT_COUNT] = {
        [MACHINE_STATE_OPEN] =
            {
                [MACHINE_DOOR_MOVING] = transition_set_state_closing,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        [MACHINE_STATE_OPENING] =
            {
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_MOVING_TIMEOUT] = transition_set_state_unknown,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        [MACHINE_STATE_CLOSED] =
            {
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_MOVING] = transition_set_state_opening,
            },
        [MACHINE_STATE_CLOSING] =
            {
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_MOVING_TIMEOUT] = transition_set_state_unknown,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        [MACHINE_STATE_STOPPED] =
            {
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
        // MACHINE_STATE_UNKNOWN
        [MACHINE_STATE_COUNT - 1] =
            {
                [MACHINE_DOOR_OPEN] = transition_set_state_open,
                [MACHINE_DOOR_CLOSED] = transition_set_state_closed,
            },
};

void transition_init(uint16_t movement_timeout) {
  g_movement_timer_duration = movement_timeout;
  transition_disarm_movement_timer();
  sdk_os_timer_setfn(&g_movement_timer, transition_handle_movement_timeout,
                     NULL);
}
