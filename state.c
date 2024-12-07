#include "state.h"

void handle_state_machine(state_machine_t *sm, void *context) {
  switch(sm->state) {
    case STATUS:
      sm->status(sm, context);
      break;
    case CALIBRATION:
      sm->calibration(sm, context);
      break;
    case ERROR:
      sm->error(sm, context);
      break;
    case CLEAR_ERROR:
      sm->error(sm, context);
      break;
    default:
      break;
  }
}
