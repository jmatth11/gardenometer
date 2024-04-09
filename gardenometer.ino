#include <string.h>

#include "state.h"

// define constants
const int calibration_pin = 9;

// define structures
struct calibration {
  int airValue;
  int waterValue;
};

struct metrics {
  int temp;
  int light;
  int moisture;
};

struct state {
  struct metrics metrics;
  struct calibration calibration;
};

// define functions
void garden_status(state_machine_t *machine, void* context);
void garden_calibration(state_machine_t *machine, void* context);
void garden_error(state_machine_t *machine, void* context);

// initialize globals
struct metrics garden_metrics;
struct calibration garden_calibration;
struct state garden_state;
state_machine_t state_machine;

// setup function
void setup() {
  Serial.begin(9600);
  // set pins
  pinMode(calibration_pin, OUTPUT);
  // initialize global structures
  state_machine.state = STATUS;
  state_machine.status = garden_status;
  state_machine.calibration = garden_calibration;
  state_machine.error = garden_error;
  memset(&garden_metrics, 0, sizeof(struct metrics));
  memset(&garden_calibration, 0, sizeof(struct calibration));
  garden_state.metrics = garden_metrics;
  garden_state.calibration = garden_calibration;
}

// loop function
void loop() {
  int wait = 1000;
  int espResponse = -1;
  // TODO see if this is the right way to read data from the serial port
  if (Serial.available()) {
    espResponse = Serial.read();
  }
  handle_state_machine(&state_machine);
  // if the state is status then we want to wait about a minute
  // before reporting again.
  if (state_machine.state == STATUS) {
    wait = 1000 * 60;
  }
  delay(wait);
}

int avg_moisture() {
  // TODO collect and average the values of the moisture sensor
}

// implement state functions
void garden_status(state_machine_t *machine, void* context) {
  struct garden_state *state = (struct garden_state *)context;
  Serial.printf("t=%d;l=%d;m=%d",
                state->metrics.temp, state->metrics.light, state->metrics.moisture);
}

void garden_calibration(state_machine_t *machine, void* context) {
  struct garden_state *state = (struct garden_state *)context;
  writePin(calibration_pin, HIGH);
  delay(1000);
  writePin(calibration_pin, LOW);
  state->calibration.airValue = avg_moisture();
  writePin(calibration_pin, HIGH);
  delay(1000);
  writePin(calibration_pin, LOW);
  state->calibration.waterValue = avg_moisture();
}

void garden_error(state_machine_t *machine, void* context) {
  Serial.println("error state...");
}
