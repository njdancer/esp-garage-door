#ifndef TRANSITIONS_H
#define TRANSITIONS_H

#include "machine.h"

extern const machine_transition_fn
    g_state_transition_matrix[MACHINE_STATE_COUNT][MACHINE_EVENT_COUNT];

const char *machine_state_description(machine_state_t machine_state);
const char *machine_event_description(machine_event_t machine_event);

void transition_init(uint16_t movement_timeout);

#endif /* TRANSITIONS_H */
