#ifndef DOOR_H
#define DOOR_H

#include <FreeRTOS.h>

typedef enum {
  DOOR_OPEN = 2,
  DOOR_MOVING,
  DOOR_CLOSED,
  DOOR_UNKNOWN,
} door_state_t;

typedef void (*door_callback_fn)(door_state_t new_state);

void door_init(uint8_t p_bottom_sensor_gpio, uint8_t p_top_sensor_gpio,
               door_callback_fn g_callback);
door_state_t door_current_state();

#endif /* DOOR_H */
