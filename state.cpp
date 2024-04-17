#include "state.h"

void handle_state_machine(state_machine_t *sm, void *context) {
  switch(sm->state) {
    case STATUS_CALL:
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
    case CONFIG:
      sm->config(sm, context);
      break;
    default:
      break;
  }
}
