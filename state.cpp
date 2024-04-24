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

garden_state_t int_to_garden_state_t(int c) {
  switch(c) {
    case ERROR:
    case CLEAR_ERROR:
    case STATUS_CALL:
    case CALIBRATION:
    case CONFIG:
      return (garden_state_t)c;
    default:
      return NONE;
  }

}