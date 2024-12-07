#ifndef GARDENOMETER_STATE_H
#define GARDENOMETER_STATE_H

typedef enum garden_state {
  NONE = 0,
  ERROR,
  CLEAR_ERROR,
  STATUS_CALL,
  CALIBRATION,
  CONFIG
} garden_state_t;

typedef struct state_machine {
  garden_state_t state;
  void(*status)(struct state_machine* sm, void* context);
  void(*calibration)(struct state_machine* sm, void* context);
  void(*error)(struct state_machine* sm, void* context);
  void(*config)(struct state_machine* sm, void* context);
} state_machine_t;

void handle_state_machine(state_machine_t* sm, void* context);

#endif
