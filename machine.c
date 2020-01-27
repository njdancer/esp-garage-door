#include "machine.h"

#include <assert.h>

static const machine_transition_fn (
    *gp_state_transition_matrix)[MACHINE_EVENT_COUNT];
static machine_callback_fn g_on_transition;

static machine_state_t g_current_state;

void machine_init(
    const machine_transition_fn state_transition_matrix[][MACHINE_EVENT_COUNT],
    machine_state_t initial_state, machine_callback_fn on_transition) {
  assert(gp_state_transition_matrix == 0);
  assert(g_current_state == 0);
  assert(g_on_transition == 0);
  gp_state_transition_matrix = state_transition_matrix;
  g_current_state = initial_state;
  g_on_transition = on_transition;
}

void machine_handle_event(machine_event_t event) {
  machine_transition_fn transitioner =
      gp_state_transition_matrix[g_current_state][event];

  if (transitioner != NULL) {
    g_current_state = transitioner(g_current_state, event);

    if (g_on_transition != NULL) {
      g_on_transition(g_current_state);
    }
  }
}

machine_state_t machine_current_state() { return g_current_state; }
