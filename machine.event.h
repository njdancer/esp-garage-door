#ifndef MACHINE_EVENT_H
#define MACHINE_EVENT_H

// Enum values match those of door_state_t
#define MACHINE_EVENT_COUNT 5
typedef enum {
  MACHINE_DOOR_OPEN = 0,
  MACHINE_DOOR_MOVING = 1,
  MACHINE_DOOR_CLOSED = 2,
  MACHINE_DOOR_UNKNOWN = 3,
  MACHINE_DOOR_MOVING_TIMEOUT = 4,
} machine_event_t;

#endif /* MACHINE_EVENT_H */
