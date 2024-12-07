#include <string.h>
#include "state.h"

// define constants
#define MOISTURE_PIN A0
#define LUX_PIN A1
#define ONE_WIRE_BUS 4
const int calibration_pin = 9;
const int error_pin = 10;

// define structures
struct calibration {
  int airValue;
  int waterValue;
};

struct garden_state {
  struct calibration calibration;
  DallasTemperature temperature;
};

// define functions
void garden_status(state_machine_t *machine, void* context);
void garden_calibration(state_machine_t *machine, void* context);
void garden_error(state_machine_t *machine, void* context);

/**
 * Read and convert moisture value to a percentage value.
 * @param[in] cal The calibration data.
 * @return Percent value of moisture.
 */
int read_soil_moisture(const struct calibration cal) {
  int raw_value = analogRead(MOISTURE_PIN);
  return map(raw_value, cal.airValue, cal.waterValue, 0, 100);
}

/**
 * Read the value of the light sensor and convert it to Lux value.
 * @return The lux value.
 */
float read_lux() {
  float raw_value = analogRead(LUX_PIN);
  // multiply value by constant to get lux value.
  // this constant assumes AREF is 5v.
  return raw_value * 0.9765625;
}

/**
 * Read the temperature sensor and convert it to Fahrenheit value.
 * @param[in] sensor The Dallas Temperature sensor.
 * @return The temperature in Fahrenheit.
 */
float read_temperature(DallasTemperature *sensor) {
  sensor->requestTemperatures();
  return sensor->getTempFByIndex(0);
}

// initialize globals
struct garden_state state;
state_machine_t state_machine;

// setup function
void setup() {
  Serial.begin(9600);
  // set pins
  pinMode(calibration_pin, OUTPUT);
  pinMode(error_pin, OUTPUT);
  pinMode(MOISTURE_PIN, INPUT);
  pinMode(LUX_PIN, INPUT);
  // Setup a oneWire instance to communicate with any OneWire devices
  OneWire oneWire(ONE_WIRE_BUS);
  // Pass our oneWire reference to Dallas Temperature sensor
  DallasTemperature sensors(&oneWire);
  sensors.begin();
  // initialize global structures
  state_machine.state = STATUS;
  state_machine.status = garden_status;
  state_machine.calibration = garden_calibration;
  state_machine.error = garden_error;
  struct calibration calibration;
  memset(&calibration, 0, sizeof(struct calibration));
  state.calibration = calibration;
  state.temperature = sensors;
}

// loop function
void loop() {
  int wait = 1000;
  int espResponse = -1;
  // TODO see if this is the right way to read data from the serial port
  if (Serial.available()) {
    espResponse = Serial.read();
  }
  handle_state_machine(&state_machine, state);
  // if the state is status then we want to wait about a minute
  // before reporting again.
  if (state_machine.state == STATUS) {
    wait = 1000 * 60;
  }
  delay(wait);
}

int avg_moisture() {
  int tmp = 0;
  for(int i = 0; i < 5; ++i) {
    tmp += analogRead(MOISTURE_PIN);
    delay(100);
  }
  return tmp / 5;
}

// implement state functions
void garden_status(state_machine_t *machine, void* context) {
  struct garden_state *state = (struct garden_state *)context;
  int moisture = read_soil_moisture(state->calibration);
  float lux = read_lux();
  float temp = read_temperature(state->temperature);
  Serial.printf("t=%f;l=%f;m=%d", temp, lux, moisture);
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
  if (machine->state == ERROR) {
    writePin(error_pin, HIGH);
  } else {
    writePin(error_pin, LOW);
  }
}
