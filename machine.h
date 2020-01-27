#ifndef MACHINE_H
#define MACHINE_H

#include <FreeRTOS.h>

#include "machine.event.h"
#include "machine.state.h"

typedef machine_state_t (*machine_transition_fn)(machine_state_t current_state,
                                                 machine_event_t event);
typedef void (*machine_callback_fn)(machine_state_t new_state);

void machine_init(
    const machine_transition_fn state_transition_matrix[][MACHINE_EVENT_COUNT],
    machine_state_t initial_state, machine_callback_fn on_transition);
void machine_handle_event(machine_event_t event);
machine_state_t machine_current_state();

#endif /* MACHINE_H */
