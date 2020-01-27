#ifndef ACCESSORY_H
#define ACCESSORY_H

#include <FreeRTOS.h>

#include "machine.state.h"

typedef struct accessory_config_t {
  char *name;
  char *manufacturer;
  char *model;
  char *serial_number;
  char *version;
  char *password;
  uint16_t movement_timeout;
  uint16_t reverse_delay;
} accessory_config_t;

void accessory_init(const accessory_config_t *config,
                    const machine_state_t initial_state);
void accessory_notify_state();

#endif /* ACCESSORY_H */
