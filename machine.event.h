#ifndef MACHINE_EVENT_H
#define MACHINE_EVENT_H

// Enum values 0 and 1 correspond to HOMEKIT_CHARACTERISTIC_TARGET_DOOR_STATE
// Enum values 3-6, match those of door_state_t
#define MACHINE_EVENT_COUNT 8
typedef enum {
  MACHINE_INPUT_OPEN = 0,
  MACHINE_INPUT_CLOSE = 1,
  MACHINE_DOOR_OPEN = 2,
  MACHINE_DOOR_MOVING,
  MACHINE_DOOR_CLOSED,
  MACHINE_DOOR_UNKNOWN,
  MACHINE_TIMEOUT_MOVEMENT,
  MACHINE_TIMEOUT_REVERSE,
} machine_event_t;

#endif /* MACHINE_EVENT_H */
